#include "Radio/protocol/control_protocol.h"

#include <string.h>

static uint8_t checksum_calculate(uint8_t const * p_data, uint32_t length)
{
    uint8_t checksum = 0U;
    for (uint32_t i = 0U; i < length; i++)
    {
        checksum ^= p_data[i];
    }
    return checksum;
}

bool ControlProtocol_Decode(uint8_t const * p_payload,
                            uint8_t payload_size,
                            control_packet_t * p_packet,
                            uint16_t * p_value)
{
    if ((NULL == p_payload) || (NULL == p_packet) || (NULL == p_value) ||
        (CONTROL_PACKET_SIZE != payload_size))
    {
        return false;
    }

    (void) memcpy(p_packet, p_payload, sizeof(*p_packet));
    if ((CONTROL_PACKET_MAGIC != p_packet->magic) ||
        (CONTROL_PACKET_VERSION != p_packet->version) ||
        (p_packet->checksum != checksum_calculate(p_payload, CONTROL_PACKET_SIZE - 1U)))
    {
        return false;
    }

    *p_value = (uint16_t) p_packet->value_lsb |
               (uint16_t) ((uint16_t) p_packet->value_msb << 8U);
    return true;
}
