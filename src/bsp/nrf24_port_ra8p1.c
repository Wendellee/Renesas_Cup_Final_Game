#include "bsp/nrf24_port.h"

#define NRF24_PORT_SPI_HALF_CYCLE_US (1U)
#define NRF24_PORT_SPI_TIMEOUT_US    (5000U)

typedef struct st_nrf24_port_context
{
    nrf24_port_module_t module;
    bsp_io_port_pin_t   miso;
    bsp_io_port_pin_t   mosi;
    bsp_io_port_pin_t   sck;
    bsp_io_port_pin_t   csn;
    bsp_io_port_pin_t   ce;
    bsp_io_port_pin_t   irq;
} nrf24_port_context_t;

static nrf24_port_context_t g_nrf24_port_contexts[NRF24_PORT_MODULE_COUNT] =
{
    {
        .module = NRF24_PORT_MODULE_SPI0,
        .miso   = BSP_IO_PORT_07_PIN_00,
        .mosi   = BSP_IO_PORT_07_PIN_01,
        .sck    = BSP_IO_PORT_07_PIN_02,
        .csn    = BSP_IO_PORT_07_PIN_03,
        .ce     = BSP_IO_PORT_07_PIN_04,
        /* P705 is IRQ19 and conflicts with GT911/P111. Route RX IRQ to P105/IRQ0. */
        .irq    = BSP_IO_PORT_01_PIN_05,
    },
    {
        .module = NRF24_PORT_MODULE_SPI1,
        .miso   = BSP_IO_PORT_07_PIN_09,
        .mosi   = BSP_IO_PORT_07_PIN_08,
        .sck    = BSP_IO_PORT_04_PIN_15,
        .csn    = BSP_IO_PORT_04_PIN_14,
        .ce     = BSP_IO_PORT_01_PIN_04,
        /* The transmitter does not require IRQ; P705 is kept as an unused input. */
        .irq    = BSP_IO_PORT_07_PIN_05,
    },
};

volatile uint32_t    g_nrf24_spi_callback_count;
volatile uint32_t    g_nrf24_spi0_transaction_count;
volatile uint32_t    g_nrf24_spi1_transaction_count;
volatile uint32_t    g_nrf24_spi_timeout_count;
volatile spi_event_t g_nrf24_spi_last_event;
volatile fsp_err_t   g_nrf24_spi_last_fsp_error;
static volatile bool     g_nrf24_rx_irq_pending;
static volatile uint32_t g_nrf24_rx_irq_callback_count;
static volatile bool     g_spi_transfer_done[NRF24_PORT_MODULE_COUNT];
static volatile spi_event_t g_spi_transfer_event[NRF24_PORT_MODULE_COUNT];

