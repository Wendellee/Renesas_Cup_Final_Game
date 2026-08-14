#include "encode_thread.h"
#include "Camera/camera_capture.h"
#include "Camera/camera_sensor.h"
#include "ImageUpload/Image_JPEG_Encoder.h"
#include "IPC/shared_jpeg_cpu0.h"
#include "SEGGER_RTT/bsp_print.h"

#define VIDEO_SOURCE_WIDTH          (1024U)
#define VIDEO_SOURCE_HEIGHT         (600U)
#define VIDEO_ENCODE_WIDTH          (200U)
#define VIDEO_ENCODE_HEIGHT         (112U)
#define VIDEO_ENCODE_QUALITY        (35U)
#define VIDEO_TARGET_PERIOD_MS      (100U)
#define VIDEO_GRAY_SIZE             (VIDEO_ENCODE_WIDTH * VIDEO_ENCODE_HEIGHT)

static uint8_t g_video_gray[VIDEO_GRAY_SIZE]
    BSP_ALIGN_VARIABLE(32) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
static uint8_t g_video_gray_previous[VIDEO_GRAY_SIZE]
    BSP_ALIGN_VARIABLE(32) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
static uint8_t g_video_jpeg[SHARED_VIDEO_SLOT_CAPACITY]
    BSP_ALIGN_VARIABLE(32) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
static bool g_video_previous_valid;

static void video_rgb565_to_gray8(const uint16_t * p_source)
{
    uint32_t const crop_height =
        (VIDEO_SOURCE_WIDTH * VIDEO_ENCODE_HEIGHT) / VIDEO_ENCODE_WIDTH;
    uint32_t const crop_y = (VIDEO_SOURCE_HEIGHT - crop_height) / 2U;

    for(uint32_t y = 0U; y < VIDEO_ENCODE_HEIGHT; y++)
    {
        uint32_t const source_y = crop_y +
            ((y * crop_height) / VIDEO_ENCODE_HEIGHT);
        for(uint32_t x = 0U; x < VIDEO_ENCODE_WIDTH; x++)
        {
            uint32_t const source_x =
                (x * VIDEO_SOURCE_WIDTH) / VIDEO_ENCODE_WIDTH;
            uint32_t luminance_sum = 0U;

            /* 4x4 盒式采样抑制缩小后的摩尔纹和传感器噪声。 */
            for(uint32_t fy = 0U; fy < 4U; fy++)
            {
                const uint16_t * const p_row =
                    &p_source[(source_y + fy) * VIDEO_SOURCE_WIDTH];
                for(uint32_t fx = 0U; fx < 4U; fx++)
                {
                    uint16_t const rgb565 = p_row[source_x + fx];
                    uint8_t const r5 = (uint8_t) ((rgb565 >> 11U) & 0x1FU);
                    uint8_t const g6 = (uint8_t) ((rgb565 >> 5U) & 0x3FU);
                    uint8_t const b5 = (uint8_t) (rgb565 & 0x1FU);
                    uint8_t const r8 = (uint8_t) ((r5 << 3U) | (r5 >> 2U));
                    uint8_t const g8 = (uint8_t) ((g6 << 2U) | (g6 >> 4U));
                    uint8_t const b8 = (uint8_t) ((b5 << 3U) | (b5 >> 2U));
                    luminance_sum += (77U * r8) + (150U * g8) + (29U * b8);
                }
            }

            uint32_t const index = (y * VIDEO_ENCODE_WIDTH) + x;
            uint8_t const current = (uint8_t) ((luminance_sum + 2048U) >> 12U);
            g_video_gray[index] = g_video_previous_valid ?
                (uint8_t) ((((uint32_t) current * 3U) +
                            g_video_gray_previous[index] + 2U) >> 2U) : current;
            g_video_gray_previous[index] = current;
        }
    }
    g_video_previous_valid = true;
}

/*
 *[@name] encode_thread_entry
 *[@type] thread entry function
 *[@usage] 每100ms获取最新相机帧，编码200x112灰度JPEG并发布到双槽IPC共享区
 *[@argument] pvParameters FSP传入的线程参数，当前未使用
 *[@return] none
 */
void encode_thread_entry(void *pvParameters) {
	FSP_PARAMETER_NOT_USED(pvParameters);
	TickType_t wake_tick = xTaskGetTickCount();
	uint32_t last_sequence = 0U;

	g_printf("[VIDEO ENC] gray JPEG %ux%u q=%u period=%ums.\r\n",
	         VIDEO_ENCODE_WIDTH, VIDEO_ENCODE_HEIGHT,
	         VIDEO_ENCODE_QUALITY, VIDEO_TARGET_PERIOD_MS);
	while (1) {
		uint32_t sequence = 0U;
		uint8_t * const p_frame = camera_completed_frame_get(&sequence);
		if((NULL != p_frame) && (sequence != last_sequence))
		{
#if BSP_CFG_DCACHE_ENABLED
			SCB_InvalidateDCache_by_Addr((void *) p_frame,
			                            (int32_t) VIN_BYTES_PER_FRAME);
#endif
			video_rgb565_to_gray8((const uint16_t *) p_frame);
			size_t jpeg_size = 0U;
			fsp_err_t const encode_result = ImageJpeg_EncodeGray8(
				g_video_gray, VIDEO_ENCODE_WIDTH, VIDEO_ENCODE_HEIGHT,
				VIDEO_ENCODE_QUALITY, g_video_jpeg, sizeof(g_video_jpeg),
				&jpeg_size);
			if(FSP_SUCCESS == encode_result)
			{
				shared_jpeg_cpu0_result_t const publish_result =
					shared_video_cpu0_publish(g_video_jpeg, jpeg_size, sequence,
					                          VIDEO_ENCODE_WIDTH, VIDEO_ENCODE_HEIGHT);
				if((SHARED_JPEG_CPU0_SUCCESS != publish_result) &&
				   (SHARED_JPEG_CPU0_BUSY != publish_result) &&
				   (SHARED_JPEG_CPU0_NOTIFY_PENDING != publish_result))
				{
					g_printf("[VIDEO ENC][ERR] publish=%u frame=%u.\r\n",
					         (unsigned int) publish_result, (unsigned int) sequence);
				}
			}
			else
			{
				g_printf("[VIDEO ENC][ERR] encode=%u frame=%u.\r\n",
				         (unsigned int) encode_result, (unsigned int) sequence);
			}
			last_sequence = sequence;
		}
		vTaskDelayUntil(&wake_tick, pdMS_TO_TICKS(VIDEO_TARGET_PERIOD_MS));
	}
}
