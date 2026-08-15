/* generated thread header file - do not edit */
#ifndef NAVIGATION_THREAD_H_
#define NAVIGATION_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void navigation_thread_entry(void * pvParameters);
                #else
extern void navigation_thread_entry(void *pvParameters);
#endif
FSP_HEADER
FSP_FOOTER
#endif /* NAVIGATION_THREAD_H_ */
