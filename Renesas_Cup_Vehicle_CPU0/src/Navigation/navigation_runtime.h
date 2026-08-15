#ifndef NAVIGATION_NAVIGATION_RUNTIME_H_
#define NAVIGATION_NAVIGATION_RUNTIME_H_

#include <stdint.h>

/*
 * Encode Thread 在 Gray8 转换完成后调用。
 * 本接口只发布最新帧指针并唤醒 Navigation Thread，不复制图像、不阻塞等待。
 */
void navigation_frame_submit(const uint8_t * p_gray, uint32_t frame_sequence);

#endif /* NAVIGATION_NAVIGATION_RUNTIME_H_ */
