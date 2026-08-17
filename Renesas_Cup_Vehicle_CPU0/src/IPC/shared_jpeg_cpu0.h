#ifndef IPC_SHARED_JPEG_CPU0_H_
#define IPC_SHARED_JPEG_CPU0_H_

#include "IPC/shared_jpeg_protocol.h"
#include "hal_data.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum e_shared_jpeg_cpu0_result
{
    SHARED_JPEG_CPU0_SUCCESS = 0,
    SHARED_JPEG_CPU0_NOT_INITIALIZED,
    SHARED_JPEG_CPU0_BUSY,
    SHARED_JPEG_CPU0_INVALID_ARGUMENT,
    SHARED_JPEG_CPU0_TOO_LARGE,
    SHARED_JPEG_CPU0_PROTOCOL_ERROR,
    SHARED_JPEG_CPU0_NOTIFY_PENDING,
    SHARED_JPEG_CPU0_TIMEOUT
} shared_jpeg_cpu0_result_t;

typedef struct st_shared_jpeg_completion
{
    bool completed;
    bool succeeded;
    uint32_t frame_sequence;
    uint32_t error_code;
} shared_jpeg_completion_t;

/*
 *[@name] shared_jpeg_cpu0_init
 *[@type] function
 *[@usage] 初始化CPU0共享JPEG控制块并打开IPC Channel 0
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t shared_jpeg_cpu0_init(void);

/*
 *[@name] shared_jpeg_cpu0_on_ipc_message_isr
 *[@type] IPC interrupt service function
 *[@usage] 保存CPU1发来的DONE或ERROR短消息，不在中断中读取JPEG或计算CRC
 *[@argument] message IPC接收到的32位短消息
 *[@return] none
 */
void shared_jpeg_cpu0_on_ipc_message_isr(uint32_t message);

/*
 *[@name] shared_jpeg_cpu0_publish
 *[@type] function
 *[@usage] 将完整JPEG复制到共享SDRAM，生成CRC并通过IPC通知CPU1
 *[@argument] p_jpeg_data CPU0私有JPEG只读首地址
 *[@argument] jpeg_length JPEG有效长度，单位为字节
 *[@argument] frame_sequence JPEG对应的摄像头帧序号
 *[@argument] confidence_milli 异常检测置信度千分值，范围0到1000
 *[@return] 返回CPU0共享JPEG模块状态
 */
shared_jpeg_cpu0_result_t shared_jpeg_cpu0_publish(
    const uint8_t * p_jpeg_data,
    size_t jpeg_length,
    uint32_t frame_sequence,
    uint16_t confidence_milli);

/*
 *[@name] shared_jpeg_cpu0_poll
 *[@type] function
 *[@usage] 在任务上下文重发门铃、处理CPU1回执并释放共享缓冲区
 *[@argument] p_completion 返回完成状态、帧序号和CPU1错误码
 *[@return] 返回CPU0共享JPEG模块状态
 */
shared_jpeg_cpu0_result_t shared_jpeg_cpu0_poll(shared_jpeg_completion_t * p_completion);

/* 发布一帧实时图传 JPEG。双槽已满时返回 BUSY；调用者应直接丢弃该帧。 */
shared_jpeg_cpu0_result_t shared_video_cpu0_publish(
    const uint8_t * p_jpeg_data,
    size_t jpeg_length,
    uint32_t frame_sequence,
    uint16_t width,
    uint16_t height);

#endif /* IPC_SHARED_JPEG_CPU0_H_ */
