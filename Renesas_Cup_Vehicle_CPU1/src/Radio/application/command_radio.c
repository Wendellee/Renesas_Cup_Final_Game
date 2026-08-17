#include "Radio/application/command_radio.h"
#include "SEGGER_RTT/bsp_print.h"

#include "Radio/platform/fsp_nrf24_port.h"
#include "Radio/protocol/control_protocol.h"
#include "Vehicle/adapters/rtos/vehicle_command_mailbox.h"
#include "hal_data.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define COMMAND_RADIO_CHANNEL          (76U)
#define COMMAND_RADIO_MAX_FIFO_PACKETS (3U)

static nrf24_transport_t g_transport;
static uint8_t g_last_sequence;
static bool g_have_sequence;
static bool g_run_enabled;
static bool g_automatic_mode;
static uint8_t g_speed_percent = 50U;
static uint8_t const g_command_radio_address[NRF24_ADDRESS_WIDTH_MAX] =
{
    0x43U, 0x4DU, 0x44U, 0x52U, 0x58U /* "CMDRX" */
};

static uint8_t speed_index_to_percent(uint16_t index)
{
    static uint8_t const speed_table[] = {50U, 63U, 75U, 88U, 100U};
    return (index < (sizeof(speed_table) / sizeof(speed_table[0]))) ?
           speed_table[index] : 100U;
}

static bool sequence_is_new(uint8_t sequence)
{
    if (!g_have_sequence)
    {
        g_have_sequence = true;
        g_last_sequence = sequence;
        return true;
    }

    uint8_t const delta = (uint8_t) (sequence - g_last_sequence);
    if ((0U == delta) || (delta >= 128U))
    {
        return false;
    }

    g_last_sequence = sequence;
    return true;
}

static bool submit_vehicle_command(control_packet_t const * p_packet, uint16_t value)
{
    vehicle_command_t command =
    {
        .source = VEHICLE_COMMAND_SOURCE_NRF,
        .sequence = p_packet->sequence,
        .received_tick = xTaskGetTickCount(),
        .speed_percent = g_speed_percent,
    };

    switch ((control_id_t) p_packet->control)
    {
        case CONTROL_ID_DIRECTION:
            command.kind = VEHICLE_COMMAND_MANUAL;
            if (CONTROL_ACTION_RELEASED == p_packet->action)
            {
                command.manual_action = VEHICLE_MANUAL_STOP;
            }
            else
            {
                switch ((control_direction_t) value)
                {
                    case CONTROL_DIRECTION_FORWARD: command.manual_action = VEHICLE_MANUAL_FORWARD; break;
                    case CONTROL_DIRECTION_BACK:    command.manual_action = VEHICLE_MANUAL_REVERSE; break;
                    case CONTROL_DIRECTION_LEFT:    command.manual_action = VEHICLE_MANUAL_TURN_LEFT; break;
                    case CONTROL_DIRECTION_RIGHT:   command.manual_action = VEHICLE_MANUAL_TURN_RIGHT; break;
                    default:                        command.manual_action = VEHICLE_MANUAL_STOP; break;
                }
            }
            break;

        case CONTROL_ID_RUN_STOP:
            g_run_enabled = (0U != value);
            if (g_run_enabled && g_automatic_mode)
            {
                command.kind = VEHICLE_COMMAND_START_AUTOMATIC;
            }
            else
            {
                command.kind = VEHICLE_COMMAND_MANUAL;
                command.manual_action = VEHICLE_MANUAL_STOP;
            }
            break;

        case CONTROL_ID_SPEED:
            g_speed_percent = speed_index_to_percent(value);
            command.kind = VEHICLE_COMMAND_SET_SPEED;
            command.speed_percent = g_speed_percent;
            break;

        case CONTROL_ID_MODE:
            g_automatic_mode = (0U != value);
            command.kind = VEHICLE_COMMAND_SET_MODE;
            command.mode = g_automatic_mode ? VEHICLE_MODE_AUTOMATIC : VEHICLE_MODE_MANUAL;
            break;

        case CONTROL_ID_FAN:
            g_printf("[CMD NRF] fan=%s seq=%u.\r\n",
                     (0U != value) ? "ON" : "OFF",
                     (unsigned int) p_packet->sequence);
            command.kind = VEHICLE_COMMAND_SET_SUCTION;
            command.suction_enable = (0U != value);
            command.suction_percent = 80U;
            break;

        case CONTROL_ID_LED:
            return FSP_SUCCESS == R_IOPORT_PinWrite(&g_ioport_ctrl,
                                                    BSP_IO_PORT_04_PIN_06,
                                                    (0U != value) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);

        /* Wi-Fi和页面属于手持功能，不应直接写底盘。 */
        case CONTROL_ID_WIFI:
        case CONTROL_ID_PAGE:
            return true;

        default:
            return false;
    }

    return vehicle_command_mailbox_submit(&command);
}

