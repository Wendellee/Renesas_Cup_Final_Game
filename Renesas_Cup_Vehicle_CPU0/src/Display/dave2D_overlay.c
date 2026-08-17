/*
 * dave2D_overlay.c
 *
 *  Created on: 2026年8月10日
 *      Author: lingk
 */
#include "dave2D_overlay.h"
#include "dave_driver.h"
#include "dave_math.h"

#define DAVE2D_RENDER_BUFFER_INITIAL_SIZE    (256U)
#define DAVE2D_RENDER_BUFFER_STEP_SIZE       (128U)
#define FONT5X7_WIDTH                 (5)
#define FONT5X7_HEIGHT                (7)
#define FONT5X7_SOURCE_PITCH_PIXELS   (8)
#define FONT5X7_GLYPH_BYTES           (64U)

/*
 *[@name] font5x7_build_alpha8_atlas
 *[@type] static function
 *[@usage] 将内置列式 5x7 字模转换为 D/AVE 2D 可读取的 Alpha8 字模图集。
 *[@argument] none
 *[@return] none
 */
static void font5x7_build_alpha8_atlas(void);

/* 当前绘制帧的有效尺寸，由 begin() 保存，供后续坐标裁剪使用。 */
static int  g_framebuffer_width;
static int  g_framebuffer_height;
/* 软件绘制状态：true 表示当前处于 begin() 与 end() 之间的命令录制阶段。 */
static bool g_frame_active;

/* D/AVE 2D 设备句柄。 */
static d2_device       * gp_d2_device = NULL;
/* 保存一帧内批量绘图命令的 render buffer。 */
static d2_renderbuffer * gp_d2_render_buffer = NULL;
/* 保存最近一次适配层检查或 D/AVE 2D API 返回的错误码。 */
static int32_t g_d2_last_error = D2_OK;

static const char g_font5x7_char_map[] =
    "? :0123456789BCDPacdeiklnoprstuyghm";

/*适用于alpha8通道的字模数组*/
static uint8_t
    g_font5x7_alpha8[sizeof(g_font5x7_char_map) - 1U][FONT5X7_GLYPH_BYTES]
    BSP_ALIGN_VARIABLE(64)
    BSP_PLACE_IN_SECTION(".sdram_noinit_nocache");

/*
 *[@name] rgb565_to_d2_color
 *[@type] static function
 *[@usage] 将 RGB565 颜色扩展为 D/AVE 2D 使用的 0x00RRGGBB 颜色值。
 *[@argument] rgb565：16 bit RGB565 颜色值。
 *[@return] 转换后的 D/AVE 2D 颜色值。
 */
static d2_color rgb565_to_d2_color(uint16_t rgb565)
{
    uint32_t red_5   = (rgb565 >> 11) & 0x1FU;
    uint32_t green_6 = (rgb565 >> 5)  & 0x3FU;
    uint32_t blue_5  = rgb565 & 0x1FU;

    /* 将 5/6 bit 色彩分量扩展到 8 bit。 */
    uint32_t red_8   = (red_5 << 3) | (red_5 >> 2);
    uint32_t green_8 = (green_6 << 2) | (green_6 >> 4);
    uint32_t blue_8  = (blue_5 << 3) | (blue_5 >> 2);

    return (d2_color) ((red_8 << 16) |
                       (green_8 << 8) |
                       blue_8);
}

/*
 *[@name] dave2d_overlay_init
 *[@type] function
 *[@usage] 创建并初始化 D/AVE 2D 设备、命令缓冲区和内置字模图集，重复调用不会重复创建设备。
 *[@argument] none
 *[@return] true：初始化成功或此前已经初始化；false：初始化失败，可读取最近错误码。
 */
