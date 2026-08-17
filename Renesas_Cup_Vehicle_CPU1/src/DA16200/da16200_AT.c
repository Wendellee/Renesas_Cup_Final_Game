/*
 * da16200_AT.c
 *
 *  Created on: 2026年7月24日
 *      Author: lingk
 */

#include "DA16200/da16200_AT.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <string.h>

static ring_buffer_t g_da16200_rx_ring;//UART RX 中断收到的每一个字节都进入这里
static volatile bool g_da16200_tx_done = false;//等到 UART_EVENT_TX_COMPLETE 才置位
static volatile bool g_da16200_rx_overflow = false;//环形缓冲满时记录错误
static volatile fsp_err_t g_da16200_uart_error = FSP_SUCCESS;//记录 UART 错误事件

static volatile uint32_t g_da16200_rx_char_count = 0U;
static volatile uint32_t g_da16200_rx_drop_count = 0U;

static StaticSemaphore_t g_da16200_tx_semaphore_control;
static StaticSemaphore_t g_da16200_rx_semaphore_control;
static SemaphoreHandle_t g_da16200_tx_semaphore;
static SemaphoreHandle_t g_da16200_rx_semaphore;

static fsp_err_t da16200_wait_tx_complete(uint32_t timeout_ms);

/*
 *[@name] da16200_task_delay_ms
 *[@type] static function
 *[@usage] 将DA16200阻塞等待转换为FreeRTOS任务延时，使Wi-Fi等待期间M33其他任务继续调度
 *[@argument] delay_ms 需要阻塞当前Wi-Fi Upload Thread的时间，单位为毫秒
 *[@return] none
 */
static void da16200_task_delay_ms(uint32_t delay_ms)
{
    TickType_t delay_ticks = pdMS_TO_TICKS(delay_ms);

    if((delay_ms > 0U) && (0U == delay_ticks))
    {
        delay_ticks = 1U;
    }

    if(delay_ticks > 0U)
    {
        vTaskDelay(delay_ticks);
    }
}

/*
 *[@name] da16200_wait_rx_activity
 *[@type] static function
 *[@usage] 使用UART0接收中断释放的二值信号量阻塞Wi-Fi任务，避免轮询占用M33处理时间
 *[@argument] timeout_ms 最大等待时间，单位为毫秒
 *[@return] none
 */
static void da16200_wait_rx_activity(uint32_t timeout_ms)
{
    TickType_t wait_ticks = pdMS_TO_TICKS(timeout_ms);

    if((timeout_ms > 0U) && (0U == wait_ticks))
    {
        wait_ticks = 1U;
    }

    if(NULL != g_da16200_rx_semaphore)
    {
        (void) xSemaphoreTake(g_da16200_rx_semaphore, wait_ticks);
    }
    else
    {
        da16200_task_delay_ms(timeout_ms);
    }
}


/**
 * @brief 初始化 SCI0 UART、DA16200 接收环形缓冲区和驱动状态变量。
 * @param 无。
 * @return 初始化成功时返回 FSP_SUCCESS，否则返回 FSP UART 驱动错误码。
 * @note 系统启动阶段只调用一次；本函数不是线程安全函数，也不可在中断中调用。
 */
fsp_err_t DA16200_UartInit(void)
{
    fsp_err_t err;

    g_da16200_tx_semaphore = xSemaphoreCreateBinaryStatic(
        &g_da16200_tx_semaphore_control);
    g_da16200_rx_semaphore = xSemaphoreCreateBinaryStatic(
        &g_da16200_rx_semaphore_control);
    if((NULL == g_da16200_tx_semaphore) ||
       (NULL == g_da16200_rx_semaphore))
    {
        return FSP_ERR_OUT_OF_MEMORY;
    }

    RingBuffer_Init(&g_da16200_rx_ring);
    g_da16200_tx_done = false;
    g_da16200_rx_overflow = false;
    g_da16200_uart_error = FSP_SUCCESS;
    g_da16200_rx_drop_count = 0U;

    err = g_uart0.p_api->open(g_uart0.p_ctrl, g_uart0.p_cfg);
    if (FSP_SUCCESS != err)
    {
        g_printf("DA16200: UART open failed: %d\r\n", err);
        return err;
    }

    g_printf("DA16200: SCI0 opened, 115200-8-N-1\r\n");
    return FSP_SUCCESS;
}

/**
 * @brief 通过 SCI0 启动一次原始字节序列发送。
 * @param[in] p_data 待发送数据的首地址。
 * @param[in] length 待发送数据长度，单位为字节。
 * @return 成功启动发送时返回 FSP_SUCCESS，否则返回参数或 FSP UART 驱动错误码。
 * @note 本函数只启动异步发送，不等待 UART_EVENT_TX_COMPLETE；不可在中断中调用，当前不支持并发发送。
 */
static fsp_err_t da16200_send_raw(const uint8_t * p_data, uint16_t length)
{
    fsp_err_t err;

    if ((NULL == p_data) || (0U == length))
    {
        return FSP_ERR_ASSERTION;
    }

    g_da16200_tx_done = false;
    while(pdTRUE == xSemaphoreTake(g_da16200_tx_semaphore, 0U))
    {
        /* Drain a stale completion token before starting a new transfer. */
    }

    err = g_uart0.p_api->write(g_uart0.p_ctrl, p_data, length);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return FSP_SUCCESS;
}

/**
 * @brief 在关闭 SCI0 接收中断的临界区内清空 DA16200 接收环形缓冲区。
 * @param 无。
 * @return 无。
 * @note 仅在开始新的同步 AT 事务前调用；会丢弃尚未处理的异步数据，当前不支持并发调用。
 */
