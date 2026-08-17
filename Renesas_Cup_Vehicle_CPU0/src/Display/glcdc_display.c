/*
 * glcdc_display.c
 *
 *  Created on: 2026年8月10日
 *      Author: lingk
 */
#include "glcdc_display.h"
#include "common/common.h"
#include <string.h>

/* VIN帧与GLCDC帧缓冲区必须具有相同的行跨度和总字节数，才能整帧直接复制。 */
#if (VIN_CFG_BYTES_PER_LINE != DISPLAY_BUFFER_STRIDE_BYTES_INPUT0)
 #error "VIN and GLCDC line strides must match"
#endif

#if (VIN_BYTES_PER_FRAME != (DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0))
 #error "VIN and GLCDC frame buffer sizes must match"
#endif

/*
 *[@name] glcdc_vsync_isr
 *[@type] function
 *[@usage] 接收GLCDC行检测和Graphics 1下溢事件，并用ISR安全方式通知Display Thread
 *[@argument] p_args FSP传入的GLCDC事件和用户上下文
 *[@return] none
 */
void glcdc_vsync_isr(display_callback_args_t * p_args)
{
    BaseType_t x_higher_priority_task_woken = pdFALSE;
    BaseType_t x_result;

    if (NULL == p_args)
    {
        return;
    }

    if (DISPLAY_EVENT_GR1_UNDERFLOW == p_args->event)
    {
        return;
    }

    if (DISPLAY_EVENT_LINE_DETECTION != p_args->event)
    {
        return;
    }

    x_result = xEventGroupSetBitsFromISR(g_ai_app_event,
                                         GLCDC_VSYNC,
                                         &x_higher_priority_task_woken);

    if (pdFAIL == x_result)
    {
        return;
    }

    portYIELD_FROM_ISR(x_higher_priority_task_woken);
}

/*
 *[@name] init_display
 *[@type] function
 *[@usage] 清空初始扫描缓冲区，打开并启动GLCDC，只能在Display Thread任务上下文调用
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，失败返回对应FSP错误码
 */
fsp_err_t init_display(void)
{
    fsp_err_t err;

    memset(&fb_background[0][0], 0, sizeof(fb_background[0]));/*完全初始化俩图层*/
    memset(&fb_foreground[0][0], 0, sizeof(fb_foreground));/*l2填入透明像素*/

#if BSP_CFG_DCACHE_ENABLED

    SCB_CleanDCache_by_Addr(
        (uint32_t *) &fb_background[0][0],
        (int32_t) sizeof(fb_background[0]));

    SCB_CleanDCache_by_Addr(
        (uint32_t *) &fb_foreground[0][0],
        (int32_t) sizeof(fb_foreground));

#endif

    __DMB();

    err = R_GLCDC_Open(&g_display_ctrl, &g_display_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = R_GLCDC_Start(&g_display_ctrl);
    if (FSP_SUCCESS != err)
    {
        (void) R_GLCDC_Close(&g_display_ctrl);
        return err;
    }

    return FSP_SUCCESS;
}

/*
 *[@name] display_wait_next_vsync
 *[@type] function
 *[@usage] 清除旧同步事件并阻塞等待新的GLCDC行检测事件，只能在Display Thread中调用
 *[@argument] timeout 最大等待时间，单位为FreeRTOS Tick
 *[@return] 收到同步事件返回FSP_SUCCESS，超时返回FSP_ERR_TIMEOUT
 */
fsp_err_t display_wait_next_vsync(TickType_t timeout)
{
    EventBits_t events;

    xEventGroupClearBits(g_ai_app_event, GLCDC_VSYNC);

    events = xEventGroupWaitBits(g_ai_app_event,
                                 GLCDC_VSYNC,
                                 pdTRUE,
                                 pdFALSE,
                                 timeout);

    if (0U != (events & GLCDC_VSYNC))
    {
        return FSP_SUCCESS;
    }

    return FSP_ERR_TIMEOUT;
}
