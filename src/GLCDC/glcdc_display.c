/*
 * glcdc_display.c
 *
 * RA8P1 GLCDC 显示驱动 — LVGL 适配层
 * 参考 STM32H7 lcd_rgb.c 的 LCD_CopyBuffer 实现
 *
 *  Created on: 2026-03-31
 *      Author: Wendell
 */

#include "glcdc_display.h"
#include <string.h>

volatile uint8_t g_vsync_flag = RESET_FLAG;

uint16_t g_image_width  = 800;   /* 默认水平分辨率 */
uint16_t g_image_height = 480;   /* 默认垂直分辨率 */

/*******************************************************************************
 * GLCDC 初始化
 *
 * 复用原 FSP 生成的 g_display_ctrl / g_display_cfg，
 * 根据 g_image_width / g_image_height 覆盖实际分辨率，
 * 启动后等待首个 VSYNC 信号。
 ******************************************************************************/
fsp_err_t glcdc_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    if (DISPLAY_STATE_CLOSED != g_display_ctrl.state)
    {
        err = R_GLCDC_Close(&g_display_ctrl);
        APP_ERR_RET(FSP_SUCCESS != err, err, "** R_GLCDC_Close API FAILED **");
    }

    display_cfg_t g_display_user_cfg;
    memcpy(&g_display_user_cfg, &g_display_cfg, sizeof(display_cfg_t));

    g_display_user_cfg.input[0].hsize = g_image_width;
    g_display_user_cfg.input[0].vsize = g_image_height;

    err = R_GLCDC_Open(&g_display_ctrl, &g_display_user_cfg);
    APP_ERR_RET(FSP_SUCCESS != err, err, "** R_GLCDC_Open API FAILED **");

    err = R_GLCDC_Start(&g_display_ctrl);
    APP_ERR_RET(FSP_SUCCESS != err, err, "** R_GLCDC_Start API FAILED **");

    /* 等待首个 VSYNC 信号（超时 ~500ms），确保 GLCDC 已开始输出 */
    g_vsync_flag = RESET_FLAG;
    {
        uint32_t timeout = 500000;
        while (!g_vsync_flag && (timeout > 0))
        {
            timeout--;
        }
    }

    return err;
}

/*******************************************************************************
 * VSYNC 中断回调
 *
 * 每个场同步周期触发一次，设置标志位供上层同步使用。
 ******************************************************************************/
void glcdc_vsync_isr(display_callback_args_t *p_args)
{
    if (p_args->event == DISPLAY_EVENT_LINE_DETECTION)
    {
        g_vsync_flag = SET_FLAG;
    }
}

void screen_display(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    /* Variables to store resolution information */
    uint16_t g_hz_size, g_vr_size;
    /* Variables used for buffer usage */
    uint32_t g_buffer_size;
    uint8_t * g_p_single_buffer, * g_p_double_buffer;

    /* Get LCDC configuration */
    g_hz_size = (g_display_cfg.input[0].hsize);
    g_vr_size = (g_display_cfg.input[0].vsize);
    /* Initialize buffer pointers */
    g_buffer_size = (uint32_t) (g_hz_size * g_vr_size * BYTES_PER_PIXEL);
    g_p_single_buffer = (uint8_t *) g_display_cfg.input[0].p_base;
    /* Double buffer for drawing color bands with good quality */
    g_p_double_buffer = g_p_single_buffer + g_buffer_size;


    /* Declare local variables */
        uint16_t start_x, start_y, display_length, display_height;
        uint32_t start_addr;

        /* Assign coordinate values and calculate start address */
        start_x = x1;
        start_y = y1;
        start_addr = (uint32_t)((start_x * BYTES_PER_PIXEL) + (start_y * g_hz_size* BYTES_PER_PIXEL));

        /* Calculate display box length and height */
        display_length = (uint16_t)((x2 - x1) * BYTES_PER_PIXEL);
        display_height = (y2 - y1);

        /* Display required color band */
        for(uint16_t ver_value = Y1_CO_ORDINATE; ver_value < (display_height - INC_DEC_VALUE); ver_value++)
        {
            for(uint32_t hor_value = start_addr; hor_value < (start_addr + display_length); hor_value += BYTES_PER_PIXEL)
            {
                *(uint32_t *) (g_p_single_buffer + hor_value) = color;
                *(uint32_t *) (g_p_double_buffer + hor_value) = color;
            }
            start_addr = (uint32_t)(start_addr + (g_hz_size * BYTES_PER_PIXEL));
        }
}

/*******************************************************************************************************************//**
 * @brief      This function displays eight horizontal color bands on the graphical LCD.
 * @param[IN]  None
 * @retval     None
 **********************************************************************************************************************/
void color_band_display(void)
{
    uint32_t color[COLOR_BAND_COUNT]= {RED, GREEN, BLUE, BLACK, WHITE, YELLOW, MAGENTA, CYAN};
    uint16_t width = 480/COLOR_BAND_COUNT;
    for (uint8_t display_count = RESET_VALUE; display_count < COLOR_BAND_COUNT; display_count++)
    {
        screen_display((uint16_t)X1_CO_ORDINATE, (display_count * width), 800,\
                           (uint16_t)(((display_count * width) + width) + INC_DEC_VALUE), color[display_count]);
    }
}
/*******************************************************************************
 * 背光控制 (可选)
 *
 * 如果你的板子有背光 GPIO，根据实际引脚在这里实现。
 * 例如: R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_BL, level);
 ******************************************************************************/
 void LCD_Backlight_ON(void)
 {
     R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_BL, BSP_IO_LEVEL_HIGH);
 }

 void LCD_Backlight_OFF(void)
 {
     R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_BL, BSP_IO_LEVEL_LOW);
 }
