#include "ipc_thread.h"
#include "app_runtime.h"
#include "IPC/shared_jpeg_cpu1.h"
#include "WifiUpload/wifi_upload_mailbox.h"
#include "Radio/adapters/rtos/video_frame_mailbox.h"
#include "Radio/protocol/video_protocol.h"
#include "SEGGER_RTT/bsp_print.h"

extern TaskHandle_t ipc_thread;

/*
 *[@name] g_ipc1_callback
 *[@type] IPC interrupt callback
 *[@usage] 保存CPU0的DATA_READY门铃并使用任务通知唤醒CPU1 IPC Thread
 *[@argument] p_args FSP提供的IPC通道、消息、事件和用户上下文
 *[@return] none
 */
void g_ipc1_callback(ipc_callback_args_t * p_args)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if((NULL != p_args) &&
       (0U != ((uint32_t) p_args->event & (uint32_t) IPC_EVENT_MESSAGE_RECEIVED)))
    {
        shared_jpeg_cpu1_on_ipc_message_isr(p_args->message);
        vTaskNotifyGiveFromISR(ipc_thread, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/*
 *[@name] ipc_thread_entry
 *[@type] thread entry function
 *[@usage] 初始化CPU1共享JPEG消费者，校验JPEG边界与CRC并通过IPC返回处理结果
 *[@argument] pvParameters FSP传入的线程参数，当前未使用
 *[@return] none
 */
void ipc_thread_entry(void * pvParameters)
{
    fsp_err_t err;
    uint32_t video_sequence_in_flight = 0U;

    FSP_PARAMETER_NOT_USED(pvParameters);

    if(!app_runtime_init())
    {
        g_printf("[SYSTEM][FATAL] IPC runtime initialization failed.\r\n");
        vTaskSuspend(NULL);
    }

    if(!wifi_upload_mailbox_init())
    {
        g_printf("[SHM1][FATAL] Wi-Fi upload queue init failed.\r\n");
        vTaskSuspend(NULL);
    }

    err = shared_jpeg_cpu1_init();
    if(FSP_SUCCESS != err)
    {
        g_printf("[SHM1][FATAL] Init failed: %u.\r\n", (unsigned int) err);
        vTaskSuspend(NULL);
    }

    g_printf("[SHM1] CPU1 ready: base=0x%08X capacity=%u.\r\n",
             (unsigned int) SHARED_JPEG_BASE_ADDRESS,
             (unsigned int) SHARED_JPEG_PAYLOAD_CAPACITY);

    /* IPC callback and shared-memory receiver are ready before business dispatch opens. */
    app_runtime_wait_for_start();

    for(;;)
    {
        shared_jpeg_cpu1_report_t report;
        shared_jpeg_cpu1_result_t const result =
            shared_jpeg_cpu1_process(&report);

        if(report.upload_ready)
        {
            wifi_upload_job_t const job =
            {
                .p_jpeg_data = report.p_payload,
                .jpeg_length = report.payload_length,
                .frame_sequence = report.frame_sequence,
                .jpeg_crc32 = report.actual_crc32,
                .width = 240U,
                .height = 240U,
                .confidence_milli = report.confidence_milli
            };

            if(!wifi_upload_mailbox_submit(&job))
            {
                shared_jpeg_cpu1_result_t const completion_result =
                    shared_jpeg_cpu1_complete_upload(
                        report.frame_sequence,
                        false,
                        SHARED_JPEG_ERROR_UPLOAD_QUEUE);

                g_printf("[SHM1][ERR] Wi-Fi queue busy frame=%u.\r\n",
                         (unsigned int) report.frame_sequence);

                if(SHARED_JPEG_CPU1_IPC_ERROR == completion_result)
                {
                    g_printf("[SHM1][WARN] Upload error acknowledgement retry pending.\r\n");
                }
            }
        }
        else if(report.completed)
        {
            if(!report.succeeded)
            {
                g_printf("[SHM1][ERR] JPEG rejected frame=%u error=%u expected=0x%08X actual=0x%08X.\r\n",
                         (unsigned int) report.frame_sequence,
                         (unsigned int) report.error_code,
                         (unsigned int) report.expected_crc32,
                         (unsigned int) report.actual_crc32);
            }

            if(SHARED_JPEG_CPU1_IPC_ERROR == result)
            {
                g_printf("[SHM1][WARN] Result acknowledgement retry pending.\r\n");
            }
        }
        else if((SHARED_JPEG_CPU1_SUCCESS != result) &&
                (SHARED_JPEG_CPU1_NO_DATA != result))
        {
            g_printf("[SHM1][ERR] Process failed: %u.\r\n",
                     (unsigned int) result);
        }

        uint16_t completed_frame_id = 0U;
        bool video_send_succeeded = false;
        if(VideoFrameMailbox_CompletionTake(&completed_frame_id,
                                             &video_send_succeeded) &&
           ((uint16_t) video_sequence_in_flight == completed_frame_id))
        {
            (void) shared_video_cpu1_complete(video_sequence_in_flight,
                                              video_send_succeeded);
            video_sequence_in_flight = 0U;
        }

        if(0U == video_sequence_in_flight)
        {
            shared_video_cpu1_report_t video_report;
            shared_jpeg_cpu1_result_t const video_result =
                shared_video_cpu1_process(&video_report);
            if(video_report.frame_ready)
            {
                video_frame_t const frame =
                {
                    .p_jpeg = video_report.p_payload,
                    .jpeg_size = video_report.payload_length,
                    .crc32 = video_report.payload_crc32,
                    .frame_id = (uint16_t) video_report.frame_sequence,
                    .source_width = video_report.width,
                    .source_height = video_report.height
                };
                if(VideoFrameMailbox_Publish(&frame))
                {
                    video_sequence_in_flight = video_report.frame_sequence;
                }
                else
                {
                    (void) shared_video_cpu1_complete(video_report.frame_sequence,
                                                      false);
                }
            }
            else if((SHARED_JPEG_CPU1_SUCCESS != video_result) &&
                    (SHARED_JPEG_CPU1_NO_DATA != video_result))
            {
                g_printf("[VIDEO IPC][ERR] process=%u.\r\n",
                         (unsigned int) video_result);
            }
        }

        (void) ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5U));
    }
}
