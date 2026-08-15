#include "nrf24/wireless_touch_tx.h"

#include <string.h>

#include "bsp/nrf24_port.h"
#include "FreeRTOS.h"
#include "task.h"

#define WIRELESS_TOUCH_TIMEOUT_MS  (50U)

typedef struct st_wireless_radio_ring_slot
{
    uint8_t length;
    bool    no_ack;
    uint8_t data[WIRELESS_RADIO_MAX_PAYLOAD_LENGTH];
} wireless_radio_ring_slot_t;

static nrf24_transport_t g_touch_tx_transport;
static nrf24_transport_t g_touch_rx_transport;
static bool              g_touch_tx_radio_ready;
static bool              g_touch_rx_radio_ready;
static bool              g_touch_tx_connected;
static bool              g_touch_rx_connected;
static nrf24_result_t    g_touch_tx_init_result;
static nrf24_result_t    g_touch_rx_init_result;
static uint8_t           g_touch_sequence;
static wireless_radio_ring_slot_t g_radio_tx_ring[WIRELESS_RADIO_TX_RING_CAPACITY];
static wireless_radio_ring_slot_t g_radio_rx_ring[WIRELESS_RADIO_RX_RING_CAPACITY];
static uint32_t g_radio_tx_ring_head;
static uint32_t g_radio_tx_ring_tail;
static uint32_t g_radio_tx_ring_count;
static uint32_t g_radio_rx_ring_head;
static uint32_t g_radio_rx_ring_tail;
static uint32_t g_radio_rx_ring_count;
static uint8_t const g_command_radio_address[WIRELESS_RADIO_ADDRESS_WIDTH] =
    WIRELESS_COMMAND_ADDRESS_BYTES;
static uint8_t const g_video_radio_address[WIRELESS_RADIO_ADDRESS_WIDTH] =
    WIRELESS_VIDEO_ADDRESS_BYTES;

static uint8_t wireless_touch_checksum(uint8_t const * p_data, uint32_t length)
{
    uint8_t checksum = 0U;

    for (uint32_t i = 0U; i < length; i++)
    {
        checksum ^= p_data[i];
    }

    return checksum;
}

nrf24_result_t WirelessTouchTx_Init(void)
{
    return WirelessRemoteLinks_Init();
}

