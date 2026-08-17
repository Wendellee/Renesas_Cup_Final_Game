#ifndef RADIO_ADAPTERS_RTOS_VIDEO_FRAME_MAILBOX_H_
#define RADIO_ADAPTERS_RTOS_VIDEO_FRAME_MAILBOX_H_

#include <stdbool.h>

#include "Radio/protocol/video_protocol.h"

/**
 * IPC Thread 在确认共享内存可读且已完成Cache维护后发布描述符。
 * 本模块不复制JPEG；发布成功后，生产者必须保持该内存不变，直到完成状态返回。
 */
bool VideoFrameMailbox_Publish(video_frame_t const * p_frame);

/** 仅供Video TX Thread获取待发送帧；获取后槽位仍保持占用。 */
bool VideoFrameMailbox_Acquire(video_frame_t * p_frame);

/** Video TX Thread发送结束后释放共享内存所有权。 */
void VideoFrameMailbox_Complete(uint16_t frame_id, bool success);

/** IPC Thread可读取最近一次完成结果，然后通知M85复用缓冲区。 */
bool VideoFrameMailbox_CompletionTake(uint16_t * p_frame_id, bool * p_success);

#endif /* RADIO_ADAPTERS_RTOS_VIDEO_FRAME_MAILBOX_H_ */
