#include "display_thread.h"
#include "Display/glcdc_display.h"
#include "common/common.h"
#include "Camera/camera_capture.h"
#include "Helium/helium_rgb565_resize.h"
#include "AI/ai_inference_result.h"
#include "AI/ai_preprocess.h"
#include "Display/dave2D_overlay.h"
#include "SEGGER_RTT/bsp_print.h"

#include <string.h>
#include <stdio.h>

#define DISPLAY_SCALE_SOURCE_WIDTH          (1024U)         /*显示缩放源宽度*/
#define DISPLAY_SCALE_SOURCE_HEIGHT         (600U)          /*显示缩放源高度*/
#define DISPLAY_SCALE_CROP_X                (12U)           /*显示缩放裁剪X轴参数*/
#define DISPLAY_SCALE_CROP_Y                (0U)            /*显示缩放裁剪Y轴参数*/
#define DISPLAY_SCALE_CROP_WIDTH            (1000U)         /*显示缩放裁剪宽度*/
#define DISPLAY_SCALE_CROP_HEIGHT           (600U)          /*显示缩放裁剪高度*/
#define DISPLAY_SCALE_DESTINATION_WIDTH     (800U)          /*显示缩放目标宽度*/
#define DISPLAY_SCALE_DESTINATION_HEIGHT    (480U)          /*显示缩放目标高度*/
#define OVERLAY_WIDTH                       (800)           /*图层二宽度*/
#define OVERLAY_HEIGHT                      (480)           /*图层二高度*/
#define OVERLAY_AI_REGION_X                 (160.0f)        /*AI图层二X轴区域*/
#define OVERLAY_AI_REGION_Y                 (0.0f)          /*AI图层二Y轴区域*/
#define OVERLAY_MODEL_TO_SCREEN_X           (3.75f)         /*图层二模型到屏幕X轴*/
#define OVERLAY_MODEL_TO_SCREEN_Y           (3.75f)         /*图层二模型到屏幕Y轴*/
#define OVERLAY_BOX_COLOR                   (0xF800U)       /*画框颜色*/
#define OVERLAY_BOX_LINE_WIDTH              (4)             /*画框线条粗细*/
#define OVERLAY_TEXT_SCALE                  (3)
#define OVERLAY_TEXT_COLOR                  (OVERLAY_BOX_COLOR) /*文字颜色与检测框保持一致*/
#define OVERLAY_TEXT_MARGIN                 (2)
#define OVERLAY_GLYPH_ADVANCE               (6)
#define OVERLAY_GLYPH_HEIGHT                (7)
#define OVERLAY_LABEL_BUFFER_SIZE           (32U)

/*
 *[@type] static variable
 *[@usage] Helium横向源像素索引工作区，只允许Display Thread访问
 */
static uint16_t
    g_display_resize_horizontal_map[DISPLAY_SCALE_DESTINATION_WIDTH]
    BSP_ALIGN_VARIABLE(16);

/*
 *[@name] display_coordinate_clamp
 *[@type] static function
 *[@usage] 将一个整数坐标限制在闭区间minimum到maximum内。
 *[@argument] value 待限制的坐标值
 *[@argument] minimum 允许的最小值
 *[@argument] maximum 允许的最大值
 *[@return] 返回限制后的坐标值
 */
static int display_coordinate_clamp(int value,
                                    int minimum,
                                    int maximum)
{
    if(value < minimum)
    {
        return value = minimum;
    }

    if(value > maximum)
    {
        return value = maximum;
    }

    return value;
}

/*
 *[@name] display_detection_to_rectangle
 *[@type] static function
 *[@usage] 将模型128x128坐标系中的一个YOLO检测框映射并裁剪到800x480叠加图层坐标系。
 *[@argument] p_detection 待转换的YOLO检测结果
 *[@argument] p_x0 用于接收矩形左边界坐标的非空指针
 *[@argument] p_y0 用于接收矩形上边界坐标的非空指针
 *[@argument] p_x1 用于接收矩形右边界坐标的非空指针
 *[@argument] p_y1 用于接收矩形下边界坐标的非空指针
 *[@return] 检测结果及转换后矩形有效时返回true，否则返回false
 */