static void da16200_clear_rx_ring(void)
{
    R_BSP_IrqDisable(g_uart0_cfg.rxi_irq);
    RingBuffer_Clear(&g_da16200_rx_ring);
    R_BSP_IrqEnableNoClear(g_uart0_cfg.rxi_irq);

    while((NULL != g_da16200_rx_semaphore) &&
          (pdTRUE == xSemaphoreTake(g_da16200_rx_semaphore, 0U)))
    {
        /* Drain stale wakeups together with the stale bytes. */
    }
}

/**
 * @brief 从 DA16200 接收环形缓冲区中安全读取一个字节。
 * @param[out] p_byte 保存读取到的字节。
 * @return 成功读取一个字节时返回 true，当前没有数据时返回 false。
 * @note 通过短暂关闭 SCI0 RXI 中断保护环形缓冲区读索引；调用者必须传入有效指针。
 */
static bool da16200_read_rx_byte(uint8_t * p_byte)
{
    bool data_available;

    R_BSP_IrqDisable(g_uart0_cfg.rxi_irq);
    data_available = RingBuffer_Read(&g_da16200_rx_ring, p_byte);
    R_BSP_IrqEnableNoClear(g_uart0_cfg.rxi_irq);

    return data_available;
}

/**
 * @brief 检查响应字符串中是否存在独立且完整的终止行。
 * @param[in] p_response 以空字符结尾的响应字符串。
 * @param[in] p_terminal 待查找的终止行内容，例如 OK。
 * @return 找到独立终止行时返回 true，否则返回 false。
 * @note 本函数只读取字符串，不修改缓冲区；调用者必须保证两个指针有效。
 */
static bool da16200_response_has_terminal_line(const char * p_response,
                                               const char * p_terminal)
{
    const char * p_match = p_response;
    size_t terminal_length = strlen(p_terminal);

    while (NULL != (p_match = strstr(p_match, p_terminal)))
    {
        bool const line_start = (p_match == p_response) || ('\n' == p_match[-1]);
        char const following = p_match[terminal_length];
        bool const line_end = ('\0' == following) || ('\r' == following) || ('\n' == following);

        if (line_start && line_end)
        {
            return true;
        }

        p_match++;
    }

    return false;
}

/**
 * @brief 检查响应字符串中是否已经收到完整的 ERROR 行。
 * @param[in] p_response 以空字符结尾的响应字符串。
 * @return 收到以 ERROR 开头且以换行结束的完整错误行时返回 true，否则返回 false。
 * @note 本函数只进行字符串检查，不解析具体错误码；调用者必须传入有效指针。
 */
static bool da16200_response_has_complete_error_line(const char * p_response)
{
    const char * p_match = p_response;

    while (NULL != (p_match = strstr(p_match, "ERROR")))
    {
        bool const line_start = (p_match == p_response) || ('\n' == p_match[-1]);

        if (line_start && (NULL != strchr(p_match, '\n')))
        {
            return true;
        }

        p_match++;
    }

    return false;
}

/*
 *[@name] da16200_log_response_line
 *[@type] static function
 *[@usage] 仅输出响应中的指定行，避免把可能包含密码的完整CWJAPA响应写入RTT
 *[@argument] p_prefix 日志行前缀
 *[@argument] p_line 响应中目标行的首地址
 *[@return] none
 */
static void da16200_log_response_line(const char * p_prefix,
                                      const char * p_line)
{
    char safe_line[DA16200_STR_LEN_128] = {0};
    const char * p_line_end;
    size_t line_length;

    if((NULL == p_prefix) || (NULL == p_line))
    {
        return;
    }

    p_line_end = strchr(p_line, '\n');
    line_length = (NULL != p_line_end) ?
                  (size_t) (p_line_end - p_line) : strlen(p_line);
    if((line_length > 0U) && ('\r' == p_line[line_length - 1U]))
    {
        line_length--;
    }
    if(line_length >= sizeof(safe_line))
    {
        line_length = sizeof(safe_line) - 1U;
    }

    memcpy(safe_line, p_line, line_length);
    safe_line[line_length] = '\0';
    g_printf("%s%s\r\n", p_prefix, safe_line);
}

/**
 * @brief 发送一条 AT 指令，并阻塞等待完整的 OK 或 ERROR 响应。
 * @param[in] p_command 以回车换行结尾的 AT 指令字符串。
 * @param[out] p_response 保存模块原始响应的缓冲区。
 * @param[in] response_size 响应缓冲区容量，单位为字节且至少为 2。
 * @param[in] timeout_ms UART 发送和模块响应阶段的最大等待时间，单位为毫秒。
 * @return 收到 OK 时返回 FSP_SUCCESS，否则返回参数、超时、溢出、UART 或模块错误码。
 * @note 本函数会在事务开始前清空旧接收数据；不可在中断中调用，当前不支持并发 AT 事务。
 */
fsp_err_t DA16200_SendCommandAndGetResponse(const char * p_command,
                                            char * p_response,
                                            uint16_t response_size,
                                            uint32_t timeout_ms)
{
    uint8_t  received_byte;
    uint16_t response_length = 0U;
    uint32_t elapsed_ms = 0U;
    fsp_err_t err;

    if ((NULL == p_command) || (NULL == p_response) ||
        ('\0' == p_command[0]) || (response_size < 2U))
    {
        return FSP_ERR_ASSERTION;
    }

    p_response[0] = '\0';
    da16200_clear_rx_ring();
    g_da16200_rx_overflow = false;
    g_da16200_uart_error = FSP_SUCCESS;
    g_da16200_tx_done = false;
    g_da16200_rx_char_count = 0U;
    g_da16200_rx_drop_count = 0U;

    g_printf("TX: %s", p_command);
    err = da16200_send_raw((const uint8_t *) p_command, (uint16_t) strlen(p_command));
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = da16200_wait_tx_complete(timeout_ms);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    elapsed_ms = 0U;
    while (elapsed_ms < timeout_ms)
    {
        if (FSP_SUCCESS != g_da16200_uart_error)
        {
            return g_da16200_uart_error;
        }

        if (g_da16200_rx_overflow)
        {
            g_printf("RX RING OVERFLOW: dropped=%lu, received=%lu\r\n",
                     (unsigned long) g_da16200_rx_drop_count,
                     (unsigned long) g_da16200_rx_char_count);
            return FSP_ERR_RXBUF_OVERFLOW;
        }

        if (da16200_read_rx_byte(&received_byte))
        {

            if (response_length >= (response_size - 1U))
            {
                g_printf("RX BUFFER OVERFLOW: %s\r\n", p_response);
                return FSP_ERR_RXBUF_OVERFLOW;
            }

            p_response[response_length++] = (char) received_byte;
            p_response[response_length] = '\0';

            if (da16200_response_has_terminal_line(p_response, "OK"))
            {
                g_printf("RX: %s\r\n", p_response);
                return FSP_SUCCESS;
            }

            if (da16200_response_has_complete_error_line(p_response))
            {
                g_printf("RX ERROR: %s\r\n", p_response);
                return FSP_ERR_ASSERTION;
            }
        }
        else
        {
            da16200_wait_rx_activity(1U);
            elapsed_ms++;
        }
    }

    g_printf("RX TIMEOUT: %s [chars=%lu]\r\n",
             p_response,
             (unsigned long) g_da16200_rx_char_count);
    return FSP_ERR_TIMEOUT;
}

