#ifndef RADIO_APPLICATION_VIDEO_RADIO_H_
#define RADIO_APPLICATION_VIDEO_RADIO_H_

#include "Radio/driver/nrf24.h"
#include "Radio/protocol/video_protocol.h"

/** 初始化图传链路：SPI1、NRF24发射模式、频道100。 */
nrf24_result_t VideoRadio_Init(void);

/** 按手持端现有协议同步发送一帧JPEG；只能由Video TX Thread调用。 */
nrf24_result_t VideoRadio_SendFrame(video_frame_t const * p_frame);

#endif /* RADIO_APPLICATION_VIDEO_RADIO_H_ */