bool dave2d_overlay_init(void)
{
    if (gp_d2_device != NULL)
    {
        return true;
    }

    gp_d2_device = d2_opendevice(0);

    if (gp_d2_device == NULL)
    {
        g_d2_last_error = D2_INVALIDDEVICE;
        return false;
    }

    g_d2_last_error = d2_inithw(gp_d2_device, 0);

    if (g_d2_last_error != D2_OK)
    {
        d2_closedevice(gp_d2_device);
        gp_d2_device = NULL;
        return false;
    }

    gp_d2_render_buffer =
        d2_newrenderbuffer(gp_d2_device,
                           DAVE2D_RENDER_BUFFER_INITIAL_SIZE,
                           DAVE2D_RENDER_BUFFER_STEP_SIZE);

    if (gp_d2_render_buffer == NULL)
    {
        g_d2_last_error = D2_NOMEMORY;
        d2_closedevice(gp_d2_device);
        gp_d2_device = NULL;
        return false;
    }

    /*
     * 纯色、不透明、直接覆盖目标像素。
     */
    g_d2_last_error =
        d2_selectrendermode(gp_d2_device, d2_rm_solid);

    if (g_d2_last_error != D2_OK)
    {
        return false;
    }

    g_d2_last_error =
        d2_setblendmode(gp_d2_device, d2_bm_one, d2_bm_zero);

    if (g_d2_last_error != D2_OK)
    {
        return false;
    }
    /*
     * 把 CPU 使用的按列 5×7 字模转换为 D/AVE 2D 可读取的
     * 按行 Alpha8 字模。字模存放在 non-cache SDRAM 中，只生成一次。
     */
    font5x7_build_alpha8_atlas();

    return true;
}

/*
 *[@name] dave2d_overlay_get_last_error
 *[@type] function
 *[@usage] 返回最近一次适配层检查或 D/AVE 2D API 调用产生的错误码。
 *[@argument] none
 *[@return] D2_OK 表示最近一次操作成功，其他值见 dave_errorcodes.h。
 */
int32_t dave2d_overlay_get_last_error(void)
{
    return g_d2_last_error;
}