nrf24_result_t WirelessRemoteLinks_Init(void)
{
    nrf24_config_t tx_config;
    nrf24_config_t rx_config;
    nrf24_result_t result;
    nrf24_result_t port_result;
    uint8_t status = 0U;

    g_touch_tx_radio_ready = false;
    g_touch_rx_radio_ready = false;
    g_touch_tx_connected   = false;
    g_touch_rx_connected   = false;
    g_touch_tx_init_result = NRF24_RESULT_TRANSPORT_ERROR;
    g_touch_rx_init_result = NRF24_RESULT_TRANSPORT_ERROR;
    g_touch_sequence       = 0U;
    g_radio_tx_ring_head   = 0U;
    g_radio_tx_ring_tail   = 0U;
    g_radio_tx_ring_count  = 0U;
    g_radio_rx_ring_head   = 0U;
    g_radio_rx_ring_tail   = 0U;
    g_radio_rx_ring_count  = 0U;
    (void) memset(g_radio_tx_ring, 0, sizeof(g_radio_tx_ring));
    (void) memset(g_radio_rx_ring, 0, sizeof(g_radio_rx_ring));
    (void) memset(&g_touch_tx_transport, 0, sizeof(g_touch_tx_transport));
    (void) memset(&g_touch_rx_transport, 0, sizeof(g_touch_rx_transport));

    if (FSP_SUCCESS != Nrf24Port_Init())
    {
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    port_result = Nrf24Port_GetTransport(NRF24_RX_MODULE, &g_touch_rx_transport);
    if (NRF24_RESULT_SUCCESS != port_result)
    {
        return port_result;
    }

    port_result = Nrf24Port_GetTransport(NRF24_TX_MODULE, &g_touch_tx_transport);
    if (NRF24_RESULT_SUCCESS != port_result)
    {
        return port_result;
    }

    /* Both nRF24L01+ modules need up to 100 ms after power is applied. */
    g_touch_rx_transport.delay_ms(g_touch_rx_transport.p_context, 100U);

    Nrf24_GetDefaultConfig(&rx_config);
    rx_config.channel                 = WIRELESS_VIDEO_RX_CHANNEL;
    rx_config.payload_width           = WIRELESS_RADIO_MAX_PAYLOAD_LENGTH;
    rx_config.initial_role            = NRF24_ROLE_RECEIVER;
    /* Command TX uses hardware ACK and the driver's retry policy.  Video RX
     * accepts the vehicle's dynamic 32-byte packets on its separate channel;
     * reliability of the video framing is defined by the shared protocol. */
    rx_config.auto_ack_enabled        = true;
    rx_config.dynamic_payload_enabled = true;
    rx_config.ack_payload_enabled     = false;
    rx_config.dynamic_ack_enabled     = true;
    rx_config.data_rate               = NRF24_DATA_RATE_2MBPS;
    (void) memcpy(rx_config.tx_address,
                  g_video_radio_address,
                  sizeof(g_video_radio_address));
    (void) memcpy(rx_config.rx_pipe0_address,
                  g_video_radio_address,
                  sizeof(g_video_radio_address));
#if WIRELESS_RADIO_NEAR_FIELD_LOOPBACK_TEST
    rx_config.power                    = NRF24_POWER_NEGATIVE_18_DBM;
#endif

    tx_config = rx_config;
    tx_config.channel       = WIRELESS_COMMAND_TX_CHANNEL;
    tx_config.payload_width = WIRELESS_TOUCH_PAYLOAD_LENGTH;
    tx_config.initial_role  = NRF24_ROLE_TRANSMITTER;
    (void) memcpy(tx_config.tx_address,
                  g_command_radio_address,
                  sizeof(g_command_radio_address));
    (void) memcpy(tx_config.rx_pipe0_address,
                  g_command_radio_address,
                  sizeof(g_command_radio_address));

    /* Probe and initialize both radios independently so one wiring fault does not hide the other result. */
    result = Nrf24_TestConnection(&g_touch_rx_transport, &g_touch_rx_connected);
    if ((NRF24_RESULT_SUCCESS == result) && g_touch_rx_connected)
    {
        g_touch_rx_init_result = Nrf24_Initialize(&g_touch_rx_transport, &rx_config, &status);
        g_touch_rx_radio_ready = (NRF24_RESULT_SUCCESS == g_touch_rx_init_result);
    }
    else
    {
        g_touch_rx_init_result = (NRF24_RESULT_SUCCESS == result) ? NRF24_RESULT_TRANSPORT_ERROR : result;
    }

    result = Nrf24_TestConnection(&g_touch_tx_transport, &g_touch_tx_connected);
    if ((NRF24_RESULT_SUCCESS == result) && g_touch_tx_connected)
    {
        g_touch_tx_init_result = Nrf24_Initialize(&g_touch_tx_transport, &tx_config, &status);
        g_touch_tx_radio_ready = (NRF24_RESULT_SUCCESS == g_touch_tx_init_result);
    }
    else
    {
        g_touch_tx_init_result = (NRF24_RESULT_SUCCESS == result) ? NRF24_RESULT_TRANSPORT_ERROR : result;
    }

#if NRF24_RX_IRQ_NOTIFICATION_ENABLE
    if (g_touch_rx_radio_ready && (FSP_SUCCESS != Nrf24Port_RxIrqOpen()))
    {
        g_touch_rx_radio_ready = false;
        g_touch_rx_init_result = NRF24_RESULT_TRANSPORT_ERROR;
    }
#endif

    if (NRF24_RESULT_SUCCESS != g_touch_rx_init_result)
    {
        return g_touch_rx_init_result;
    }

    return g_touch_tx_init_result;
}

bool WirelessTouchTx_IsReady(void)
{
    return g_touch_tx_radio_ready;
}

bool WirelessTouchRx_IsReady(void)
{
    return g_touch_rx_radio_ready;
}

nrf24_result_t WirelessTouchTx_GetInitResult(void)
{
    return g_touch_tx_init_result;
}

nrf24_result_t WirelessTouchRx_GetInitResult(void)
{
    return g_touch_rx_init_result;
}

bool WirelessTouchTx_IsConnected(void)
{
    return g_touch_tx_connected;
}

bool WirelessTouchRx_IsConnected(void)
{
    return g_touch_rx_connected;
}

nrf24_result_t WirelessTouchTx_Send(wireless_touch_control_t control,
                                    wireless_touch_action_t action,
                                    uint16_t value)
{
    wireless_touch_packet_t packet;

    packet.magic     = WIRELESS_TOUCH_MAGIC;
    packet.version   = WIRELESS_TOUCH_VERSION;
    packet.control   = (uint8_t) control;
    packet.action    = (uint8_t) action;
    packet.value_lsb = (uint8_t) value;
    packet.value_msb = (uint8_t) (value >> 8U);
    packet.sequence  = g_touch_sequence++;
    packet.checksum  = wireless_touch_checksum((uint8_t const *) &packet,
                                               WIRELESS_TOUCH_PAYLOAD_LENGTH - 1U);

    return WirelessRadioTx_SendPayload((uint8_t const *) &packet,
                                       WIRELESS_TOUCH_PAYLOAD_LENGTH);
}

nrf24_result_t WirelessRadioTx_SendPayload(uint8_t const * p_payload,
                                           uint8_t payload_length)
{
    wireless_radio_ring_slot_t * p_slot;

    if ((NULL == p_payload) || (0U == payload_length) ||
        (WIRELESS_RADIO_MAX_PAYLOAD_LENGTH < payload_length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    if (!g_touch_tx_radio_ready)
    {
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    taskENTER_CRITICAL();
    if (g_radio_tx_ring_count >= WIRELESS_RADIO_TX_RING_CAPACITY)
    {
        taskEXIT_CRITICAL();
        return NRF24_RESULT_QUEUE_FULL;
    }

    p_slot = &g_radio_tx_ring[g_radio_tx_ring_tail];
    p_slot->length = payload_length;
    p_slot->no_ack = false;
    (void) memcpy(p_slot->data, p_payload, payload_length);
    g_radio_tx_ring_tail = (g_radio_tx_ring_tail + 1U) % WIRELESS_RADIO_TX_RING_CAPACITY;
    g_radio_tx_ring_count++;
    taskEXIT_CRITICAL();

    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t WirelessRadioTx_SendPayloadNoAck(uint8_t const * p_payload,
                                                uint8_t payload_length)
{
    wireless_radio_ring_slot_t * p_slot;

    if ((NULL == p_payload) || (0U == payload_length) ||
        (WIRELESS_RADIO_MAX_PAYLOAD_LENGTH < payload_length))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    if (!g_touch_tx_radio_ready)
    {
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    taskENTER_CRITICAL();
    if (g_radio_tx_ring_count >= WIRELESS_RADIO_TX_RING_CAPACITY)
    {
        taskEXIT_CRITICAL();
        return NRF24_RESULT_QUEUE_FULL;
    }

    p_slot = &g_radio_tx_ring[g_radio_tx_ring_tail];
    p_slot->length = payload_length;
    p_slot->no_ack = true;
    (void) memcpy(p_slot->data, p_payload, payload_length);
    g_radio_tx_ring_tail = (g_radio_tx_ring_tail + 1U) % WIRELESS_RADIO_TX_RING_CAPACITY;
    g_radio_tx_ring_count++;
    taskEXIT_CRITICAL();

    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t WirelessRadioTx_Service(uint32_t * p_packets_sent)
{
    wireless_radio_ring_slot_t * p_slot;
    nrf24_tx_result_t tx_result;
    nrf24_result_t result;

    if (NULL == p_packets_sent)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    *p_packets_sent = 0U;
    if (!g_touch_tx_radio_ready)
    {
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    if (0U == g_radio_tx_ring_count)
    {
        return NRF24_RESULT_SUCCESS;
    }

    p_slot = &g_radio_tx_ring[g_radio_tx_ring_head];
    if (p_slot->no_ack)
    {
        uint8_t const * payloads[3];
        uint8_t lengths[3];
        uint32_t batch_count = 0U;

        /* Only combine consecutive NO_ACK image payloads.  An acknowledged
         * control packet at the ring head is always sent separately. */
        while ((batch_count < 3U) && (batch_count < g_radio_tx_ring_count))
        {
            uint32_t slot_index =
                (g_radio_tx_ring_head + batch_count) % WIRELESS_RADIO_TX_RING_CAPACITY;
            wireless_radio_ring_slot_t * p_batch_slot = &g_radio_tx_ring[slot_index];

            if (!p_batch_slot->no_ack)
            {
                break;
            }
            payloads[batch_count] = p_batch_slot->data;
            lengths[batch_count] = p_batch_slot->length;
            batch_count++;
        }

        result = Nrf24_SendBatchNoAck(&g_touch_tx_transport,
                                      payloads,
                                      lengths,
                                      (uint8_t) batch_count,
                                      WIRELESS_TOUCH_TIMEOUT_MS,
                                      &tx_result);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }

        for (uint32_t index = 0U; index < batch_count; index++)
        {
            taskENTER_CRITICAL();
            p_slot = &g_radio_tx_ring[g_radio_tx_ring_head];
            p_slot->length = 0U;
            p_slot->no_ack = false;
            g_radio_tx_ring_head =
                (g_radio_tx_ring_head + 1U) % WIRELESS_RADIO_TX_RING_CAPACITY;
            g_radio_tx_ring_count--;
            taskEXIT_CRITICAL();
        }
        *p_packets_sent = batch_count;
        return NRF24_RESULT_SUCCESS;
    }

    result = Nrf24_Send(&g_touch_tx_transport,
                        p_slot->data,
                        p_slot->length,
                        false,
                        WIRELESS_TOUCH_TIMEOUT_MS,
                        &tx_result);
    if (NRF24_RESULT_SUCCESS != result)
    {
        /* Keep the head packet in the ring so the next service call retries exactly the same packet. */
        return result;
    }

    taskENTER_CRITICAL();
    p_slot->length = 0U;
    p_slot->no_ack = false;
    g_radio_tx_ring_head = (g_radio_tx_ring_head + 1U) % WIRELESS_RADIO_TX_RING_CAPACITY;
    g_radio_tx_ring_count--;
    taskEXIT_CRITICAL();
    *p_packets_sent = 1U;
    return NRF24_RESULT_SUCCESS;
}

uint32_t WirelessRadioTx_RingCountGet(void)
{
    uint32_t count;
    taskENTER_CRITICAL();
    count = g_radio_tx_ring_count;
    taskEXIT_CRITICAL();
    return count;
}

nrf24_result_t WirelessRadioRx_PollPayload(uint8_t * p_payload,
                                           uint8_t payload_capacity,
                                           uint8_t * p_payload_length,
                                           bool * p_received)
{
    nrf24_result_t result;
    uint8_t status = 0U;
    uint8_t pipe = 0U;
    uint8_t payload_width = 0U;
    bool available = false;

    if ((NULL == p_payload) || (NULL == p_payload_length) || (NULL == p_received) ||
        (0U == payload_capacity))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    *p_received = false;
    *p_payload_length = 0U;
    if (!g_touch_rx_radio_ready)
    {
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    result = Nrf24_IsRxPayloadAvailable(&g_touch_rx_transport, &available, &pipe, &status);
    if ((NRF24_RESULT_SUCCESS != result) || !available)
    {
        return result;
    }

    result = Nrf24_GetRxPayloadWidth(&g_touch_rx_transport, &payload_width, &status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        (void) Nrf24_ClearIrqFlags(&g_touch_rx_transport, NRF24_STATUS_RX_DR, &status);
        return result;
    }

    if ((0U == payload_width) || (WIRELESS_RADIO_MAX_PAYLOAD_LENGTH < payload_width) ||
        (payload_capacity < payload_width))
    {
        (void) Nrf24_FlushRx(&g_touch_rx_transport, &status);
        (void) Nrf24_ClearIrqFlags(&g_touch_rx_transport, NRF24_STATUS_RX_DR, &status);
        return NRF24_RESULT_INVALID_PAYLOAD_WIDTH;
    }

    result = Nrf24_ReadRxPayload(&g_touch_rx_transport,
                                 p_payload,
                                 payload_width,
                                 &status);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    (void) Nrf24_ClearIrqFlags(&g_touch_rx_transport, NRF24_STATUS_RX_DR, &status);
    *p_payload_length = payload_width;
    *p_received = true;
    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t WirelessRadioRx_Service(uint32_t max_packets, uint32_t * p_queued_packets)
{
    nrf24_result_t result = NRF24_RESULT_SUCCESS;

    if ((0U == max_packets) || (NULL == p_queued_packets))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    *p_queued_packets = 0U;
    for (uint32_t packet_index = 0U; packet_index < max_packets; packet_index++)
    {
        wireless_radio_ring_slot_t * p_slot;
        bool received = false;
        uint8_t payload_length = 0U;

        if (g_radio_rx_ring_count >= WIRELESS_RADIO_RX_RING_CAPACITY)
        {
            return NRF24_RESULT_QUEUE_FULL;
        }

        p_slot = &g_radio_rx_ring[g_radio_rx_ring_tail];
        result = WirelessRadioRx_PollPayload(p_slot->data,
                                             WIRELESS_RADIO_MAX_PAYLOAD_LENGTH,
                                             &payload_length,
                                             &received);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }
        if (!received)
        {
            break;
        }

        p_slot->length = payload_length;
        g_radio_rx_ring_tail = (g_radio_rx_ring_tail + 1U) % WIRELESS_RADIO_RX_RING_CAPACITY;
        g_radio_rx_ring_count++;
        (*p_queued_packets)++;
    }

    return NRF24_RESULT_SUCCESS;
}

nrf24_result_t WirelessRadioRx_DequeuePayload(uint8_t * p_payload,
                                              uint8_t payload_capacity,
                                              uint8_t * p_payload_length,
                                              bool * p_received)
{
    wireless_radio_ring_slot_t * p_slot;

    if ((NULL == p_payload) || (NULL == p_payload_length) || (NULL == p_received) ||
        (0U == payload_capacity))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    *p_payload_length = 0U;
    *p_received = false;
    if (0U == g_radio_rx_ring_count)
    {
        return NRF24_RESULT_SUCCESS;
    }

    p_slot = &g_radio_rx_ring[g_radio_rx_ring_head];
    if (payload_capacity < p_slot->length)
    {
        return NRF24_RESULT_INVALID_PAYLOAD_WIDTH;
    }

    (void) memcpy(p_payload, p_slot->data, p_slot->length);
    *p_payload_length = p_slot->length;
    *p_received = true;
    p_slot->length = 0U;
    g_radio_rx_ring_head = (g_radio_rx_ring_head + 1U) % WIRELESS_RADIO_RX_RING_CAPACITY;
    g_radio_rx_ring_count--;

    return NRF24_RESULT_SUCCESS;
}

uint32_t WirelessRadioRx_RingCountGet(void)
{
    return g_radio_rx_ring_count;
}

nrf24_result_t WirelessTouchRx_Poll(wireless_touch_packet_t * p_packet,
                                    bool * p_received)
{
    nrf24_result_t result;
    uint8_t payload_length = 0U;

    if ((NULL == p_packet) || (NULL == p_received))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    result = WirelessRadioRx_PollPayload((uint8_t *) p_packet,
                                         WIRELESS_TOUCH_PAYLOAD_LENGTH,
                                         &payload_length,
                                         p_received);
    if ((NRF24_RESULT_SUCCESS == result) && *p_received &&
        (WIRELESS_TOUCH_PAYLOAD_LENGTH != payload_length))
    {
        *p_received = false;
        return NRF24_RESULT_INVALID_PAYLOAD_WIDTH;
    }

    return result;
}

bool WirelessTouchPacket_IsValid(wireless_touch_packet_t const * p_packet)
{
    if (NULL == p_packet)
    {
        return false;
    }

    return ((WIRELESS_TOUCH_MAGIC == p_packet->magic) &&
            (WIRELESS_TOUCH_VERSION == p_packet->version) &&
            (p_packet->checksum == wireless_touch_checksum((uint8_t const *) p_packet,
                                                            WIRELESS_TOUCH_PAYLOAD_LENGTH - 1U)));
}
