#include "wifi_upload_thread.h"
#include "app_runtime.h"
#include "DA16200/da16200_AT.h"
#include "IPC/shared_jpeg_cpu1.h"
#include "WifiUpload/wifi_upload_mailbox.h"
#include "SEGGER_RTT/bsp_print.h"
#include <string.h>

#define WIFI_UPLOAD_POWER_STABLE_MS          (3000U)
#define WIFI_UPLOAD_AT_TIMEOUT_MS            (3000U)
#define WIFI_UPLOAD_RETRY_DELAY_MS           (5000U)

#define WIFI_UPLOAD_SSID                     "527_RA8P1"
#define WIFI_UPLOAD_PASSWORD                 "060117klj"
#define WIFI_UPLOAD_CONNECT_TIMEOUT_MS       (60000U)

#define WIFI_UPLOAD_SERVER_IP                "192.168.137.1"
#define WIFI_UPLOAD_SERVER_PORT              (5000U)

#define WIFI_UPLOAD_PROTOCOL_VERSION         (1U)
#define WIFI_UPLOAD_HEADER_SIZE              (24U)
#define WIFI_UPLOAD_JPEG_CHUNK_SIZE          (1024U)
#define WIFI_UPLOAD_SEND_TIMEOUT_MS          (5000U)
#define WIFI_UPLOAD_FRONTEND_MAX_JPEG_SIZE   (64U * 1024U)

/*
 *[@name] wifi_upload_store_u16_be
 *[@type] static function
 *[@usage] 按网络大端字节序写入一个16位无符号整数
 *[@argument] p_destination 两字节输出缓冲区首地址
 *[@argument] value 需要序列化的16位数值
 *[@return] none
 */
static void wifi_upload_store_u16_be(uint8_t * p_destination, uint16_t value)
{
    p_destination[0] = (uint8_t) (value >> 8U);
    p_destination[1] = (uint8_t) value;
}

/*
 *[@name] wifi_upload_store_u32_be
 *[@type] static function
 *[@usage] 按网络大端字节序写入一个32位无符号整数
 *[@argument] p_destination 四字节输出缓冲区首地址
 *[@argument] value 需要序列化的32位数值
 *[@return] none
 */
static void wifi_upload_store_u32_be(uint8_t * p_destination, uint32_t value)
{
    p_destination[0] = (uint8_t) (value >> 24U);
    p_destination[1] = (uint8_t) (value >> 16U);
    p_destination[2] = (uint8_t) (value >> 8U);
    p_destination[3] = (uint8_t) value;
}

/*
 *[@name] wifi_upload_complete_job
 *[@type] static function
 *[@usage] 将网页上传结果提交给共享JPEG模块，由模块写入最终状态并向M85发送IPC回执
 *[@argument] p_job 当前Wi-Fi上传作业描述符
 *[@argument] succeeded 完整上传成功时为true，否则为false
 *[@argument] error_code 上传失败原因，成功时使用SHARED_JPEG_ERROR_NONE
 *[@return] none
 */
static void wifi_upload_complete_job(
    const wifi_upload_job_t * p_job,
    bool succeeded,
    shared_jpeg_error_t error_code)
{
    shared_jpeg_cpu1_result_t const result =
        shared_jpeg_cpu1_complete_upload(p_job->frame_sequence,
                                         succeeded,
                                         error_code);

    if(SHARED_JPEG_CPU1_IPC_ERROR == result)
    {
        g_printf("[WIFI][WARN] IPC acknowledgement retry pending frame=%u.\r\n",
                 (unsigned int) p_job->frame_sequence);
    }
    else if(SHARED_JPEG_CPU1_SUCCESS != result)
    {
        g_printf("[WIFI][ERR] Shared JPEG completion failed frame=%u result=%u.\r\n",
                 (unsigned int) p_job->frame_sequence,
                 (unsigned int) result);
    }
}

/*
 *[@name] wifi_upload_reject_pending_job
 *[@type] static function
 *[@usage] 网络初始化失败时取出已经等待的单槽作业并立即向M85返回明确错误，避免共享载荷悬挂
 *[@argument] error_code 当前网络初始化失败原因
 *[@return] none
 */
