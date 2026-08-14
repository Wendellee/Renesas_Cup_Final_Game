#ifndef RADIO_PROTOCOL_CONTROL_PROTOCOL_H_
#define RADIO_PROTOCOL_CONTROL_PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>

#define CONTROL_PACKET_MAGIC          (0xA5U)
#define CONTROL_PACKET_VERSION        (1U)
#define CONTROL_PACKET_SIZE           (8U)

typedef enum e_control_id
{
    CONTROL_ID_DIRECTION = 1,
    CONTROL_ID_RUN_STOP,
    CONTROL_ID_SPEED,
    CONTROL_ID_MODE,
    CONTROL_ID_LED,
    CONTROL_ID_FAN,
    CONTROL_ID_WIFI,
    CONTROL_ID_PAGE
} control_id_t;

typedef enum e_control_action
{
    CONTROL_ACTION_RELEASED = 0,
    CONTROL_ACTION_PRESSED,
    CONTROL_ACTION_CHANGED
} control_action_t;

typedef enum e_control_direction
{
    CONTROL_DIRECTION_STOP = 0,
    CONTROL_DIRECTION_FORWARD,
    CONTROL_DIRECTION_BACK,
    CONTROL_DIRECTION_LEFT,
    CONTROL_DIRECTION_RIGHT
} control_direction_t;

/** 与手持端 wireless_touch_packet_t 字节布局完全一致。 */
typedef struct st_control_packet
{
    uint8_t magic;
    uint8_t version;
    uint8_t control;
    uint8_t action;
    uint8_t value_lsb;
    uint8_t value_msb;
    uint8_t sequence;
    uint8_t checksum;
} control_packet_t;

bool ControlProtocol_Decode(uint8_t const * p_payload,
                            uint8_t payload_size,
                            control_packet_t * p_packet,
                            uint16_t * p_value);

#endif /* RADIO_PROTOCOL_CONTROL_PROTOCOL_H_ */
