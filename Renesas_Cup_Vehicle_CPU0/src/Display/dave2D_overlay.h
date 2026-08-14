/*
 * dave2D_overlay.h
 *
 *  Created on: 2026年8月10日
 *      Author: lingk
 */
#ifndef DAVE2D_OVERLAY_H
#define DAVE2D_OVERLAY_H

#include "hal_data.h"
#include <stdbool.h>
#include <stdint.h>

/*
 *[@name] dave2d_overlay_init
 *[@type] function
 *[@usage] 初始化 D/AVE 2D 设备、命令缓冲区和内置字模图集，整个应用生命周期只需成功调用一次。
 *[@argument] none
 *[@return] true：初始化成功或此前已经初始化；false：初始化失败，可读取最近错误码。
 */
bool dave2d_overlay_init(void);

/*
 *[@name] dave2d_overlay_begin
 *[@type] function
 *[@usage] 开始一帧批量绘制并绑定目标 ARGB4444 framebuffer，必须与 dave2d_overlay_end() 成对调用。
 *[@argument] framebuffer：目标帧缓冲区首地址；width、height：有效像素尺寸；pitch_pixels：相邻行起点的像素跨度。
 *[@return] true：绘制会话建立成功；false：参数、状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_begin(void * framebuffer,
                          int width,
                          int height,
                          int pitch_pixels);

/*
 *[@name] dave2d_overlay_draw_rect
 *[@type] function
 *[@usage] 向当前帧命令缓冲区追加一个矩形框，不立即提交硬件执行。
 *[@argument] x0、y0、x1、y1：两个对角点坐标；rgb565：RGB565 颜色值；line_width：边框像素宽度。
 *[@return] true：命令追加成功或矩形完全在画面外；false：参数、状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_draw_rect(int x0,
                              int y0,
                              int x1,
                              int y1,
                              uint16_t rgb565,
                              int line_width);

/*
 *[@name] dave2d_overlay_end
 *[@type] function
 *[@usage] 提交当前帧全部命令并等待 D/AVE 2D 硬件完成，同时结束当前绘制会话。
 *[@argument] none
 *[@return] true：硬件执行完成；false：状态无效或命令提交、执行失败。
 */
bool dave2d_overlay_end(void);

/*
 *[@name] dave2d_overlay_get_last_error
 *[@type] function
 *[@usage] 获取最近一次适配层检查或 D/AVE 2D API 调用产生的错误码。
 *[@argument] none
 *[@return] D2_OK 表示最近一次操作成功，其他值见 dave_errorcodes.h。
 */
int32_t dave2d_overlay_get_last_error(void);

/*
 *[@name] dave2d_overlay_draw_filled_rect
 *[@type] function
 *[@usage] 向当前帧命令缓冲区追加一个纯色实心矩形，不立即提交硬件执行。
 *[@argument] x0、y0、x1、y1：两个对角点坐标；rgb565：RGB565 填充颜色值。
 *[@return] true：命令追加成功或矩形完全在画面外；false：状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_draw_filled_rect(int x0,
                                     int y0,
                                     int x1,
                                     int y1,
                                     uint16_t rgb565);

/*
 *[@name] dave2d_overlay_draw_text
 *[@type] function
 *[@usage] 使用 D/AVE 2D Alpha8 字模向当前帧命令缓冲区追加文本。
 *[@argument] x、y：文本左上角坐标；text：零结尾字符串；rgb565：RGB565 文字颜色；scale：整数缩放倍数。
 *[@return] true：全部字符命令追加成功；false：参数、状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_draw_text(int x,
                              int y,
                              char const * text,
                              uint16_t rgb565,
                              int scale);

#endif // DAVE2D_OVERLAY_H