static void wifi_upload_reject_pending_job(shared_jpeg_error_t error_code)
{
    wifi_upload_job_t job;

    if(wifi_upload_mailbox_take(&job, 0U))
    {
        g_printf("[WIFI][ERR] Upload unavailable frame=%u error=%u.\r\n",
                 (unsigned int) job.frame_sequence,
                 (unsigned int) error_code);
        wifi_upload_complete_job(&job, false, error_code);
    }
}

/*
 *[@name] wifi_upload_ensure_station_mode
 *[@type] static function
 *[@usage] 按单核已验证流程确保DA16200运行在Station模式，模式变更后复位并再次查询确认
 *[@argument] none
 *[@return] Station模式已经生效返回FSP_SUCCESS，否则返回AT通信、复位或模式校验错误
 */
static fsp_err_t wifi_upload_ensure_station_mode(void)
{
    da16200_wifi_mode_t mode;
    fsp_err_t err = DA16200_QueryWifiMode(&mode);

    if(FSP_SUCCESS != err)
    {
        return err;
    }

    if(DA16200_WIFI_MODE_STA == mode)
    {
        g_printf("[WIFI] DA16200 Station mode already active.\r\n");
        return FSP_SUCCESS;
    }

    g_printf("[WIFI] Switching DA16200 mode %u -> 0 (Station).\r\n",
             (unsigned int) mode);

    err = DA16200_SetWifiMode(DA16200_WIFI_MODE_STA);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    err = DA16200_ResetAndWaitReady();
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    err = DA16200_QueryWifiMode(&mode);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    if(DA16200_WIFI_MODE_STA != mode)
    {
        return FSP_ERR_ASSERTION;
    }

    g_printf("[WIFI] DA16200 Station mode enabled.\r\n");
    return FSP_SUCCESS;
}

/*
 *[@name] wifi_upload_connect_frontend
 *[@type] static function
 *[@usage] 复用单核工程参数连接指定Wi-Fi并建立到前端接收器的TCP Client会话
 *[@argument] p_cid 返回DA16200分配的TCP Client会话编号
 *[@argument] p_error_code 返回Wi-Fi连接或TCP连接阶段的共享协议错误码
 *[@return] Wi-Fi状态和TCP会话均验证成功返回FSP_SUCCESS，否则返回对应驱动错误码
 */
static fsp_err_t wifi_upload_connect_frontend(
    uint8_t * p_cid,
    shared_jpeg_error_t * p_error_code)
{
    static char response[DA16200_STR_LEN_512];
    fsp_err_t err;

    if((NULL == p_cid) || (NULL == p_error_code))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    *p_cid = 0xFFU;
    *p_error_code = SHARED_JPEG_ERROR_WIFI_CONNECT;

    err = wifi_upload_ensure_station_mode();
    if(FSP_SUCCESS != err)
    {
        g_printf("[WIFI][ERR] Station mode setup failed: %u.\r\n",
                 (unsigned int) err);
        return err;
    }

    err = DA16200_EnsureWifiConnected(WIFI_UPLOAD_SSID,
                                      WIFI_UPLOAD_PASSWORD,
                                      WIFI_UPLOAD_CONNECT_TIMEOUT_MS);
    if(FSP_SUCCESS != err)
    {
        memset(response, 0, sizeof(response));
        fsp_err_t const status_err =
            DA16200_SendCommandAndGetResponse("AT+CWSTAT\r\n",
                                              response,
                                              (uint16_t) sizeof(response),
                                              5000U);
        if(FSP_SUCCESS != status_err)
        {
            g_printf("[WIFI][WARN] CWSTAT after join failure also failed: %u.\r\n",
                     (unsigned int) status_err);
        }
        return err;
    }

    memset(response, 0, sizeof(response));
    err = DA16200_SendCommandAndGetResponse("AT+CWSTAT\r\n",
                                            response,
                                            (uint16_t) sizeof(response),
                                            5000U);
    if((FSP_SUCCESS != err) ||
       (NULL == strstr(response, "wpa_state=COMPLETED")))
    {
        return (FSP_SUCCESS != err) ? err : FSP_ERR_ASSERTION;
    }

    *p_error_code = SHARED_JPEG_ERROR_TCP_CONNECT;
    err = DA16200_TcpCloseAll();
    if(FSP_SUCCESS != err)
    {
        g_printf("[WIFI][WARN] Close old TCP sessions failed: %u.\r\n",
                 (unsigned int) err);
    }

    err = DA16200_TcpClientOpen(WIFI_UPLOAD_SERVER_IP,
                                WIFI_UPLOAD_SERVER_PORT,
                                p_cid);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    *p_error_code = SHARED_JPEG_ERROR_NONE;
    return FSP_SUCCESS;
}

