/*
 * touch_GT911.c
 */

#include "touch_GT911.h"
#include "i2c_control.h"
#include <string.h>

#define GT911_I2C_ADDRESS_0X5D     0x5D
#define GT911_I2C_ADDRESS_0X14     0x14

volatile bool g_gt911_irq_pending = false;
volatile fsp_err_t g_gt911_last_error = FSP_SUCCESS;
volatile uint8_t g_gt911_product_id[4] = {0};
volatile uint8_t g_gt911_last_status = 0;
volatile uint8_t g_gt911_last_count = 0;
volatile uint8_t g_gt911_active_address = 0;
volatile fsp_err_t g_gt911_try_5d_error = FSP_SUCCESS;
volatile fsp_err_t g_gt911_try_14_error = FSP_SUCCESS;
volatile uint16_t g_gt911_last_x = 0;
volatile uint16_t g_gt911_last_y = 0;
volatile uint32_t g_gt911_irq_count = 0;
volatile uint32_t g_gt911_read_ok_count = 0;
volatile uint32_t g_gt911_read_error_count = 0;

static fsp_err_t gt911_read_reg(uint16_t addr, uint8_t * p_data);
static fsp_err_t gt911_write_reg(uint16_t addr, uint8_t data);
static fsp_err_t gt911_read_multi(uint16_t addr, uint8_t * p_data, uint8_t len);
static void gt911_reset(uint8_t i2c_addr);
static fsp_err_t gt911_try_init(uint8_t i2c_addr);

void touch_irq_callback(external_irq_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    g_gt911_irq_pending = true;
    g_gt911_irq_count++;
}

static fsp_err_t gt911_read_reg(uint16_t addr, uint8_t * p_data)
{
    return read_reg_16bit(addr, p_data);
}

static fsp_err_t gt911_write_reg(uint16_t addr, uint8_t data)
{
    return write_reg_16bit(addr, data);
}

static fsp_err_t gt911_read_multi(uint16_t addr, uint8_t * p_data, uint8_t len)
{
    fsp_err_t err = FSP_SUCCESS;

    for (uint8_t i = 0; i < len; i++)
    {
        err = gt911_read_reg((uint16_t) (addr + i), &p_data[i]);
        if (FSP_SUCCESS != err)
        {
            break;
        }
    }

    return err;
}

static void gt911_reset(uint8_t i2c_addr)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_RST, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinCfg(&g_ioport_ctrl,
                    TCH_INT,
                    (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW);

    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);

    if (GT911_I2C_ADDRESS_0X14 == i2c_addr)
    {
        R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_INT, BSP_IO_LEVEL_HIGH);
    }
    else
    {
        R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_INT, BSP_IO_LEVEL_LOW);
    }

    R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_MILLISECONDS);

    R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_RST, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinCfg(&g_ioport_ctrl,
                    TCH_INT,
                    (uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
}

static fsp_err_t gt911_try_init(uint8_t i2c_addr)
{
    fsp_err_t err;
    uint8_t product_id[4] = {0};

    g_gt911_active_address = 0;
    memset((void *) g_gt911_product_id, 0, sizeof(g_gt911_product_id));
    gt911_reset(i2c_addr);

    err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master_for_peripheral_ctrl,
                                       i2c_addr,
                                       I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS != err)
    {
        g_gt911_last_error = err;
        if (GT911_I2C_ADDRESS_0X5D == i2c_addr)
        {
            g_gt911_try_5d_error = err;
        }
        else
        {
            g_gt911_try_14_error = err;
        }
        return err;
    }

    err = gt911_read_multi(GT911_REG_PRODUCT_ID, product_id, sizeof(product_id));
    if (FSP_SUCCESS != err)
    {
        g_gt911_last_error = err;
        if (GT911_I2C_ADDRESS_0X5D == i2c_addr)
        {
            g_gt911_try_5d_error = err;
        }
        else
        {
            g_gt911_try_14_error = err;
        }
        return err;
    }

    for (uint8_t i = 0; i < sizeof(product_id); i++)
    {
        g_gt911_product_id[i] = product_id[i];
    }
    g_gt911_active_address = i2c_addr;

    err = gt911_write_reg(GT911_REG_COMMAND, 0x00U);
    g_gt911_last_error = err;
    if (GT911_I2C_ADDRESS_0X5D == i2c_addr)
    {
        g_gt911_try_5d_error = err;
    }
    else
    {
        g_gt911_try_14_error = err;
    }
    return err;
}

