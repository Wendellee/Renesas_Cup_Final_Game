/* generated thread header file - do not edit */
#ifndef CAMERA_THREAD_H_
#define CAMERA_THREAD_H_
#include "bsp_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_data.h"
#ifdef __cplusplus
                extern "C" void camera_thread_entry(void * pvParameters);
                #else
extern void camera_thread_entry(void *pvParameters);
#endif
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
FSP_HEADER
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer1;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer1_ctrl;
extern const transfer_cfg_t g_transfer1_cfg;
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer0;

/** Access the DTC instance using these structures when calling API functions directly (::p_api is not used). */
extern dtc_instance_ctrl_t g_transfer0_ctrl;
extern const transfer_cfg_t g_transfer0_cfg;
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_i2c_master_for_peripheral;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master_for_peripheral_ctrl;
extern const i2c_master_cfg_t g_i2c_master_for_peripheral_cfg;

#ifndef g_i2c_master_for_peripheral_callback
void g_i2c_master_for_peripheral_callback(i2c_master_callback_args_t *p_args);
#endif
FSP_FOOTER
#endif /* CAMERA_THREAD_H_ */
