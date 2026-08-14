#ifndef IPC_SHARED_JPEG_PROTOCOL_H_
#define IPC_SHARED_JPEG_PROTOCOL_H_

#include <stddef.h>
#include <stdint.h>

#define SHARED_JPEG_BASE_ADDRESS       (0x6FFE0000UL)
#define SHARED_JPEG_TOTAL_SIZE         (0x00011000UL)
#define SHARED_JPEG_HEADER_SIZE        (64UL)
#define SHARED_JPEG_PAYLOAD_OFFSET     (SHARED_JPEG_HEADER_SIZE)
#define SHARED_JPEG_PAYLOAD_CAPACITY   (SHARED_JPEG_TOTAL_SIZE - SHARED_JPEG_PAYLOAD_OFFSET)

#define SHARED_JPEG_MAGIC              (0x4A504547UL)
#define SHARED_JPEG_PROTOCOL_VERSION   (1UL)
#define SHARED_JPEG_CONFIDENCE_INDEX   (0U)

#define SHARED_JPEG_IPC_DATA_READY     (0x4A500002UL)
#define SHARED_JPEG_IPC_DATA_DONE      (0x4A500003UL)
#define SHARED_JPEG_IPC_DATA_ERROR     (0x4A500004UL)

typedef enum e_shared_jpeg_state
{
    SHARED_JPEG_STATE_FREE = 0,
    SHARED_JPEG_STATE_M85_FILLING,
    SHARED_JPEG_STATE_READY_FOR_M33,
    SHARED_JPEG_STATE_M33_PROCESSING,
    SHARED_JPEG_STATE_DONE,
    SHARED_JPEG_STATE_ERROR
} shared_jpeg_state_t;

typedef enum e_shared_jpeg_error
{
    SHARED_JPEG_ERROR_NONE = 0,
    SHARED_JPEG_ERROR_BAD_MAGIC,
    SHARED_JPEG_ERROR_BAD_VERSION,
    SHARED_JPEG_ERROR_BAD_HEADER,
    SHARED_JPEG_ERROR_BAD_STATE,
    SHARED_JPEG_ERROR_BAD_LENGTH,
    SHARED_JPEG_ERROR_BAD_SOI,
    SHARED_JPEG_ERROR_BAD_EOI,
    SHARED_JPEG_ERROR_BAD_CRC,
    SHARED_JPEG_ERROR_IPC_SEND,
    SHARED_JPEG_ERROR_TIMEOUT,
    SHARED_JPEG_ERROR_UPLOAD_QUEUE,
    SHARED_JPEG_ERROR_WIFI_CONNECT,
    SHARED_JPEG_ERROR_TCP_CONNECT,
    SHARED_JPEG_ERROR_TCP_SEND
} shared_jpeg_error_t;

typedef struct st_shared_jpeg_control
{
    uint32_t magic;
    uint32_t protocol_version;
    uint32_t header_size;
    uint32_t state;

    uint32_t message_type;
    uint32_t frame_sequence;
    uint32_t payload_offset;
    uint32_t payload_length;

    uint32_t payload_crc32;
    uint32_t producer_error;
    uint32_t consumer_error;

    uint32_t reserved[5];
} shared_jpeg_control_t;

_Static_assert(sizeof(shared_jpeg_control_t) == SHARED_JPEG_HEADER_SIZE,
               "shared_jpeg_control_t must be 64 bytes");

/* 实时图传独占 SHAREMEM 的后 60 KiB，使用双槽 latest-frame 语义。
 * CPU0 可覆盖尚未被 CPU1 取走的 READY 槽，但绝不覆盖 IN_USE 槽。 */
#define SHARED_VIDEO_BASE_ADDRESS      (0x6FFF1000UL)
#define SHARED_VIDEO_TOTAL_SIZE        (0x0000F000UL)
#define SHARED_VIDEO_HEADER_SIZE       (64UL)
#define SHARED_VIDEO_SLOT_COUNT        (2UL)
#define SHARED_VIDEO_SLOT_CAPACITY     \
    ((SHARED_VIDEO_TOTAL_SIZE - SHARED_VIDEO_HEADER_SIZE) / SHARED_VIDEO_SLOT_COUNT)
#define SHARED_VIDEO_MAGIC             (0x56494430UL)
#define SHARED_VIDEO_PROTOCOL_VERSION  (1UL)
#define SHARED_VIDEO_IPC_FRAME_READY   (0x56440001UL)

typedef enum e_shared_video_slot_state
{
    SHARED_VIDEO_SLOT_FREE = 0,
    SHARED_VIDEO_SLOT_WRITING,
    SHARED_VIDEO_SLOT_READY,
    SHARED_VIDEO_SLOT_IN_USE
} shared_video_slot_state_t;

typedef struct st_shared_video_slot
{
    uint32_t state;
    uint32_t frame_sequence;
    uint32_t payload_length;
    uint32_t payload_crc32;
    uint32_t dimensions; /* width 位于低16位，height 位于高16位 */
    uint32_t reserved;
} shared_video_slot_t;

typedef struct st_shared_video_control
{
    uint32_t magic;
    uint32_t protocol_version;
    uint32_t header_size;
    uint32_t slot_capacity;
    shared_video_slot_t slots[SHARED_VIDEO_SLOT_COUNT];
} shared_video_control_t;

_Static_assert(sizeof(shared_video_control_t) == SHARED_VIDEO_HEADER_SIZE,
               "shared_video_control_t must be 64 bytes");
_Static_assert((SHARED_JPEG_BASE_ADDRESS + SHARED_JPEG_TOTAL_SIZE) == SHARED_VIDEO_BASE_ADDRESS,
               "fault JPEG and realtime video windows must be contiguous");
_Static_assert((SHARED_VIDEO_BASE_ADDRESS + SHARED_VIDEO_TOTAL_SIZE) == 0x70000000UL,
               "shared windows must match the current SHAREMEM region");

/*
 *[@name] shared_jpeg_crc32
 *[@type] function
 *[@usage] 使用CRC-32/ISO-HDLC算法计算共享JPEG载荷校验值
 *[@argument] p_data 待校验数据的只读首地址
 *[@argument] length 待校验数据长度，单位为字节
 *[@return] 返回32位CRC；参数无效时返回0
 */
uint32_t shared_jpeg_crc32(const uint8_t * p_data, size_t length);

#endif /* IPC_SHARED_JPEG_PROTOCOL_H_ */