/*
 *[@name] dave2d_overlay_begin
 *[@type] function
 *[@usage] 选择当前命令缓冲区，绑定目标 ARGB4444 framebuffer，并开始一帧命令录制。
 *[@argument] framebuffer：目标帧缓冲区首地址；width、height：有效像素尺寸；pitch_pixels：相邻行起点的像素跨度。
 *[@return] true：绘制会话建立成功；false：参数、状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_begin(void * framebuffer,
                          int width,
                          int height,
                          int pitch_pixels)
{
    if(true == g_frame_active)
    {
        g_d2_last_error = D2_DEVICEBUSY;
        return false;
    }
    if(NULL == gp_d2_device)
    {
        g_d2_last_error = D2_INVALIDDEVICE;
        return false;
    }
    if(NULL == gp_d2_render_buffer)
    {
        g_d2_last_error = D2_INVALIDBUFFER;
        return false;
    }
    if(NULL == framebuffer)
    {
        g_d2_last_error = D2_NULLPOINTER;
        return false;
    }
    if(width <= 0)
    {
        g_d2_last_error = D2_INVALIDWIDTH;
        return false;
    }
    if(height <= 0)
    {
        g_d2_last_error = D2_INVALIDHEIGHT;
        return false;
    }
    if(pitch_pixels < width)
    {
        g_d2_last_error = D2_INVALIDWIDTH;
        return false;
    }

    g_d2_last_error = d2_selectrenderbuffer(gp_d2_device, gp_d2_render_buffer);//gp_d2_render_buffer是指令表
    if(D2_OK != g_d2_last_error)
    {
        return false;
    }

    g_d2_last_error =
        d2_framebuffer(gp_d2_device,
                       framebuffer,
                       pitch_pixels,
                       (d2_u32) width,
                       (d2_u32) height,
                       d2_mode_argb4444);
    if(D2_OK != g_d2_last_error)
    {
        return false;
    }

    g_d2_last_error =
        d2_cliprect(gp_d2_device,
                    0,
                    0,
                    (d2_border) (width - 1),
                    (d2_border) (height - 1));
    if(D2_OK != g_d2_last_error)
    {
        return false;
    }

    g_framebuffer_width = width;
    g_framebuffer_height = height;
    g_frame_active = true;

    return true;
}

/*
 *[@name] dave2d_overlay_draw_rect
 *[@type] function
 *[@usage] 向当前帧命令缓冲区追加一个矩形框，不提交命令，也不等待硬件执行。
 *[@argument] x0、y0、x1、y1：两个对角点坐标；rgb565：RGB565 颜色值；line_width：边框像素宽度。
 *[@return] true：命令追加成功或矩形完全在画面外；false：参数、状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_draw_rect(int x0,
                              int y0,
                              int x1,
                              int y1,
                              uint16_t rgb565,
                              int line_width)
{
    if (gp_d2_device == NULL)
    {
        g_d2_last_error = D2_INVALIDDEVICE;
        return false;
    }

    if (!g_frame_active)
    {
        g_d2_last_error = D2_INVALIDCONTEXT;
        return false;
    }

    /*检查线宽*/
    if (line_width <= 0)
    {
        g_d2_last_error = D2_INVALIDWIDTH;
        return false;
    }

    /*统一坐标方向*/
    if (x0 > x1)
    {
        x0 ^= x1;
        x1 ^= x0;
        x0 ^= x1;
    }

    if (y0 > y1)
    {
        y0 ^= y1;
        y1 ^= y0;
        y0 ^= y1;
    }//统一成左上右下角

    /*判断矩形是否完全在屏幕外*/
    if ((x1 < 0) ||
    (y1 < 0) ||
    (x0 >= g_framebuffer_width) ||
    (y0 >= g_framebuffer_height))
    {
        g_d2_last_error = D2_OK;
        return true;
    }

    /*裁剪到有效区域(framebuffer范围)*/
    if (x0 < 0)                         { x0 = 0; }
    if (y0 < 0)                         { y0 = 0; }
    if (x1 >= g_framebuffer_width)      { x1 = g_framebuffer_width - 1; }
    if (y1 >= g_framebuffer_height)     { y1 = g_framebuffer_height - 1; }

    d2_point d2_x0 = (d2_point) D2_FIX4(x0);
    d2_point d2_y0 = (d2_point) D2_FIX4(y0);
    d2_point d2_x1 = (d2_point) D2_FIX4(x1);
    d2_point d2_y1 = (d2_point) D2_FIX4(y1);
    d2_width d2_line_width = (d2_width) D2_FIX4(line_width);

    /*设置颜色*/
    g_d2_last_error = d2_setcolor(gp_d2_device, 0, rgb565_to_d2_color(rgb565));
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    /* 绘制上边：(x0, y0) -> (x1, y0) */
    g_d2_last_error = d2_renderline(gp_d2_device,
                                    d2_x0,
                                    d2_y0,
                                    d2_x1,
                                    d2_y0,
                                    d2_line_width,
                                    d2_le_exclude_none);
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    /* 绘制右边：(x1, y0) -> (x1, y1) */
    g_d2_last_error = d2_renderline(gp_d2_device,
                                    d2_x1,
                                    d2_y0,
                                    d2_x1,
                                    d2_y1,
                                    d2_line_width,
                                    d2_le_exclude_none);
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    /* 绘制下边：(x1, y1) -> (x0, y1) */
    g_d2_last_error = d2_renderline(gp_d2_device,
                                    d2_x1,
                                    d2_y1,
                                    d2_x0,
                                    d2_y1,
                                    d2_line_width,
                                    d2_le_exclude_none);
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    /* 绘制左边：(x0, y1) -> (x0, y0) */
    g_d2_last_error = d2_renderline(gp_d2_device,
                                    d2_x0,
                                    d2_y1,
                                    d2_x0,
                                    d2_y0,
                                    d2_line_width,
                                    d2_le_exclude_none);
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    return true;
}

/*
 *[@name] dave2d_overlay_end
 *[@type] function
 *[@usage] 提交当前帧累计的全部命令，等待 D/AVE 2D 完成，并结束当前绘制会话。
 *[@argument] none
 *[@return] true：硬件执行完成；false：状态无效或命令提交、执行失败。
 */
