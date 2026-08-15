#ifndef NRF24_PORT_H_
#define NRF24_PORT_H_

#include "hal_data.h"
#include "nrf24/nrf24.h"

#ifndef NRF24_PORT_SOFTWARE_SPI
#define NRF24_PORT_SOFTWARE_SPI    (0U)
#endif
#define NRF24_RX_MODULE            NRF24_PORT_MODULE_SPI1
#define NRF24_TX_MODULE            NRF24_PORT_MODULE_SPI0

/*
 * 1: P105/IRQ0 notifies the application when the SPI0 receiver has data.
 * 0: Keep the synchronous non-blocking polling path for comparison/fallback.
 */
#ifndef NRF24_RX_IRQ_NOTIFICATION_ENABLE
#define NRF24_RX_IRQ_NOTIFICATION_ENABLE    (1U)
#endif

typedef enum e_nrf24_port_module
{
    NRF24_PORT_MODULE_SPI0 = 0,
    NRF24_PORT_MODULE_SPI1,
    NRF24_PORT_MODULE_COUNT
} nrf24_port_module_t;

fsp_err_t Nrf24Port_Init(void);
nrf24_result_t Nrf24Port_GetTransport(nrf24_port_module_t module,
                                      nrf24_transport_t * p_transport);
fsp_err_t Nrf24Port_RxIrqOpen(void);
fsp_err_t Nrf24Port_RxIrqPendingTake(bool * p_pending);
uint32_t Nrf24Port_RxIrqCallbackCountGet(void);
void spi0_callback(spi_callback_args_t * p_args);
void spi1_callback(spi_callback_args_t * p_args);
void nrf24_video_rx_irq_callback(external_irq_callback_args_t * p_args);

extern volatile uint32_t    g_nrf24_spi_callback_count;
extern volatile uint32_t    g_nrf24_spi0_transaction_count;
extern volatile uint32_t    g_nrf24_spi1_transaction_count;
extern volatile uint32_t    g_nrf24_spi_timeout_count;
extern volatile spi_event_t g_nrf24_spi_last_event;
extern volatile fsp_err_t   g_nrf24_spi_last_fsp_error;

#endif /* NRF24_PORT_H_ */
