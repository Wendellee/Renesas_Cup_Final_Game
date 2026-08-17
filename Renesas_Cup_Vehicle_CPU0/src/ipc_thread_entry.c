#include "ipc_thread.h"
#include "IPC/shared_jpeg_cpu0.h"
#include "SEGGER_RTT/bsp_print.h"

extern TaskHandle_t ipc_thread;

/*
 *[@name] g_ipc0_callback
 *[@type] IPC interrupt callback
 *[@usage] 保存CPU1的共享JPEG回执并使用任务通知唤醒CPU0 IPC Thread
 *[@argument] p_args FSP提供的IPC通道、消息、事件和用户上下文
 *[@return] none
 */
void g_ipc0_callback(ipc_callback_args_t * p_args)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if((NULL != p_args) &&
       (0U != ((uint32_t) p_args->event & (uint32_t) IPC_EVENT_MESSAGE_RECEIVED)))
    {
        shared_jpeg_cpu0_on_ipc_message_isr(p_args->message);
        vTaskNotifyGiveFromISR(ipc_thread, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/*
 *[@name] ipc_thread_entry
 *[@type] thread entry function
 *[@usage] 初始化CPU0共享JPEG生产端，阻塞等待CPU1回执并释放共享载荷所有权
 *[@argument] pvParameters FSP传入的线程参数，当前未使用
 *[@return] none
 */
void ipc_thread_entry(void * pvParameters)
{
    fsp_err_t err;

    FSP_PARAMETER_NOT_USED(pvParameters);

    err = shared_jpeg_cpu0_init();
    if(FSP_SUCCESS != err)
    {
        g_printf("[SHM0][FATAL] Init failed: %u.\r\n", (unsigned int) err);
        vTaskSuspend(NULL);
    }

    g_printf("[SHM0] Ready: base=0x%08X capacity=%u.\r\n",
             (unsigned int) SHARED_JPEG_BASE_ADDRESS,
             (unsigned int) SHARED_JPEG_PAYLOAD_CAPACITY);

    for(;;)
    {
        shared_jpeg_completion_t completion;
        shared_jpeg_cpu0_result_t const result =
            shared_jpeg_cpu0_poll(&completion);

        if(SHARED_JPEG_CPU0_TIMEOUT == result)
        {
            g_printf("[SHM0][ERR] CPU1 JPEG acknowledgement timeout frame=%u.\r\n",
                     (unsigned int) completion.frame_sequence);
        }
        else if(SHARED_JPEG_CPU0_NOTIFY_PENDING == result)
        {
            g_printf("[SHM0][WARN] Doorbell retry pending.\r\n");
        }
        else if(SHARED_JPEG_CPU0_SUCCESS != result)
        {
            g_printf("[SHM0][ERR] Poll failed: %u.\r\n",
                     (unsigned int) result);
        }
        else if(completion.completed)
        {
            if(completion.succeeded)
            {
            }
            else
            {
                g_printf("[SHM0][ERR] CPU1 rejected frame=%u error=%u.\r\n",
                         (unsigned int) completion.frame_sequence,
                         (unsigned int) completion.error_code);
            }
        }

        (void) ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100U));
    }
}