bool dave2d_overlay_end(void)
{
    if(NULL == gp_d2_device)
    {
        g_frame_active      = false;
        g_framebuffer_width = 0;
        g_framebuffer_height = 0;
        g_d2_last_error = D2_INVALIDDEVICE;
        return false;
    }
    if(NULL == gp_d2_render_buffer)
    {
        g_frame_active      = false;
        g_framebuffer_width = 0;
        g_framebuffer_height = 0;
        g_d2_last_error = D2_INVALIDBUFFER;
        return false;
    }
    if(!g_frame_active)//检查是否已经成功调用 begin()
    {
        g_d2_last_error = D2_INVALIDCONTEXT;

        g_frame_active      = false;
        g_framebuffer_width = 0;
        g_framebuffer_height = 0;
        return false;
    }

    g_d2_last_error =
    d2_executerenderbuffer(gp_d2_device,
                           gp_d2_render_buffer,
                           0);
    if(D2_OK != g_d2_last_error)
    {
        g_frame_active      = false;
        g_framebuffer_width = 0;
        g_framebuffer_height = 0;
        return false;
    }

    /* 等待硬件完成。 */
    g_d2_last_error = d2_flushframe(gp_d2_device);
    if(D2_OK != g_d2_last_error)
    {
        g_frame_active      = false;
        g_framebuffer_width = 0;
        g_framebuffer_height = 0;
        return false;
    }

    g_frame_active      = false;
    g_framebuffer_width = 0;
    g_framebuffer_height = 0;

    return true;
}

/*
 *[@name] dave2d_overlay_draw_filled_rect
 *[@type] function
 *[@usage] 向当前帧命令缓冲区追加一个纯色实心矩形，不提交命令，也不等待硬件执行。
 *[@argument] x0、y0、x1、y1：两个对角点坐标；rgb565：RGB565 填充颜色值。
 *[@return] true：命令追加成功或矩形完全在画面外；false：状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_draw_filled_rect(int x0,
                                     int y0,
                                     int x1,
                                     int y1,
                                     uint16_t rgb565)
{
    /*检查设备和帧状态*/
    if(NULL == gp_d2_device)
    {
        g_d2_last_error = D2_INVALIDDEVICE;
        return false;
    }
    if(!g_frame_active)
    {
        g_d2_last_error = D2_INVALIDCONTEXT;
        return false;
    }

    /*统一坐标方向*/
    if (x0 > x1)
    {
        x0 ^= x1;
        x1 ^= x0;
        x0 ^= x1;
    }
    if (y0 > y1)
    {
        y0 ^= y1;
        y1 ^= y0;
        y0 ^= y1;
    }

    /*判断矩形是否完全在屏幕外*/
    if ((x1 < 0) ||
        (y1 < 0) ||
        (x0 >= g_framebuffer_width) ||
        (y0 >= g_framebuffer_height))
        {
            g_d2_last_error = D2_OK;
            return true;
        }

    /*裁剪到有效区域(framebuffer范围)*/
    if (x0 < 0)                         { x0 = 0; }
    if (y0 < 0)                         { y0 = 0; }
    if (x1 >= g_framebuffer_width)      { x1 = g_framebuffer_width - 1; }
    if (y1 >= g_framebuffer_height)     { y1 = g_framebuffer_height - 1; }

    /*矩形尺寸计算*/
    int box_width  = x1 - x0 + 1;
    int box_height = y1 - y0 + 1;

    /*设置颜色*/
    g_d2_last_error = d2_setcolor(gp_d2_device,
                                  0,
                                  rgb565_to_d2_color(rgb565));
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    /*录入绘制矩形命令*/
    g_d2_last_error =d2_renderbox(gp_d2_device,
                                  (d2_point) D2_FIX4(x0),
                                  (d2_point) D2_FIX4(y0),
                                  (d2_width) D2_FIX4(box_width),
                                  (d2_width) D2_FIX4(box_height));
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    return true;
}

/*
 *[@name] font5x7_get_glyph
 *[@type] static function
 *[@usage] 从内置 5x7 列式字库读取指定字符，未收录字符返回问号字模。
 *[@argument] character：待查询字符；glyph：用于接收 5 列点阵数据的数组。
 *[@return] none
 */
