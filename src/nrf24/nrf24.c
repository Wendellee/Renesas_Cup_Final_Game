#include "nrf24/nrf24.h"

#include <stddef.h>
#include <string.h>

static nrf24_result_t nrf24_check_transport(nrf24_transport_t const * p_transport)
{
    if ((NULL == p_transport) || (NULL == p_transport->transfer))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    return NRF24_RESULT_SUCCESS;
}

static nrf24_result_t nrf24_check_control_transport(nrf24_transport_t const * p_transport)
{
    nrf24_result_t result = nrf24_check_transport(p_transport);

    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    if ((NULL == p_transport->ce_write) || (NULL == p_transport->delay_us) ||
        (NULL == p_transport->delay_ms))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    return NRF24_RESULT_SUCCESS;
}

static nrf24_result_t nrf24_transfer(nrf24_transport_t const * p_transport,
                                      uint8_t const * p_tx,
                                      uint8_t * p_rx,
                                      uint32_t length)
{
    nrf24_result_t result = nrf24_check_transport(p_transport);

    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    if ((NULL == p_tx) || (NULL == p_rx) || (0U == length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    return p_transport->transfer(p_transport->p_context, p_tx, p_rx, length);
}

static nrf24_result_t nrf24_read_modify_write(nrf24_transport_t const * p_transport,
                                              uint8_t register_address,
                                              uint8_t clear_mask,
                                              uint8_t set_mask,
                                              uint8_t * p_status)
{
    nrf24_result_t result;
    uint8_t value;

    result = Nrf24_ReadRegister(p_transport, register_address, &value, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    value = (uint8_t) ((value & (uint8_t) ~clear_mask) | set_mask);
    return Nrf24_WriteRegister(p_transport, register_address, value, p_status);
}

static nrf24_result_t nrf24_set_feature(nrf24_transport_t const * p_transport,
                                        uint8_t feature,
                                        uint8_t * p_status)
{
    nrf24_result_t result;
    uint8_t readback;
    uint8_t tx[2] = {NRF24_CMD_ACTIVATE, NRF24_ACTIVATE_DATA};
    uint8_t rx[2] = {0U, 0U};

    result = Nrf24_WriteRegister(p_transport, NRF24_REG_FEATURE, feature, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = Nrf24_ReadRegister(p_transport, NRF24_REG_FEATURE, &readback, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    if (feature == (readback & (NRF24_FEATURE_EN_DPL | NRF24_FEATURE_EN_ACK_PAY | NRF24_FEATURE_EN_DYN_ACK)))
    {
        return NRF24_RESULT_SUCCESS;
    }

    result = nrf24_transfer(p_transport, tx, rx, sizeof(tx));
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = Nrf24_WriteRegister(p_transport, NRF24_REG_FEATURE, feature, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = Nrf24_ReadRegister(p_transport, NRF24_REG_FEATURE, &readback, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    if (feature != (readback & (NRF24_FEATURE_EN_DPL | NRF24_FEATURE_EN_ACK_PAY | NRF24_FEATURE_EN_DYN_ACK)))
    {
        return NRF24_RESULT_FEATURE_UNAVAILABLE;
    }

    return NRF24_RESULT_SUCCESS;
}

void Nrf24_GetDefaultConfig(nrf24_config_t * p_config)
{
    static uint8_t const default_address[NRF24_ADDRESS_WIDTH_MAX] = {0xE7U, 0xE7U, 0xE7U, 0xE7U, 0xE7U};

    if (NULL == p_config)
    {
        return;
    }

    (void) memset(p_config, 0, sizeof(*p_config));
    (void) memcpy(p_config->tx_address, default_address, sizeof(default_address));
    (void) memcpy(p_config->rx_pipe0_address, default_address, sizeof(default_address));
    p_config->address_width = NRF24_ADDRESS_WIDTH_MAX;
    p_config->channel = 2U;
    p_config->payload_width = NRF24_MAX_PAYLOAD_SIZE;
    p_config->retransmit_delay = 5U;
    p_config->retransmit_count = 15U;
    p_config->data_rate = NRF24_DATA_RATE_1MBPS;
    p_config->power = NRF24_POWER_0_DBM;
    p_config->crc_mode = NRF24_CRC_2_BYTES;
    p_config->initial_role = NRF24_ROLE_RECEIVER;
    p_config->auto_ack_enabled = true;
    p_config->dynamic_payload_enabled = true;
    p_config->ack_payload_enabled = false;
    p_config->dynamic_ack_enabled = false;
}

nrf24_result_t Nrf24_ReadRegister(nrf24_transport_t const * p_transport,
                                  uint8_t register_address,
                                  uint8_t * p_value,
                                  uint8_t * p_status)
{
    uint8_t tx[2];
    uint8_t rx[2] = {0U, 0U};
    nrf24_result_t result;

    if ((NULL == p_value) || (NULL == p_status) || (register_address > NRF24_REG_MASK))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    tx[0] = (uint8_t) (NRF24_CMD_R_REGISTER | (register_address & NRF24_REG_MASK));
    tx[1] = NRF24_CMD_NOP;
    result = nrf24_transfer(p_transport, tx, rx, sizeof(tx));
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx[0];
        *p_value = rx[1];
    }

    return result;
}

nrf24_result_t Nrf24_WriteRegister(nrf24_transport_t const * p_transport,
                                   uint8_t register_address,
                                   uint8_t value,
                                   uint8_t * p_status)
{
    uint8_t tx[2];
    uint8_t rx[2] = {0U, 0U};
    nrf24_result_t result;

    if ((NULL == p_status) || (register_address > NRF24_REG_MASK))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    tx[0] = (uint8_t) (NRF24_CMD_W_REGISTER | (register_address & NRF24_REG_MASK));
    tx[1] = value;
    result = nrf24_transfer(p_transport, tx, rx, sizeof(tx));
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx[0];
    }

    return result;
}

nrf24_result_t Nrf24_ReadRegisterBuffer(nrf24_transport_t const * p_transport,
                                        uint8_t register_address,
                                        uint8_t * p_buffer,
                                        uint8_t length,
                                        uint8_t * p_status)
{
    uint8_t tx[NRF24_ADDRESS_WIDTH_MAX + 1U];
    uint8_t rx[NRF24_ADDRESS_WIDTH_MAX + 1U] = {0U};
    nrf24_result_t result;

    if ((NULL == p_buffer) || (NULL == p_status) || (0U == length) ||
        (NRF24_ADDRESS_WIDTH_MAX < length) || (register_address > NRF24_REG_MASK))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    tx[0] = (uint8_t) (NRF24_CMD_R_REGISTER | (register_address & NRF24_REG_MASK));
    (void) memset(&tx[1], NRF24_CMD_NOP, length);
    result = nrf24_transfer(p_transport, tx, rx, (uint32_t) length + 1U);
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx[0];
        (void) memcpy(p_buffer, &rx[1], length);
    }

    return result;
}

nrf24_result_t Nrf24_WriteRegisterBuffer(nrf24_transport_t const * p_transport,
                                         uint8_t register_address,
                                         uint8_t const * p_buffer,
                                         uint8_t length,
                                         uint8_t * p_status)
{
    uint8_t tx[NRF24_ADDRESS_WIDTH_MAX + 1U];
    uint8_t rx[NRF24_ADDRESS_WIDTH_MAX + 1U] = {0U};
    nrf24_result_t result;

    if ((NULL == p_buffer) || (NULL == p_status) || (0U == length) ||
        (NRF24_ADDRESS_WIDTH_MAX < length) || (register_address > NRF24_REG_MASK))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    tx[0] = (uint8_t) (NRF24_CMD_W_REGISTER | (register_address & NRF24_REG_MASK));
    (void) memcpy(&tx[1], p_buffer, length);
    result = nrf24_transfer(p_transport, tx, rx, (uint32_t) length + 1U);
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx[0];
    }

    return result;
}

nrf24_result_t Nrf24_Command(nrf24_transport_t const * p_transport,
                             uint8_t command,
                             uint8_t * p_status)
{
    uint8_t rx = 0U;
    nrf24_result_t result;

    if (NULL == p_status)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = nrf24_transfer(p_transport, &command, &rx, 1U);
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx;
    }

    return result;
}

nrf24_result_t Nrf24_ReadStatus(nrf24_transport_t const * p_transport,
                                uint8_t * p_status)
{
    return Nrf24_Command(p_transport, NRF24_CMD_NOP, p_status);
}

nrf24_result_t Nrf24_ClearIrqFlags(nrf24_transport_t const * p_transport,
                                   uint8_t irq_flags,
                                   uint8_t * p_status)
{
    if ((0U == irq_flags) || (0U != (irq_flags & (uint8_t) ~NRF24_STATUS_IRQ_MASK)))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    return Nrf24_WriteRegister(p_transport, NRF24_REG_STATUS, irq_flags, p_status);
}

nrf24_result_t Nrf24_FlushTx(nrf24_transport_t const * p_transport,
                             uint8_t * p_status)
{
    return Nrf24_Command(p_transport, NRF24_CMD_FLUSH_TX, p_status);
}

nrf24_result_t Nrf24_FlushRx(nrf24_transport_t const * p_transport,
                             uint8_t * p_status)
{
    return Nrf24_Command(p_transport, NRF24_CMD_FLUSH_RX, p_status);
}

nrf24_result_t Nrf24_ReadFifoStatus(nrf24_transport_t const * p_transport,
                                    uint8_t * p_fifo_status,
                                    uint8_t * p_status)
{
    return Nrf24_ReadRegister(p_transport, NRF24_REG_FIFO_STATUS, p_fifo_status, p_status);
}

nrf24_result_t Nrf24_GetRxPayloadWidth(nrf24_transport_t const * p_transport,
                                       uint8_t * p_width,
                                       uint8_t * p_status)
{
    uint8_t tx[2] = {NRF24_CMD_R_RX_PL_WID, NRF24_CMD_NOP};
    uint8_t rx[2] = {0U, 0U};
    nrf24_result_t result;

    if ((NULL == p_width) || (NULL == p_status))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = nrf24_transfer(p_transport, tx, rx, sizeof(tx));
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    *p_status = rx[0];
    *p_width = rx[1];
    if (NRF24_MAX_PAYLOAD_SIZE < *p_width)
    {
        (void) Nrf24_FlushRx(p_transport, p_status);
        return NRF24_RESULT_INVALID_PAYLOAD_WIDTH;
    }

    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t Nrf24_ReadRxPayload(nrf24_transport_t const * p_transport,
                                   uint8_t * p_buffer,
                                   uint8_t length,
                                   uint8_t * p_status)
{
    uint8_t tx[NRF24_MAX_PAYLOAD_SIZE + 1U];
    uint8_t rx[NRF24_MAX_PAYLOAD_SIZE + 1U] = {0U};
    nrf24_result_t result;

    if ((NULL == p_buffer) || (NULL == p_status) || (0U == length) ||
        (NRF24_MAX_PAYLOAD_SIZE < length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    tx[0] = NRF24_CMD_R_RX_PAYLOAD;
    (void) memset(&tx[1], NRF24_CMD_NOP, length);
    result = nrf24_transfer(p_transport, tx, rx, (uint32_t) length + 1U);
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx[0];
        (void) memcpy(p_buffer, &rx[1], length);
    }

    return result;
}

nrf24_result_t Nrf24_WriteTxPayload(nrf24_transport_t const * p_transport,
                                    uint8_t const * p_buffer,
                                    uint8_t length,
                                    bool no_ack,
                                    uint8_t * p_status)
{
    uint8_t tx[NRF24_MAX_PAYLOAD_SIZE + 1U];
    uint8_t rx[NRF24_MAX_PAYLOAD_SIZE + 1U] = {0U};
    nrf24_result_t result;

    if ((NULL == p_buffer) || (NULL == p_status) || (0U == length) ||
        (NRF24_MAX_PAYLOAD_SIZE < length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    tx[0] = no_ack ? NRF24_CMD_W_TX_PAYLOAD_NO_ACK : NRF24_CMD_W_TX_PAYLOAD;
    (void) memcpy(&tx[1], p_buffer, length);
    result = nrf24_transfer(p_transport, tx, rx, (uint32_t) length + 1U);
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx[0];
    }

    return result;
}

nrf24_result_t Nrf24_WriteAckPayload(nrf24_transport_t const * p_transport,
                                     uint8_t pipe,
                                     uint8_t const * p_buffer,
                                     uint8_t length,
                                     uint8_t * p_status)
{
    uint8_t tx[NRF24_MAX_PAYLOAD_SIZE + 1U];
    uint8_t rx[NRF24_MAX_PAYLOAD_SIZE + 1U] = {0U};
    nrf24_result_t result;

    if ((NRF24_PIPE_COUNT <= pipe) || (NULL == p_buffer) || (NULL == p_status) ||
        (0U == length) || (NRF24_MAX_PAYLOAD_SIZE < length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    tx[0] = (uint8_t) (NRF24_CMD_W_ACK_PAYLOAD | pipe);
    (void) memcpy(&tx[1], p_buffer, length);
    result = nrf24_transfer(p_transport, tx, rx, (uint32_t) length + 1U);
    if (NRF24_RESULT_SUCCESS == result)
    {
        *p_status = rx[0];
    }

    return result;
}

nrf24_result_t Nrf24_SetAddressWidth(nrf24_transport_t const * p_transport,
                                     uint8_t address_width_bytes,
                                     uint8_t * p_status)
{
    if ((NRF24_ADDRESS_WIDTH_MIN > address_width_bytes) ||
        (NRF24_ADDRESS_WIDTH_MAX < address_width_bytes))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    return Nrf24_WriteRegister(p_transport, NRF24_REG_SETUP_AW,
                               (uint8_t) (address_width_bytes - 2U), p_status);
}

nrf24_result_t Nrf24_SetChannel(nrf24_transport_t const * p_transport,
                                uint8_t channel,
                                uint8_t * p_status)
{
    if (NRF24_RF_CHANNEL_MAX < channel)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    return Nrf24_WriteRegister(p_transport, NRF24_REG_RF_CH, channel, p_status);
}

nrf24_result_t Nrf24_SetDataRate(nrf24_transport_t const * p_transport,
                                 nrf24_data_rate_t data_rate,
                                 uint8_t * p_status)
{
    nrf24_result_t result;
    uint8_t rf_setup;

    if ((NRF24_DATA_RATE_250KBPS != data_rate) && (NRF24_DATA_RATE_1MBPS != data_rate) &&
        (NRF24_DATA_RATE_2MBPS != data_rate))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = Nrf24_ReadRegister(p_transport, NRF24_REG_RF_SETUP, &rf_setup, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    rf_setup &= (uint8_t) ~(NRF24_RF_SETUP_RF_DR_LOW | NRF24_RF_SETUP_RF_DR_HIGH);
    if (NRF24_DATA_RATE_250KBPS == data_rate)
    {
        rf_setup |= NRF24_RF_SETUP_RF_DR_LOW;
    }
    else if (NRF24_DATA_RATE_2MBPS == data_rate)
    {
        rf_setup |= NRF24_RF_SETUP_RF_DR_HIGH;
    }

    return Nrf24_WriteRegister(p_transport, NRF24_REG_RF_SETUP, rf_setup, p_status);
}

nrf24_result_t Nrf24_SetPower(nrf24_transport_t const * p_transport,
                              nrf24_power_t power,
                              uint8_t * p_status)
{
    nrf24_result_t result;
    uint8_t rf_setup;

    if ((NRF24_POWER_NEGATIVE_18_DBM != power) && (NRF24_POWER_NEGATIVE_12_DBM != power) &&
        (NRF24_POWER_NEGATIVE_6_DBM != power) && (NRF24_POWER_0_DBM != power))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = Nrf24_ReadRegister(p_transport, NRF24_REG_RF_SETUP, &rf_setup, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    rf_setup &= (uint8_t) ~NRF24_RF_SETUP_RF_PWR_MASK;
    rf_setup |= (uint8_t) ((uint8_t) power << 1U);
    return Nrf24_WriteRegister(p_transport, NRF24_REG_RF_SETUP, rf_setup, p_status);
}

nrf24_result_t Nrf24_SetTxAddress(nrf24_transport_t const * p_transport,
                                  uint8_t const * p_address,
                                  uint8_t address_width,
                                  uint8_t * p_status)
{
    if ((NULL == p_address) || (NRF24_ADDRESS_WIDTH_MIN > address_width) ||
        (NRF24_ADDRESS_WIDTH_MAX < address_width))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    return Nrf24_WriteRegisterBuffer(p_transport, NRF24_REG_TX_ADDR,
                                     p_address, address_width, p_status);
}

nrf24_result_t Nrf24_SetRxAddress(nrf24_transport_t const * p_transport,
                                  uint8_t pipe,
                                  uint8_t const * p_address,
                                  uint8_t address_width,
                                  uint8_t * p_status)
{
    uint8_t length;

    if ((NRF24_PIPE_COUNT <= pipe) || (NULL == p_address) ||
        (NRF24_ADDRESS_WIDTH_MIN > address_width) || (NRF24_ADDRESS_WIDTH_MAX < address_width))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    length = (pipe < 2U) ? address_width : 1U;
    return Nrf24_WriteRegisterBuffer(p_transport,
                                     (uint8_t) (NRF24_REG_RX_ADDR_P0 + pipe),
                                     p_address, length, p_status);
}

nrf24_result_t Nrf24_PowerDown(nrf24_transport_t const * p_transport,
                               uint8_t * p_status)
{
    nrf24_result_t result = nrf24_check_control_transport(p_transport);

    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = p_transport->ce_write(p_transport->p_context, false);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    return nrf24_read_modify_write(p_transport, NRF24_REG_CONFIG,
                                   NRF24_CONFIG_PWR_UP, 0U, p_status);
}

nrf24_result_t Nrf24_SetRole(nrf24_transport_t const * p_transport,
                             nrf24_role_t role,
                             uint8_t * p_status)
{
    nrf24_result_t result;
    uint8_t config_current;
    uint8_t config_next;
    uint8_t config_set;
    bool power_up_transition;

    if ((NRF24_ROLE_TRANSMITTER != role) && (NRF24_ROLE_RECEIVER != role))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = nrf24_check_control_transport(p_transport);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = p_transport->ce_write(p_transport->p_context, false);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    config_set = NRF24_CONFIG_PWR_UP;
    if (NRF24_ROLE_RECEIVER == role)
    {
        config_set |= NRF24_CONFIG_PRIM_RX;
    }

    result = Nrf24_ReadRegister(p_transport, NRF24_REG_CONFIG, &config_current, p_status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    power_up_transition = (0U == (config_current & NRF24_CONFIG_PWR_UP));
    config_next = (uint8_t) ((config_current &
                              (uint8_t) ~(NRF24_CONFIG_PWR_UP | NRF24_CONFIG_PRIM_RX)) |
                             config_set);

    /* Rewriting an unchanged CONFIG register does not require another
     * power-up settling interval. */
    if (config_next != config_current)
    {
        result = Nrf24_WriteRegister(p_transport, NRF24_REG_CONFIG, config_next, p_status);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }
    }

    /* 从掉电状态进入待机状态至少需要 1.5 毫秒。 */
    if (power_up_transition)
    {
        p_transport->delay_ms(p_transport->p_context, 2U);
    }

    if (NRF24_ROLE_RECEIVER == role)
    {
        result = p_transport->ce_write(p_transport->p_context, true);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }

        /* CE 拉高后，接收机进入监听状态需要约 130 微秒。 */
        p_transport->delay_us(p_transport->p_context, 150U);
    }

    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t Nrf24_Initialize(nrf24_transport_t const * p_transport,
                                 nrf24_config_t const * p_config,
                                 uint8_t * p_status)
{
    nrf24_result_t result;
    uint8_t config_value;
    uint8_t feature_value = 0U;
    uint8_t dynpd_value = 0U;

    if ((NULL == p_config) || (NULL == p_status) ||
        (NRF24_ADDRESS_WIDTH_MIN > p_config->address_width) ||
        (NRF24_ADDRESS_WIDTH_MAX < p_config->address_width) ||
        (NRF24_RF_CHANNEL_MAX < p_config->channel) ||
        (0U == p_config->payload_width) || (NRF24_MAX_PAYLOAD_SIZE < p_config->payload_width) ||
        (15U < p_config->retransmit_delay) || (15U < p_config->retransmit_count) ||
        ((NRF24_DATA_RATE_250KBPS != p_config->data_rate) &&
         (NRF24_DATA_RATE_1MBPS != p_config->data_rate) &&
         (NRF24_DATA_RATE_2MBPS != p_config->data_rate)) ||
        ((NRF24_POWER_NEGATIVE_18_DBM != p_config->power) &&
         (NRF24_POWER_NEGATIVE_12_DBM != p_config->power) &&
         (NRF24_POWER_NEGATIVE_6_DBM != p_config->power) &&
         (NRF24_POWER_0_DBM != p_config->power)) ||
        ((NRF24_CRC_DISABLED != p_config->crc_mode) && (NRF24_CRC_1_BYTE != p_config->crc_mode) &&
         (NRF24_CRC_2_BYTES != p_config->crc_mode)) ||
        ((NRF24_ROLE_TRANSMITTER != p_config->initial_role) &&
         (NRF24_ROLE_RECEIVER != p_config->initial_role)))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = nrf24_check_control_transport(p_transport);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = p_transport->ce_write(p_transport->p_context, false);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = Nrf24_PowerDown(p_transport, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_FlushTx(p_transport, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_FlushRx(p_transport, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_ClearIrqFlags(p_transport, NRF24_STATUS_IRQ_MASK, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    result = Nrf24_SetAddressWidth(p_transport, p_config->address_width, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_SetChannel(p_transport, p_config->channel, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_SetDataRate(p_transport, p_config->data_rate, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_SetPower(p_transport, p_config->power, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_SetTxAddress(p_transport, p_config->tx_address, p_config->address_width, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_SetRxAddress(p_transport, 0U, p_config->rx_pipe0_address,
                                p_config->address_width, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    result = Nrf24_WriteRegister(p_transport, NRF24_REG_EN_AA,
                                 p_config->auto_ack_enabled ? NRF24_PIPE0_MASK : 0U, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_WriteRegister(p_transport, NRF24_REG_EN_RXADDR, NRF24_PIPE0_MASK, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_WriteRegister(p_transport, NRF24_REG_SETUP_RETR,
                                 (uint8_t) ((p_config->retransmit_delay << 4U) |
                                            p_config->retransmit_count), p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    if (p_config->dynamic_payload_enabled)
    {
        feature_value |= NRF24_FEATURE_EN_DPL;
        dynpd_value = NRF24_PIPE0_MASK;
    }
    if (p_config->ack_payload_enabled)
    {
        feature_value |= NRF24_FEATURE_EN_ACK_PAY;
        feature_value |= NRF24_FEATURE_EN_DPL;
        dynpd_value = NRF24_PIPE0_MASK;
    }
    if (p_config->dynamic_ack_enabled)
    {
        feature_value |= NRF24_FEATURE_EN_DYN_ACK;
    }

    result = nrf24_set_feature(p_transport, feature_value, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_WriteRegister(p_transport, NRF24_REG_DYNPD, dynpd_value, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_WriteRegister(p_transport, NRF24_REG_RX_PW_P0,
                                 (p_config->dynamic_payload_enabled || p_config->ack_payload_enabled) ?
                                 0U : p_config->payload_width,
                                 p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    config_value = 0U;
    if (NRF24_CRC_DISABLED != p_config->crc_mode)
    {
        config_value |= NRF24_CONFIG_EN_CRC;
    }
    if (NRF24_CRC_2_BYTES == p_config->crc_mode)
    {
        config_value |= NRF24_CONFIG_CRCO;
    }
    if (p_config->auto_ack_enabled)
    {
        config_value |= NRF24_CONFIG_EN_CRC;
    }

    result = Nrf24_WriteRegister(p_transport, NRF24_REG_CONFIG, config_value, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_FlushTx(p_transport, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_FlushRx(p_transport, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_ClearIrqFlags(p_transport, NRF24_STATUS_IRQ_MASK, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    return Nrf24_SetRole(p_transport, p_config->initial_role, p_status);
}

nrf24_result_t Nrf24_TestConnection(nrf24_transport_t const * p_transport,
                                    bool * p_connected)
{
    nrf24_result_t result;
    uint8_t original_channel;
    uint8_t verify_channel;
    uint8_t test_channel;
    uint8_t status = 0U;

    if (NULL == p_connected)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    *p_connected = false;
    result = Nrf24_ReadRegister(p_transport, NRF24_REG_RF_CH, &original_channel, &status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    test_channel = (uint8_t) ((original_channel + 37U) % (NRF24_RF_CHANNEL_MAX + 1U));
    result = Nrf24_SetChannel(p_transport, test_channel, &status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_ReadRegister(p_transport, NRF24_REG_RF_CH, &verify_channel, &status);
    (void) Nrf24_SetChannel(p_transport, original_channel, &status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    *p_connected = (test_channel == verify_channel);
    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t Nrf24_Send(nrf24_transport_t const * p_transport,
                          uint8_t const * p_payload,
                          uint8_t length,
                          bool no_ack,
                          uint32_t timeout_ms,
                          nrf24_tx_result_t * p_result)
{
    nrf24_result_t result;
    uint8_t status = 0U;
    uint8_t observe_tx = 0U;
    uint8_t event_status;
    uint32_t elapsed_ms;

    if ((NULL == p_payload) || (NULL == p_result) || (0U == length) ||
        (NRF24_MAX_PAYLOAD_SIZE < length) || (0U == timeout_ms))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = nrf24_check_control_transport(p_transport);
    if (NRF24_RESULT_SUCCESS != result) return result;
    (void) memset(p_result, 0, sizeof(*p_result));

    result = Nrf24_SetRole(p_transport, NRF24_ROLE_TRANSMITTER, &status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_FlushTx(p_transport, &status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_ClearIrqFlags(p_transport, NRF24_STATUS_IRQ_MASK, &status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_WriteTxPayload(p_transport, p_payload, length, no_ack, &status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    result = p_transport->ce_write(p_transport->p_context, true);
    if (NRF24_RESULT_SUCCESS != result) return result;
    /* 数据手册要求 CE 高电平至少保持 10 微秒；取 30 微秒以覆盖软件延时误差。 */
    p_transport->delay_us(p_transport->p_context, 30U);
    result = p_transport->ce_write(p_transport->p_context, false);
    if (NRF24_RESULT_SUCCESS != result) return result;

    for (elapsed_ms = 0U; elapsed_ms < timeout_ms; elapsed_ms++)
    {
        bool irq_active = false;

        if (NULL != p_transport->irq_read)
        {
            result = p_transport->irq_read(p_transport->p_context, &irq_active);
            if (NRF24_RESULT_SUCCESS != result) return result;
        }

        /* IRQ 是加速提示，不作为判断发送结束的唯一依据。
         * 即使 IRQ 引脚配置或连线异常，轮询 STATUS 仍能得到 TX_DS 或 MAX_RT。 */
        result = Nrf24_ReadStatus(p_transport, &status);
        if (NRF24_RESULT_SUCCESS != result) return result;

        if (0U != (status & NRF24_STATUS_MAX_RT))
        {
            event_status = status;
            (void) Nrf24_ReadRegister(p_transport, NRF24_REG_OBSERVE_TX, &observe_tx, &status);
            (void) Nrf24_ClearIrqFlags(p_transport, NRF24_STATUS_MAX_RT, &status);
            (void) Nrf24_FlushTx(p_transport, &status);
            p_result->status = event_status;
            p_result->observe_tx = observe_tx;
            return NRF24_RESULT_MAX_RETRANSMIT;
        }

        if (0U != (status & NRF24_STATUS_TX_DS))
        {
            (void) Nrf24_ReadRegister(p_transport, NRF24_REG_OBSERVE_TX, &observe_tx, &status);
            p_result->status = status;
            p_result->observe_tx = observe_tx;
            p_result->ack_payload_available = (0U != (status & NRF24_STATUS_RX_DR));
            (void) Nrf24_ClearIrqFlags(p_transport, NRF24_STATUS_TX_DS, &status);
            return NRF24_RESULT_SUCCESS;
        }

        /* 读取 IRQ 仍有价值：调试器可在断点处观察它是否与 STATUS 事件一致。 */
        (void) irq_active;
        p_transport->delay_ms(p_transport->p_context, 1U);
    }

    (void) Nrf24_ReadStatus(p_transport, &p_result->status);
    (void) Nrf24_ReadRegister(p_transport, NRF24_REG_OBSERVE_TX, &p_result->observe_tx, &status);
    (void) Nrf24_ReadRegister(p_transport, NRF24_REG_CONFIG, &p_result->config, &status);
    (void) Nrf24_ReadFifoStatus(p_transport, &p_result->fifo_status, &status);
    return NRF24_RESULT_TIMEOUT;
}

nrf24_result_t Nrf24_SendBatchNoAck(nrf24_transport_t const * p_transport,
                                    uint8_t const * const p_payloads[],
                                    uint8_t const lengths[],
                                    uint8_t payload_count,
                                    uint32_t timeout_ms,
                                    nrf24_tx_result_t * p_result)
{
    nrf24_result_t result;
    uint8_t status = 0U;
    uint8_t fifo_status = 0U;
    uint32_t burst_time_us;

    if ((NULL == p_payloads) || (NULL == lengths) || (NULL == p_result) ||
        (0U == payload_count) || (3U < payload_count) || (0U == timeout_ms))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    for (uint32_t index = 0U; index < payload_count; index++)
    {
        if ((NULL == p_payloads[index]) || (0U == lengths[index]) ||
            (NRF24_MAX_PAYLOAD_SIZE < lengths[index]))
        {
            return NRF24_RESULT_INVALID_ARGUMENT;
        }
    }

    result = nrf24_check_control_transport(p_transport);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }
    (void) memset(p_result, 0, sizeof(*p_result));

    /* The SPI1 radio is initialized once in PTX mode and remains there.  Do
     * not read/rewrite CONFIG or flush TX for every image payload. */
    result = p_transport->ce_write(p_transport->p_context, false);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_ClearIrqFlags(p_transport,
                                 NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT,
                                 &status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    /* nRF24L01+ has a three-payload TX FIFO.  Preload all available image
     * fragments before asserting CE so the radio sends them back-to-back. */
    for (uint32_t index = 0U; index < payload_count; index++)
    {
        result = Nrf24_WriteTxPayload(p_transport,
                                      p_payloads[index],
                                      lengths[index],
                                      true,
                                      &status);
        if (NRF24_RESULT_SUCCESS != result)
        {
            (void) Nrf24_FlushTx(p_transport, &status);
            return result;
        }
    }

    result = p_transport->ce_write(p_transport->p_context, true);
    if (NRF24_RESULT_SUCCESS != result)
    {
        (void) Nrf24_FlushTx(p_transport, &status);
        return result;
    }

    /* Do not access SPI while CE is high and the radio is actively draining
     * its TX FIFO.  Some modules/clones behave poorly when FIFO_STATUS is
     * repeatedly read during a continuous RF burst.  At 2 Mbps a 32-byte
     * NO_ACK payload plus state transition fits comfortably in 350 us, so
     * this bounded wait covers all one-to-three payload bursts without the
     * previous 50 ms polling stall. */
    burst_time_us = 150U + ((uint32_t) payload_count * 350U);
    p_transport->delay_us(p_transport->p_context, burst_time_us);
    result = p_transport->ce_write(p_transport->p_context, false);
    if (NRF24_RESULT_SUCCESS != result)
    {
        (void) Nrf24_FlushTx(p_transport, &status);
        return result;
    }

    result = Nrf24_ReadFifoStatus(p_transport, &fifo_status, &status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        (void) Nrf24_FlushTx(p_transport, &status);
        return result;
    }

    p_result->status = status;
    p_result->fifo_status = fifo_status;
    if (0U == (fifo_status & NRF24_FIFO_STATUS_TX_EMPTY))
    {
        (void) Nrf24_FlushTx(p_transport, &status);
        return NRF24_RESULT_TIMEOUT;
    }

    (void) Nrf24_ClearIrqFlags(p_transport, NRF24_STATUS_TX_DS, &status);
    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t Nrf24_IsRxPayloadAvailable(nrf24_transport_t const * p_transport,
                                          bool * p_available,
                                          uint8_t * p_pipe,
                                          uint8_t * p_status)
{
    nrf24_result_t result;
    uint8_t fifo_status;

    if ((NULL == p_available) || (NULL == p_pipe) || (NULL == p_status))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = Nrf24_ReadStatus(p_transport, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;
    result = Nrf24_ReadFifoStatus(p_transport, &fifo_status, p_status);
    if (NRF24_RESULT_SUCCESS != result) return result;

    *p_available = (0U == (fifo_status & NRF24_FIFO_STATUS_RX_EMPTY));
    *p_pipe = (uint8_t) ((*p_status & NRF24_STATUS_RX_P_NO_MASK) >> NRF24_STATUS_RX_P_NO_SHIFT);
    if (NRF24_PIPE_COUNT <= *p_pipe)
    {
        *p_pipe = 0xFFU;
    }

    return NRF24_RESULT_SUCCESS;
}
