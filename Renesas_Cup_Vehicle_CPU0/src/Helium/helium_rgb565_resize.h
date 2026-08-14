/*
 * helium_rgb565_resize.h
 *
 *  Created on: 2026年8月11日
 *      Author: lingk
 */

#ifndef HELIUM_HELIUM_RGB565_RESIZE_H_
#define HELIUM_HELIUM_RGB565_RESIZE_H_

#include <stddef.h>
#include <stdint.h>

typedef enum e_helium_rgb565_resize_status
{
    HELIUM_RGB565_RESIZE_SUCCESS = 0,
    HELIUM_RGB565_RESIZE_INVALID_ARGUMENT,
    HELIUM_RGB565_RESIZE_WORKSPACE_TOO_SMALL,
    HELIUM_RGB565_RESIZE_OVERLAPPING_BUFFERS,
    HELIUM_RGB565_RESIZE_SIZE_OVERFLOW
} helium_rgb565_resize_status_t;

/*
 *[@name] helium_rgb565_resize_cfg_t
 *[@type] structure
 *[@usage] 描述RGB565源图像、裁剪区域和目标图像，所有stride的单位都是像素而不是字节
 */
typedef struct st_helium_rgb565_resize_cfg
{
    uint16_t const * p_source;
    uint16_t          source_width;
    uint16_t          source_height;
    uint16_t          source_stride_pixels;
    uint16_t          crop_x;
    uint16_t          crop_y;
    uint16_t          crop_width;
    uint16_t          crop_height;
    uint16_t        * p_destination;
    uint16_t          destination_width;
    uint16_t          destination_height;
    uint16_t          destination_stride_pixels;
} helium_rgb565_resize_cfg_t;

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
    size_t horizontal_map_length);

#endif /* HELIUM_HELIUM_RGB565_RESIZE_H_ */