static void font5x7_get_glyph(char character, uint8_t glyph[5])
{
    static const uint8_t unknown[5] = {0x02U, 0x01U, 0x59U, 0x09U, 0x06U};
    static const uint8_t space[5] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    static const uint8_t colon[5] = {0x00U, 0x36U, 0x36U, 0x00U, 0x00U};
    static const uint8_t digit_0[5] = {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU};
    static const uint8_t digit_1[5] = {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U};
    static const uint8_t digit_2[5] = {0x42U, 0x61U, 0x51U, 0x49U, 0x46U};
    static const uint8_t digit_3[5] = {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U};
    static const uint8_t digit_4[5] = {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U};
    static const uint8_t digit_5[5] = {0x27U, 0x45U, 0x45U, 0x45U, 0x39U};
    static const uint8_t digit_6[5] = {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U};
    static const uint8_t digit_7[5] = {0x01U, 0x71U, 0x09U, 0x05U, 0x03U};
    static const uint8_t digit_8[5] = {0x36U, 0x49U, 0x49U, 0x49U, 0x36U};
    static const uint8_t digit_9[5] = {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU};
    static const uint8_t letter_B[5] = {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U};
    static const uint8_t letter_C[5] = {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U};
    static const uint8_t letter_D[5] = {0x7FU, 0x41U, 0x41U, 0x22U, 0x1CU};
    static const uint8_t letter_P[5] = {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U};
    static const uint8_t letter_a[5] = {0x20U, 0x54U, 0x54U, 0x54U, 0x78U};
    static const uint8_t letter_c[5] = {0x38U, 0x44U, 0x44U, 0x44U, 0x20U};
    static const uint8_t letter_d[5] = {0x38U, 0x44U, 0x44U, 0x48U, 0x7FU};
    static const uint8_t letter_e[5] = {0x38U, 0x54U, 0x54U, 0x54U, 0x18U};
    static const uint8_t letter_i[5] = {0x00U, 0x44U, 0x7DU, 0x40U, 0x00U};
    static const uint8_t letter_k[5] = {0x7FU, 0x10U, 0x28U, 0x44U, 0x00U};
    static const uint8_t letter_l[5] = {0x00U, 0x41U, 0x7FU, 0x40U, 0x00U};
    static const uint8_t letter_n[5] = {0x7CU, 0x08U, 0x04U, 0x04U, 0x78U};
    static const uint8_t letter_o[5] = {0x38U, 0x44U, 0x44U, 0x44U, 0x38U};
    static const uint8_t letter_p[5] = {0xFCU, 0x24U, 0x24U, 0x24U, 0x18U};
    static const uint8_t letter_r[5] = {0x7CU, 0x08U, 0x04U, 0x04U, 0x08U};
    static const uint8_t letter_s[5] = {0x48U, 0x54U, 0x54U, 0x54U, 0x20U};
    static const uint8_t letter_t[5] = {0x04U, 0x3FU, 0x44U, 0x40U, 0x20U};
    static const uint8_t letter_u[5] = {0x3CU, 0x40U, 0x40U, 0x20U, 0x7CU};
    static const uint8_t letter_y[5] = {0x0CU, 0x50U, 0x50U, 0x50U, 0x3CU};
    static const uint8_t letter_g[5] = {0x18U, 0xA4U, 0xA4U, 0xA4U, 0x7CU};
    static const uint8_t letter_h[5] = {0x7FU, 0x08U, 0x04U, 0x04U, 0x78U};
    static const uint8_t letter_m[5] = {0x7CU, 0x04U, 0x78U, 0x04U, 0x78U};
    const uint8_t * p_glyph = unknown;

    switch (character)
    {
        case ' ': p_glyph = space; break;
        case ':': p_glyph = colon; break;
        case '0': p_glyph = digit_0; break;
        case '1': p_glyph = digit_1; break;
        case '2': p_glyph = digit_2; break;
        case '3': p_glyph = digit_3; break;
        case '4': p_glyph = digit_4; break;
        case '5': p_glyph = digit_5; break;
        case '6': p_glyph = digit_6; break;
        case '7': p_glyph = digit_7; break;
        case '8': p_glyph = digit_8; break;
        case '9': p_glyph = digit_9; break;
        case 'B': p_glyph = letter_B; break;
        case 'C': p_glyph = letter_C; break;
        case 'D': p_glyph = letter_D; break;
        case 'P': p_glyph = letter_P; break;
        case 'a': p_glyph = letter_a; break;
        case 'c': p_glyph = letter_c; break;
        case 'd': p_glyph = letter_d; break;
        case 'e': p_glyph = letter_e; break;
        case 'i': p_glyph = letter_i; break;
        case 'k': p_glyph = letter_k; break;
        case 'l': p_glyph = letter_l; break;
        case 'n': p_glyph = letter_n; break;
        case 'o': p_glyph = letter_o; break;
        case 'p': p_glyph = letter_p; break;
        case 'r': p_glyph = letter_r; break;
        case 's': p_glyph = letter_s; break;
        case 't': p_glyph = letter_t; break;
        case 'u': p_glyph = letter_u; break;
        case 'y': p_glyph = letter_y; break;
        case 'g': p_glyph = letter_g; break;
        case 'h': p_glyph = letter_h; break;
        case 'm': p_glyph = letter_m; break;
        default: break;
    }

    for (int column = 0; column < 5; column++)
    {
        glyph[column] = p_glyph[column];
    }
}

