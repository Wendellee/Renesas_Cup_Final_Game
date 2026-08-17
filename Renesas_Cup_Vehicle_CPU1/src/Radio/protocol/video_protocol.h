#ifndef RADIO_PROTOCOL_VIDEO_PROTOCOL_H_
#define RADIO_PROTOCOL_VIDEO_PROTOCOL_H_

#include <stdint.h>

#define VIDEO_PACKET_MAGIC             (0x49U)
#define VIDEO_PACKET_VERSION           (1U)
#define VIDEO_PACKET_TYPE_START        (1U)
#define VIDEO_PACKET_TYPE_DATA         (2U)
#define VIDEO_PACKET_TYPE_END          (3U)
#define VIDEO_PACKET_SIZE              (32U)
#define VIDEO_PACKET_DATA_OFFSET       (4U)
#define VIDEO_PACKET_DATA_SIZE         (VIDEO_PACKET_SIZE - VIDEO_PACKET_DATA_OFFSET)
#define VIDEO_PACKET_CHECKSUM_INDEX    (31U)
#define VIDEO_CODEC_JPEG_RGB888        (2U)
#define VIDEO_LV_COLOR_FORMAT_RGB888   (0x0FU)

typedef struct st_video_frame
{
    uint8_t const * p_jpeg;
    uint32_t jpeg_size;
    uint32_t crc32;
    uint16_t frame_id;
    uint16_t source_width;
    uint16_t source_height;
} video_frame_t;

uint32_t VideoProtocol_Crc32(uint8_t const * p_data, uint32_t length);
uint16_t VideoProtocol_ChunkCountGet(uint32_t jpeg_size);
void VideoProtocol_StartPacketBuild(video_frame_t const * p_frame,
                                    uint8_t packet[VIDEO_PACKET_SIZE]);
void VideoProtocol_DataPacketBuild(video_frame_t const * p_frame,
                                   uint16_t chunk_index,
                                   uint8_t packet[VIDEO_PACKET_SIZE]);
void VideoProtocol_EndPacketBuild(video_frame_t const * p_frame,
                                  uint8_t packet[VIDEO_PACKET_SIZE]);

#endif /* RADIO_PROTOCOL_VIDEO_PROTOCOL_H_ */
