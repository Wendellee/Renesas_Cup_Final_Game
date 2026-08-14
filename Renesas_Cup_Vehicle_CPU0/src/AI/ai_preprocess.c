/*
 * ai_preprocess.c
 */

#include "AI/ai_preprocess.h"
#include <stdbool.h>
#include <stdint.h>

#if !defined(__ARM_FEATURE_MVE) || ((__ARM_FEATURE_MVE & 1) == 0)
 #error "Arm Helium MVE integer instructions are not enabled"
#endif
#include <arm_mve.h>

#define AI_PREPROCESS_PIXELS_PER_VECTOR    (8U)

/*
 *[@name] ai_preprocess_ranges_overlap
 *[@type] static function
 *[@usage] 判断两个连续内存区间是否重叠，防止预处理原始帧、模型输入和工作区互相覆盖
 *[@argument] p_first 第一个内存区间的首地址
 *[@argument] first_size_bytes 第一个内存区间的长度，单位为字节
 *[@argument] p_second 第二个内存区间的首地址
 *[@argument] second_size_bytes 第二个内存区间的长度，单位为字节
 *[@return] 两个区间重叠返回true，否则返回false
 */
static bool ai_preprocess_ranges_overlap(void const * p_first,
                                         size_t first_size_bytes,
                                         void const * p_second,
                                         size_t second_size_bytes)
{
    uintptr_t const first_begin = (uintptr_t) p_first;
    uintptr_t const second_begin = (uintptr_t) p_second;
    uintptr_t const first_end = first_begin + first_size_bytes;
    uintptr_t const second_end = second_begin + second_size_bytes;

    return ((first_begin < second_end) &&
            (second_begin < first_end));
}

/*
 *[@name] ai_preprocess_rgb565_to_int8
 *[@type] function
 *[@usage] 使用Helium将1024x600 RGB565画面中心裁剪并缩放为128x128 RGB INT8模型输入
 *[@argument] p_source VIN完成帧的只读首地址
 *[@argument] source_size_bytes 源缓冲区可访问长度，单位为字节
 *[@argument] p_destination 模型INT8输入张量首地址
 *[@argument] destination_size_bytes 目标张量容量，单位为字节
 *[@argument] p_horizontal_map 横向最近邻采样索引工作区
 *[@argument] horizontal_map_length 横向索引工作区的uint16_t元素数量
 *[@return] 返回预处理状态，成功时返回AI_PREPROCESS_SUCCESS
 */