/*
 *[@name] font5x7_build_alpha8_atlas
 *[@type] static function
 *[@usage] 将全部列式 5x7 字模转换为 8x8 Alpha8 字模图集，透明区域填零。
 *[@argument] none
 *[@return] none
 */
static void font5x7_build_alpha8_atlas(void)
{
    size_t glyph_count = sizeof(g_font5x7_char_map) - 1U;

    for (size_t glyph_index = 0U;
         glyph_index < glyph_count;
         glyph_index++)
    {
        uint8_t column_data[FONT5X7_WIDTH];

        font5x7_get_glyph(g_font5x7_char_map[glyph_index],
                          column_data);

        for (int row = 0; row < 8; row++)
            {
                for (int column = 0; column < FONT5X7_WIDTH; column++)
                {
                    bool pixel_on =
                        (column_data[column] & (1U << row)) != 0U;

                    g_font5x7_alpha8[glyph_index][row * 8 + column] =
                        pixel_on ? 0xFFU : 0x00U;
                }

                /* 第 6～8 列补透明 */
                for (int column = FONT5X7_WIDTH; column < 8; column++)
                {
                    g_font5x7_alpha8[glyph_index][row * 8 + column] = 0x00U;
                }
            }

            /* 第 8 行全部补透明 */
            for (int column = 0; column < 8; column++)
            {
                g_font5x7_alpha8[glyph_index][7 * 8 + column] = 0x00U;
            }
        /* 第8字节仅用于补齐和对齐。 */
    }
}

/*
 *[@name] font5x7_find_glyph_index
 *[@type] static function
 *[@usage] 查找字符在 Alpha8 字模图集中的序号，未收录字符映射到问号。
 *[@argument] character：待查询字符。
 *[@return] 字符在 g_font5x7_alpha8 中的数组下标。
 */
static size_t font5x7_find_glyph_index(char character)
{
    size_t glyph_count = sizeof(g_font5x7_char_map) - 1U;

    for (size_t index = 0U; index < glyph_count; index++)
    {
        if (g_font5x7_char_map[index] == character)
        {
            return index;
        }
    }

    return 0U;
}

/*
 *[@name] dave2d_overlay_restore_blend_and_fail
 *[@type] static function
 *[@usage] 文本绘制失败后恢复默认不透明混合模式，并保留最初的错误码。
 *[@argument] original_error：真正导致本次绘制失败的 D/AVE 2D 错误码。
 *[@return] 固定返回 false。
 */
static bool dave2d_overlay_restore_blend_and_fail(int32_t original_error)
{
    (void) d2_setblendmode(gp_d2_device, d2_bm_one, d2_bm_zero);
    g_d2_last_error = original_error;

    return false;
}

