/*
 * yolo_postprocess.h
 *
 * YOLO INT8 output decoding and non-maximum suppression.
 * This module contains no drawing or RTOS code.
 */

#ifndef AI_YOLO_POSTPROCESS_H_
#define AI_YOLO_POSTPROCESS_H_

#include <stdint.h>

#define YOLO_INPUT_SIZE             (128)
#define YOLO_CLASS_COUNT            (1)
#define YOLO_OUTPUT_BOX_COUNT       (336)
#define YOLO_OUTPUT_ATTRS           (5)
#define YOLO_MAX_DETECTIONS         (4)

typedef struct st_yolo_detection
{
    float   x1;       /* Left edge in 128 x 128 model coordinates. */
    float   y1;       /* Top edge in 128 x 128 model coordinates. */
    float   x2;       /* Right edge in 128 x 128 model coordinates. */
    float   y2;       /* Bottom edge in 128 x 128 model coordinates. */
    float   score;    /* Confidence in the range 0.0 to 1.0. */
    uint8_t class_id;
} yolo_detection_t;

extern const char * const g_yolo_class_names[YOLO_CLASS_COUNT];

/*
 *[@name] yolo_decode_int8_output
 *[@type] function
 *[@usage] 反量化并解码模型INT8输出，筛除低置信度和无效框，并保留固定数量的高分候选框。
 *[@argument] p_output 模型INT8输出张量首地址
 *[@argument] p_detections 用于接收检测结果的数组首地址
 *[@argument] capacity 检测结果数组容量
 *[@argument] confidence_threshold 置信度阈值，有效范围将被限制到0.0至1.0
 *[@return] 返回写入p_detections的有效候选框数量，参数无效时返回0
 */
int yolo_decode_int8_output(const int8_t * p_output,
                            yolo_detection_t * p_detections,
                            int capacity,
                            float confidence_threshold);

/*
 *[@name] yolo_nms
 *[@type] function
 *[@usage] 按置信度降序整理候选框，并对同类别重叠框执行非极大值抑制。
 *[@argument] p_detections 待原地排序和压缩的检测结果数组首地址
 *[@argument] count 输入检测结果有效元素数量
 *[@argument] iou_threshold NMS交并比阈值，有效范围将被限制到0.0至1.0
 *[@return] 返回NMS后保留在数组前部的有效检测框数量，参数无效时返回0
 */
int yolo_nms(yolo_detection_t * p_detections,
             int count,
             float iou_threshold);

#endif /* AI_YOLO_POSTPROCESS_H_ */
