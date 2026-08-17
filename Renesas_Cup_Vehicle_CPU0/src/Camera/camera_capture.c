/*
 * camera_capture.c
 *
 *  Created on: 2026年8月11日
 *      Author: lingk
 */

#include "hal_data.h"
#include "Camera/camera_sensor.h"
#include "Camera/camera_capture.h"

/*
 *[@name] gp_camera_completed_frame
 *[@type] static volatile global variable
 *[@usage] 保存VIN最近一次完成写入的帧缓冲区地址，由VIN回调写入，由任务上下文读取
 */
static uint8_t * volatile gp_camera_completed_frame;

/*
 *[@name] g_camera_frame_sequence
 *[@type] static volatile global variable
 *[@usage] 记录VIN完成帧累计序号，只能由vin_callback递增
 */
static volatile uint32_t g_camera_frame_sequence;

/*
 *[@name] camera_capture_open
 *[@type] function
 *[@usage] 清空采集状态、维护VIN缓冲区Cache并打开VIN和MIPI-CSI，只能在任务上下文调用
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，失败返回对应FSP错误码
 */
fsp_err_t camera_capture_open(void)
{
    fsp_err_t err;

    gp_camera_completed_frame = NULL;
    g_camera_frame_sequence = 0U;

#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanInvalidateDCache_by_Addr(
        (uint32_t *) vin_image_buffer_1,
        (int32_t) VIN_BYTES_PER_FRAME);

    SCB_CleanInvalidateDCache_by_Addr(
        (uint32_t *) vin_image_buffer_2,
        (int32_t) VIN_BYTES_PER_FRAME);

    SCB_CleanInvalidateDCache_by_Addr(
        (uint32_t *) vin_image_buffer_3,
        (int32_t) VIN_BYTES_PER_FRAME);
#endif

    __DMB();

    err = R_VIN_Open(&g_vin_ctrl, &g_vin_cfg);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    return FSP_SUCCESS;
}

/*
 *[@name] camera_capture_start
 *[@type] function
 *[@usage] 先启动VIN接收，再启动OV5640连续图像输出，只能在Camera Thread任务上下文调用
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，失败返回对应FSP错误码并释放已打开的VIN资源
 */
fsp_err_t camera_capture_start(void)
{
    fsp_err_t err;

    err = R_VIN_CaptureStart(&g_vin_ctrl, NULL);
    if(FSP_SUCCESS != err)
    {
        (void) R_VIN_Close(&g_vin_ctrl);
        return err;
    }

    err = camera_stream_on();
    if(FSP_SUCCESS != err)
    {
        (void) camera_stream_off();
        vTaskDelay(pdMS_TO_TICKS(40U));
        (void) R_VIN_Close(&g_vin_ctrl);
        return err;
    }

    return FSP_SUCCESS;
}

/*
 *[@name] camera_capture_stop
 *[@type] function
 *[@usage] 停止OV5640输出，等待链路停止后关闭VIN和MIPI-CSI，不可在中断中调用
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，失败返回对应FSP错误码
 */
fsp_err_t camera_capture_stop(void)
{
    fsp_err_t err;

    err = camera_stream_off();
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(40U));

    err = R_VIN_Close(&g_vin_ctrl);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    return FSP_SUCCESS;
}

/*
 *[@name] camera_completed_frame_get
 *[@type] function
 *[@usage] 在任务上下文原子读取最近完成帧地址及对应帧序号，不改变采集状态
 *[@argument] p_sequence 用于接收完成帧序号的非空指针
 *[@return] 返回最近完成帧缓冲区地址，无有效帧或参数无效时返回NULL
 */
uint8_t * camera_completed_frame_get(uint32_t * p_sequence)
{
    uint8_t * p_completed_frame;

    if(NULL == p_sequence)
    {
        return NULL;
    }

    taskENTER_CRITICAL();
    p_completed_frame = gp_camera_completed_frame;
    *p_sequence = g_camera_frame_sequence;
    taskEXIT_CRITICAL();

    return p_completed_frame;
}

/*
 *[@name] vin_callback
 *[@type] callback function
 *[@usage] 在VIN中断上下文发布完成帧地址、帧序号和错误事件，不执行图像复制和日志输出
 *[@argument] p_args FSP提供的VIN事件、状态和完成缓冲区信息
 *[@return] none
 */
void vin_callback(capture_callback_args_t * p_args)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    vin_interrupt_status_t interrupt_status;

    if(NULL == p_args)
    {
        return;
    }

    interrupt_status = (vin_interrupt_status_t) p_args->interrupt_status;

    switch(p_args->event)
    {
        case VIN_EVENT_NOTIFY:
        {
            if(interrupt_status.bits.frame_complete)
            {
                if(NULL != p_args->p_buffer)
                {
                    gp_camera_completed_frame = p_args->p_buffer;
                    g_camera_frame_sequence++;
                    __DMB();
                    (void) xEventGroupSetBitsFromISR(g_ai_app_event,
                                                     CAMERA_FRAME_READY |
                                                     AI_INFERENCE_INPUT_IMAGE_READY,
                                                     &higher_priority_task_woken);
                }
                else
                {
                    __DMB();
                    (void) xEventGroupSetBitsFromISR(g_ai_app_event,
                                                     CAMERA_CAPTURE_ERROR,
                                                     &higher_priority_task_woken);
                }
            }

            break;
        }

        case VIN_EVENT_ERROR:
        {
            __DMB();
            (void) xEventGroupSetBitsFromISR(g_ai_app_event,
                                             CAMERA_CAPTURE_ERROR,
                                             &higher_priority_task_woken);
            break;
        }

        default:
        {
            break;
        }
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/*
 *[@name] mipi_csi0_callback
 *[@type] callback function
 *[@usage] 接收MIPI-CSI中断事件，当前不向任务发布普通接收活动事件
 *[@argument] p_args FSP提供的MIPI-CSI事件信息
 *[@return] none
 */
void mipi_csi0_callback(mipi_csi_callback_args_t * p_args)
{
    if(NULL == p_args)
    {
        return;
    }

    switch(p_args->event)
    {
        case MIPI_CSI_EVENT_DATA_LANE:
        case MIPI_CSI_EVENT_FRAME_DATA:
        case MIPI_CSI_EVENT_POWER:
        case MIPI_CSI_EVENT_SHORT_PACKET_FIFO:
        case MIPI_CSI_EVENT_VIRTUAL_CHANNEL:
        {
            break;
        }

        default:
        {
            break;
        }
    }
}