static nrf24_result_t nrf24_port_transfer(void * p_context,
                                          uint8_t const * p_tx,
                                          uint8_t * p_rx,
                                          uint32_t length)
{
    nrf24_port_context_t const * p_port = (nrf24_port_context_t const *) p_context;

    if ((NULL == p_port) || (NULL == p_tx) || (NULL == p_rx) || (0U == length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    fsp_err_t err;

#if !NRF24_PORT_SOFTWARE_SPI
    spi_instance_t const * p_spi = (NRF24_PORT_MODULE_SPI0 == p_port->module) ? &g_spi0 : &g_spi1;
    nrf24_port_module_t module = p_port->module;

    g_spi_transfer_done[module] = false;
    g_spi_transfer_event[module] = (spi_event_t) 0;
    err = p_spi->p_api->writeRead(p_spi->p_ctrl,
                                  p_tx,
                                  p_rx,
                                  length,
                                  SPI_BIT_WIDTH_8_BITS);
    if (FSP_SUCCESS != err)
    {
        g_nrf24_spi_last_fsp_error = err;
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    for (uint32_t elapsed_us = 0U; elapsed_us < NRF24_PORT_SPI_TIMEOUT_US; elapsed_us++)
    {
        if (g_spi_transfer_done[module])
        {
            if (SPI_EVENT_TRANSFER_COMPLETE != g_spi_transfer_event[module])
            {
                return NRF24_RESULT_TRANSPORT_ERROR;
            }

            g_nrf24_spi_callback_count++;
            if (NRF24_PORT_MODULE_SPI0 == module)
            {
                g_nrf24_spi0_transaction_count++;
            }
            else
            {
                g_nrf24_spi1_transaction_count++;
            }
            g_nrf24_spi_last_fsp_error = FSP_SUCCESS;
            return NRF24_RESULT_SUCCESS;
        }
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
    }

    g_nrf24_spi_timeout_count++;
    g_nrf24_spi_last_fsp_error = FSP_ERR_TIMEOUT;
    return NRF24_RESULT_TIMEOUT;
#else

    err = R_IOPORT_PinWrite(&g_ioport_ctrl, p_port->csn, BSP_IO_LEVEL_LOW);
    if (FSP_SUCCESS != err)
    {
        g_nrf24_spi_last_fsp_error = err;
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    R_BSP_SoftwareDelay(2U, BSP_DELAY_UNITS_MICROSECONDS);

    for (uint32_t byte_index = 0U; byte_index < length; byte_index++)
    {
        uint8_t received = 0U;

        for (uint8_t mask = 0x80U; 0U != mask; mask >>= 1U)
        {
            err = R_IOPORT_PinWrite(&g_ioport_ctrl,
                                    p_port->mosi,
                                    (0U != (p_tx[byte_index] & mask)) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
            if (FSP_SUCCESS != err)
            {
                goto software_spi_error;
            }

            R_BSP_SoftwareDelay(NRF24_PORT_SPI_HALF_CYCLE_US, BSP_DELAY_UNITS_MICROSECONDS);

            err = R_IOPORT_PinWrite(&g_ioport_ctrl, p_port->sck, BSP_IO_LEVEL_HIGH);
            if (FSP_SUCCESS != err)
            {
                goto software_spi_error;
            }

            bsp_io_level_t miso_level = BSP_IO_LEVEL_LOW;
            err = R_IOPORT_PinRead(&g_ioport_ctrl, p_port->miso, &miso_level);
            if (FSP_SUCCESS != err)
            {
                goto software_spi_error;
            }

            if (BSP_IO_LEVEL_HIGH == miso_level)
            {
                received |= mask;
            }

            R_BSP_SoftwareDelay(NRF24_PORT_SPI_HALF_CYCLE_US, BSP_DELAY_UNITS_MICROSECONDS);

            err = R_IOPORT_PinWrite(&g_ioport_ctrl, p_port->sck, BSP_IO_LEVEL_LOW);
            if (FSP_SUCCESS != err)
            {
                goto software_spi_error;
            }
        }

        p_rx[byte_index] = received;
    }

    R_BSP_SoftwareDelay(2U, BSP_DELAY_UNITS_MICROSECONDS);
    (void) R_IOPORT_PinWrite(&g_ioport_ctrl, p_port->csn, BSP_IO_LEVEL_HIGH);
    g_nrf24_spi_callback_count++;
    if (NRF24_PORT_MODULE_SPI0 == p_port->module)
    {
        g_nrf24_spi0_transaction_count++;
    }
    else
    {
        g_nrf24_spi1_transaction_count++;
    }
    g_nrf24_spi_last_fsp_error = FSP_SUCCESS;
    return NRF24_RESULT_SUCCESS;

software_spi_error:
    g_nrf24_spi_last_fsp_error = err;
    (void) R_IOPORT_PinWrite(&g_ioport_ctrl, p_port->sck, BSP_IO_LEVEL_LOW);
    (void) R_IOPORT_PinWrite(&g_ioport_ctrl, p_port->csn, BSP_IO_LEVEL_HIGH);
    return NRF24_RESULT_TRANSPORT_ERROR;
#endif
}

static nrf24_result_t nrf24_port_ce_write(void * p_context, bool high)
{
    nrf24_port_context_t const * p_port = (nrf24_port_context_t const *) p_context;

    if (NULL == p_port)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl,
                                      p_port->ce,
                                      high ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    return (FSP_SUCCESS == err) ? NRF24_RESULT_SUCCESS : NRF24_RESULT_TRANSPORT_ERROR;
}

static nrf24_result_t nrf24_port_irq_read(void * p_context, bool * p_active)
{
    bsp_io_level_t level;
    nrf24_port_context_t const * p_port = (nrf24_port_context_t const *) p_context;

    if ((NULL == p_port) || (NULL == p_active))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    fsp_err_t err = R_IOPORT_PinRead(&g_ioport_ctrl, p_port->irq, &level);
    if (FSP_SUCCESS != err)
    {
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    *p_active = (BSP_IO_LEVEL_LOW == level);
    return NRF24_RESULT_SUCCESS;
}

static void nrf24_port_delay_us(void * p_context, uint32_t delay)
{
    FSP_PARAMETER_NOT_USED(p_context);
    R_BSP_SoftwareDelay(delay, BSP_DELAY_UNITS_MICROSECONDS);
}

static void nrf24_port_delay_ms(void * p_context, uint32_t delay)
{
    FSP_PARAMETER_NOT_USED(p_context);
    R_BSP_SoftwareDelay(delay, BSP_DELAY_UNITS_MILLISECONDS);
}

fsp_err_t Nrf24Port_Init(void)
{
    g_nrf24_spi_callback_count    = 0U;
    g_nrf24_spi0_transaction_count = 0U;
    g_nrf24_spi1_transaction_count = 0U;
    g_nrf24_spi_timeout_count     = 0U;
    g_nrf24_spi_last_event        = (spi_event_t) 0;
    g_nrf24_spi_last_fsp_error    = FSP_SUCCESS;
    g_nrf24_rx_irq_pending        = false;
    g_nrf24_rx_irq_callback_count = 0U;
    (void) memset((void *) g_spi_transfer_done, 0, sizeof(g_spi_transfer_done));
    (void) memset((void *) g_spi_transfer_event, 0, sizeof(g_spi_transfer_event));

    fsp_err_t err;

#if !NRF24_PORT_SOFTWARE_SPI
    err = R_SPI_B_Open(&g_spi0_ctrl, &g_spi0_cfg);
    if (FSP_SUCCESS != err) return err;
    err = R_SPI_B_Open(&g_spi1_ctrl, &g_spi1_cfg);
    if (FSP_SUCCESS != err)
    {
        (void) R_SPI_B_Close(&g_spi0_ctrl);
        return err;
    }
#endif

    for (uint32_t i = 0U; i < NRF24_PORT_MODULE_COUNT; i++)
    {
        nrf24_port_context_t const * p_port = &g_nrf24_port_contexts[i];

#if NRF24_PORT_SOFTWARE_SPI
        err = R_IOPORT_PinCfg(&g_ioport_ctrl, p_port->miso,
                              (uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT |
                              (uint32_t) IOPORT_CFG_PULLUP_ENABLE);
        if (FSP_SUCCESS != err) return err;

        err = R_IOPORT_PinCfg(&g_ioport_ctrl, p_port->mosi,
                              (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                              (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW);
        if (FSP_SUCCESS != err) return err;

        err = R_IOPORT_PinCfg(&g_ioport_ctrl, p_port->sck,
                              (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                              (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW);
        if (FSP_SUCCESS != err) return err;

        err = R_IOPORT_PinCfg(&g_ioport_ctrl, p_port->csn,
                              (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                              (uint32_t) IOPORT_CFG_PORT_OUTPUT_HIGH);
        if (FSP_SUCCESS != err) return err;
#endif

        err = R_IOPORT_PinCfg(&g_ioport_ctrl, p_port->ce,
                              (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT |
                              (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW);
        if (FSP_SUCCESS != err) return err;

        uint32_t irq_pin_cfg = (uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT |
                               (uint32_t) IOPORT_CFG_PULLUP_ENABLE;
#if NRF24_RX_IRQ_NOTIFICATION_ENABLE
        if (NRF24_PORT_MODULE_SPI0 == p_port->module)
        {
            irq_pin_cfg |= (uint32_t) IOPORT_CFG_IRQ_ENABLE;
        }
#endif
        err = R_IOPORT_PinCfg(&g_ioport_ctrl, p_port->irq, irq_pin_cfg);
        if (FSP_SUCCESS != err) return err;
    }

    return FSP_SUCCESS;
}

nrf24_result_t Nrf24Port_GetTransport(nrf24_port_module_t module,
                                      nrf24_transport_t * p_transport)
{
    if ((NULL == p_transport) || (NRF24_PORT_MODULE_COUNT <= module))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    p_transport->p_context = &g_nrf24_port_contexts[module];
    p_transport->transfer  = nrf24_port_transfer;
    p_transport->ce_write  = nrf24_port_ce_write;
    p_transport->irq_read  = nrf24_port_irq_read;
    p_transport->delay_us  = nrf24_port_delay_us;
    p_transport->delay_ms  = nrf24_port_delay_ms;
    return NRF24_RESULT_SUCCESS;
}

fsp_err_t Nrf24Port_RxIrqOpen(void)
{
#if NRF24_RX_IRQ_NOTIFICATION_ENABLE
    fsp_err_t err;

    g_nrf24_rx_irq_pending = false;
    g_nrf24_rx_irq_callback_count = 0U;

    err = R_ICU_ExternalIrqOpen(&g_external_irq0_ctrl, &g_external_irq0_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = R_ICU_ExternalIrqEnable(&g_external_irq0_ctrl);
    if (FSP_SUCCESS != err)
    {
        (void) R_ICU_ExternalIrqClose(&g_external_irq0_ctrl);
    }
    else
    {
        /* Do not miss data that arrived before the falling-edge interrupt was enabled. */
        bsp_io_level_t irq_level = BSP_IO_LEVEL_HIGH;
        if ((FSP_SUCCESS == R_IOPORT_PinRead(&g_ioport_ctrl,
                                             g_nrf24_port_contexts[NRF24_RX_MODULE].irq,
                                             &irq_level)) &&
            (BSP_IO_LEVEL_LOW == irq_level))
        {
            g_nrf24_rx_irq_pending = true;
        }
    }
    return err;
#else
    return FSP_SUCCESS;
#endif
}

fsp_err_t Nrf24Port_RxIrqPendingTake(bool * p_pending)
{
    if (NULL == p_pending)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

#if NRF24_RX_IRQ_NOTIFICATION_ENABLE
    FSP_CRITICAL_SECTION_DEFINE;
    FSP_CRITICAL_SECTION_ENTER;
    *p_pending = g_nrf24_rx_irq_pending;
    g_nrf24_rx_irq_pending = false;
    FSP_CRITICAL_SECTION_EXIT;
#else
    *p_pending = false;
#endif
    return FSP_SUCCESS;
}

uint32_t Nrf24Port_RxIrqCallbackCountGet(void)
{
    return g_nrf24_rx_irq_callback_count;
}

void spi0_callback(spi_callback_args_t * p_args)
{
    if (NULL == p_args)
    {
        return;
    }

    g_nrf24_spi_last_event = p_args->event;
    g_spi_transfer_event[NRF24_PORT_MODULE_SPI0] = p_args->event;
    g_spi_transfer_done[NRF24_PORT_MODULE_SPI0] = true;
}

void spi1_callback(spi_callback_args_t * p_args)
{
    if (NULL == p_args)
    {
        return;
    }

    g_nrf24_spi_last_event = p_args->event;
    g_spi_transfer_event[NRF24_PORT_MODULE_SPI1] = p_args->event;
    g_spi_transfer_done[NRF24_PORT_MODULE_SPI1] = true;
}

void nrf24_spi0_irq_callback(external_irq_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    g_nrf24_rx_irq_callback_count++;
    g_nrf24_rx_irq_pending = true;
}