fsp_err_t gt911_init(void)
{
    fsp_err_t err;

    err = gt911_try_init(GT911_I2C_ADDRESS_0X5D);
    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    return gt911_try_init(GT911_I2C_ADDRESS_0X14);
}

fsp_err_t gt911_enable(void)
{
    fsp_err_t err = gt911_init();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /*
     * Touch reading is polling-based from LVGL. The external IRQ is useful, but it
     * must not block bring-up if the IRQ channel/pin is not ready yet.
     */
    err = R_ICU_ExternalIrqOpen(&g_external_irq19_ctrl, &g_external_irq19_cfg);
    if (FSP_SUCCESS == err)
    {
        (void) R_ICU_ExternalIrqEnable(&g_external_irq19_ctrl);
    }

    g_gt911_last_error = FSP_SUCCESS;
    return FSP_SUCCESS;
}

fsp_err_t gt911_read_touch(TouchCoordinate_t * coords, uint8_t * count, touch_event_t * event)
{
    fsp_err_t err;
    uint8_t status_reg = 0;
    uint8_t num_points;
    uint8_t read_data[GT911_POINT_DATA_SIZE];

    if ((NULL == coords) || (NULL == count) || (NULL == event))
    {
        g_gt911_last_error = FSP_ERR_ASSERTION;
        return FSP_ERR_ASSERTION;
    }

    *count = 0;
    *event = TOUCH_EVENT_NONE;

    err = gt911_read_reg(GT911_REG_READ_COORD_ADDR, &status_reg);
    if (FSP_SUCCESS != err)
    {
        g_gt911_last_error = err;
        g_gt911_read_error_count++;
        return err;
    }

    g_gt911_last_status = status_reg;

    if (0U == (status_reg & BUFFER_READY))
    {
        g_gt911_last_error = FSP_SUCCESS;
        return FSP_SUCCESS;
    }

    num_points = (uint8_t) (status_reg & NUM_TOUCH_POINTS_MASK);
    if (num_points > GT911_MAX_TOUCH_POINTS)
    {
        num_points = GT911_MAX_TOUCH_POINTS;
    }

    if (0U == num_points)
    {
        *event = TOUCH_EVENT_UP;
    }
    else
    {
        for (uint8_t i = 0; i < num_points; i++)
        {
            err = gt911_read_multi((uint16_t) (GT911_REG_POINT1_X_ADDR + (i * GT911_POINT_DATA_SIZE)),
                                  read_data,
                                  GT911_POINT_DATA_SIZE);
            if (FSP_SUCCESS != err)
            {
                *count = i;
                g_gt911_last_count = i;
                g_gt911_last_error = err;
                g_gt911_read_error_count++;
                return err;
            }

            coords[i].x = (uint16_t) ((read_data[2] << 8) | read_data[1]);
            coords[i].y = (uint16_t) ((read_data[4] << 8) | read_data[3]);
        }

        *count = num_points;
        *event = TOUCH_EVENT_DOWN;
        g_gt911_last_x = coords[0].x;
        g_gt911_last_y = coords[0].y;
    }

    g_gt911_last_count = *count;
    g_gt911_irq_pending = false;
    err = gt911_write_reg(GT911_REG_READ_COORD_ADDR, 0x00U);
    g_gt911_last_error = err;
    if (FSP_SUCCESS == err)
    {
        g_gt911_read_ok_count++;
    }
    else
    {
        g_gt911_read_error_count++;
    }

    return err;
}
