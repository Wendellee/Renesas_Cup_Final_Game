/* generated thread header file - do not edit */
#ifndef WIFI_UPLOAD_THREAD_H_
#define WIFI_UPLOAD_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void wifi_upload_thread_entry(void * pvParameters);
                #else
extern void wifi_upload_thread_entry(void *pvParameters);
#endif
#include "r_sci_b_uart.h"
#include "r_uart_api.h"
FSP_HEADER
/** UART on SCI Instance. */
extern const uart_instance_t g_uart0;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_b_uart_instance_ctrl_t g_uart0_ctrl;
extern const uart_cfg_t g_uart0_cfg;
extern const sci_b_uart_extended_cfg_t g_uart0_cfg_extend;

#ifndef UART0_CallBack
void UART0_CallBack(uart_callback_args_t *p_args);
#endif
FSP_FOOTER
#endif /* WIFI_UPLOAD_THREAD_H_ */
