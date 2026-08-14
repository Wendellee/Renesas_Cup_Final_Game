/*
 * ai_preprocess.h
 *
 * Helium-assisted camera-frame preprocessing for the 128 x 128 INT8 model.
 */

#ifndef AI_AI_PREPROCESS_H_
#define AI_AI_PREPROCESS_H_

#include <stddef.h>
#include <stdint.h>

#define AI_PREPROCESS_SOURCE_WIDTH          (1024U)
#define AI_PREPROCESS_SOURCE_HEIGHT         (600U)
#define AI_PREPROCESS_SOURCE_STRIDE_BYTES   (2048U)
#define AI_PREPROCESS_CROP_X                (212U)
#define AI_PREPROCESS_CROP_Y                (0U)
#define AI_PREPROCESS_CROP_WIDTH            (600U)
#define AI_PREPROCESS_CROP_HEIGHT           (600U)
#define AI_PREPROCESS_DESTINATION_WIDTH     (128U)
#define AI_PREPROCESS_DESTINATION_HEIGHT    (128U)
#define AI_PREPROCESS_CHANNEL_COUNT         (3U)

#define AI_PREPROCESS_SOURCE_BYTES                                      \
    ((AI_PREPROCESS_SOURCE_HEIGHT - 1U) *                               \
     AI_PREPROCESS_SOURCE_STRIDE_BYTES +                                \
     AI_PREPROCESS_SOURCE_WIDTH * sizeof(uint16_t))

#define AI_PREPROCESS_DESTINATION_BYTES                                 \
    (AI_PREPROCESS_DESTINATION_WIDTH *                                  \
     AI_PREPROCESS_DESTINATION_HEIGHT *                                 \
     AI_PREPROCESS_CHANNEL_COUNT)

#define AI_PREPROCESS_HORIZONTAL_MAP_LENGTH \
    (AI_PREPROCESS_DESTINATION_WIDTH)

typedef enum e_ai_preprocess_status
{
    AI_PREPROCESS_SUCCESS = 0,
    AI_PREPROCESS_INVALID_ARGUMENT,
    AI_PREPROCESS_SOURCE_TOO_SMALL,
    AI_PREPROCESS_OUTPUT_TOO_SMALL,
    AI_PREPROCESS_WORKSPACE_TOO_SMALL,
    AI_PREPROCESS_OVERLAPPING_BUFFERS
} ai_preprocess_status_t;

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
    size_t horizontal_map_length);

#endif /* AI_AI_PREPROCESS_H_ */
