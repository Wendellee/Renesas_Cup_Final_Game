#include "Radio/protocol/video_protocol.h"

#include <string.h>

static void write_u16(uint8_t * p_data, uint16_t value)
{
    p_data[0] = (uint8_t) value;
    p_data[1] = (uint8_t) (value >> 8U);
}

static void write_u32(uint8_t * p_data, uint32_t value)
{
    p_data[0] = (uint8_t) value;
    p_data[1] = (uint8_t) (value >> 8U);
    p_data[2] = (uint8_t) (value >> 16U);
    p_data[3] = (uint8_t) (value >> 24U);
}

static uint8_t checksum_calculate(uint8_t const * p_packet)
{
    uint8_t checksum = 0U;
    for (uint32_t i = 0U; i < VIDEO_PACKET_CHECKSUM_INDEX; i++)
    {
        checksum ^= p_packet[i];
    }
    return checksum;
}

static void packet_init(uint8_t type,
                        uint16_t frame_id,
                        uint8_t packet[VIDEO_PACKET_SIZE])
{
    (void) memset(packet, 0, VIDEO_PACKET_SIZE);
    packet[0] = VIDEO_PACKET_MAGIC;
    packet[1] = type;
    write_u16(&packet[2], frame_id);
}

uint32_t VideoProtocol_Crc32(uint8_t const * p_data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0U; i < length; i++)
    {
        crc ^= p_data[i];
        for (uint32_t bit = 0U; bit < 8U; bit++)
        {
            uint32_t const mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

uint16_t VideoProtocol_ChunkCountGet(uint32_t jpeg_size)
{
    uint32_t const count = (jpeg_size + VIDEO_PACKET_DATA_SIZE - 1U) /
                           VIDEO_PACKET_DATA_SIZE;
    return (count <= UINT16_MAX) ? (uint16_t) count : 0U;
}

void VideoProtocol_StartPacketBuild(video_frame_t const * p_frame,
                                    uint8_t packet[VIDEO_PACKET_SIZE])
{
    packet_init(VIDEO_PACKET_TYPE_START, p_frame->frame_id, packet);
    packet[4] = VIDEO_PACKET_VERSION;
    packet[5] = VIDEO_LV_COLOR_FORMAT_RGB888;
    write_u16(&packet[6], p_frame->source_width);
    write_u16(&packet[8], p_frame->source_height);
    write_u32(&packet[10], p_frame->jpeg_size);
    write_u32(&packet[14], p_frame->crc32);
    write_u16(&packet[18], VideoProtocol_ChunkCountGet(p_frame->jpeg_size));
    packet[20] = VIDEO_CODEC_JPEG_RGB888;
    write_u16(&packet[21], p_frame->source_width);
    write_u16(&packet[23], p_frame->source_height);
    packet[25] = 0U;
    packet[VIDEO_PACKET_CHECKSUM_INDEX] = checksum_calculate(packet);
}

void VideoProtocol_DataPacketBuild(video_frame_t const * p_frame,
                                   uint16_t chunk_index,
                                   uint8_t packet[VIDEO_PACKET_SIZE])
{
    uint32_t const offset = (uint32_t) chunk_index * VIDEO_PACKET_DATA_SIZE;
    uint32_t const remaining = p_frame->jpeg_size - offset;
    uint8_t const data_length = (uint8_t) ((remaining < VIDEO_PACKET_DATA_SIZE) ?
                                          remaining : VIDEO_PACKET_DATA_SIZE);

    /* DATA 包把 byte2..3 复用为分片号，byte4..31 全部装 JPEG。
     * DATA 的完整性由 nRF24 两字节硬件 CRC 与整帧 CRC32 保证。 */
    packet_init(VIDEO_PACKET_TYPE_DATA, chunk_index, packet);
    (void) memcpy(&packet[VIDEO_PACKET_DATA_OFFSET],
                  &p_frame->p_jpeg[offset],
                  data_length);
}

void VideoProtocol_EndPacketBuild(video_frame_t const * p_frame,
                                  uint8_t packet[VIDEO_PACKET_SIZE])
{
    packet_init(VIDEO_PACKET_TYPE_END, p_frame->frame_id, packet);
    write_u16(&packet[4], VideoProtocol_ChunkCountGet(p_frame->jpeg_size));
    write_u32(&packet[6], p_frame->jpeg_size);
    write_u32(&packet[10], p_frame->crc32);
    packet[VIDEO_PACKET_CHECKSUM_INDEX] = checksum_calculate(packet);
}