ai_preprocess_status_t ai_preprocess_rgb565_to_int8(
    uint8_t const * p_source,
    size_t source_size_bytes,
    int8_t * p_destination,
    size_t destination_size_bytes,
    uint16_t * p_horizontal_map,
    size_t horizontal_map_length)
{
    if ((NULL == p_source) ||
        (NULL == p_destination) ||
        (NULL == p_horizontal_map) ||
        (((uintptr_t) p_source & 1U) != 0U) ||
        (((uintptr_t) p_horizontal_map & 1U) != 0U))
    {
        return AI_PREPROCESS_INVALID_ARGUMENT;
    }

    if (source_size_bytes < AI_PREPROCESS_SOURCE_BYTES)
    {
        return AI_PREPROCESS_SOURCE_TOO_SMALL;
    }

    if (destination_size_bytes < AI_PREPROCESS_DESTINATION_BYTES)
    {
        return AI_PREPROCESS_OUTPUT_TOO_SMALL;
    }

    if (horizontal_map_length < AI_PREPROCESS_HORIZONTAL_MAP_LENGTH)
    {
        return AI_PREPROCESS_WORKSPACE_TOO_SMALL;
    }

    size_t const workspace_size_bytes =
        AI_PREPROCESS_HORIZONTAL_MAP_LENGTH * sizeof(uint16_t);

    if (ai_preprocess_ranges_overlap(p_source,
                                     AI_PREPROCESS_SOURCE_BYTES,
                                     p_destination,
                                     AI_PREPROCESS_DESTINATION_BYTES) ||
        ai_preprocess_ranges_overlap(p_horizontal_map,
                                     workspace_size_bytes,
                                     p_source,
                                     AI_PREPROCESS_SOURCE_BYTES) ||
        ai_preprocess_ranges_overlap(p_horizontal_map,
                                     workspace_size_bytes,
                                     p_destination,
                                     AI_PREPROCESS_DESTINATION_BYTES))
    {
        return AI_PREPROCESS_OVERLAPPING_BUFFERS;
    }

    for (uint32_t destination_x = 0U;
         destination_x < AI_PREPROCESS_DESTINATION_WIDTH;
         destination_x++)
    {
        p_horizontal_map[destination_x] =
            (uint16_t) ((destination_x * AI_PREPROCESS_CROP_WIDTH) /
                        AI_PREPROCESS_DESTINATION_WIDTH);
    }

    for (uint32_t destination_y = 0U;
         destination_y < AI_PREPROCESS_DESTINATION_HEIGHT;
         destination_y++)
    {
        uint32_t const source_y =
            AI_PREPROCESS_CROP_Y +
            ((destination_y * AI_PREPROCESS_CROP_HEIGHT) /
             AI_PREPROCESS_DESTINATION_HEIGHT);

        uint8_t const * p_source_row_bytes =
            p_source +
            ((size_t) source_y * AI_PREPROCESS_SOURCE_STRIDE_BYTES) +
            (AI_PREPROCESS_CROP_X * sizeof(uint16_t));

        uint16_t const * p_source_row =
            (uint16_t const *) p_source_row_bytes;

        int8_t * p_destination_row =
            p_destination +
            ((size_t) destination_y *
             AI_PREPROCESS_DESTINATION_WIDTH *
             AI_PREPROCESS_CHANNEL_COUNT);

        for (uint32_t destination_x = 0U;
             destination_x < AI_PREPROCESS_DESTINATION_WIDTH;
             destination_x += AI_PREPROCESS_PIXELS_PER_VECTOR)
        {
            uint32_t active_lanes =
                AI_PREPROCESS_DESTINATION_WIDTH - destination_x;

            if (active_lanes > AI_PREPROCESS_PIXELS_PER_VECTOR)
            {
                active_lanes = AI_PREPROCESS_PIXELS_PER_VECTOR;
            }

            mve_pred16_t const predicate = vctp16q(active_lanes);

            uint16x8_t const source_offset_vector =
                vldrhq_z_u16(&p_horizontal_map[destination_x], predicate);

            uint16x8_t const native_pixel_vector =
                vldrhq_gather_shifted_offset_z_u16(p_source_row,
                                                   source_offset_vector,
                                                   predicate);

            /* VIN stores the RGB565 MSB first; Cortex-M85 loads halfwords
             * little-endian, so swap the two bytes in every active lane. */
            uint16x8_t const pixel_vector =
                vorrq_u16(vshlq_n_u16(native_pixel_vector, 8),
                          vshrq_n_u16(native_pixel_vector, 8));

            uint16_t pixel_batch[AI_PREPROCESS_PIXELS_PER_VECTOR]
                __attribute__((aligned(16)));

            vstrhq_p_u16(pixel_batch, pixel_vector, predicate);

            for (uint32_t lane = 0U; lane < active_lanes; lane++)
            {
                uint16_t const pixel = pixel_batch[lane];
                uint8_t const red =
                    (uint8_t) (((uint32_t) ((pixel >> 11) & 0x1FU) * 255U) /
                               31U);
                uint8_t const green =
                    (uint8_t) (((uint32_t) ((pixel >> 5) & 0x3FU) * 255U) /
                               63U);
                uint8_t const blue =
                    (uint8_t) (((uint32_t) (pixel & 0x1FU) * 255U) /
                               31U);

                size_t const output_index =
                    ((size_t) destination_x + lane) *
                    AI_PREPROCESS_CHANNEL_COUNT;

                p_destination_row[output_index + 0U] =
                    (int8_t) ((int32_t) red - 128);
                p_destination_row[output_index + 1U] =
                    (int8_t) ((int32_t) green - 128);
                p_destination_row[output_index + 2U] =
                    (int8_t) ((int32_t) blue - 128);
            }
        }
    }

    return AI_PREPROCESS_SUCCESS;
}
