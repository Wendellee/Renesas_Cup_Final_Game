#ifndef NRF24_H_
#define NRF24_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf24/nrf24_regs.h"

/* 通用驱动返回值，不依赖任何 MCU 厂商的错误码 */
typedef enum
{
    NRF24_RESULT_SUCCESS = 0,
    NRF24_RESULT_INVALID_ARGUMENT,
    NRF24_RESULT_TRANSPORT_ERROR,
    NRF24_RESULT_TIMEOUT,
    NRF24_RESULT_MAX_RETRANSMIT,
    NRF24_RESULT_NO_DATA,
    NRF24_RESULT_INVALID_PAYLOAD_WIDTH,
    NRF24_RESULT_FEATURE_UNAVAILABLE,
    NRF24_RESULT_QUEUE_FULL,
    NRF24_RESULT_QUEUE_CORRUPTED
} nrf24_result_t;

/* 无线芯片工作角色 */
typedef enum
{
    NRF24_ROLE_TRANSMITTER = 0,
    NRF24_ROLE_RECEIVER
} nrf24_role_t;

/* 空中数据速率 */
typedef enum
{
    NRF24_DATA_RATE_250KBPS = 0,
    NRF24_DATA_RATE_1MBPS,
    NRF24_DATA_RATE_2MBPS
} nrf24_data_rate_t;

/* 标准 nRF24L01+ 的发射功率等级 */
typedef enum
{
    NRF24_POWER_NEGATIVE_18_DBM = 0,
    NRF24_POWER_NEGATIVE_12_DBM,
    NRF24_POWER_NEGATIVE_6_DBM,
    NRF24_POWER_0_DBM
} nrf24_power_t;

/* CRC 长度配置 */
typedef enum
{
    NRF24_CRC_DISABLED = 0,
    NRF24_CRC_1_BYTE,
    NRF24_CRC_2_BYTES
} nrf24_crc_mode_t;

/* SPI 全双工传输回调。一次回调必须对应一条完整 nRF SPI 命令。 */
typedef nrf24_result_t (* nrf24_transfer_fn_t)(void * p_context,
                                                uint8_t const * p_tx,
                                                uint8_t * p_rx,
                                                uint32_t length);

/* CE 电平控制回调。high 为 true 时输出高电平。 */
typedef nrf24_result_t (* nrf24_ce_write_fn_t)(void * p_context, bool high);

/* IRQ 读取回调。active 为 true 表示 nRF 的低有效 IRQ 已触发。 */
typedef nrf24_result_t (* nrf24_irq_read_fn_t)(void * p_context, bool * p_active);

/* 驱动使用的延时回调。 */
typedef void (* nrf24_delay_fn_t)(void * p_context, uint32_t delay);

/* 通用驱动与板级驱动之间的唯一连接对象 */
typedef struct
{
    void * p_context;
    nrf24_transfer_fn_t transfer;
    nrf24_ce_write_fn_t ce_write;
    nrf24_irq_read_fn_t irq_read;
    nrf24_delay_fn_t delay_us;
    nrf24_delay_fn_t delay_ms;
} nrf24_transport_t;

/* 单管道基础通信配置 */
typedef struct
{
    uint8_t tx_address[NRF24_ADDRESS_WIDTH_MAX];
    uint8_t rx_pipe0_address[NRF24_ADDRESS_WIDTH_MAX];
    uint8_t address_width;
    uint8_t channel;
    uint8_t payload_width;
    uint8_t retransmit_delay;
    uint8_t retransmit_count;
    nrf24_data_rate_t data_rate;
    nrf24_power_t power;
    nrf24_crc_mode_t crc_mode;
    nrf24_role_t initial_role;
    bool auto_ack_enabled;
    bool dynamic_payload_enabled;
    bool ack_payload_enabled;
    bool dynamic_ack_enabled;
} nrf24_config_t;

/* 发送后的状态信息 */
typedef struct
{
    uint8_t status;
    uint8_t observe_tx;
    uint8_t config;
    uint8_t fifo_status;
    bool ack_payload_available;
} nrf24_tx_result_t;

/* 获取一套便于首次联调的默认配置。 */
void Nrf24_GetDefaultConfig(nrf24_config_t * p_config);

/* 单字节与多字节寄存器读写 API */
nrf24_result_t Nrf24_ReadRegister(nrf24_transport_t const * p_transport,
                                  uint8_t register_address,
                                  uint8_t * p_value,
                                  uint8_t * p_status);
nrf24_result_t Nrf24_WriteRegister(nrf24_transport_t const * p_transport,
                                   uint8_t register_address,
                                   uint8_t value,
                                   uint8_t * p_status);
