/*
 * touch_GT911.h
 */

#ifndef GUI_TOUCH_GT911_H_
#define GUI_TOUCH_GT911_H_

#include "hal_data.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t x;
    uint16_t y;
} TouchCoordinate_t;

typedef enum
{
    TOUCH_EVENT_NONE,
    TOUCH_EVENT_DOWN,
    TOUCH_EVENT_HOLD,
    TOUCH_EVENT_MOVE,
    TOUCH_EVENT_UP
} touch_event_t;

#define GT911_REG_COMMAND          0x8040
#define GT911_REG_CONFIG_VERSION   0x8047
#define GT911_REG_CONFIG_CHECKSUM  0x80FF
#define GT911_REG_CONFIG_FRESH     0x8100
#define GT911_REG_PRODUCT_ID       0x8140
#define GT911_REG_FW_VER_HIGH      0x8145
#define GT911_REG_READ_COORD_ADDR  0x814E
#define GT911_REG_POINT1_X_ADDR    0x814F

#define GT911_POINT_DATA_SIZE      8U
#define GT911_MAX_TOUCH_POINTS     5U

#define BUFFER_READY               (1U << 7)
#define NUM_TOUCH_POINTS_MASK      0x0FU

extern volatile bool g_gt911_irq_pending;
extern volatile fsp_err_t g_gt911_last_error;
extern volatile uint8_t g_gt911_product_id[4];
extern volatile uint8_t g_gt911_last_status;
extern volatile uint8_t g_gt911_last_count;
extern volatile uint8_t g_gt911_active_address;
extern volatile fsp_err_t g_gt911_try_5d_error;
extern volatile fsp_err_t g_gt911_try_14_error;
extern volatile uint16_t g_gt911_last_x;
extern volatile uint16_t g_gt911_last_y;
extern volatile uint32_t g_gt911_irq_count;
extern volatile uint32_t g_gt911_read_ok_count;
extern volatile uint32_t g_gt911_read_error_count;

fsp_err_t gt911_init(void);
fsp_err_t gt911_enable(void);
fsp_err_t gt911_read_touch(TouchCoordinate_t * coords, uint8_t * count, touch_event_t * event);

#endif /* GUI_TOUCH_GT911_H_ */