/**
 * @brief 发送一条 AT 指令，并检查成功响应中是否包含指定关键字。
 * @param[in] p_command 以回车换行结尾的 AT 指令字符串。
 * @param[in] p_expected 期望出现在完整响应中的非空字符串。
 * @param[in] timeout_ms 最大等待时间，单位为毫秒。
 * @return 收到 OK 且找到关键字时返回 FSP_SUCCESS，否则返回对应错误码。
 * @note 内部使用固定 512 字节响应缓冲区；不可在中断中调用，当前不支持并发 AT 事务。
 */
fsp_err_t DA16200_SendCommandAndWait(const char * p_command,
                                     const char * p_expected,
                                     uint32_t timeout_ms)
{
    char response[DA16200_STR_LEN_512] = {0};
    fsp_err_t err;

    if ((NULL == p_expected) || ('\0' == p_expected[0]))
    {
        return FSP_ERR_ASSERTION;
    }

    err = DA16200_SendCommandAndGetResponse(p_command, response, sizeof(response), timeout_ms);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return (NULL != strstr(response, p_expected)) ? FSP_SUCCESS : FSP_ERR_ASSERTION;
}

/**
 * @brief 查询 E103-W12 当前的 Wi-Fi 工作模式。
 * @param[out] p_mode 返回 Station、SoftAP 或 Station+SoftAP 模式枚举值。
 * @return 查询和解析成功时返回 FSP_SUCCESS，否则返回参数、通信或响应格式错误码。
 * @note 本函数执行阻塞式 AT 事务；不可在中断中调用，当前不支持并发调用。
 */
fsp_err_t DA16200_QueryWifiMode(da16200_wifi_mode_t * p_mode)
{
    char response[DA16200_STR_LEN_512] = {0};
    const char * p_mode_text;
    fsp_err_t err;

    if (NULL == p_mode)
    {
        return FSP_ERR_ASSERTION;
    }

    err = DA16200_SendCommandAndGetResponse("AT+CWMODE=?\r\n",
                                             response,
                                             sizeof(response),
                                             5000U);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    p_mode_text = strstr(response, "+CWMODE:");
    if ((NULL == p_mode_text) ||
        ((p_mode_text[8] < '0') || (p_mode_text[8] > '2')))
    {
        return FSP_ERR_ASSERTION;
    }

    *p_mode = (da16200_wifi_mode_t) (p_mode_text[8] - '0');
    return FSP_SUCCESS;
}

/**
 * @brief 设置 E103-W12 的 Wi-Fi 工作模式。
 * @param[in] mode 需要写入的 Station、SoftAP 或 Station+SoftAP 模式枚举值。
 * @return 模块接受设置时返回 FSP_SUCCESS，否则返回参数、通信或模块错误码。
 * @note 配置写入 NVRAM 后必须重启 DA16200 才能生效；不可在中断中调用。
 */
fsp_err_t DA16200_SetWifiMode(da16200_wifi_mode_t mode)
{
    const char * p_command;

    switch (mode)
    {
        case DA16200_WIFI_MODE_STA:
        {
            p_command = "AT+CWMODE=0\r\n";
            break;
        }

        case DA16200_WIFI_MODE_SOFT_AP:
        {
            p_command = "AT+CWMODE=1\r\n";
            break;
        }

        case DA16200_WIFI_MODE_STA_AP:
        {
            p_command = "AT+CWMODE=2\r\n";
            break;
        }

        default:
        {
            return FSP_ERR_ASSERTION;
        }
    }

    return DA16200_SendCommandAndWait(p_command,
                                      "OK",
                                      5000U);
}

/**
 * @brief 复位 DA16200，等待模块重新启动，并使用 AT 指令确认通信恢复。
 * @param 无。
 * @return 复位和通信确认均成功时返回 FSP_SUCCESS，否则返回对应错误码。
 * @note 本函数包含秒级阻塞延时；不可在中断中调用，当前不支持并发调用。
 */
fsp_err_t DA16200_ResetAndWaitReady(void)
{
    fsp_err_t err;
    err = DA16200_SendCommandAndWait("AT+RST\r\n", "OK", 5000U);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    da16200_task_delay_ms(3000U);

    return DA16200_SendCommandAndWait("AT\r\n", "OK", 5000U);
}

/**
 * @brief 配置 E103-W12 SoftAP 的 SSID、密码、信道和国家代码。
 * @param[in] p_cfg 指向 SoftAP 配置结构体，结构体及其字符串成员必须有效。
 * @return 参数合法且模块接受配置时返回 FSP_SUCCESS，否则返回对应错误码。
 * @note 配置内容会写入模块 NVRAM；函数不会输出密码，不可在中断中调用。
 */
