/*
 * ai_inference_result.h
 *
 *  Created on: 2026年8月12日
 *      Author: lingk
 */

#ifndef AI_AI_INFERENCE_RESULT_H_
#define AI_AI_INFERENCE_RESULT_H_

#include "AI/yolo_postprocess.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct st_ai_inference_result_snapshot
{
    uint32_t frame_sequence;  /*产生结果的VIN帧序号*/
    uint32_t detection_count;
    yolo_detection_t detections[YOLO_MAX_DETECTIONS];
} ai_inference_result_snapshot_t;

/*
 *[@name] ai_inference_result_publish
 *[@type] function
 *[@usage] 将AI Thread本次后处理结果复制到线程安全的最新结果快照中，可发布零检测结果。
 *[@argument] frame_sequence 产生本批检测结果的VIN帧序号
 *[@argument] p_detections YOLO检测结果数组首地址，detection_count为0时允许为NULL
 *[@argument] detection_count 检测结果有效元素数量，有效范围为0到YOLO_MAX_DETECTIONS
 *[@return] 发布成功返回true，参数无效返回false
 */
bool ai_inference_result_publish(uint32_t frame_sequence,
                                 yolo_detection_t *p_detection,
                                 int detection_count);

/*
 *[@name] ai_inference_result_get_latest
 *[@type] function
 *[@usage] 将最新一份完整结果快照复制到调用者缓冲区，供Display Thread读取。
 *[@argument] p_snapshot 用于接收结果快照的非空指针
 *[@return] 已有有效结果并完成复制时返回true，尚未发布结果或参数无效时返回false
 */
bool ai_inference_result_get_latest(
    ai_inference_result_snapshot_t * p_snapshot);

#endif /* AI_AI_INFERENCE_RESULT_H_ */