static bool display_detection_to_rectangle(yolo_detection_t const * p_detection,
                                           int * p_x0,
                                           int * p_y0,
                                           int * p_x1,
                                           int * p_y1)
{
    int x0, y0, x1, y1;

    if ((NULL == p_detection) || (NULL == p_x0) || (NULL == p_y0) || (NULL == p_x1) || (NULL == p_y1))
    {
        return false;
    }

    if (p_detection->class_id >= YOLO_CLASS_COUNT)
    {
        return false;
    }

    x0 = (int) (OVERLAY_AI_REGION_X + p_detection->x1 * OVERLAY_MODEL_TO_SCREEN_X + 0.5f);

    y0 = (int) (OVERLAY_AI_REGION_Y + p_detection->y1 * OVERLAY_MODEL_TO_SCREEN_Y + 0.5f);

    x1 = (int) (OVERLAY_AI_REGION_X + p_detection->x2 * OVERLAY_MODEL_TO_SCREEN_X + 0.5f);

    y1 = (int) (OVERLAY_AI_REGION_Y + p_detection->y2 * OVERLAY_MODEL_TO_SCREEN_Y + 0.5f);

    x0 = display_coordinate_clamp(x0, 0, OVERLAY_WIDTH - 1);
    y0 = display_coordinate_clamp(y0, 0, OVERLAY_HEIGHT - 1);
    x1 = display_coordinate_clamp(x1, 0, OVERLAY_WIDTH - 1);
    y1 = display_coordinate_clamp(y1, 0, OVERLAY_HEIGHT - 1);

    if ((x1 <= x0) || (y1 <= y0))
    {
        return false;
    }

    *p_x0 = x0;
    *p_y0 = y0;
    *p_x1 = x1;
    *p_y1 = y1;

    return true;
}
/*
 *[@name] display_thread_entry
 *[@type] thread entry function
 *[@usage] 初始化GLCDC，接收Camera完成帧，使用Helium缩放到后台缓冲区并安全换帧
 *[@argument] pvParameters FSP传入的线程参数，当前未使用
 *[@return] none
 */