fsp_err_t DA16200_ConfigSoftAp(const da16200_softap_cfg_t * p_cfg)
{
    //fsp_err_t err;
    char command[DA16200_STR_LEN_256] = {0};
    size_t ssid_length;
    size_t password_length;
    int command_length;

    if ((NULL == p_cfg) ||
        (NULL == p_cfg->p_ssid) ||
        (NULL == p_cfg->p_password) ||
        (NULL == p_cfg->p_country))
    {
        return FSP_ERR_ASSERTION;
    }

    ssid_length = strlen(p_cfg->p_ssid);
    password_length = strlen(p_cfg->p_password);

    if ((0U == ssid_length) || (ssid_length > 32U) ||
        (password_length < 8U) || (password_length > 63U) ||
        (strlen(p_cfg->p_country) != 2U))
    {
        return FSP_ERR_ASSERTION;
    }

    if ((NULL != strchr(p_cfg->p_ssid, ',')) ||
        (NULL != strchr(p_cfg->p_ssid, '\'')) ||
        (NULL != strchr(p_cfg->p_password, ',')) ||
        (NULL != strchr(p_cfg->p_password, '\'')))
    {
        return FSP_ERR_ASSERTION;
    }

    command_length = snprintf(command,
                              sizeof(command),
                              "AT+CWSAP=%s,3,1,%s,%u,%s\r\n",
                              p_cfg->p_ssid,
                              p_cfg->p_password,
                              (unsigned int) p_cfg->channel,
                              p_cfg->p_country);

    if ((command_length < 0) || ((size_t) command_length >= sizeof(command)))
    {
        return FSP_ERR_ASSERTION;
    }

    return DA16200_SendCommandAndWait(command, "+CWSAP", 5000U);
}

/**
 * @brief 使用已经保存的 SoftAP 配置启动无线热点。
 * @param 无。
 * @return 模块返回 OK 时返回 FSP_SUCCESS，否则返回通信或模块错误码。
 * @note 本函数执行阻塞式 AT 事务；不可在中断中调用，热点已启动时模块可能返回错误。
 */
fsp_err_t DA16200_StartSoftAp(void)
{
    return DA16200_SendCommandAndWait("AT+CWOAP\r\n", "OK", 5000U);
}

/**
 * @brief 非阻塞读取 DA16200 接收环形缓冲区中的异步数据。
 * @param[out] p_data 保存读取结果的调用者缓冲区。
 * @param[in] capacity 最多允许读取的字节数。
 * @return 实际读取的字节数；参数无效或当前没有数据时返回 0。
 * @note 本函数不添加字符串结束符；读取期间短暂关闭 RXI 中断，不可与同步 AT 事务并发使用。
 */
size_t DA16200_ReadAsync(uint8_t * p_data, size_t capacity)
{
    size_t length = 0U;

    if ((NULL == p_data) || (0U == capacity))
    {
        return 0U;
    }

    while ((length < capacity) &&
           da16200_read_rx_byte(&p_data[length]))
    {
        length++;
    }

    return length;
}

/**
 * @brief 阻塞等待 SCI0 产生 UART_EVENT_TX_COMPLETE 发送完成事件。
 * @param[in] timeout_ms 最大等待时间，单位为毫秒。
 * @return 发送完成时返回 FSP_SUCCESS，否则返回 UART 或超时错误码。
 * @note 必须在 da16200_send_raw() 成功后调用；不可在中断中调用，当前不支持并发发送。
 */
static fsp_err_t da16200_wait_tx_complete(uint32_t timeout_ms)
{
    TickType_t wait_ticks = pdMS_TO_TICKS(timeout_ms);

    if((0U == timeout_ms) || (NULL == g_da16200_tx_semaphore))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if(0U == wait_ticks)
    {
        wait_ticks = 1U;
    }

    if((!g_da16200_tx_done) &&
       (pdTRUE != xSemaphoreTake(g_da16200_tx_semaphore, wait_ticks)))
    {
        return (FSP_SUCCESS != g_da16200_uart_error) ?
               g_da16200_uart_error : FSP_ERR_TIMEOUT;
    }

    return (FSP_SUCCESS != g_da16200_uart_error) ?
           g_da16200_uart_error : FSP_SUCCESS;
}

/**
 * @brief 等待 DA16200 返回独立的 OK 或 ERROR 响应行。
 * @param[in] timeout_ms 最大等待时间，单位为毫秒。
 * @return 收到 OK 时返回 FSP_SUCCESS；收到 ERROR、UART 错误、缓冲区溢出
 *         或发生超时时返回对应错误码。
 * @note 本函数只读取现有接收环形缓冲区，不发送数据，也不清空环形缓冲区；
 *       只能在线程或主循环上下文调用，不可在中断中调用，当前不支持并发调用。
 */
static fsp_err_t da16200_wait_response_ok(uint32_t timeout_ms)
{
    char response[DA16200_STR_LEN_128] = {0};
    uint8_t received_byte;
    uint16_t response_length = 0U;
    uint32_t elapsed_ms = 0U;

    if (0U == timeout_ms)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    while (elapsed_ms < timeout_ms)
    {
        if (FSP_SUCCESS != g_da16200_uart_error)
        {
            return g_da16200_uart_error;
        }

        if (g_da16200_rx_overflow)
        {
            g_printf("DA16200: RX ring overflow while waiting response\r\n");
            return FSP_ERR_RXBUF_OVERFLOW;
        }

        if (da16200_read_rx_byte(&received_byte))
        {
            if (response_length >=
                ((uint16_t) sizeof(response) - 1U))
            {
                g_printf("DA16200: response buffer overflow\r\n");
                return FSP_ERR_RXBUF_OVERFLOW;
            }

            response[response_length] = (char) received_byte;
            response_length++;
            response[response_length] = '\0';

            if (da16200_response_has_terminal_line(response, "OK"))
            {
                return FSP_SUCCESS;
            }

            if (da16200_response_has_complete_error_line(response))
            {
                g_printf("DA16200: module returned error: %s\r\n", response);
                return FSP_ERR_ASSERTION;
            }
        }
        else
        {
            da16200_wait_rx_activity(1U);
            elapsed_ms++;
        }
    }

    g_printf("DA16200: response timeout, received=%s\r\n",
             response);
    return FSP_ERR_TIMEOUT;
}

