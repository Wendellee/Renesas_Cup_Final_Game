#ifndef WIRELESS_TOUCH_TX_H_
#define WIRELESS_TOUCH_TX_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf24/nrf24.h"

#define WIRELESS_TOUCH_MAGIC           (0xA5U)
#define WIRELESS_TOUCH_VERSION         (1U)
#define WIRELESS_TOUCH_CHANNEL         (76U)
#define WIRELESS_TOUCH_PAYLOAD_LENGTH  (8U)
#define WIRELESS_RADIO_MAX_PAYLOAD_LENGTH (32U)
#define WIRELESS_RADIO_TX_RING_CAPACITY (8U)
/* One radio-task pass may send eight bursts of three payloads.  Keep extra
 * room for START/END and a high-priority control packet without overflowing. */
#define WIRELESS_RADIO_RX_RING_CAPACITY (32U)

/*
 * 1: Two radios are mounted close together for the SPI1 -> SPI0 loopback test.
 *    Use -18 dBm to avoid near-field receiver/ACK saturation.
 * 0: Long-range vehicle build; use the driver's normal 0 dBm power.
 */
#ifndef WIRELESS_RADIO_NEAR_FIELD_LOOPBACK_TEST
#define WIRELESS_RADIO_NEAR_FIELD_LOOPBACK_TEST (1U)
#endif

typedef enum
{
    WIRELESS_TOUCH_CONTROL_DIRECTION = 1,
    WIRELESS_TOUCH_CONTROL_RUN_STOP,
    WIRELESS_TOUCH_CONTROL_SPEED,
    WIRELESS_TOUCH_CONTROL_MODE,
    WIRELESS_TOUCH_CONTROL_LED,
    WIRELESS_TOUCH_CONTROL_FAN,
    WIRELESS_TOUCH_CONTROL_WIFI,
    WIRELESS_TOUCH_CONTROL_PAGE
} wireless_touch_control_t;

typedef enum
{
    WIRELESS_TOUCH_ACTION_RELEASED = 0,
    WIRELESS_TOUCH_ACTION_PRESSED,
    WIRELESS_TOUCH_ACTION_CHANGED
} wireless_touch_action_t;

typedef enum
{
    WIRELESS_TOUCH_DIRECTION_STOP = 0,
    WIRELESS_TOUCH_DIRECTION_FORWARD,
    WIRELESS_TOUCH_DIRECTION_BACK,
    WIRELESS_TOUCH_DIRECTION_LEFT,
    WIRELESS_TOUCH_DIRECTION_RIGHT
} wireless_touch_direction_t;

typedef struct
{
    uint8_t magic;
    uint8_t version;
    uint8_t control;
    uint8_t action;
    uint8_t value_lsb;
    uint8_t value_msb;
    uint8_t sequence;
    uint8_t checksum;
} wireless_touch_packet_t;

nrf24_result_t WirelessTouchTx_Init(void);
bool WirelessTouchTx_IsReady(void);
bool WirelessTouchRx_IsReady(void);
nrf24_result_t WirelessTouchTx_GetInitResult(void);
nrf24_result_t WirelessTouchRx_GetInitResult(void);
bool WirelessTouchTx_IsConnected(void);
bool WirelessTouchRx_IsConnected(void);
nrf24_result_t WirelessTouchTx_Send(wireless_touch_control_t control,
                                    wireless_touch_action_t action,
                                    uint16_t value);
nrf24_result_t WirelessRadioTx_SendPayload(uint8_t const * p_payload,
                                           uint8_t payload_length);
nrf24_result_t WirelessRadioTx_SendPayloadNoAck(uint8_t const * p_payload,
                                                uint8_t payload_length);
nrf24_result_t WirelessRadioTx_Service(uint32_t * p_packets_sent);
uint32_t WirelessRadioTx_RingCountGet(void);
nrf24_result_t WirelessRadioRx_PollPayload(uint8_t * p_payload,
                                           uint8_t payload_capacity,
                                           uint8_t * p_payload_length,
                                           bool * p_received);
nrf24_result_t WirelessRadioRx_Service(uint32_t max_packets, uint32_t * p_queued_packets);
nrf24_result_t WirelessRadioRx_DequeuePayload(uint8_t * p_payload,
                                              uint8_t payload_capacity,
                                              uint8_t * p_payload_length,
                                              bool * p_received);
uint32_t WirelessRadioRx_RingCountGet(void);
nrf24_result_t WirelessTouchRx_Poll(wireless_touch_packet_t * p_packet,
                                    bool * p_received);
bool WirelessTouchPacket_IsValid(wireless_touch_packet_t const * p_packet);

#endif /* WIRELESS_TOUCH_TX_H_ */
