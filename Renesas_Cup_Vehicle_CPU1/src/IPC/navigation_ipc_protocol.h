#ifndef IPC_NAVIGATION_IPC_PROTOCOL_H_
#define IPC_NAVIGATION_IPC_PROTOCOL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 32位短消息格式：魔数8位、动作8位、序号8位、校验8位。 */
#define NAV_IPC_MAGIC                    (0xA7U)
#define NAV_IPC_CHECK_XOR                (0x5AU)

typedef enum e_nav_ipc_action
{
    NAV_IPC_ACTION_STOP = 0,
    NAV_IPC_ACTION_FORWARD,
    NAV_IPC_ACTION_TURN_LEFT,
    NAV_IPC_ACTION_COUNT,
} nav_ipc_action_t;

static inline uint32_t nav_ipc_message_encode(nav_ipc_action_t action,
                                              uint8_t sequence)
{
    uint8_t const checksum =
        (uint8_t) (NAV_IPC_MAGIC ^ (uint8_t) action ^ sequence ^ NAV_IPC_CHECK_XOR);
    return ((uint32_t) NAV_IPC_MAGIC << 24U) |
           ((uint32_t) (uint8_t) action << 16U) |
           ((uint32_t) sequence << 8U) |
           checksum;
}

static inline bool nav_ipc_message_decode(uint32_t message,
                                          nav_ipc_action_t * p_action,
                                          uint8_t * p_sequence)
{
    uint8_t const magic = (uint8_t) (message >> 24U);
    uint8_t const action = (uint8_t) (message >> 16U);
    uint8_t const sequence = (uint8_t) (message >> 8U);
    uint8_t const checksum = (uint8_t) message;

    if((NULL == p_action) || (NULL == p_sequence) ||
       (NAV_IPC_MAGIC != magic) || (action >= (uint8_t) NAV_IPC_ACTION_COUNT) ||
       (checksum != (uint8_t) (magic ^ action ^ sequence ^ NAV_IPC_CHECK_XOR)))
    {
        return false;
    }

    *p_action = (nav_ipc_action_t) action;
    *p_sequence = sequence;
    return true;
}

#endif /* IPC_NAVIGATION_IPC_PROTOCOL_H_ */
