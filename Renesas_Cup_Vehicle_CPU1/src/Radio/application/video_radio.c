#include "Radio/application/video_radio.h"

#include "Radio/platform/fsp_nrf24_port.h"
#include "SEGGER_RTT/bsp_print.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define VIDEO_RADIO_CHANNEL              (100U)
#define VIDEO_RADIO_TIMEOUT_MS           (50U)
#define VIDEO_BATCH_SIZE                 (3U)
#define VIDEO_BATCHES_BEFORE_DELAY        (8U)
#define VIDEO_EXPECTED_WIDTH              (200U)
#define VIDEO_EXPECTED_HEIGHT             (112U)

static uint8_t const g_video_radio_address[NRF24_ADDRESS_WIDTH_MAX] =
{
    0x56U, 0x49U, 0x44U, 0x45U, 0x4FU /* "VIDEO" */
};

static nrf24_transport_t g_transport;

static nrf24_result_t packet_send_ack(uint8_t const packet[VIDEO_PACKET_SIZE])
{
    nrf24_tx_result_t tx_result;
    return Nrf24_Send(&g_transport,
                      packet,
                      VIDEO_PACKET_SIZE,
                      false,
                      VIDEO_RADIO_TIMEOUT_MS,
                      &tx_result);
}

nrf24_result_t VideoRadio_Init(void)
{
    nrf24_config_t config;
    uint8_t status = 0U;
    bool connected = false;

    nrf24_result_t result = FspNrf24Port_Open(NRF24_PORT_VIDEO_TX, &g_transport);
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
        g_printf("[VIDEO NRF] register-loopback mismatch\r\n");
        return NRF24_RESULT_TRANSPORT_ERROR;
    }

    Nrf24_GetDefaultConfig(&config);
    config.channel = VIDEO_RADIO_CHANNEL;
    config.payload_width = VIDEO_PACKET_SIZE;
    config.initial_role = NRF24_ROLE_TRANSMITTER;
    config.data_rate = NRF24_DATA_RATE_2MBPS;
    config.auto_ack_enabled = true;
    config.dynamic_payload_enabled = true;
    config.dynamic_ack_enabled = true;
    (void) memcpy(config.tx_address, g_video_radio_address, sizeof(g_video_radio_address));
    (void) memcpy(config.rx_pipe0_address, g_video_radio_address, sizeof(g_video_radio_address));
    return Nrf24_Initialize(&g_transport, &config, &status);
}

nrf24_result_t VideoRadio_SendFrame(video_frame_t const * p_frame)
{
    if ((NULL == p_frame) || (NULL == p_frame->p_jpeg) ||
        (0U == p_frame->jpeg_size) ||
        (VIDEO_EXPECTED_WIDTH != p_frame->source_width) ||
        (VIDEO_EXPECTED_HEIGHT != p_frame->source_height))
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }

    video_frame_t frame = *p_frame;
    uint16_t const chunk_count = VideoProtocol_ChunkCountGet(frame.jpeg_size);
    if (0U == chunk_count)
    {
        return NRF24_RESULT_INVALID_ARGUMENT;
    }
    if (0U == frame.crc32)
    {
        frame.crc32 = VideoProtocol_Crc32(frame.p_jpeg, frame.jpeg_size);
    }

    uint8_t packet[VIDEO_PACKET_SIZE];
    VideoProtocol_StartPacketBuild(&frame, packet);
    nrf24_result_t result = packet_send_ack(packet);
    if (NRF24_RESULT_SUCCESS != result)
    {
        return result;
    }

    uint32_t batch_number = 0U;
    for (uint16_t chunk = 0U; chunk < chunk_count;)
    {
        uint8_t packets[VIDEO_BATCH_SIZE][VIDEO_PACKET_SIZE];
        uint8_t const * packet_ptrs[VIDEO_BATCH_SIZE];
        uint8_t lengths[VIDEO_BATCH_SIZE];
        uint8_t count = 0U;

        while ((count < VIDEO_BATCH_SIZE) && (chunk < chunk_count))
        {
            VideoProtocol_DataPacketBuild(&frame, chunk, packets[count]);
            packet_ptrs[count] = packets[count];
            lengths[count] = VIDEO_PACKET_SIZE;
            count++;
            chunk++;
        }

        nrf24_tx_result_t tx_result;
        result = Nrf24_SendBatchNoAck(&g_transport,
                                      packet_ptrs,
                                      lengths,
                                      count,
                                      VIDEO_RADIO_TIMEOUT_MS,
                                      &tx_result);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }

        batch_number++;
        if (0U == (batch_number % VIDEO_BATCHES_BEFORE_DELAY))
        {
            /* 图传是持续负载，主动阻塞1 tick，保证Wi-Fi等低优先级线程能运行。 */
            vTaskDelay(1U);
        }
    }

    VideoProtocol_EndPacketBuild(&frame, packet);
    return packet_send_ack(packet);
}
