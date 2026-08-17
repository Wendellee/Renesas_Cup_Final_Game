/*
 * helium_rgb565_resize.c
 *
 *  Created on: 2026年8月11日
 *      Author: lingk
 */
#include "helium_rgb565_resize.h"

#include <stdbool.h>
#include <stdint.h>

#if !defined(__ARM_FEATURE_MVE) || ((__ARM_FEATURE_MVE & 1) == 0)
 #error "Arm Helium MVE integer instructions are not enabled"
#endif

#include <arm_mve.h>

#define HELIUM_RGB565_PIXELS_PER_VECTOR    (8U)

/*
 *[@name] helium_rgb565_ranges_overlap
 *[@type] static function
 *[@usage] 判断两个半开地址区间是否重叠
 *[@argument] p_source 第一个区间起始地址
 *[@argument] source_size_bytes 第一个区间字节数
 *[@argument] p_destination 第二个区间起始地址
 *[@argument] destination_size_bytes 第二个区间字节数
 *[@return] 区间重叠返回true，否则返回false
 */
static bool helium_rgb565_ranges_overlap(uint16_t const * p_source,
                                         size_t source_size_bytes,
                                         uint16_t const * p_destination,
                                         size_t destination_size_bytes)
{
    uintptr_t const source_begin = (uintptr_t) p_source;
    uintptr_t const destination_begin = (uintptr_t) p_destination;
    uintptr_t const source_end = source_begin + source_size_bytes;
    uintptr_t const destination_end = destination_begin + destination_size_bytes;

    return ((source_begin < destination_end) &&
            (destination_begin < source_end));
}

/*
 *[@name] helium_rgb565_resize_nearest
 *[@type] function
 *[@usage] 使用Arm Helium MVE对RGB565裁剪区域执行最近邻缩放，不执行Cache维护且不可在中断中调用
 *[@argument] p_cfg 源图像、裁剪区域、目标图像及行跨度配置
 *[@argument] p_horizontal_map 调用者提供的横向源像素索引工作区，不得与图像缓冲区重叠
 *[@argument] horizontal_map_length 横向索引工作区容量，单位为uint16_t元素个数
 *[@return] 成功返回HELIUM_RGB565_RESIZE_SUCCESS，失败返回对应状态码
 */
