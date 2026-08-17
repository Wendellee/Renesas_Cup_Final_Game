#include "Radio/application/video_radio.h"

#include "Radio/platform/fsp_nrf24_port.h"
#include "SEGGER_RTT/bsp_print.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define VIDEO_RADIO_CHANNEL              (100U)
#define VIDEO_RADIO_TIMEOUT_MS           (50U)
#ifndef VIDEO_AUTO_ACK_ENABLE
#define VIDEO_AUTO_ACK_ENABLE             (0U)
#endif
#ifndef VIDEO_FAST_BATCH_NO_ACK_ENABLE
#define VIDEO_FAST_BATCH_NO_ACK_ENABLE    (0U)
#endif
#define VIDEO_NO_ACK_DATA_REPEAT          (2U)
#define VIDEO_NO_ACK_BOUNDARY_REPEAT      (3U)
#define VIDEO_NO_ACK_PACING_CHUNKS        (4U)
#if VIDEO_FAST_BATCH_NO_ACK_ENABLE
#define VIDEO_BATCH_SIZE                 (3U)
#define VIDEO_BATCHES_BEFORE_DELAY        (8U)
#endif
#define VIDEO_EXPECTED_WIDTH              (200U)
#define VIDEO_EXPECTED_HEIGHT             (112U)

static uint8_t const g_video_radio_address[NRF24_ADDRESS_WIDTH_MAX] =
{
    0x56U, 0x49U, 0x44U, 0x45U, 0x4FU /* "VIDEO" */
};

static nrf24_transport_t g_transport;

static nrf24_result_t packet_send(uint8_t const packet[VIDEO_PACKET_SIZE], bool no_ack)
{
    nrf24_tx_result_t tx_result;
    return Nrf24_Send(&g_transport,
                      packet,
                      VIDEO_PACKET_SIZE,
                      no_ack,
                      VIDEO_RADIO_TIMEOUT_MS,
                      &tx_result);
}

static nrf24_result_t packet_send_repeated(uint8_t const packet[VIDEO_PACKET_SIZE],
                                           uint32_t repeat_count)
{
    for (uint32_t repeat = 0U; repeat < repeat_count; repeat++)
    {
        nrf24_result_t const result = packet_send(packet, true);
        if (NRF24_RESULT_SUCCESS != result)
        {
            return result;
        }
    }
    return NRF24_RESULT_SUCCESS;
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
    config.auto_ack_enabled = (0U != VIDEO_AUTO_ACK_ENABLE);
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
#if VIDEO_AUTO_ACK_ENABLE
    nrf24_result_t result = packet_send(packet, false);
#else
    nrf24_result_t result = packet_send_repeated(packet, VIDEO_NO_ACK_BOUNDARY_REPEAT);
#endif
    if (NRF24_RESULT_SUCCESS != result)
    {
        g_printf("[VIDEO NRF] frame=%u START failed=%u\r\n",
                 (uint32_t) frame.frame_id,
                 (uint32_t) result);
        return result;
    }

#if VIDEO_FAST_BATCH_NO_ACK_ENABLE
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
#elif VIDEO_AUTO_ACK_ENABLE
    /* A complete JPEG is useful only when every fragment arrives.  Hardware
     * ACK/retry therefore provides receiver back-pressure and retransmits an
     * occasional lost DATA packet.  The former three-packet NO_ACK burst is
     * retained above for later throughput experiments. */
    for (uint16_t chunk = 0U; chunk < chunk_count; chunk++)
    {
        VideoProtocol_DataPacketBuild(&frame, chunk, packet);
        result = packet_send(packet, false);
        if (NRF24_RESULT_SUCCESS != result)
        {
            g_printf("[VIDEO NRF] frame=%u chunk=%u/%u failed=%u\r\n",
                     (uint32_t) frame.frame_id,
                     (uint32_t) chunk,
                     (uint32_t) chunk_count,
                     (uint32_t) result);
            return result;
        }
    }
#else
    /* ACK return path is not required in the production video direction.
     * Send every fragment twice before advancing, so the receiver can accept
     * the duplicate when the first copy was lost and ignore it otherwise. */
    for (uint16_t chunk = 0U; chunk < chunk_count; chunk++)
    {
        VideoProtocol_DataPacketBuild(&frame, chunk, packet);
        result = packet_send_repeated(packet, VIDEO_NO_ACK_DATA_REPEAT);
        if (NRF24_RESULT_SUCCESS != result)
        {
            g_printf("[VIDEO NRF] frame=%u chunk=%u/%u noack failed=%u\r\n",
                     (uint32_t) frame.frame_id,
                     (uint32_t) chunk,
                     (uint32_t) chunk_count,
                     (uint32_t) result);
            return result;
        }
        if (0U == (((uint32_t) chunk + 1U) % VIDEO_NO_ACK_PACING_CHUNKS))
        {
            vTaskDelay(1U);
        }
    }
#endif

    VideoProtocol_EndPacketBuild(&frame, packet);
#if VIDEO_AUTO_ACK_ENABLE
    return packet_send(packet, false);
#else
    return packet_send_repeated(packet, VIDEO_NO_ACK_BOUNDARY_REPEAT);
#endif
}
