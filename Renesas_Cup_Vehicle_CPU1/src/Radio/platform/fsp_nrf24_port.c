#include "Radio/platform/fsp_nrf24_port.h"

#include "command_rx_thread.h"
#include "video_tx_thread.h"
#include "common_data.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "SEGGER_RTT/bsp_print.h"

#define NRF24_SPI_TIMEOUT_MS       (10U)
#define NRF24_POWER_UP_DELAY_MS    (100U)

typedef struct st_fsp_nrf24_context
{
    spi_instance_t const * p_spi;
    bsp_io_port_pin_t ce_pin;
    bsp_io_port_pin_t irq_pin;
    SemaphoreHandle_t transfer_done;
    StaticSemaphore_t transfer_done_storage;
    volatile spi_event_t transfer_event;
    bool opened;
} fsp_nrf24_context_t;

/*
 * SPI0 使用 P700/P701/P702/P703，CE=P704，IRQ=P705/IRQ19。
 * SPI1 使用 P100/P101/P102/P103，CE=P104；发射端不使用 IRQ。
 * CSN 由 SPI 外设的 SSLA0/SSLB0 自动控制，因此不在此处手工翻转。
 */
static fsp_nrf24_context_t g_ports[NRF24_PORT_COUNT] =
{
    [NRF24_PORT_COMMAND_RX] =
    {
        .p_spi   = &g_spi0,
        .ce_pin = BSP_IO_PORT_07_PIN_04,
        .irq_pin = BSP_IO_PORT_07_PIN_05,
    },
    [NRF24_PORT_VIDEO_TX] =
    {
        .p_spi   = &g_spi1,
        .ce_pin = BSP_IO_PORT_01_PIN_04,
        .irq_pin = BSP_IO_PORT_07_PIN_05,
    },
};

static StaticSemaphore_t g_command_irq_storage;
static SemaphoreHandle_t g_command_irq;

