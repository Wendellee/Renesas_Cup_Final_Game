#ifndef __TOUCH_GT1151_H
#define __TOUCH_GT1151_H

#include "i2c_control.h"

/* 触摸坐标结构体 */
typedef struct {
    uint16_t x;
    uint16_t y;
} TouchCoordinate_t;

/* 触摸事件类型 */
typedef enum {
    TOUCH_EVENT_NONE,
    TOUCH_EVENT_DOWN,
    TOUCH_EVENT_HOLD,
    TOUCH_EVENT_MOVE,
    TOUCH_EVENT_UP
} touch_event_t;

/* GT1151 I2C 地址 */
#define GT1151_I2C_ADDRESS_0x5D   0x5D
#define GT1151_I2C_ADDRESS_0x14   0x14
#define GT1151_I2C_ADDRESS        GT1151_I2C_ADDRESS_0x5D

/* GT1151 寄存器地址 (16位) */
#define GT1151_REG_COMMAND          0x8040
#define GT1151_REG_CONFIG_VERSION   0x8047
#define GT1151_REG_CONFIG_CHECKSUM  0x80FF
#define GT1151_REG_CONFIG_FRESH     0x8100
#define GT1151_REG_PRODUCT_ID       0x8140
#define GT1151_REG_FW_VER_HIGH      0x8145
#define GT1151_REG_READ_COORD_ADDR  0x814E
#define GT1151_REG_POINT1_X_ADDR    0x814F

/* 寄存器 0x814E 位域定义 */
#define BUFFER_READY            (1 << 7)
#define NUM_TOUCH_POINTS_MASK   0x0F

/* GT1151 每点坐标数据长度 (X_L, X_H, Y_L, Y_H, ...共8字节) */
#define GT1151_POINT_DATA_SIZE  8

/* I2C 读写接口函数 */
fsp_err_t gt1151_write_reg(uint16_t addr, uint8_t data);
fsp_err_t gt1151_read_reg(uint16_t addr, uint8_t *p_data);
fsp_err_t gt1151_read_multi(uint16_t addr, uint8_t *p_data, uint8_t len);

/* GT1151 初始化和使能 */
fsp_err_t gt1151_init(void);
fsp_err_t gt1151_enable(void);

/* 触摸数据读取（供 LVGL 调用） */
fsp_err_t gt1151_read_touch(TouchCoordinate_t *coords, uint8_t *count, touch_event_t *event);

#endif /* __TOUCH_GT1151_H */
