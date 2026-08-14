/*
 * ai_inference_result.c
 *
 *  Created on: 2026年8月12日
 *      Author: lingk
 */
#include "ai_inference_result.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
#include <string.h>
static ai_inference_result_snapshot_t g_latest_snapshot;
static bool g_result_valid;

/*
 *[@name] ai_inference_result_publish
 *[@type] function
 *[@usage] 将AI Thread本次后处理结果复制到受临界区保护的最新结果快照中，可发布零检测结果。
 *[@argument] frame_sequence 产生本批检测结果的VIN帧序号
 *[@argument] p_detection YOLO检测结果数组首地址，detection_count为0时允许为NULL
 *[@argument] detection_count 检测结果有效元素数量，有效范围为0到YOLO_MAX_DETECTIONS
 *[@return] 发布成功返回true，参数无效返回false
 */
bool ai_inference_result_publish(uint32_t frame_sequence,
                                 yolo_detection_t *p_detection,
                                 int detection_count)
{
    ai_inference_result_snapshot_t next_snapshot = {0};

    if((detection_count < 0) || (detection_count > YOLO_MAX_DETECTIONS))
    {
        return false;
    }

    if((detection_count > 0) && (NULL == p_detection))
    {
        return false;
    }
    
    next_snapshot.frame_sequence = frame_sequence;
    next_snapshot.detection_count =
        (uint32_t) detection_count;

    if (detection_count > 0)
    {
        memcpy(
            next_snapshot.detections,
            p_detection,
            (size_t) detection_count *
            sizeof(next_snapshot.detections[0]));
    }
    
    taskENTER_CRITICAL();//原子操作开始

    g_latest_snapshot = next_snapshot;
    g_result_valid = true;

    taskEXIT_CRITICAL(); //原子操作结束

    return true;
}

/*
 *[@name] ai_inference_result_get_latest
 *[@type] function
 *[@usage] 在临界区内将最新一份完整结果快照复制到调用者缓冲区，供Display Thread读取。
 *[@argument] p_snapshot 用于接收结果快照的非空指针
 *[@return] 已有有效结果并完成复制时返回true，尚未发布结果或参数无效时返回false
 */
bool ai_inference_result_get_latest( ai_inference_result_snapshot_t *p_snapshot)
{
    bool result_vaild;

    if(NULL == p_snapshot)
    {
        return false;
    }

    taskENTER_CRITICAL();//原子操作开始

    result_vaild = g_result_valid;
    if(result_vaild)
    {
        *p_snapshot = g_latest_snapshot;
    }

    taskEXIT_CRITICAL(); //原子操作结束

    return result_vaild;
}
