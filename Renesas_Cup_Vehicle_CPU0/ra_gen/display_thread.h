/* generated thread header file - do not edit */
#ifndef DISPLAY_THREAD_H_
#define DISPLAY_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void display_thread_entry(void * pvParameters);
                #else
extern void display_thread_entry(void *pvParameters);
#endif
FSP_HEADER
FSP_FOOTER
#endif /* DISPLAY_THREAD_H_ */
