/* generated thread header file - do not edit */
#ifndef IPC_THREAD_H_
#define IPC_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void ipc_thread_entry(void * pvParameters);
                #else
extern void ipc_thread_entry(void *pvParameters);
#endif
#include "r_ipc.h"
FSP_HEADER
/** IPC Instance. */
extern const ipc_instance_t g_ipc1;

/** Access the IPC instance using these structures when calling API functions directly
 (::p_api is not used). */
extern ipc_instance_ctrl_t g_ipc1_ctrl;
extern const ipc_cfg_t g_ipc1_cfg;

#ifndef g_ipc1_callback
void g_ipc1_callback(ipc_callback_args_t *p_args);
#endif
/** IPC Instance. */
extern const ipc_instance_t g_ipc0;

/** Access the IPC instance using these structures when calling API functions directly
 (::p_api is not used). */
extern ipc_instance_ctrl_t g_ipc0_ctrl;
extern const ipc_cfg_t g_ipc0_cfg;

#ifndef NULL
void NULL(ipc_callback_args_t *p_args);
#endif
FSP_FOOTER
#endif /* IPC_THREAD_H_ */