nrf24_result_t Nrf24_ReadRegisterBuffer(nrf24_transport_t const * p_transport,
                                        uint8_t register_address,
                                        uint8_t * p_buffer,
                                        uint8_t length,
                                        uint8_t * p_status);
nrf24_result_t Nrf24_WriteRegisterBuffer(nrf24_transport_t const * p_transport,
                                         uint8_t register_address,
                                         uint8_t const * p_buffer,
                                         uint8_t length,
                                         uint8_t * p_status);

/* SPI 命令、状态与 FIFO API */
nrf24_result_t Nrf24_Command(nrf24_transport_t const * p_transport,
                             uint8_t command,
                             uint8_t * p_status);
nrf24_result_t Nrf24_ReadStatus(nrf24_transport_t const * p_transport,
                                uint8_t * p_status);
nrf24_result_t Nrf24_ClearIrqFlags(nrf24_transport_t const * p_transport,
                                   uint8_t irq_flags,
                                   uint8_t * p_status);
nrf24_result_t Nrf24_FlushTx(nrf24_transport_t const * p_transport,
                             uint8_t * p_status);
nrf24_result_t Nrf24_FlushRx(nrf24_transport_t const * p_transport,
                             uint8_t * p_status);
nrf24_result_t Nrf24_ReadFifoStatus(nrf24_transport_t const * p_transport,
                                    uint8_t * p_fifo_status,
                                    uint8_t * p_status);
nrf24_result_t Nrf24_GetRxPayloadWidth(nrf24_transport_t const * p_transport,
                                       uint8_t * p_width,
                                       uint8_t * p_status);
nrf24_result_t Nrf24_ReadRxPayload(nrf24_transport_t const * p_transport,
                                   uint8_t * p_buffer,
                                   uint8_t length,
                                   uint8_t * p_status);
nrf24_result_t Nrf24_WriteTxPayload(nrf24_transport_t const * p_transport,
                                    uint8_t const * p_buffer,
                                    uint8_t length,
                                    bool no_ack,
                                    uint8_t * p_status);
nrf24_result_t Nrf24_WriteAckPayload(nrf24_transport_t const * p_transport,
                                     uint8_t pipe,
                                     uint8_t const * p_buffer,
                                     uint8_t length,
                                     uint8_t * p_status);

/* 单项无线参数配置 API */
nrf24_result_t Nrf24_SetAddressWidth(nrf24_transport_t const * p_transport,
                                     uint8_t address_width_bytes,
                                     uint8_t * p_status);
nrf24_result_t Nrf24_SetChannel(nrf24_transport_t const * p_transport,
                                uint8_t channel,
                                uint8_t * p_status);
nrf24_result_t Nrf24_SetDataRate(nrf24_transport_t const * p_transport,
                                 nrf24_data_rate_t data_rate,
                                 uint8_t * p_status);
nrf24_result_t Nrf24_SetPower(nrf24_transport_t const * p_transport,
                              nrf24_power_t power,
                              uint8_t * p_status);
nrf24_result_t Nrf24_SetTxAddress(nrf24_transport_t const * p_transport,
                                  uint8_t const * p_address,
                                  uint8_t address_width,
                                  uint8_t * p_status);
nrf24_result_t Nrf24_SetRxAddress(nrf24_transport_t const * p_transport,
                                  uint8_t pipe,
                                  uint8_t const * p_address,
                                  uint8_t address_width,
                                  uint8_t * p_status);

/* 工作模式、初始化与收发 API */
nrf24_result_t Nrf24_PowerDown(nrf24_transport_t const * p_transport,
                               uint8_t * p_status);
nrf24_result_t Nrf24_SetRole(nrf24_transport_t const * p_transport,
                             nrf24_role_t role,
                             uint8_t * p_status);
nrf24_result_t Nrf24_Initialize(nrf24_transport_t const * p_transport,
                                 nrf24_config_t const * p_config,
                                 uint8_t * p_status);
nrf24_result_t Nrf24_TestConnection(nrf24_transport_t const * p_transport,
                                    bool * p_connected);
nrf24_result_t Nrf24_Send(nrf24_transport_t const * p_transport,
                          uint8_t const * p_payload,
                          uint8_t length,
                          bool no_ack,
                          uint32_t timeout_ms,
                          nrf24_tx_result_t * p_result);
nrf24_result_t Nrf24_SendBatchNoAck(nrf24_transport_t const * p_transport,
                                    uint8_t const * const p_payloads[],
                                    uint8_t const lengths[],
                                    uint8_t payload_count,
                                    uint32_t timeout_ms,
                                    nrf24_tx_result_t * p_result);
nrf24_result_t Nrf24_IsRxPayloadAvailable(nrf24_transport_t const * p_transport,
                                          bool * p_available,
                                          uint8_t * p_pipe,
                                          uint8_t * p_status);

#endif
