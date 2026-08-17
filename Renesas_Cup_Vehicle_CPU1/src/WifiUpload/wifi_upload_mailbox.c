#include "WifiUpload/wifi_upload_mailbox.h"

#include "queue.h"

#define WIFI_UPLOAD_QUEUE_LENGTH    (1U)

static StaticQueue_t g_wifi_upload_queue_control;
static uint8_t g_wifi_upload_queue_storage[sizeof(wifi_upload_job_t)];
static QueueHandle_t g_wifi_upload_queue;

/*
 *[@name] wifi_upload_mailbox_init
 *[@type] function
 *[@usage] 创建IPC Thread到Wi-Fi Upload Thread之间的单槽静态FreeRTOS队列
 *[@argument] none
 *[@return] 队列创建成功返回true，否则返回false
 */
bool wifi_upload_mailbox_init(void)
{
    if(NULL == g_wifi_upload_queue)
    {
        g_wifi_upload_queue = xQueueCreateStatic(
            WIFI_UPLOAD_QUEUE_LENGTH,
            (UBaseType_t) sizeof(wifi_upload_job_t),
            g_wifi_upload_queue_storage,
            &g_wifi_upload_queue_control);
    }

    return NULL != g_wifi_upload_queue;
}

/*
 *[@name] wifi_upload_mailbox_submit
 *[@type] function
 *[@usage] 将一个只含共享JPEG描述信息的单槽作业投递给Wi-Fi Upload Thread，不复制JPEG载荷
 *[@argument] p_job IPC Thread完成协议和CRC校验后提供的只读作业描述符
 *[@return] 单槽空闲且投递成功返回true，否则返回false
 */
bool wifi_upload_mailbox_submit(const wifi_upload_job_t * p_job)
{
    if((NULL == p_job) ||
       (NULL == p_job->p_jpeg_data) ||
       (0U == p_job->jpeg_length) ||
       (NULL == g_wifi_upload_queue))
    {
        return false;
    }

    return pdPASS == xQueueSend(g_wifi_upload_queue, p_job, 0U);
}

/*
 *[@name] wifi_upload_mailbox_take
 *[@type] function
 *[@usage] 在Wi-Fi Upload Thread中等待并取得一个上传作业，取得后释放邮箱槽但不释放共享JPEG所有权
 *[@argument] p_job 返回待上传的共享JPEG描述符
 *[@argument] wait_ticks 等待任务通知的最大FreeRTOS Tick数
 *[@return] 取得作业返回true，超时或参数无效返回false
 */
bool wifi_upload_mailbox_take(wifi_upload_job_t * p_job, TickType_t wait_ticks)
{
    if((NULL == p_job) || (NULL == g_wifi_upload_queue))
    {
        return false;
    }

    return pdPASS == xQueueReceive(g_wifi_upload_queue, p_job, wait_ticks);
}