/**
 * @brief 通过 E103-W12 TCP Server 会话向指定远端客户端发送短文本。
 * @param[in] p_remote_ip 已连接 TCP 客户端的 IPv4 地址字符串。
 * @param[in] remote_port 已连接 TCP 客户端的端口号。
 * @param[in] p_text 不包含逗号、回车和换行的非空短文本。
 * @return 成功启动完整命令发送时返回 FSP_SUCCESS，否则返回参数或 UART 错误码。
 * @note 固定使用 CID 0，仅适用于模块作为 TCP Server；当前只等待 UART 发送完成，不等待模块 OK。
 */
fsp_err_t DA16200_TcpServerSendText(const char * p_remote_ip,
                                    uint16_t remote_port,
                                    const char * p_text)
{
    char command[DA16200_STR_LEN_256] = {0};
    size_t text_length;
    int command_length;
    fsp_err_t err;

    if ((NULL == p_remote_ip) || (NULL == p_text) ||
        ('\0' == p_remote_ip[0]) || ('\0' == p_text[0]) ||
        (0U == remote_port))
    {
        return FSP_ERR_ASSERTION;
    }

    text_length = strlen(p_text);

    if ((text_length > 128U) ||
        (NULL != strchr(p_text, ',')) ||
        (NULL != strchr(p_text, '\r')) ||
        (NULL != strchr(p_text, '\n')))
    {
        return FSP_ERR_ASSERTION;
    }

    command_length = snprintf(command,
                              sizeof(command),
                              "AT+CIPSEND=0,%u,%s,%u,%s\r\n",
                              (unsigned int) text_length,
                              p_remote_ip,
                              (unsigned int) remote_port,
                              p_text);

    if ((command_length < 0) || ((size_t) command_length >= sizeof(command)))
    {
        return FSP_ERR_ASSERTION;
    }

    g_printf("TCP TX to %s:%u: %s\r\n",
             p_remote_ip,
             (unsigned int) remote_port,
             p_text);

    err = da16200_send_raw((const uint8_t *) command,
                           (uint16_t) command_length);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return da16200_wait_tx_complete(1000U);
}        


/**
 * @brief 连接指定 Wi-Fi 热点并等待 Station 接口进入已连接状态。
 * @param[in] p_ssid 目标热点 SSID 字符串。
 * @param[in] p_password WPA/WPA2 热点密码字符串。
 * @param[in] timeout_ms 等待联网完成的最大时间，单位为毫秒。
 * @return 成功联网时返回 FSP_SUCCESS，否则返回参数、UART、模块响应或超时错误码。
 * @note 本函数包含阻塞式 AT 事务；不会输出密码，不可在中断中调用，当前不支持并发调用。
 */
fsp_err_t DA16200_ConnectWifi(const char * p_ssid,
                              const char * p_password,
                              uint32_t timeout_ms)
{
    char command[DA16200_STR_LEN_256] = {0};
    char response[DA16200_STR_LEN_512] = {0};
    const char * p_result;
    size_t ssid_length;
    size_t password_length;
    uint16_t response_length = 0U;
    uint32_t elapsed_ms = 0U;
    uint8_t received_byte;
    int command_length;
    fsp_err_t err;

    if(NULL == p_ssid || NULL == p_password || timeout_ms == 0U)
    {
        return FSP_ERR_ASSERTION;
    }

/*固件只能处理WPA/WPA2热点
*SSID长度32字节，密码8~63字节
*/
    ssid_length = strlen(p_ssid);
    password_length = strlen(p_password);
    if ((0U == ssid_length) ||
        (ssid_length > 32U) ||
        (password_length < 8U) ||
        (password_length > 63U))
    {
        return FSP_ERR_ASSERTION;
    }

/*
* 逗号是 AT 指令参数分隔符。
* 回车和换行会提前终止 AT 指令。
*/
    if ((NULL != strchr(p_ssid, ',')) ||
        (NULL != strchr(p_ssid, '\r')) ||
        (NULL != strchr(p_ssid, '\n')) ||
        (NULL != strchr(p_password, ',')) ||
        (NULL != strchr(p_password, '\r')) ||
        (NULL != strchr(p_password, '\n')))
    {
        return FSP_ERR_ASSERTION;
    }

    command_length = snprintf(command,
                              sizeof(command),
                              "AT+CWJAPA=%s,%s\r\n",
                              p_ssid,
                              p_password);    

if ((command_length < 0) ||
        ((size_t) command_length >= sizeof(command)))
    {
        return FSP_ERR_ASSERTION;
    }

    /*
     * 清除上一条命令留下的接收数据和状态。
     */
    da16200_clear_rx_ring();
    g_da16200_rx_overflow = false;
    g_da16200_uart_error = FSP_SUCCESS;
    g_da16200_tx_done = false;
    g_da16200_rx_char_count = 0U;
    g_da16200_rx_drop_count = 0U;

    /*
     * 日志只显示 SSID
     */
    g_printf("TX: AT+CWJAPA=%s,<password hidden>\r\n", p_ssid);

    err = da16200_send_raw((const uint8_t *) command,
                           (uint16_t) command_length);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = da16200_wait_tx_complete(1000U);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /*
     * 继续读取异步结果。
     * 最初收到的 OK 只表示命令已被接受。
     */
    while (elapsed_ms < timeout_ms)
    {
        if (FSP_SUCCESS != g_da16200_uart_error)
        {
            return g_da16200_uart_error;
        }

        if (g_da16200_rx_overflow)
        {
            return FSP_ERR_RXBUF_OVERFLOW;
        }

        if (da16200_read_rx_byte(&received_byte))
        {
            if (response_length >= (sizeof(response) - 1U))
            {
                return FSP_ERR_RXBUF_OVERFLOW;
            }

            response[response_length++] = (char) received_byte;
            response[response_length] = '\0';

            /*
             * 等待成功结果所在行完整接收。
             */
            p_result = strstr(response, "+CWJAP:1");
            if ((NULL != p_result) &&
                (NULL != strchr(p_result, '\n')))
            {
                g_printf("DA16200: Wi-Fi connected\r\n");
                return FSP_SUCCESS;
            }

            /*
             * +CWJAP:0 表示连接过程失败。
             */
            p_result = strstr(response, "+CWJAP:0");
            if ((NULL != p_result) &&
                (NULL != strchr(p_result, '\n')))
            {
                da16200_log_response_line(
                    "DA16200: Wi-Fi connection failed: ",
                    p_result);
                return FSP_ERR_ASSERTION;
            }

            /*
             * ERROR 表示命令格式、热点或认证等阶段出错。
             */
            if (da16200_response_has_complete_error_line(response))
            {
                p_result = strstr(response, "ERROR");
                da16200_log_response_line(
                    "DA16200: Wi-Fi command rejected: ",
                    p_result);
                return FSP_ERR_ASSERTION;
            }
        }
        else
        {
            da16200_wait_rx_activity(1U);
            elapsed_ms++;
        }
    }

    g_printf("DA16200: Wi-Fi connection timeout\r\n");
    return FSP_ERR_TIMEOUT;
}

