#ifndef RADIO_APPLICATION_COMMAND_RADIO_H_
#define RADIO_APPLICATION_COMMAND_RADIO_H_

#include <stdint.h>

#include "Radio/driver/nrf24.h"

/** 初始化命令接收链路：SPI0、NRF24接收模式、频道76。 */
nrf24_result_t CommandRadio_Init(void);

/** 排空NRF硬件FIFO并把合法控制包转换成Vehicle命令；返回本次处理包数。 */
nrf24_result_t CommandRadio_Service(uint32_t * p_processed_packets);

#endif /* RADIO_APPLICATION_COMMAND_RADIO_H_ */
