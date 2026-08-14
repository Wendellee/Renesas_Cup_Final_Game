/*
 * camera_capture.h
 *
 *  Created on: 2026年8月11日
 *      Author: lingk
 */

#ifndef CAMERA_CAMERA_CAPTURE_H_
#define CAMERA_CAMERA_CAPTURE_H_

#include "hal_data.h"
#include "common/common.h"

/*
 *[@name] camera_capture_open
 *[@type] function
 *[@usage] 清空采集状态、维护VIN缓冲区Cache并打开VIN和MIPI-CSI，只能在任务上下文调用
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，失败返回对应FSP错误码
 */
fsp_err_t camera_capture_open(void);

/*
 *[@name] camera_capture_start
 *[@type] function
 *[@usage] 先启动VIN接收，再启动OV5640连续图像输出，只能在Camera Thread任务上下文调用
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，失败返回对应FSP错误码并释放已打开的VIN资源
 */
fsp_err_t camera_capture_start(void);

/*
 *[@name] camera_capture_stop
 *[@type] function
 *[@usage] 停止OV5640输出，等待链路停止后关闭VIN和MIPI-CSI，不可在中断中调用
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，失败返回对应FSP错误码
 */
fsp_err_t camera_capture_stop(void);

/*
 *[@name] camera_completed_frame_get
 *[@type] function
 *[@usage] 在任务上下文读取最近完成帧地址及对应帧序号，不改变采集状态
 *[@argument] p_sequence 用于接收完成帧序号的非空指针
 *[@return] 返回最近完成帧缓冲区地址，无有效帧或参数无效时返回NULL
 */
uint8_t * camera_completed_frame_get(uint32_t * p_sequence);

/*
 *[@name] vin_callback
 *[@type] callback function
 *[@usage] 在VIN中断上下文发布完成帧地址、帧序号和错误事件
 *[@argument] p_args FSP提供的VIN事件、状态和完成缓冲区信息
 *[@return] none
 */
void vin_callback(capture_callback_args_t * p_args);

/*
 *[@name] mipi_csi0_callback
 *[@type] callback function
 *[@usage] 接收MIPI-CSI中断事件，当前不向任务发布普通接收活动事件
 *[@argument] p_args FSP提供的MIPI-CSI事件信息
 *[@return] none
 */
void mipi_csi0_callback(mipi_csi_callback_args_t * p_args);

#endif /* CAMERA_CAMERA_CAPTURE_H_ */
