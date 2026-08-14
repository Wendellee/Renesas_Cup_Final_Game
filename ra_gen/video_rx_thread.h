/* generated thread header file - do not edit */
#ifndef VIDEO_RX_THREAD_H_
#define VIDEO_RX_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void video_rx_thread_entry(void * pvParameters);
                #else
extern void video_rx_thread_entry(void *pvParameters);
#endif
FSP_HEADER
FSP_FOOTER
#endif /* VIDEO_RX_THREAD_H_ */