helium_rgb565_resize_status_t helium_rgb565_resize_nearest(
    helium_rgb565_resize_cfg_t const * p_cfg,
    uint16_t * p_horizontal_map,
    size_t horizontal_map_length)
{
    size_t source_storage_pixels;
    size_t destination_storage_pixels;

    if ((NULL == p_cfg) ||
        (NULL == p_horizontal_map) ||
        (NULL == p_cfg->p_source) ||
        (NULL == p_cfg->p_destination))
    {
        return HELIUM_RGB565_RESIZE_INVALID_ARGUMENT;
    }

    if ((0U == p_cfg->source_width) ||
        (0U == p_cfg->source_height) ||
        (0U == p_cfg->source_stride_pixels) ||
        (0U == p_cfg->crop_width) ||
        (0U == p_cfg->crop_height) ||
        (0U == p_cfg->destination_width) ||
        (0U == p_cfg->destination_height) ||
        (0U == p_cfg->destination_stride_pixels))
    {
        return HELIUM_RGB565_RESIZE_INVALID_ARGUMENT;
    }

    if ((p_cfg->source_stride_pixels < p_cfg->source_width) ||
        (p_cfg->destination_stride_pixels < p_cfg->destination_width))
    {
        return HELIUM_RGB565_RESIZE_INVALID_ARGUMENT;
    }

    if ((((uint32_t) p_cfg->crop_x + p_cfg->crop_width) >
         p_cfg->source_width) ||
        (((uint32_t) p_cfg->crop_y + p_cfg->crop_height) >
         p_cfg->source_height))
    {
        return HELIUM_RGB565_RESIZE_INVALID_ARGUMENT;
    }

    if ((((uintptr_t) p_cfg->p_source & 1U) != 0U) ||
        (((uintptr_t) p_cfg->p_destination & 1U) != 0U) ||
        (((uintptr_t) p_horizontal_map & 1U) != 0U))
    {
        return HELIUM_RGB565_RESIZE_INVALID_ARGUMENT;
    }

    if (horizontal_map_length < p_cfg->destination_width)
    {
        return HELIUM_RGB565_RESIZE_WORKSPACE_TOO_SMALL;
    }

    source_storage_pixels =
        ((size_t) (p_cfg->source_height - 1U) *
         p_cfg->source_stride_pixels) + p_cfg->source_width;

    destination_storage_pixels =
        ((size_t) (p_cfg->destination_height - 1U) *
         p_cfg->destination_stride_pixels) + p_cfg->destination_width;

    if ((source_storage_pixels > (SIZE_MAX / sizeof(uint16_t))) ||
        (destination_storage_pixels > (SIZE_MAX / sizeof(uint16_t))) ||
        ((size_t) p_cfg->destination_width >
         (SIZE_MAX / sizeof(uint16_t))))
    {
        return HELIUM_RGB565_RESIZE_SIZE_OVERFLOW;
    }

    if (helium_rgb565_ranges_overlap(p_cfg->p_source,
                                     source_storage_pixels * sizeof(uint16_t),
                                     p_cfg->p_destination,
                                     destination_storage_pixels * sizeof(uint16_t)))
    {
        return HELIUM_RGB565_RESIZE_OVERLAPPING_BUFFERS;
    }

    if (helium_rgb565_ranges_overlap(p_horizontal_map,
                                     (size_t) p_cfg->destination_width * sizeof(uint16_t),
                                     p_cfg->p_source,
                                     source_storage_pixels * sizeof(uint16_t)) ||
        helium_rgb565_ranges_overlap(p_horizontal_map,
                                     (size_t) p_cfg->destination_width * sizeof(uint16_t),
                                     p_cfg->p_destination,
                                     destination_storage_pixels * sizeof(uint16_t)))
    {
        return HELIUM_RGB565_RESIZE_OVERLAPPING_BUFFERS;
    }

    for (uint32_t destination_x = 0U;
         destination_x < p_cfg->destination_width;
         destination_x++)
    {
        p_horizontal_map[destination_x] =
            (uint16_t) ((destination_x * p_cfg->crop_width) /
                        p_cfg->destination_width);
    }

    for (uint32_t destination_y = 0U;
         destination_y < p_cfg->destination_height;
         destination_y++)
    {
        uint32_t const source_y =
            (uint32_t) p_cfg->crop_y +
            ((destination_y * p_cfg->crop_height) /
             p_cfg->destination_height);

        uint16_t const * p_source_row =
            p_cfg->p_source +
            ((size_t) source_y * p_cfg->source_stride_pixels) +
            p_cfg->crop_x;

        uint16_t * p_destination_row =
            p_cfg->p_destination +
            ((size_t) destination_y *
             p_cfg->destination_stride_pixels);

        for (uint32_t destination_x = 0U;
             destination_x < p_cfg->destination_width;
             destination_x += HELIUM_RGB565_PIXELS_PER_VECTOR)
        {
            uint32_t active_lanes =
                (uint32_t) p_cfg->destination_width - destination_x;

            if (active_lanes > HELIUM_RGB565_PIXELS_PER_VECTOR)
            {
                active_lanes = HELIUM_RGB565_PIXELS_PER_VECTOR;
            }

            mve_pred16_t const predicate = vctp16q(active_lanes);

            uint16x8_t const source_offset_vector =
                vldrhq_z_u16(&p_horizontal_map[destination_x], predicate);

            uint16x8_t const pixel_vector =
                vldrhq_gather_shifted_offset_z_u16(p_source_row,
                                                   source_offset_vector,
                                                   predicate);

            vstrhq_p_u16(&p_destination_row[destination_x],
                         pixel_vector,
                         predicate);
        }
    }

    return HELIUM_RGB565_RESIZE_SUCCESS;
}
