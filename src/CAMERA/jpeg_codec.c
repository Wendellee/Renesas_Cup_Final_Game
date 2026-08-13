#include "jpeg_codec.h"

#include "camera_capture.h"
#include "tjpgd.h"

#include <string.h>

#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"

#define JPEG_DECODER_WORK_SIZE (4096U)

/* Previous unblended 200x112 grayscale frame.  A light 3:1 temporal blend
 * reduces random camera noise without building a long persistence trail. */
static uint8_t g_gray8_previous[JPEG_CODEC_GRAY8_SIZE] BSP_ALIGN_VARIABLE(32);
static bool    g_gray8_previous_valid;

typedef struct st_jpeg_encode_context
{
    uint8_t  * p_buffer;
    uint32_t   capacity;
    uint32_t   size;
    bool       overflow;
} jpeg_encode_context_t;

typedef struct st_jpeg_decode_context
{
    uint8_t const * p_input;
    uint32_t        input_size;
    uint32_t        input_offset;
    uint8_t       * p_output;
    uint32_t        output_capacity;
    uint16_t        width;
    uint16_t        height;
} jpeg_decode_context_t;

static void jpeg_write_callback(void * p_context, void * p_data, int size)
{
    jpeg_encode_context_t * p_writer = (jpeg_encode_context_t *) p_context;
    uint32_t write_size;

    if ((NULL == p_writer) || (NULL == p_data) || (0 >= size) || p_writer->overflow)
    {
        return;
    }

    write_size = (uint32_t) size;
    if ((write_size > p_writer->capacity) || (p_writer->size > (p_writer->capacity - write_size)))
    {
        p_writer->overflow = true;
        return;
    }

    (void) memcpy(&p_writer->p_buffer[p_writer->size], p_data, write_size);
    p_writer->size += write_size;
}

static size_t jpeg_input_callback(JDEC * p_decoder, uint8_t * p_buffer, size_t byte_count)
{
    jpeg_decode_context_t * p_reader = (jpeg_decode_context_t *) p_decoder->device;
    uint32_t available;
    uint32_t requested;

    if ((NULL == p_reader) || (p_reader->input_offset > p_reader->input_size))
    {
        return 0U;
    }

    available = p_reader->input_size - p_reader->input_offset;
    requested = (byte_count > (size_t) UINT32_MAX) ? UINT32_MAX : (uint32_t) byte_count;
    if (requested > available)
    {
        requested = available;
    }

    if ((NULL != p_buffer) && (0U != requested))
    {
        (void) memcpy(p_buffer, &p_reader->p_input[p_reader->input_offset], requested);
    }
    p_reader->input_offset += requested;

    return (size_t) requested;
}

static int jpeg_output_callback(JDEC * p_decoder, void * p_bitmap, JRECT * p_rect)
{
    jpeg_decode_context_t * p_writer = (jpeg_decode_context_t *) p_decoder->device;
    uint32_t block_width;
    uint32_t block_height;
    uint32_t source_stride;

    if ((NULL == p_writer) || (NULL == p_bitmap) || (NULL == p_rect) ||
        (p_rect->right >= p_writer->width) || (p_rect->bottom >= p_writer->height))
    {
        return 0;
    }

    block_width  = (uint32_t) p_rect->right - p_rect->left + 1U;
    block_height = (uint32_t) p_rect->bottom - p_rect->top + 1U;
    source_stride = block_width * 3U;

    for (uint32_t row = 0U; row < block_height; row++)
    {
        uint8_t const * p_source_row = &((uint8_t const *) p_bitmap)[row * source_stride];

        for (uint32_t column = 0U; column < block_width; column++)
        {
            uint8_t blue  = p_source_row[(column * 3U)];
            uint8_t green = p_source_row[(column * 3U) + 1U];
            uint8_t red   = p_source_row[(column * 3U) + 2U];
            uint32_t source_x = (uint32_t) p_rect->left + column;
            uint32_t source_y = (uint32_t) p_rect->top + row;
            uint32_t output_x_begin = (source_x * JPEG_DISPLAY_WIDTH) / JPEG_CODEC_WIDTH;
            uint32_t output_x_end = ((source_x + 1U) * JPEG_DISPLAY_WIDTH) / JPEG_CODEC_WIDTH;
            uint32_t output_y_begin = (source_y * JPEG_DISPLAY_HEIGHT) / JPEG_CODEC_HEIGHT;
            uint32_t output_y_end = ((source_y + 1U) * JPEG_DISPLAY_HEIGHT) / JPEG_CODEC_HEIGHT;

            for (uint32_t output_y = output_y_begin; output_y < output_y_end; output_y++)
            {
                for (uint32_t output_x = output_x_begin; output_x < output_x_end; output_x++)
                {
                    uint32_t destination_offset =
                        (((output_y * JPEG_DISPLAY_WIDTH) + output_x) * 3U);
                    if ((destination_offset > p_writer->output_capacity) ||
                        (3U > (p_writer->output_capacity - destination_offset)))
                    {
                        return 0;
                    }

                    /* TJpgDec RGB888 output and LVGL RGB888 memory are both B,G,R. */
                    p_writer->p_output[destination_offset]      = blue;
                    p_writer->p_output[destination_offset + 1U] = green;
                    p_writer->p_output[destination_offset + 2U] = red;
                }
            }
        }
    }

    return 1;
}

