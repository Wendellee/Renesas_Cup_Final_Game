#ifndef IPC_SHARED_JPEG_CPU1_H_
#define IPC_SHARED_JPEG_CPU1_H_

#include "IPC/shared_jpeg_protocol.h"
#include "hal_data.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum e_shared_jpeg_cpu1_result
{
    SHARED_JPEG_CPU1_SUCCESS = 0,
    SHARED_JPEG_CPU1_NOT_INITIALIZED,
    SHARED_JPEG_CPU1_NO_DATA,
    SHARED_JPEG_CPU1_PROTOCOL_ERROR,
    SHARED_JPEG_CPU1_IPC_ERROR
} shared_jpeg_cpu1_result_t;

typedef struct st_shared_jpeg_cpu1_report
{
    bool completed;
    bool succeeded;
    bool upload_ready;
    const uint8_t * p_payload;
    uint32_t frame_sequence;
    uint32_t payload_length;
    uint32_t expected_crc32;
    uint32_t actual_crc32;
    uint32_t error_code;
    uint16_t confidence_milli;
} shared_jpeg_cpu1_report_t;

typedef struct st_shared_video_cpu1_report
{
    bool frame_ready;
    const uint8_t * p_payload;
    uint32_t frame_sequence;
    uint32_t payload_length;
    uint32_t payload_crc32;
    uint16_t width;
    uint16_t height;
    uint8_t slot_index;
} shared_video_cpu1_report_t;

/*
 *[@name] shared_jpeg_cpu1_init
 *[@type] function
 *[@usage] 打开CPU1 IPC Channel 0并准备接收共享JPEG门铃
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t shared_jpeg_cpu1_init(void);

/*
 *[@name] shared_jpeg_cpu1_on_ipc_message_isr
 *[@type] IPC interrupt service function
 *[@usage] 保存CPU0发来的DATA_READY短消息，不在中断中读取共享SDRAM
 *[@argument] message IPC接收到的32位短消息
 *[@return] none
 */
void shared_jpeg_cpu1_on_ipc_message_isr(uint32_t message);

/*
 *[@name] shared_jpeg_cpu1_process
 *[@type] function
 *[@usage] 在CPU1 IPC任务中校验控制块、JPEG边界和CRC并向CPU0回执
 *[@argument] p_report 返回帧序号、长度、CRC和处理结果
 *[@return] 返回CPU1共享JPEG模块状态
 */
shared_jpeg_cpu1_result_t shared_jpeg_cpu1_process(shared_jpeg_cpu1_report_t * p_report);

/*
 *[@name] shared_jpeg_cpu1_complete_upload
 *[@type] function
 *[@usage] 在Wi-Fi Upload Thread完成或放弃上传后发布DONE或ERROR状态，并向CPU0发送最终IPC回执
 *[@argument] frame_sequence 当前上传作业对应的摄像头帧序号
 *[@argument] succeeded 网页端TCP上传完整成功时为true，否则为false
 *[@argument] error_code 上传失败原因，成功时必须为SHARED_JPEG_ERROR_NONE
 *[@return] 返回CPU1共享JPEG模块状态
 */
shared_jpeg_cpu1_result_t shared_jpeg_cpu1_complete_upload(
    uint32_t frame_sequence,
    bool succeeded,
    shared_jpeg_error_t error_code);

shared_jpeg_cpu1_result_t shared_video_cpu1_process(
    shared_video_cpu1_report_t * p_report);

shared_jpeg_cpu1_result_t shared_video_cpu1_complete(
    uint32_t frame_sequence,
    bool succeeded);

#endif /* IPC_SHARED_JPEG_CPU1_H_ */
