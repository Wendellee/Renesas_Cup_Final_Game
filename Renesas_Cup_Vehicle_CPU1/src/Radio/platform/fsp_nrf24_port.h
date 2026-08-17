#ifndef RADIO_PLATFORM_FSP_NRF24_PORT_H_
#define RADIO_PLATFORM_FSP_NRF24_PORT_H_

#include <stdbool.h>
#include <stdint.h>

#include "Radio/driver/nrf24.h"
#include "r_spi_api.h"
#include "r_external_irq_api.h"

/** 两块无线模块在巡检车上的固定职责。 */
typedef enum e_nrf24_port_id
{
    NRF24_PORT_COMMAND_RX = 0, /**< SPI0：接收手持控制器命令。 */
    NRF24_PORT_VIDEO_TX,       /**< SPI1：向手持控制器发送图像。 */
    NRF24_PORT_COUNT
} nrf24_port_id_t;

/** 打开指定 SPI 和 GPIO，并返回与 MCU 无关的 nRF24 transport。 */
nrf24_result_t FspNrf24Port_Open(nrf24_port_id_t port_id,
                                 nrf24_transport_t * p_transport);

/** 等待命令接收模块的低有效 IRQ；超时返回 false，用于轮询兜底。 */
bool FspNrf24Port_CommandIrqWait(uint32_t timeout_ms);

/* 下列函数名必须与 configuration.xml 中的 callback 完全一致。 */
void nrf24_command_spi_callback(spi_callback_args_t * p_args);
void nrf24_video_spi_callback(spi_callback_args_t * p_args);
void nrf24_command_irq_callback(external_irq_callback_args_t * p_args);

#endif /* RADIO_PLATFORM_FSP_NRF24_PORT_H_ */