bool JpegCodec_CameraRgb565ToGray8(uint8_t const * p_camera_rgb565,
                                   uint8_t       * p_gray8,
                                   uint32_t        gray8_capacity)
{
    /* Use the largest centered camera ROI having the display aspect ratio.
     * This removes only the unrelated border of the 1024x600 sensor image,
     * retaining the photovoltaic-panel area before downsampling to 200x112. */
    uint32_t source_crop_height =
        ((uint32_t) CAMERA_CAPTURE_WIDTH * JPEG_CODEC_HEIGHT) / JPEG_CODEC_WIDTH;
    uint32_t source_crop_y = ((uint32_t) CAMERA_CAPTURE_HEIGHT - source_crop_height) / 2U;

    if ((NULL == p_camera_rgb565) || (NULL == p_gray8) ||
        (gray8_capacity < JPEG_CODEC_GRAY8_SIZE) ||
        (source_crop_height > CAMERA_CAPTURE_HEIGHT))
    {
        return false;
    }

    /* VIN writes the camera frame through DMA, so the CPU must discard any
     * cache lines left from an older frame before reading it.  Do this once
     * for the complete frame before it is sampled. */
    SCB_InvalidateDCache_by_Addr((uint32_t *) (void *) p_camera_rgb565,
                                (int32_t) (CAMERA_CAPTURE_HEIGHT * CAMERA_CAPTURE_STRIDE));

    for (uint32_t y = 0U; y < JPEG_CODEC_HEIGHT; y++)
    {
        uint8_t * p_destination_row = p_gray8 + (y * JPEG_CODEC_WIDTH);
        uint32_t source_y = source_crop_y + ((y * source_crop_height) / JPEG_CODEC_HEIGHT);

        for (uint32_t x = 0U; x < JPEG_CODEC_WIDTH; x++)
        {
            uint32_t source_x = (x * CAMERA_CAPTURE_WIDTH) / JPEG_CODEC_WIDTH;
            uint32_t luminance_sum = 0U;
            uint32_t gray_index = (y * JPEG_CODEC_WIDTH) + x;

            for (uint32_t filter_y = 0U; filter_y < 4U; filter_y++)
            {
                uint8_t const * p_source_row =
                    p_camera_rgb565 + ((source_y + filter_y) * CAMERA_CAPTURE_STRIDE);

                for (uint32_t filter_x = 0U; filter_x < 4U; filter_x++)
                {
                    uint32_t source_offset =
                        (source_x + filter_x) * CAMERA_CAPTURE_PIXEL_BYTES;
                    uint16_t rgb565 =
                        (uint16_t) ((uint16_t) p_source_row[source_offset] << 8U) |
                        (uint16_t) p_source_row[source_offset + 1U];
                    uint8_t red5   = (uint8_t) ((rgb565 >> 11U) & 0x1FU);
                    uint8_t green6 = (uint8_t) ((rgb565 >> 5U) & 0x3FU);
                    uint8_t blue5  = (uint8_t) (rgb565 & 0x1FU);
                    uint8_t red8   = (uint8_t) ((red5 << 3U) | (red5 >> 2U));
                    uint8_t green8 = (uint8_t) ((green6 << 2U) | (green6 >> 4U));
                    uint8_t blue8  = (uint8_t) ((blue5 << 3U) | (blue5 >> 2U));
                    luminance_sum += (77U * red8) + (150U * green8) + (29U * blue8);
                }
            }

            /* Divide BT.601 sum by 16 samples and 256. */
            uint8_t current_gray = (uint8_t) ((luminance_sum + 2048U) >> 12U);
            p_destination_row[x] = g_gray8_previous_valid ?
                (uint8_t) ((((uint32_t) current_gray * 3U) +
                            g_gray8_previous[gray_index] + 2U) >> 2U) : current_gray;
            g_gray8_previous[gray_index] = current_gray;
        }
    }

    g_gray8_previous_valid = true;

    return true;
}

