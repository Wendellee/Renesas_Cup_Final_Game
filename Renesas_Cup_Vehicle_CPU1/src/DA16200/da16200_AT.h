/*
 * da16200_AT.h
 *
 *  Created on: 2026年7月24日
 *      Author: lingk
 */

#ifndef DA16200_DA16200_AT_H_
#define DA16200_DA16200_AT_H_

#include "hal_data.h"
#include "wifi_upload_thread.h"
#include "RingBuffer/ring_buffer.h"
#include "SEGGER_RTT/bsp_print.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/** Macros to define string length */
#define DA16200_STR_LEN_8        (8U)       ///< Length 8
#define DA16200_STR_LEN_16       (16U)      ///< Length 16
#define DA16200_STR_LEN_32       (32U)      ///< Length 32
#define DA16200_STR_LEN_64       (64U)      ///< Length 64
#define DA16200_STR_LEN_128      (128U)     ///< Length 128
#define DA16200_STR_LEN_256      (256U)     ///< Length 128
#define DA16200_STR_LEN_512      (512U)     ///< Length 512


/** DA16200 delay in milliseconds between AT command retry */
#define DA16200_DELAY_0      (0U)       ///< No delay
#define DA16200_DELAY_20MS   (20U)      ///< Delay of 20 milliseconds
#define DA16200_DELAY_50MS   (50U)      ///< Delay of 50 milliseconds
#define DA16200_DELAY_100MS  (100U)     ///< Delay of 100 milliseconds
#define DA16200_DELAY_200MS  (200U)     ///< Delay of 200 milliseconds
#define DA16200_DELAY_300MS  (300U)     ///< Delay of 300 milliseconds
#define DA16200_DELAY_500MS  (500U)     ///< Delay of 500 milliseconds
#define DA16200_DELAY_1000MS (1000U)    ///< Delay of 1000 milliseconds
#define DA16200_DELAY_2000MS (2000U)    ///< Delay of 2000 milliseconds
#define DA16200_DELAY_3000MS (3000U)    ///< Delay of 3000 milliseconds
#define DA16200_DELAY_4000MS (4000U)    ///< Delay of 4000 milliseconds
#define DA16200_DELAY_5000MS (5000U)    ///< Delay of 5000 milliseconds

/** DA16200 retry count for AT command set */
#define DA16200_RETRY_VALUE_0    (0U)       ///< No Retry
#define DA16200_RETRY_VALUE_1    (1U)       ///< Retry Once
#define DA16200_RETRY_VALUE_5    (5U)       ///< Retry 5 times
#define DA16200_RETRY_VALUE_10   (10U)      ///< Retry 10 times

typedef enum
{
    DA16200_WIFI_MODE_STA     = 0,
    DA16200_WIFI_MODE_SOFT_AP = 1,
    DA16200_WIFI_MODE_STA_AP  = 2
} da16200_wifi_mode_t;//模式选择

typedef struct
{
    const char * p_ssid;
    const char * p_password;
    uint8_t      channel;
    const char * p_country;
} da16200_softap_cfg_t;//设置热点密钥

/** 初始化 SCI0、接收环形缓冲区及 DA16200 驱动状态。上电后仅调用一次。 */
fsp_err_t DA16200_UartInit(void);

/**
 * 发送一条 AT 命令，并取得包含 OK/ERROR 的完整原始响应。
 * @param p_command     以 \r\n 结尾的 AT 命令字符串。
 * @param p_response    调用者提供的响应缓冲区。
 * @param response_size 响应缓冲区大小，至少为 2。
 * @param timeout_ms    等待响应的超时时间，单位毫秒。
 */
fsp_err_t DA16200_SendCommandAndGetResponse(const char * p_command,
                                             char * p_response,
                                             uint16_t response_size,
                                             uint32_t timeout_ms);

/** 发送 AT 命令，收到完整响应后检查是否包含 p_expected 指定关键字。 */
fsp_err_t DA16200_SendCommandAndWait(const char * p_command,
                                      const char * p_expected,
                                      uint32_t timeout_ms);

/** FSP SCI0 UART 回调：接收字节写入环形缓冲区，记录发送完成和 UART 错误。 */
void      UART0_CallBack(uart_callback_args_t * p_args);

/** 查询当前 Wi-Fi 模式，并将 STA/SoftAP/STA+AP 枚举值写入 p_mode。 */
fsp_err_t DA16200_QueryWifiMode(da16200_wifi_mode_t * p_mode);

/** 设置 Wi-Fi 模式；配置写入 NVRAM，之后必须重启 DA16200 才生效。 */
fsp_err_t DA16200_SetWifiMode(da16200_wifi_mode_t mode);

/** 发送 AT+RST，等待模块启动，并以 AT 命令确认 AT 通信已恢复。 */
fsp_err_t DA16200_ResetAndWaitReady(void);

/** 保存 SoftAP 的 SSID、WPA2/AES 密码、信道和国家码到 DA16200 NVRAM。 */
fsp_err_t DA16200_ConfigSoftAp(const da16200_softap_cfg_t * p_cfg);

/** 发送 AT+CWOAP 启动 SoftAP；若热点已运行，模块可能返回 ERROR:-522。 */
fsp_err_t DA16200_StartSoftAp(void);

/**
 * 非阻塞读取 DA16200 异步数据，如客户端连接/断开通知和 TCP 接收数据。
 * 没有数据时返回 0；调用者应及时读取，避免接收环形缓冲区溢出。
 */
size_t DA16200_ReadAsync(uint8_t * p_data, size_t capacity);

/**
 * 通过 TCP Server 向指定客户端发送一段纯文本。
 * remote_ip / remote_port 来自 +TRCTS 通知。
 * 第一版仅用于不含逗号、回车、换行的短控制指令，例如 W、A、S、D、STOP。
 */
fsp_err_t DA16200_TcpServerSendText(const char * p_remote_ip,
                                    uint16_t remote_port,
                                    const char * p_text);

fsp_err_t DA16200_ConnectWifi(const char * p_ssid,
                              const char * p_password,
                              uint32_t timeout_ms);

fsp_err_t DA16200_QueryStaConnected(bool * p_connected);

fsp_err_t DA16200_TcpClientOpen(const char * p_server_ip,
                                uint16_t server_port,
                                uint8_t * p_cid);    
                
fsp_err_t DA16200_EnsureWifiConnected(const char * p_ssid,
                                      const char * p_password,
                                      uint32_t timeout_ms);

fsp_err_t DA16200_TcpCloseAll(void);

fsp_err_t DA16200_TcpClientSendText(uint8_t cid,
                                    const char * p_text);

fsp_err_t DA16200_TcpClientSendBinaryChunk(
    uint8_t cid,
    const uint8_t * p_data,
    uint16_t data_length,
    uint32_t timeout_ms);
#endif /* DA16200_DA16200_AT_H_ */