/**
 * @brief 查询 DA16200 Station 接口是否已经连接 Wi-Fi。
 * @param[out] p_connected 查询成功后返回连接状态。
 * @return 查询和解析成功时返回 FSP_SUCCESS，否则返回参数、通信或响应格式错误码。
 * @note 本函数执行阻塞式 AT 事务；不可在中断中调用，当前不支持并发调用。
 */
fsp_err_t DA16200_QueryStaConnected(bool * p_connected)
{
    char response[DA16200_STR_LEN_128] = {0};
    const char * p_status;
    fsp_err_t err;

    if(NULL == p_connected)
    {
        return FSP_ERR_ASSERTION;
    }

    *p_connected = false;

    err = DA16200_SendCommandAndGetResponse("AT+CWSTA\r\n",
                                            response,
                                            (uint16_t) sizeof(response),
                                            5000U);

    if (FSP_SUCCESS != err)
    {
        return err;
    }

    p_status = strstr(response, "+CWSTA:");
    if ((NULL == p_status) ||
        (('0' != p_status[7]) && ('1' != p_status[7])))
    {
        return FSP_ERR_ASSERTION;
    }

    *p_connected = ('1' == p_status[7]);
    return FSP_SUCCESS;
}

/**
 * @brief 确保 DA16200 最终处于 Wi-Fi 已连接状态。
 * @param[in] p_ssid 目标热点 SSID 字符串。
 * @param[in] p_password 目标热点密码字符串。
 * @param[in] timeout_ms 首次联网操作的最大等待时间，单位为毫秒。
 * @return 确认 CWSTA=1 时返回 FSP_SUCCESS，否则返回查询、联网或超时错误码。
 * @note 已连接时直接返回；函数包含阻塞查询和延时，不可在中断中调用，也不会输出密码。
 */
fsp_err_t DA16200_EnsureWifiConnected(const char * p_ssid,
                                      const char * p_password,
                                      uint32_t timeout_ms)
{
    bool connected =  false;
    fsp_err_t err;

    // 查询当前连接状态
    err = DA16200_QueryStaConnected(&connected);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    if(connected)
    {
        return FSP_SUCCESS;
    }
    
    // 尝试连接 Wi-Fi
    err = DA16200_ConnectWifi(p_ssid, p_password, timeout_ms);
    if (FSP_SUCCESS != err)
    {
        g_printf("DA16200: Wi-Fi join failed, err=%d\r\n", err);
        return err;
    }

    // 等待 CWSTA=1，最多尝试 5 次，每次间隔 500ms
    for (uint32_t attempt = 0U; attempt < 5U; attempt++)
    {
        err = DA16200_QueryStaConnected(&connected);
        
        // 查询失败或尚未连接时，延时后继续重试
        if ((FSP_SUCCESS == err) && connected)
        {
            g_printf("DA16200: Wi-Fi connection verified\r\n");
            return FSP_SUCCESS;
        }

        da16200_task_delay_ms(500U);
    }

    // 查询超时
    g_printf("DA16200: CWSTA verification timeout\r\n");
    return FSP_ERR_TIMEOUT;
}

/**
 * @brief 从 TCP 客户端建立连接的响应中解析 DA16200 分配的 CID。
 * @param[in] p_response 以空字符结尾的模块响应字符串。
 * @param[out] p_cid 返回解析得到的 0 至 7 会话编号。
 * @return 找到合法的 +TRTC 或 +CIPSTART 响应时返回 true，否则返回 false。
 * @note 本函数只读取响应字符串，不修改模块状态；调用者必须保证两个指针有效。
 */
static bool da16200_parse_tcp_client_cid(const char * p_response,
                                         uint8_t * p_cid)
{
    const char * p_value;
    if ((NULL == p_response) || (NULL == p_cid))
    {
        return false;
    }

    p_value = strstr(p_response, "+TRTC:");
    if(NULL != p_value)
    {
         p_value += strlen("+TRTC:");
    }
    else
    {
        p_value = strstr(p_response, "+CIPSTART:");
        if (NULL == p_value)
        {
            return false;
        }

        p_value += strlen("+CIPSTART:");
    }

    /*
     * DA16200 最多支持八个套接字，因此有效 CID 为 0～7。
     */
    if ((p_value[0] < '0') || (p_value[0] > '7'))
    {
        return false;
    }

    *p_cid = (uint8_t) (p_value[0] - '0');
    return true;
}