/*
 *[@name] dave2d_overlay_draw_text
 *[@type] function
 *[@usage] 使用 Alpha8 字模向当前帧命令缓冲区追加文本，不在本函数内提交硬件执行。
 *[@argument] x、y：文本左上角坐标；text：零结尾字符串；rgb565：RGB565 文字颜色；scale：整数缩放倍数。
 *[@return] true：全部字符命令追加成功；false：参数、状态或 D/AVE 2D API 调用失败。
 */
bool dave2d_overlay_draw_text(int x,
                              int y,
                              char const * text,
                              uint16_t rgb565,
                              int scale)
{
    /* D/AVE 2D 设备必须已经初始化。 */
    if (NULL == gp_d2_device)
    {
        g_d2_last_error = D2_INVALIDDEVICE;
        return false;
    }

    /* 文字必须绘制在 frame_begin() 与 frame_end() 之间。 */
    if (!g_frame_active)
    {
        g_d2_last_error = D2_INVALIDCONTEXT;
        return false;
    }

    /* 防止解引用空字符串指针。 */
    if (NULL == text)
    {
        g_d2_last_error = D2_NULLPOINTER;
        return false;
    }

    /* scale 同时决定字符宽度、高度和字符间距，必须为正数。 */
    if (scale <= 0)
    {
        g_d2_last_error = D2_INVALIDWIDTH;
        return false;
    }

    /* Alpha8 字模只保存透明度，真正的文字颜色由颜色寄存器 0 提供。 */
    g_d2_last_error = d2_setcolor(gp_d2_device,
                                  0,
                                  rgb565_to_d2_color(rgb565));
    if (D2_OK != g_d2_last_error)
    {
        return false;
    }

    /*
     * 字模位为 1 的像素使用文字颜色，位为 0 的像素保留 framebuffer 原像素。
     * 当前字模像素使用 0x00 或 0xFF，可理解为一张透明度遮罩。
     */
    g_d2_last_error = d2_setblendmode(gp_d2_device,
                                      d2_bm_alpha,
                                      d2_bm_one_minus_alpha);
    if (D2_OK != g_d2_last_error)
    {
        return dave2d_overlay_restore_blend_and_fail(g_d2_last_error);
    }

    int current_x = x;

    /* 从左到右扫描字符串，每个非空格字符对应一条 D/AVE 2D 贴图命令。 */
    while ('\0' != *text)
    {
        if (' ' == *text)
        {
            current_x += 6 * scale;
            text++;
            continue;
        }

        /*查找当前字符；字库未收录的字符统一显示成 '?'*/
        size_t glyph_index = font5x7_find_glyph_index(*text);

        /*选中当前字符的 8×8 Alpha8 存储块作为贴图源*/
        g_d2_last_error = d2_setblitsrc(gp_d2_device,
                                        &g_font5x7_alpha8[glyph_index][0],
                                        FONT5X7_SOURCE_PITCH_PIXELS,
                                        FONT5X7_WIDTH,
                                        FONT5X7_HEIGHT,
                                        d2_mode_alpha8);
        if (D2_OK != g_d2_last_error)
        {
            return dave2d_overlay_restore_blend_and_fail(g_d2_last_error);
        }

        g_d2_last_error = d2_blitcopy(gp_d2_device,
                                      FONT5X7_WIDTH,
                                      FONT5X7_HEIGHT,
                                      0,
                                      0,
                                      (d2_width) D2_FIX4(FONT5X7_WIDTH * scale),
                                      (d2_width) D2_FIX4(FONT5X7_HEIGHT * scale),
                                      (d2_point) D2_FIX4(current_x),
                                      (d2_point) D2_FIX4(y),
                                      d2_bf_colorize | d2_bf_usealpha);
        if (D2_OK != g_d2_last_error)
        {
            return dave2d_overlay_restore_blend_and_fail(g_d2_last_error);
        }

        /* 当前字符完成后，把“画笔”移到下一个字符的起始位置。 */
        current_x += 6 * scale;
        text++;
    }

    /* 恢复普通不透明绘制模式，避免影响本帧后续的矩形和线条。 */
    g_d2_last_error = d2_setblendmode(gp_d2_device,
                                      d2_bm_one,
                                      d2_bm_zero);
    return (D2_OK == g_d2_last_error);
}
