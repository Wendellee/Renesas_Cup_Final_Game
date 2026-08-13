/*
 * glcdc_display.h
 *
 * RA8P1 GLCDC 显示驱动 — LVGL 适配层
 *
 *  Created on: 2026-03-31
 *      Author: Wendell
 */

#ifndef GLCDC_DISPLAY_H_
#define GLCDC_DISPLAY_H_

#include "common_utils.h"

/* 显示分辨率 */
#define LCD_Width   800
#define LCD_Height  480

/* VSYNC 标志 */
#define RESET_FLAG              (0U)
#define SET_FLAG                (1U)

#define BYTES_PER_PIXEL     (4)
#define RED         (0x00FF0000)
#define GREEN       (0x0000FF00)
#define BLUE        (0x000000FF)
#define BLACK       (0x00000000)
#define WHITE       (0xFFFFFFFF)
#define YELLOW      (0xFFFFFF00)
#define MAGENTA     (0x00FF00FF)
#define CYAN        (0x0000FFFF)

#define X1_CO_ORDINATE      (0U)
#define Y1_CO_ORDINATE      (0U)
#define COLOR_BAND_COUNT    (8U)
#define INC_DEC_VALUE       (1)

/* 全局变量 */
extern volatile uint8_t g_vsync_flag;

/* 函数声明 */
fsp_err_t glcdc_init(void);
void screen_display(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void color_band_display(void);
void LCD_Backlight_ON(void);
void LCD_Backlight_OFF(void);
#endif /* GLCDC_DISPLAY_H_ */