bool JpegCodec_EncodeGray8(uint8_t const * p_gray8,
                           uint8_t       * p_jpeg,
                           uint32_t        jpeg_capacity,
                           uint32_t      * p_jpeg_size)
{
    jpeg_encode_context_t writer =
    {
        .p_buffer = p_jpeg,
        .capacity = jpeg_capacity,
        .size = 0U,
        .overflow = false
    };
    int result;

    if ((NULL == p_gray8) || (NULL == p_jpeg) || (NULL == p_jpeg_size) || (0U == jpeg_capacity))
    {
        return false;
    }

    result = tje_encode_with_func(jpeg_write_callback,
                                  &writer,
                                  (int) JPEG_CODEC_QUALITY,
                                  (int) JPEG_CODEC_WIDTH,
                                  (int) JPEG_CODEC_HEIGHT,
                                  1,
                                  p_gray8);
    *p_jpeg_size = writer.size;

    return (0 != result) && !writer.overflow && (0U != writer.size);
}

bool JpegCodec_DecodeRgb888(uint8_t const * p_jpeg,
                            uint32_t        jpeg_size,
                            uint8_t       * p_rgb888,
                            uint32_t        rgb888_capacity,
                            uint16_t      * p_width,
                            uint16_t      * p_height,
                            uint32_t      * p_decoder_result)
{
    uint8_t work_buffer[JPEG_DECODER_WORK_SIZE];
    JDEC decoder = {0};
    jpeg_decode_context_t context =
    {
        .p_input = p_jpeg,
        .input_size = jpeg_size,
        .input_offset = 0U,
        .p_output = p_rgb888,
        .output_capacity = rgb888_capacity,
        .width = 0U,
        .height = 0U
    };
    JRESULT result;

    if ((NULL == p_jpeg) || (0U == jpeg_size) || (NULL == p_rgb888) ||
        (rgb888_capacity < JPEG_CODEC_RGB888_SIZE))
    {
        if (NULL != p_decoder_result)
        {
            *p_decoder_result = (uint32_t) JDR_PAR;
        }
        return false;
    }

    result = jd_prepare(&decoder,
                        jpeg_input_callback,
                        work_buffer,
                        sizeof(work_buffer),
                        &context);
    if (JDR_OK == result)
    {
        context.width = decoder.width;
        context.height = decoder.height;
        if ((JPEG_CODEC_WIDTH != decoder.width) || (JPEG_CODEC_HEIGHT != decoder.height))
        {
            result = JDR_PAR;
        }
        else
        {
            result = jd_decomp(&decoder, jpeg_output_callback, 0U);
        }
    }

    if (NULL != p_width)
    {
        *p_width = decoder.width;
    }
    if (NULL != p_height)
    {
        *p_height = decoder.height;
    }
    if (NULL != p_decoder_result)
    {
        *p_decoder_result = (uint32_t) result;
    }

    return JDR_OK == result;
}
