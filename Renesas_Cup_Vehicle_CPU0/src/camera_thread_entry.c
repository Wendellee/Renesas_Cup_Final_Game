#include "camera_thread.h"
#include "Camera/i2c_control.h"
#include "Camera/camera_sensor.h"
#include "Camera/camera_capture.h"
#include "common/common.h"
#include "SEGGER_RTT/bsp_print.h"

/*
 *[@name] camera_thread_entry
 *[@type] thread entry function
 *[@usage] 初始化IIC与OV5640，等待显示初始化后启动VIN连续采集，并阻塞等待采集错误事件
 *[@argument] pvParameters FSP传入的线程参数，当前未使用
 *[@return] none
 */
void camera_thread_entry(void * pvParameters)
{
    EventBits_t events;
    fsp_err_t err;

    FSP_PARAMETER_NOT_USED(pvParameters);

    err = i2c_control_init();
    if(FSP_SUCCESS != err)
    {
        g_printf("[CAM][ERR] IIC initialization failed: %u\r\n",
                 (unsigned int) err);
        APP_ERROR_TRAP(err);

        for(;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000U));
        }
    }

    vTaskDelay(pdMS_TO_TICKS(10U));

    err = camera_open();
    if(FSP_SUCCESS != err)
    {
        g_printf("[CAM][ERR] OV5640 initialization failed: %u\r\n",
                 (unsigned int) err);
        APP_ERROR_TRAP(err);

        for(;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000U));
        }
    }

    (void) xEventGroupSetBits(g_ai_app_event, HARDWARE_CAMERA_INIT_DONE);

    events = xEventGroupWaitBits(g_ai_app_event,
                                 HARDWARE_DISPLAY_INIT_DONE,
                                 pdFALSE,
                                 pdTRUE,
                                 portMAX_DELAY);
    if(0U == (events & HARDWARE_DISPLAY_INIT_DONE))
    {
        APP_ERROR_TRAP(FSP_ERR_INTERNAL);
    }

    err = camera_capture_open();
    if(FSP_SUCCESS != err)
    {
        g_printf("[CAM][ERR] VIN open failed: %u\r\n",
                 (unsigned int) err);
        APP_ERROR_TRAP(err);

        for(;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000U));
        }
    }

    err = camera_capture_start();
    if(FSP_SUCCESS != err)
    {
        g_printf("[CAM][ERR] Camera capture start failed: %u\r\n",
                 (unsigned int) err);
        APP_ERROR_TRAP(err);

        for(;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000U));
        }
    }

    g_printf("[CAM] Camera capture ready.\r\n");

    for(;;)
    {
        events = xEventGroupWaitBits(g_ai_app_event,
                                     CAMERA_CAPTURE_ERROR,
                                     pdTRUE,
                                     pdFALSE,
                                     portMAX_DELAY);

        if(0U != (events & CAMERA_CAPTURE_ERROR))
        {
            g_printf("[CAM][ERR] VIN capture error event received.\r\n");
            (void) camera_capture_stop();
            APP_ERROR_TRAP(FSP_ERR_INTERNAL);

            for(;;)
            {
                vTaskDelay(pdMS_TO_TICKS(1000U));
            }
        }
    }
}