nrf24_result_t CommandRadio_Init(void)
{
    nrf24_config_t config;
    uint8_t status = 0U;
    bool connected = false;

    nrf24_result_t result = FspNrf24Port_Open(NRF24_PORT_COMMAND_RX, &g_transport);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = Nrf24_TestConnection(&g_transport, &connected);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }
    if (!connected)
    {
        g_printf("[CMD NRF] register-loopback mismatch\r\n");
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    Nrf24_GetDefaultConfig(&config);
    config.channel = COMMAND_RADIO_CHANNEL;
    config.payload_width = CONTROL_PACKET_SIZE;
    config.initial_role = NRF24_ROLE_RECEIVER;
    config.data_rate = NRF24_DATA_RATE_2MBPS;
    config.auto_ack_enabled = true;
    config.dynamic_payload_enabled = true;
    config.dynamic_ack_enabled = true;
    (void) memcpy(config.tx_address, g_command_radio_address, sizeof(g_command_radio_address));
    (void) memcpy(config.rx_pipe0_address, g_command_radio_address, sizeof(g_command_radio_address));

    g_have_sequence = false;
    g_run_enabled = false;
    g_automatic_mode = false;
    g_speed_percent = 50U;
    return Nrf24_Initialize(&g_transport, &config, &status);
}

nrf24_result_t CommandRadio_Service(uint32_t * p_processed_packets)
{
    if (NULL == p_processed_packets)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    *p_processed_packets = 0U;
    for (uint32_t i = 0U; i < COMMAND_RADIO_MAX_FIFO_PACKETS; i++)
    {
        bool available = false;
        uint8_t pipe = 0U;
        uint8_t status = 0U;
        nrf24_result_t result = Nrf24_IsRxPayloadAvailable(&g_transport,
                                                           &available,
                                                           &pipe,
                                                           &status);
        FSP_PARAMETER_NOT_USED(pipe);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }
        if (!available)
        {
            break;
        }

        uint8_t width = 0U;
        result = Nrf24_GetRxPayloadWidth(&g_transport, &width, &status);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }
        if ((0U == width) || (NRF24_MAX_PAYLOAD_SIZE < width))
        {
            (void) Nrf24_FlushRx(&g_transport, &status);
            return NRF24_RESULT_INVALID_PAYLOAD_WIDTH;
        }

        uint8_t payload[NRF24_MAX_PAYLOAD_SIZE] = {0U};
        result = Nrf24_ReadRxPayload(&g_transport, payload, width, &status);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }

        control_packet_t packet;
        uint16_t value = 0U;
        if (ControlProtocol_Decode(payload, width, &packet, &value) &&
            sequence_is_new(packet.sequence))
        {
            (void) submit_vehicle_command(&packet, value);
            (*p_processed_packets)++;
        }
    }

    uint8_t status = 0U;
    return Nrf24_ClearIrqFlags(&g_transport, NRF24_STATUS_RX_DR, &status);
}