/*
 *[@name] wifi_upload_send_jpeg
 *[@type] static function
 *[@usage] 按单核工程已验证的RJPG帧头和1024字节分块协议，将共享SDRAM中的JPEG发送到前端
 *[@argument] cid 已建立的DA16200 TCP Client会话编号
 *[@argument] p_job 已通过共享协议、JPEG边界和CRC校验的只读作业描述符
 *[@return] 帧头和全部JPEG分块发送成功返回FSP_SUCCESS，否则返回对应错误码
 */
static fsp_err_t wifi_upload_send_jpeg(
    uint8_t cid,
    const wifi_upload_job_t * p_job)
{
    uint8_t header[WIFI_UPLOAD_HEADER_SIZE] = {0};
    uint32_t offset = 0U;
    fsp_err_t err;

    if((NULL == p_job) ||
       (NULL == p_job->p_jpeg_data) ||
       (p_job->jpeg_length < 4U) ||
       (p_job->jpeg_length > WIFI_UPLOAD_FRONTEND_MAX_JPEG_SIZE) ||
       (p_job->confidence_milli > 1000U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    header[0] = (uint8_t) 'R';
    header[1] = (uint8_t) 'J';
    header[2] = (uint8_t) 'P';
    header[3] = (uint8_t) 'G';
    header[4] = WIFI_UPLOAD_PROTOCOL_VERSION;
    header[5] = WIFI_UPLOAD_HEADER_SIZE;
    wifi_upload_store_u16_be(&header[6], 0U);
    wifi_upload_store_u32_be(&header[8], p_job->frame_sequence);
    wifi_upload_store_u16_be(&header[12], p_job->width);
    wifi_upload_store_u16_be(&header[14], p_job->height);
    wifi_upload_store_u32_be(&header[16], p_job->jpeg_length);
    wifi_upload_store_u16_be(&header[20], p_job->confidence_milli);
    wifi_upload_store_u16_be(&header[22], 0U);

    err = DA16200_TcpClientSendBinaryChunk(cid,
                                           header,
                                           (uint16_t) sizeof(header),
                                           WIFI_UPLOAD_SEND_TIMEOUT_MS);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    while(offset < p_job->jpeg_length)
    {
        uint32_t const remaining = p_job->jpeg_length - offset;
        uint16_t const chunk_size = (uint16_t)
            ((remaining > WIFI_UPLOAD_JPEG_CHUNK_SIZE) ?
             WIFI_UPLOAD_JPEG_CHUNK_SIZE : remaining);

        err = DA16200_TcpClientSendBinaryChunk(
            cid,
            &p_job->p_jpeg_data[offset],
            chunk_size,
            WIFI_UPLOAD_SEND_TIMEOUT_MS);
        if(FSP_SUCCESS != err)
        {
            g_printf("[WIFI][ERR] JPEG chunk failed frame=%u offset=%u err=%u.\r\n",
                     (unsigned int) p_job->frame_sequence,
                     (unsigned int) offset,
                     (unsigned int) err);
            return err;
        }

        offset += chunk_size;
    }

    return FSP_SUCCESS;
}

/*
 *[@name] wifi_upload_thread_entry
 *[@type] thread entry function
 *[@usage] 独占SCI0和DA16200状态机，连接Wi-Fi及前端TCP服务，并逐个处理IPC Thread投递的共享JPEG作业
 *[@argument] pvParameters FSP传入的线程参数，当前未使用
 *[@return] none
 */
void wifi_upload_thread_entry(void * pvParameters)
{
    static char response[DA16200_STR_LEN_512];
    bool frontend_ready = false;
    uint8_t tcp_cid = 0xFFU;
    fsp_err_t err;

    FSP_PARAMETER_NOT_USED(pvParameters);

    if(!app_runtime_init())
    {
        g_printf("[SYSTEM][FATAL] Wi-Fi runtime initialization failed.\r\n");
        vTaskSuspend(NULL);
    }

    err = DA16200_UartInit();
    if(FSP_SUCCESS != err)
    {
        g_printf("[WIFI][FATAL] DA16200 UART init failed: %u.\r\n",
                 (unsigned int) err);
        app_runtime_allow_start_degraded();
        vTaskSuspend(NULL);
    }

    g_printf("[WIFI] Waiting %u ms for DA16200 power stabilization.\r\n",
             (unsigned int) WIFI_UPLOAD_POWER_STABLE_MS);
    vTaskDelay(pdMS_TO_TICKS(WIFI_UPLOAD_POWER_STABLE_MS));
    g_printf("[WIFI] Starting DA16200 AT probe.\r\n");

    for(;;)
    {
        memset(response, 0, sizeof(response));
        err = DA16200_SendCommandAndGetResponse("AT\r\n",
                                                response,
                                                (uint16_t) sizeof(response),
                                                WIFI_UPLOAD_AT_TIMEOUT_MS);
        if(FSP_SUCCESS == err)
        {
            memset(response, 0, sizeof(response));
            err = DA16200_SendCommandAndGetResponse("AT+SDKVER\r\n",
                                                    response,
                                                    (uint16_t) sizeof(response),
                                                    WIFI_UPLOAD_AT_TIMEOUT_MS);
            if(FSP_SUCCESS == err)
            {
                g_printf("[WIFI] DA16200 AT protocol ready.\r\n");
                break;
            }
        }

        wifi_upload_reject_pending_job(SHARED_JPEG_ERROR_WIFI_CONNECT);
        app_runtime_wifi_frontend_set(false);
        g_printf("[WIFI][ERR] DA16200 AT probe failed: %u; retry in %u ms.\r\n",
                 (unsigned int) err,
                 (unsigned int) WIFI_UPLOAD_RETRY_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(WIFI_UPLOAD_RETRY_DELAY_MS));
    }

    for(;;)
    {
        wifi_upload_job_t job;

        if(!frontend_ready)
        {
            shared_jpeg_error_t connection_error;

            err = wifi_upload_connect_frontend(&tcp_cid, &connection_error);
            if(FSP_SUCCESS != err)
            {
                wifi_upload_reject_pending_job(connection_error);
                app_runtime_wifi_frontend_set(false);
                g_printf("[WIFI][ERR] Frontend connection failed: %u; retry in %u ms.\r\n",
                         (unsigned int) err,
                         (unsigned int) WIFI_UPLOAD_RETRY_DELAY_MS);
                vTaskDelay(pdMS_TO_TICKS(WIFI_UPLOAD_RETRY_DELAY_MS));
                continue;
            }

            frontend_ready = true;
            g_printf("[WIFI] Frontend ready: %s:%u CID=%u.\r\n",
                     WIFI_UPLOAD_SERVER_IP,
                     (unsigned int) WIFI_UPLOAD_SERVER_PORT,
                     (unsigned int) tcp_cid);
            app_runtime_wifi_frontend_set(true);
        }

        if(!wifi_upload_mailbox_take(&job, portMAX_DELAY))
        {
            continue;
        }

        err = wifi_upload_send_jpeg(tcp_cid, &job);
        if(FSP_SUCCESS == err)
        {
            g_printf("[WIFI] Uploaded frame=%u JPEG=%u crc=0x%08X confidence=%u.\r\n",
                     (unsigned int) job.frame_sequence,
                     (unsigned int) job.jpeg_length,
                     (unsigned int) job.jpeg_crc32,
                     (unsigned int) job.confidence_milli);
            wifi_upload_complete_job(&job, true, SHARED_JPEG_ERROR_NONE);
        }
        else
        {
            g_printf("[WIFI][ERR] Upload failed frame=%u err=%u.\r\n",
                     (unsigned int) job.frame_sequence,
                     (unsigned int) err);
            wifi_upload_complete_job(&job, false, SHARED_JPEG_ERROR_TCP_SEND);
            (void) DA16200_TcpCloseAll();
            frontend_ready = false;
            tcp_cid = 0xFFU;
            app_runtime_wifi_frontend_set(false);
        }
    }
}
