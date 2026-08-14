#ifndef JPEG_CODEC_H_
#define JPEG_CODEC_H_

#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"

#define JPEG_CODEC_WIDTH             (200U)
#define JPEG_CODEC_HEIGHT            (112U)
#define JPEG_DISPLAY_WIDTH           (480U)
#define JPEG_DISPLAY_HEIGHT          (272U)
#define JPEG_CODEC_GRAY8_SIZE        (JPEG_CODEC_WIDTH * JPEG_CODEC_HEIGHT)
#define JPEG_CODEC_RGB888_SIZE       (JPEG_DISPLAY_WIDTH * JPEG_DISPLAY_HEIGHT * 3U)
#define JPEG_CODEC_MAX_ENCODED_SIZE  (128U * 1024U)
#define JPEG_CODEC_QUALITY           (1U)

#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
/**
 * Center-crop the photovoltaic-panel view and scale the OV5640 VIN RGB565
 * big-endian frame to a speed-oriented 200x112 Gray8 image.
 */
bool JpegCodec_CameraRgb565ToGray8(uint8_t const * p_camera_rgb565,
                                   uint8_t       * p_gray8,
                                   uint32_t        gray8_capacity);

/** Encode a true single-component 200x112 grayscale baseline JPEG. */
bool JpegCodec_EncodeGray8(uint8_t const * p_gray8,
                           uint8_t       * p_jpeg,
                           uint32_t        jpeg_capacity,
                           uint32_t      * p_jpeg_size);
#endif

/** Decode 200x112 grayscale JPEG and scale it to 480x272 RGB888. */
bool JpegCodec_DecodeRgb888(uint8_t const * p_jpeg,
                            uint32_t        jpeg_size,
                            uint8_t       * p_rgb888,
                            uint32_t        rgb888_capacity,
                            uint16_t      * p_width,
                            uint16_t      * p_height,
                            uint32_t      * p_decoder_result);

#endif
