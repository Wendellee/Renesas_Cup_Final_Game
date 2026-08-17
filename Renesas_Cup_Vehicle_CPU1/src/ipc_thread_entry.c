#include "ipc_thread.h"
#include "app_runtime.h"
#include "IPC/navigation_ipc_protocol.h"
#include "IPC/shared_jpeg_cpu1.h"
#include "Vehicle/adapters/rtos/vehicle_command_mailbox.h"
#include "WifiUpload/wifi_upload_mailbox.h"
#include "Radio/adapters/rtos/video_frame_mailbox.h"
#include "Radio/protocol/video_protocol.h"
#include "SEGGER_RTT/bsp_print.h"

/* 导航速度参数集中在M33输入适配层，后续实车只需调整这些宏。 */
#define NAV_FORWARD_SPEED_PERCENT       (90U)  /* MPU隔离阶段的自动前进PWM百分比。 */
#define NAV_LEFT_TURN_SPEED_PERCENT     (90U)  /* MPU隔离阶段的原地左转PWM百分比。 */

static volatile bool g_navigation_message_pending;
static volatile uint32_t g_navigation_message;

static const char * navigation_action_name(nav_ipc_action_t action)
{
    switch(action)
    {
        case NAV_IPC_ACTION_FORWARD:   return "FORWARD";
        case NAV_IPC_ACTION_TURN_LEFT: return "TURN_LEFT";
        case NAV_IPC_ACTION_STOP:
        default:                       return "STOP";
    }
}

static bool navigation_message_take(uint32_t * p_message)
{
    bool pending;

    taskENTER_CRITICAL();
    pending = g_navigation_message_pending;
    if(pending)
    {
        *p_message = g_navigation_message;
        g_navigation_message_pending = false;
    }
    taskEXIT_CRITICAL();
    return pending;
}

static void navigation_command_dispatch(uint32_t message)
{
    static bool turn_left_armed;
    static nav_ipc_action_t last_logged_request = NAV_IPC_ACTION_COUNT;
    static nav_ipc_action_t last_logged_applied = NAV_IPC_ACTION_COUNT;
    nav_ipc_action_t requested_action;
    nav_ipc_action_t applied_action = NAV_IPC_ACTION_STOP;
    uint8_t sequence;

    if(!nav_ipc_message_decode(message, &requested_action, &sequence))
    {
        return;
    }

    vehicle_command_t command =
    {
        .source = VEHICLE_COMMAND_SOURCE_IPC,
        .kind = VEHICLE_COMMAND_MANUAL,
        .sequence = sequence,
        .received_tick = xTaskGetTickCount(),
        .manual_action = VEHICLE_MANUAL_STOP,
        .speed_percent = 0U,
    };

    if(NAV_IPC_ACTION_TURN_LEFT == requested_action)
    {
        if(turn_left_armed)
        {
            applied_action = NAV_IPC_ACTION_TURN_LEFT;
        }
        else
        {
            /* 首个转向帧只停车，下一帧危险仍锁存时才左转。 */
            turn_left_armed = true;
        }
    }
    else
    {
        turn_left_armed = false;
        applied_action = requested_action;
    }

    if(NAV_IPC_ACTION_FORWARD == applied_action)
    {
        command.manual_action = VEHICLE_MANUAL_FORWARD;
        command.speed_percent = NAV_FORWARD_SPEED_PERCENT;
    }
    else if(NAV_IPC_ACTION_TURN_LEFT == applied_action)
    {
        command.manual_action = VEHICLE_MANUAL_TURN_LEFT;
        command.speed_percent = NAV_LEFT_TURN_SPEED_PERCENT;
    }

    if(!vehicle_command_mailbox_submit(&command))
    {
        g_printf("[NAV IPC][ERR] Vehicle mailbox unavailable.\r\n");
        return;
    }

    if((requested_action != last_logged_request) ||
       (applied_action != last_logged_applied))
    {
        g_printf("[NAV IPC] seq=%u request=%s applied=%s speed=%u%%.\r\n",
                 (unsigned int) sequence,
                 navigation_action_name(requested_action),
                 navigation_action_name(applied_action),
                 (unsigned int) command.speed_percent);
        last_logged_request = requested_action;
        last_logged_applied = applied_action;
    }
}

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
        nav_ipc_action_t action;
        uint8_t sequence;
        if(nav_ipc_message_decode(p_args->message, &action, &sequence))
        {
            FSP_PARAMETER_NOT_USED(action);
            FSP_PARAMETER_NOT_USED(sequence);
            g_navigation_message = p_args->message;
            __DMB();
            g_navigation_message_pending = true;
        }
        else
        {
            shared_jpeg_cpu1_on_ipc_message_isr(p_args->message);
        }
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
        uint32_t navigation_message;
        if(navigation_message_take(&navigation_message))
        {
            navigation_command_dispatch(navigation_message);
        }

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