/**
 * @brief 建立到局域网 TCP 服务端的客户端连接并取得模块分配的 CID。
 * @param[in] p_server_ip TCP 服务端的 IPv4 地址字符串。
 * @param[in] server_port TCP 服务端监听端口。
 * @param[out] p_cid 返回 DA16200 分配的会话编号。
 * @return 连接成功并解析到 CID 时返回 FSP_SUCCESS，否则返回参数、通信、格式或超时错误码。
 * @note 本函数执行阻塞式 AT 事务；不可在中断中调用，失败时 p_cid 保持为 0xFF。
 */
fsp_err_t DA16200_TcpClientOpen(const char * p_server_ip,
                                uint16_t server_port,
                                uint8_t * p_cid)
{
    char command[DA16200_STR_LEN_128] = {0};
    char response[DA16200_STR_LEN_512] = {0};
    uint16_t response_length = 0U;
    uint32_t elapsed_ms = 0U;
    uint8_t received_byte;
    int command_length;
    fsp_err_t err;

    if ((NULL == p_server_ip) ||
        (NULL == p_cid) ||
        ('\0' == p_server_ip[0]) ||
        (0U == server_port))
    {
        return FSP_ERR_ASSERTION;
    }

    if ((NULL != strchr(p_server_ip, ',')) ||
        (NULL != strchr(p_server_ip, '\r')) ||
        (NULL != strchr(p_server_ip, '\n')))
    {
        return FSP_ERR_ASSERTION;
    }

    *p_cid = 0xFFU;

    // 生成 AT+CIPSTART=<IP>,<端口>,0
    command_length = snprintf(command,
                              sizeof(command),
                              "AT+CIPSTART=%s,%u,0\r\n",
                              p_server_ip,
                              (unsigned int) server_port);
    if ((command_length < 0) ||
        ((size_t) command_length >= sizeof(command)))
    {
        return FSP_ERR_ASSERTION;
    }

    err = DA16200_SendCommandAndGetResponse(command,
                                            response,
                                            (uint16_t) sizeof(response),
                                            10000U);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    if (da16200_parse_tcp_client_cid(response, p_cid))
    {
        g_printf("DA16200: TCP client connected, CID=%u\r\n",
                (unsigned int) *p_cid);
        return FSP_SUCCESS;
    }

    memset(response, 0, sizeof(response));
    response_length = 0U;

    while (elapsed_ms < 5000U)
    {
        if (FSP_SUCCESS != g_da16200_uart_error)
        {
            return g_da16200_uart_error;
        }

        if (g_da16200_rx_overflow)
        {
            return FSP_ERR_RXBUF_OVERFLOW;
        }

        if (da16200_read_rx_byte(&received_byte))
        {
            if (response_length >= (sizeof(response) - 1U))
            {
                return FSP_ERR_RXBUF_OVERFLOW;
            }

            response[response_length++] = (char) received_byte;
            response[response_length] = '\0';

            if (da16200_parse_tcp_client_cid(response, p_cid))
            {
                g_printf("DA16200: TCP client connected, CID=%u\r\n",
                         (unsigned int) *p_cid);
                return FSP_SUCCESS;
            }
        }
        else
        {
            da16200_wait_rx_activity(1U);
            elapsed_ms++;
        }
    }

    g_printf("DA16200: TCP client CID timeout\r\n");
    return FSP_ERR_TIMEOUT;

}

/**
 * @brief 关闭 DA16200 当前保存的全部 socket 会话。
 * @param 无。
 * @return 模块返回 OK 时返回 FSP_SUCCESS，否则返回通信或模块错误码。
 * @note 本函数执行阻塞式 AT 事务；只能在线程或主循环上下文调用，当前不支持并发调用。
 */
fsp_err_t DA16200_TcpCloseAll(void)
{
    char response[DA16200_STR_LEN_128] = {0};
    fsp_err_t err;

    err = DA16200_SendCommandAndGetResponse("AT+CIPCLOSEALL\r\n",
                                            response,
                                            (uint16_t) sizeof(response),
                                            5000U);
    if (FSP_SUCCESS != err)
    {
        g_printf("DA16200: close all sockets failed, err=%d\r\n",
                 (int) err);
        return err;
    }

    g_printf("DA16200: all sockets closed\r\n");
    return FSP_SUCCESS;
}
/**
 * @brief 通过已经建立的 TCP Client 会话发送短文本。
 * @param[in] cid DA16200_TcpClientOpen() 返回的会话编号。
 * @param[in] p_text 不包含逗号、回车和换行的非空短文本。
 * @return 成功启动完整命令发送时返回 FSP_SUCCESS，否则返回参数或 UART 错误码。
 * @note 当前只等待 UART 发送完成，不等待模块 OK；只适用于短文本测试，不适用于二进制图像。
 */
fsp_err_t DA16200_TcpClientSendText(uint8_t cid,
                                    const char * p_text)
{
    char command[DA16200_STR_LEN_256] = {0};
    size_t text_length;
    int command_length;
    fsp_err_t err;

    if ((cid > 7U) ||
        (NULL == p_text) ||
        ('\0' == p_text[0]))
    {
        return FSP_ERR_ASSERTION;
    }

    text_length = strlen(p_text);
    if ((text_length > 128U) ||
        (NULL != strchr(p_text, ',')) ||
        (NULL != strchr(p_text, '\r')) ||
        (NULL != strchr(p_text, '\n')))
    {
        return FSP_ERR_ASSERTION;
    }

    command_length = snprintf(command,
                              sizeof(command),
                              "AT+CIPSEND=%u,%u,0,0,%s\r\n",
                              (unsigned int) cid,
                              (unsigned int) text_length,
                              p_text);
    if ((command_length < 0) ||
        ((size_t) command_length >= sizeof(command)))
    {
        return FSP_ERR_ASSERTION;
    }

    err = da16200_send_raw((const uint8_t *) command,
                           (uint16_t) command_length);   
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return da16200_wait_tx_complete(1000U);
}