void display_thread_entry(void * pvParameters)
{
    fsp_err_t err;
    helium_rgb565_resize_status_t resize_status;

    uint8_t draw_buffer_index = 1U;
    uint32_t last_displayed_sequence = 0U;
    uint32_t completed_sequence = 0U;

    ai_inference_result_snapshot_t ai_snapshot = {0};
    uint8_t foreground_draw_buffer_index = 1U;
    bool foreground_swap_pending = false;
    bool vsync_timeout_reported = false;
    bool dave_error_reported = false;

    FSP_PARAMETER_NOT_USED(pvParameters);

    /*初始化并启动GLCDC*/
    err = init_display();
    if (FSP_SUCCESS != err)
    {
        APP_ERROR_TRAP(err);
    }

    /*初始化并启动D/AVE2D*/
    if (!dave2d_overlay_init())
    {
        int32_t d2_error = dave2d_overlay_get_last_error();
        g_printf("[DISPLAY][ERR] D/AVE 2D initialization failed: %d.\r\n",
                 (int) d2_error);
        APP_ERROR_TRAP(FSP_ERR_INTERNAL);
    }

    /*通知Camera Thread显示硬件已经初始化完成*/
    xEventGroupSetBits(g_ai_app_event, HARDWARE_DISPLAY_INIT_DONE);

    while (true)
    {
        EventBits_t events;

        /*
         * 等待Camera ISR发布新的完成帧。
         * pdTRUE:返回前清除CAMERA_FRAME_READY事件位。
         */
        events = xEventGroupWaitBits(g_ai_app_event,
                                     CAMERA_FRAME_READY,
                                     pdTRUE,
                                     pdFALSE,
                                     portMAX_DELAY);

        if (0U == (events & CAMERA_FRAME_READY))
        {
            continue;
        }

        /*在临界区保护下取得帧地址和对应序号的一致快照*/
        uint8_t * p_completed_frame =
            camera_completed_frame_get(&completed_sequence);

        if ((NULL == p_completed_frame) ||
            (completed_sequence == last_displayed_sequence))
        {
            continue;
        }

        helium_rgb565_resize_cfg_t const resize_cfg =
        {
            .p_source =
                (uint16_t const *) p_completed_frame,

            .source_width =
                DISPLAY_SCALE_SOURCE_WIDTH,

            .source_height =
                DISPLAY_SCALE_SOURCE_HEIGHT,

            .source_stride_pixels =
                VIN_CFG_IMAGE_STRIDE,

            .crop_x =
                DISPLAY_SCALE_CROP_X,

            .crop_y =
                DISPLAY_SCALE_CROP_Y,

            .crop_width =
                DISPLAY_SCALE_CROP_WIDTH,

            .crop_height =
                DISPLAY_SCALE_CROP_HEIGHT,

            .p_destination =
                (uint16_t *)
                &fb_background[draw_buffer_index][0],

            .destination_width =
                DISPLAY_SCALE_DESTINATION_WIDTH,

            .destination_height =
                DISPLAY_SCALE_DESTINATION_HEIGHT,

            .destination_stride_pixels =
                DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0
        };

#if BSP_CFG_DCACHE_ENABLED

        SCB_InvalidateDCache_by_Addr(
            (uint32_t *) p_completed_frame,
            (int32_t) VIN_BYTES_PER_FRAME);

#endif

        /*在任务上下文中执行Helium最近邻缩放*/
        resize_status = helium_rgb565_resize_nearest(
            &resize_cfg,
            g_display_resize_horizontal_map,
            sizeof(g_display_resize_horizontal_map) /
            sizeof(g_display_resize_horizontal_map[0]));

        if (HELIUM_RGB565_RESIZE_SUCCESS != resize_status)
        {
            g_printf("[DISPLAY][ERR] Helium resize failed: %u.\r\n",
                     (unsigned int) resize_status);
            APP_ERROR_TRAP(FSP_ERR_INTERNAL);
        }

#if BSP_CFG_DCACHE_ENABLED

        SCB_CleanDCache_by_Addr(
            (uint32_t *)
            &fb_background[draw_buffer_index][0],
            (int32_t)
            sizeof(fb_background[draw_buffer_index]));

#endif
        __DMB();

        EventBits_t result_events =
            xEventGroupWaitBits(g_ai_app_event,
                                AI_INFERENCE_RESULT_UPDATED,
                                pdTRUE,
                                pdFALSE,
                                0U);

        if (0U != (result_events & AI_INFERENCE_RESULT_UPDATED))
        {
            if (ai_inference_result_get_latest(&ai_snapshot))
            {
                bool foreground_buffer_ready = false;
                uint8_t * p_foreground_draw_buffer = &fb_foreground[foreground_draw_buffer_index][0];
                memset(p_foreground_draw_buffer, 0, sizeof(fb_foreground[foreground_draw_buffer_index]));

#if BSP_CFG_DCACHE_ENABLED

                SCB_CleanDCache_by_Addr(
                    (uint32_t *) p_foreground_draw_buffer,
                    (int32_t)
                    sizeof(fb_foreground[foreground_draw_buffer_index]));

#endif
                __DMB();

                if (0U == ai_snapshot.detection_count)
                {
                    foreground_buffer_ready = true;
                }
                else
                {
                    bool d2_frame_started = false;
                    int32_t d2_error = 0;
                    bool d2_ok = dave2d_overlay_begin(p_foreground_draw_buffer,
                                                      DISPLAY_HSIZE_INPUT1,
                                                      DISPLAY_VSIZE_INPUT1,
                                                      DISPLAY_BUFFER_STRIDE_PIXELS_INPUT1);
                    if (d2_ok)
                    {
                        d2_frame_started = true;
                    }
                    else
                    {
                        d2_error = dave2d_overlay_get_last_error();
                    }
                    for (uint32_t index = 0U; d2_ok && (index < ai_snapshot.detection_count); index++)
                    {
                        int x0;
                        int y0;
                        int x1;
                        int y1;

                        if (!display_detection_to_rectangle(
                                &ai_snapshot.detections[index],
                                &x0,
                                &y0,
                                &x1,
                                &y1))
                        {
                            continue;
                        }

                        d2_ok = dave2d_overlay_draw_rect(
                                    x0,
                                    y0,
                                    x1,
                                    y1,
                                    OVERLAY_BOX_COLOR,
                                    OVERLAY_BOX_LINE_WIDTH);

                        if (!d2_ok)
                        {
                            d2_error = dave2d_overlay_get_last_error();
                        }

                        if (d2_ok)
                        {
                            char         label[OVERLAY_LABEL_BUFFER_SIZE];
                            unsigned int confidence_percent;
                            size_t       label_length;
                            int          text_width;
                            int          text_height;
                            int          text_x;
                            int          text_y;
                            int          max_text_x;

                            float score = ai_snapshot.detections[index].score;

                            if (score <= 0.0f)
                            {
                                confidence_percent = 0U;
                            }
                            else if (score >= 1.0f)
                            {
                                confidence_percent = 100U;
                            }
                            else
                            {
                                confidence_percent = (unsigned int) ((score * 100.0f) + 0.5f);
                            }

                            (void) snprintf(label,
                                            sizeof(label),
                                            "%s:%u",
                                            g_yolo_class_names[ai_snapshot.detections[index].class_id],
                                            confidence_percent);

                            label[sizeof(label) - 1U] = '\0';

                            label_length = strlen(label);
                            text_width    = (int) label_length *
                                            OVERLAY_GLYPH_ADVANCE *
                                            OVERLAY_TEXT_SCALE;
                            text_height   = OVERLAY_GLYPH_HEIGHT *
                                            OVERLAY_TEXT_SCALE;

                            text_x = x0;

                            if (y0 >= (text_height + OVERLAY_TEXT_MARGIN))
                            {
                                text_y = y0 - text_height - OVERLAY_TEXT_MARGIN;
                            }
                            else
                            {
                                text_y = y0 + OVERLAY_TEXT_MARGIN;
                            }

                            max_text_x = OVERLAY_WIDTH - text_width;

                            if (max_text_x < 0)
                            {
                                max_text_x = 0;
                            }

                            if (text_x < 0)
                            {
                                text_x = 0;
                            }
                            else if (text_x > max_text_x)
                            {
                                text_x = max_text_x;
                            }

                            if (text_y < 0)
                            {
                                text_y = 0;
                            }
                            else if (text_y > (OVERLAY_HEIGHT - text_height))
                            {
                                text_y = OVERLAY_HEIGHT - text_height;
                            }

                            d2_ok = dave2d_overlay_draw_text(text_x,
                                                             text_y,
                                                             label,
                                                             OVERLAY_TEXT_COLOR,
                                                             OVERLAY_TEXT_SCALE);

                            if (!d2_ok)
                            {
                                d2_error = dave2d_overlay_get_last_error();
                            }

                        }
                    }

                    if (d2_frame_started)
                    {
                        bool const d2_end_ok = dave2d_overlay_end();

                        if (d2_ok && (!d2_end_ok))
                        {
                            d2_error = dave2d_overlay_get_last_error();
                        }

                        d2_ok = d2_ok && d2_end_ok;
                    }

                    if (d2_ok)
                    {
                        foreground_buffer_ready = true;
                        dave_error_reported = false;
                    }
                    else if (!dave_error_reported)
                    {
                        g_printf("[DISPLAY][ERR] D/AVE 2D drawing failed: %d.\r\n",
                                 (int) d2_error);
                        dave_error_reported = true;
                    }
                }

                if (foreground_buffer_ready)
                {
                    __DMB();
                    foreground_swap_pending = true;
                }
            }
        }

        bool background_swap_pending = true;

        for (;;)
        {
            err = display_wait_next_vsync(
                pdMS_TO_TICKS(50U));

            if (FSP_ERR_TIMEOUT == err)
            {
                if (!vsync_timeout_reported)
                {
                    g_printf("[DISPLAY][ERR] GLCDC VSYNC wait timed out.\r\n");
                    vsync_timeout_reported = true;
                }

                continue;
            }

            vsync_timeout_reported = false;

            if (FSP_SUCCESS != err)
            {
                APP_ERROR_TRAP(err);
            }

            if (background_swap_pending)
            {
                fsp_err_t const background_err =
                    R_GLCDC_BufferChange(
                        &g_display_ctrl,
                        (uint8_t *) fb_background[draw_buffer_index],
                        DISPLAY_FRAME_LAYER_1);

                if (FSP_SUCCESS == background_err)
                {
                    last_displayed_sequence = completed_sequence;
                    draw_buffer_index ^= 1U;
                    background_swap_pending = false;
                }
                else if (FSP_ERR_INVALID_UPDATE_TIMING == background_err)
                {
                    /*保留Layer 1待换帧状态，仍继续尝试Layer 2。*/
                }
                else
                {
                    APP_ERROR_TRAP(background_err);
                }
            }

            if (foreground_swap_pending)
            {
                fsp_err_t const foreground_err =
                    R_GLCDC_BufferChange(
                        &g_display_ctrl,
                        fb_foreground[foreground_draw_buffer_index],
                        DISPLAY_FRAME_LAYER_2);

                if (FSP_SUCCESS == foreground_err)
                {
                    foreground_draw_buffer_index ^= 1U;
                    foreground_swap_pending = false;
                }
                else if (FSP_ERR_INVALID_UPDATE_TIMING == foreground_err)
                {
                    /*保留Layer 2待换帧状态，下一次VSYNC重试。*/
                }
                else
                {
                    APP_ERROR_TRAP(foreground_err);
                }
            }

            if ((!background_swap_pending) &&
                (!foreground_swap_pending))
            {
                break;
            }
        }
    }
}