static nrf24_result_t transfer(void * p_context,
                               uint8_t const * p_tx,
                               uint8_t * p_rx,
                               uint32_t length)
{
    fsp_nrf24_context_t * p_port = (fsp_nrf24_context_t *) p_context;

    if ((NULL == p_port) || (NULL == p_tx) || (NULL == p_rx) || (0U == length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    /* 清掉上一次残留完成信号，再启动本次异步 SPI 事务。 */
    (void) xSemaphoreTake(p_port->transfer_done, 0U);
    p_port->transfer_event = (spi_event_t) 0;

    fsp_err_t err = p_port->p_spi->p_api->writeRead(p_port->p_spi->p_ctrl,
                                                     p_tx,
                                                     p_rx,
                                                     length,
                                                     SPI_BIT_WIDTH_8_BITS);
    if (FSP_SUCCESS != err)
    {
        g_printf("[NRF PORT%u] spi-start failed fsp=%u\r\n",
                 (uint32_t) (p_port - &g_ports[0]),
                 (uint32_t) err);
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    if (pdTRUE != xSemaphoreTake(p_port->transfer_done,
                                 pdMS_TO_TICKS(NRF24_SPI_TIMEOUT_MS)))
    {
        g_printf("[NRF PORT%u] spi-timeout after %u ms\r\n",
                 (uint32_t) (p_port - &g_ports[0]),
                 NRF24_SPI_TIMEOUT_MS);
        return NRF24_RESULT_TIMEOUT;
    }

    if (SPI_EVENT_TRANSFER_COMPLETE != p_port->transfer_event)
    {
        g_printf("[NRF PORT%u] spi-event=%u\r\n",
                 (uint32_t) (p_port - &g_ports[0]),
                 (uint32_t) p_port->transfer_event);
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    return NRF24_RESULT_SUCCESS;
}

static nrf24_result_t ce_write(void * p_context, bool high)
{
    fsp_nrf24_context_t const * p_port = (fsp_nrf24_context_t const *) p_context;

    if (NULL == p_port)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl,
                                      p_port->ce_pin,
                                      high ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    return (FSP_SUCCESS == err) ? NRF24_RESULT_SUCCESS : NRF24_RESULT_TRANSPORT_ERROR;
}

static nrf24_result_t irq_read(void * p_context, bool * p_active)
{
    fsp_nrf24_context_t const * p_port = (fsp_nrf24_context_t const *) p_context;
    bsp_io_level_t level = BSP_IO_LEVEL_HIGH;

    if ((NULL == p_port) || (NULL == p_active))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    fsp_err_t err = R_IOPORT_PinRead(&g_ioport_ctrl, p_port->irq_pin, &level);
    *p_active = (BSP_IO_LEVEL_LOW == level);
    return (FSP_SUCCESS == err) ? NRF24_RESULT_SUCCESS : NRF24_RESULT_TRANSPORT_ERROR;
}

static void delay_us(void * p_context, uint32_t delay)
{
    FSP_PARAMETER_NOT_USED(p_context);
    R_BSP_SoftwareDelay(delay, BSP_DELAY_UNITS_MICROSECONDS);
}

static void delay_ms(void * p_context, uint32_t delay)
{
    FSP_PARAMETER_NOT_USED(p_context);

    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState())
    {
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
    else
    {
        R_BSP_SoftwareDelay(delay, BSP_DELAY_UNITS_MILLISECONDS);
    }
}

nrf24_result_t FspNrf24Port_Open(nrf24_port_id_t port_id,
                                 nrf24_transport_t * p_transport)
{
    if ((port_id >= NRF24_PORT_COUNT) || (NULL == p_transport))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    fsp_nrf24_context_t * p_port = &g_ports[port_id];
    if (!p_port->opened)
    {
        p_port->transfer_done = xSemaphoreCreateBinaryStatic(&p_port->transfer_done_storage);
        if (NULL == p_port->transfer_done)
        {
            return NRF24_RESULT_TRANSPORT_ERROR;
        }

        fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl,
                                          p_port->ce_pin,
                                          BSP_IO_LEVEL_LOW);
        if (FSP_SUCCESS != err)
        {
            g_printf("[NRF PORT%u] ce-low failed fsp=%u\r\n",
                     (uint32_t) port_id,
                     (uint32_t) err);
            return NRF24_RESULT_TRANSPORT_ERROR;
        }

        err = p_port->p_spi->p_api->open(p_port->p_spi->p_ctrl,
                                         p_port->p_spi->p_cfg);
        if ((FSP_SUCCESS != err) && (FSP_ERR_ALREADY_OPEN != err))
        {
            g_printf("[NRF PORT%u] spi-open failed fsp=%u\r\n",
                     (uint32_t) port_id,
                     (uint32_t) err);
            return NRF24_RESULT_TRANSPORT_ERROR;
        }

        if (NRF24_PORT_COMMAND_RX == port_id)
        {
            g_command_irq = xSemaphoreCreateBinaryStatic(&g_command_irq_storage);
            if (NULL == g_command_irq)
            {
                return NRF24_RESULT_TRANSPORT_ERROR;
            }

            err = g_external_irq19.p_api->open(g_external_irq19.p_ctrl,
                                                g_external_irq19.p_cfg);
            if ((FSP_SUCCESS != err) && (FSP_ERR_ALREADY_OPEN != err))
            {
                g_printf("[NRF PORT%u] irq-open failed fsp=%u\r\n",
                         (uint32_t) port_id,
                         (uint32_t) err);
                return NRF24_RESULT_TRANSPORT_ERROR;
            }
            err = g_external_irq19.p_api->enable(g_external_irq19.p_ctrl);
            if (FSP_SUCCESS != err)
            {
                g_printf("[NRF PORT%u] irq-enable failed fsp=%u\r\n",
                         (uint32_t) port_id,
                         (uint32_t) err);
                return NRF24_RESULT_TRANSPORT_ERROR;
            }
        }

        p_port->opened = true;
        delay_ms(p_port, NRF24_POWER_UP_DELAY_MS);
    }

    p_transport->p_context = p_port;
    p_transport->transfer = transfer;
    p_transport->ce_write = ce_write;
    p_transport->irq_read = irq_read;
    p_transport->delay_us = delay_us;
    p_transport->delay_ms = delay_ms;
    return NRF24_RESULT_SUCCESS;
}

bool FspNrf24Port_CommandIrqWait(uint32_t timeout_ms)
{
    bool active = false;

    /* 若 IRQ 在开始等待前已经拉低，直接处理，避免错过下降沿。 */
    if ((NRF24_RESULT_SUCCESS == irq_read(&g_ports[NRF24_PORT_COMMAND_RX], &active)) && active)
    {
        return true;
    }

    return (NULL != g_command_irq) &&
           (pdTRUE == xSemaphoreTake(g_command_irq, pdMS_TO_TICKS(timeout_ms)));
}

static void spi_callback_common(fsp_nrf24_context_t * p_port,
                                spi_callback_args_t * p_args)
{
    if ((NULL == p_port) || (NULL == p_args) || (NULL == p_port->transfer_done))
    {
        return;
    }

    p_port->transfer_event = p_args->event;
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void) xSemaphoreGiveFromISR(p_port->transfer_done, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void nrf24_command_spi_callback(spi_callback_args_t * p_args)
{
    spi_callback_common(&g_ports[NRF24_PORT_COMMAND_RX], p_args);
}

void nrf24_video_spi_callback(spi_callback_args_t * p_args)
{
    spi_callback_common(&g_ports[NRF24_PORT_VIDEO_TX], p_args);
}

void nrf24_command_irq_callback(external_irq_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    if (NULL != g_command_irq)
    {
        BaseType_t higher_priority_task_woken = pdFALSE;
        (void) xSemaphoreGiveFromISR(g_command_irq, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}