/**
 * @brief 处理 SCI0 UART 接收、发送完成和错误事件。
 * @param[in] p_args FSP UART 驱动传入的事件参数。
 * @return 无。
 * @note 本函数运行在中断上下文；只写入环形缓冲区或更新状态标志，不执行阻塞操作。
 */
void UART0_CallBack(uart_callback_args_t *p_args)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (NULL == p_args)
        {
            return;
        }

    switch (p_args->event)
    {
        case UART_EVENT_RX_CHAR:
        {
            g_da16200_rx_char_count++;
            if (!RingBuffer_Write(&g_da16200_rx_ring, (uint8_t) p_args->data))
            {
                g_da16200_rx_overflow = true;
                g_da16200_rx_drop_count++;
            }
            else if(NULL != g_da16200_rx_semaphore)
            {
                (void) xSemaphoreGiveFromISR(g_da16200_rx_semaphore,
                                             &higher_priority_task_woken);
            }
            break;
        }

        case UART_EVENT_TX_COMPLETE:
        {
            g_da16200_tx_done = true;
            if(NULL != g_da16200_tx_semaphore)
            {
                (void) xSemaphoreGiveFromISR(g_da16200_tx_semaphore,
                                             &higher_priority_task_woken);
            }
            break;
        }

        case UART_EVENT_ERR_PARITY:
        case UART_EVENT_ERR_FRAMING:
        case UART_EVENT_ERR_OVERFLOW:
        {
            g_da16200_uart_error = FSP_ERR_ASSERTION;
            if(NULL != g_da16200_tx_semaphore)
            {
                (void) xSemaphoreGiveFromISR(g_da16200_tx_semaphore,
                                             &higher_priority_task_woken);
            }
            if(NULL != g_da16200_rx_semaphore)
            {
                (void) xSemaphoreGiveFromISR(g_da16200_rx_semaphore,
                                             &higher_priority_task_woken);
            }
            break;
        }

        default:
        {
            break;
        }
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/**
 * @brief 通过已建立的 DA16200 TCP Client 会话发送一个二进制数据块。
 * @param[in] cid DA16200_TcpClientOpen() 返回的 TCP 会话编号。
 * @param[in] p_data 待发送二进制数据首地址。
 * @param[in] data_length 本次发送的实际字节数。
 * @param[in] timeout_ms 每个 UART 发送或模块响应阶段的最大等待时间，单位为毫秒。
 * @return 两阶段发送均成功并分别收到 OK 时返回 FSP_SUCCESS，否则返回对应错误码。
 * @note 严格使用手册规定的 0x1B + H 两阶段发送格式；本函数为阻塞调用，
 *       不可在中断中调用，当前不支持并发调用。
 */
fsp_err_t DA16200_TcpClientSendBinaryChunk(
    uint8_t cid,
    const uint8_t * p_data,
    uint16_t data_length,
    uint32_t timeout_ms)
{
    uint8_t control_header[DA16200_STR_LEN_32] = {0};
    int text_length;
    uint16_t control_header_length;

    fsp_err_t err;

        if ((cid > 7U) ||
        (NULL == p_data) ||
        (0U == data_length) ||
        (0U == timeout_ms))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /*
     *  <ESC> 是控制字节 0x1B
     */
    control_header[0] = 0x1BU;

    /*
     * 从第二个字节开始生成，并按照手册统一要求以 CRLF 结束：
     * H<CID>,<length>,0,0\r\n
     */
    text_length = snprintf(
        (char *) &control_header[1],
        sizeof(control_header) - 1U,
        "H%u,%u,0,0\r\n",
        (unsigned int) cid,
        (unsigned int) data_length);

    if ((text_length < 0) ||
        ((size_t) text_length >=
         (sizeof(control_header) - 1U)))
    {
        return FSP_ERR_INVALID_SIZE;
    }

    control_header_length = (uint16_t) ((uint16_t) text_length + 1U);

    /*
     * 只在完整事务开始前清理旧响应
     * 数据发送后的第二个 OK 不能被清除
    */
    da16200_clear_rx_ring();

    g_da16200_rx_overflow = false;
    g_da16200_uart_error  = FSP_SUCCESS;
    g_da16200_rx_char_count = 0U;
    g_da16200_rx_drop_count = 0U;

    /*
     * 第一阶段：发送 0x1B + H 控制头
     */
    err = da16200_send_raw(control_header,control_header_length);

    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = da16200_wait_tx_complete(timeout_ms);
    if (FSP_SUCCESS != err)
    {
        g_printf("DA16200: binary control header TX failed, err=%d\r\n",(int) err);
        return err;
    }

    /*
     * 必须收到第一个 OK 后才能发送二进制载荷
     */
    err = da16200_wait_response_ok(timeout_ms);
    if (FSP_SUCCESS != err)
    {
        g_printf("DA16200: binary request rejected, err=%d\r\n",(int) err);
        return err;
    }

    /*
     * 第二阶段：发送恰好 data_length 字节
     * 二进制数据中的 0x00、0xFF 和换行符都不会改变发送长度
     */
    err = da16200_send_raw(p_data, data_length);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = da16200_wait_tx_complete(timeout_ms);
    if (FSP_SUCCESS != err)
    {
        g_printf("DA16200: binary payload TX failed, err=%d\r\n",(int) err);
        return err;
    }

    /*
     * 等待模块处理完本块数据后的第二个 OK
     */
    err = da16200_wait_response_ok(timeout_ms);
    if (FSP_SUCCESS != err)
    {
        g_printf( "DA16200: binary payload not confirmed, err=%d\r\n",(int) err);
        return err;
    }

    return FSP_SUCCESS;
}
