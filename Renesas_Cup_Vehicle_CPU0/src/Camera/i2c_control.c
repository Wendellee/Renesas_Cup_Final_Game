/*
 * i2c_control.c
 *
 *  Created on: 2026年8月10日
 *      Author: lingk
 */

#include "i2c_control.h"
#include "SEGGER_RTT/bsp_print.h"

#define I2C_TIMEOUT_UNIT_US    (10U)
#define I2C_TIMEOUT_COUNT      (UINT8_MAX)

static volatile i2c_master_event_t g_i2c_event_for_peripheral;

/*
 *[@name] i2c_master_wait_event
 *[@type] static function
 *[@usage] 等待IIC中断回调事件
 *[@argument] expected_event
 *[@return] FSP error code
 */
static fsp_err_t i2c_master_wait_event(const i2c_master_event_t expected_event)
{
    uint8_t timeout = I2C_TIMEOUT_COUNT;

    while (expected_event != g_i2c_event_for_peripheral)
    {
        if (I2C_MASTER_EVENT_ABORTED == g_i2c_event_for_peripheral)
        {
            g_printf("[CAM][IIC][ERR] Transfer aborted, expected event=%u.\r\n",
                     (unsigned int) expected_event);
            return FSP_ERR_TRANSFER_ABORTED;
        }

        timeout--;
        if (0U == timeout)
        {
            g_printf("[CAM][IIC][ERR] Transfer timeout, expected=%u, received=%u.\r\n",
                     (unsigned int) expected_event,
                     (unsigned int) g_i2c_event_for_peripheral);
            return FSP_ERR_TIMEOUT;
        }

        R_BSP_SoftwareDelay(I2C_TIMEOUT_UNIT_US, BSP_DELAY_UNITS_MICROSECONDS);
    }

    return FSP_SUCCESS;
}

/*
 *[@name] i2c_event_reset
 *[@type] static function
 *[@usage] 清除上一次IIC回调事件
 *[@argument] none
 *[@return] none
 */
static void i2c_event_reset(void)
{
    g_i2c_event_for_peripheral = (i2c_master_event_t) 0;
}

/*
 *[@name] i2c_control_init
 *[@type] function
 *[@usage] 初始化摄像头IIC主机
 *[@argument] none
 *[@return] FSP error code
 */
fsp_err_t i2c_control_init(void)
{
    return R_IIC_MASTER_Open(&g_i2c_master_for_peripheral_ctrl,
                             &g_i2c_master_for_peripheral_cfg);
}

/*
 *[@name] write_reg_8bit
 *[@type] function
 *[@usage] 写入8位地址寄存器
 *[@argument] address, data
 *[@return] FSP error code
 */
fsp_err_t write_reg_8bit(uint8_t address, uint8_t data)
{
    uint8_t i2c_buffer[2] = {address, data};
    i2c_event_reset();//事件清零

#if BSP_CFG_DCACHE_ENABLED
    /*
     *[@usage] CPU写入发送缓冲区后，DTC读取前将Cache数据写回SRAM
     */
    SCB_CleanDCache_by_Addr((uint32_t *) i2c_buffer,
                            (int32_t) sizeof(i2c_buffer));
#endif

    fsp_err_t err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl,
                                       i2c_buffer,
                                       sizeof(i2c_buffer),
                                       false);

    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return i2c_master_wait_event(I2C_MASTER_EVENT_TX_COMPLETE);
}

/*
 *[@name] read_reg_8bit
 *[@type] function
 *[@usage] 读取8位地址寄存器
 *[@argument] address, p_data
 *[@return] FSP error code
 */
fsp_err_t read_reg_8bit(uint8_t address, uint8_t * p_data)
{
    if (NULL == p_data)
    {
        return FSP_ERR_ASSERTION;
    }

    i2c_event_reset();

#if BSP_CFG_DCACHE_ENABLED
    /*
     *[@usage] CPU写入寄存器地址后，DTC读取前将Cache数据写回SRAM
     */
    SCB_CleanDCache_by_Addr((uint32_t *) &address,
                            (int32_t) sizeof(address));
#endif

    fsp_err_t err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl,
                                       &address,
                                       1U,
                                       true);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = i2c_master_wait_event(I2C_MASTER_EVENT_TX_COMPLETE);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    i2c_event_reset();
    err = R_IIC_MASTER_Read(&g_i2c_master_for_peripheral_ctrl, p_data, 1U, false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return i2c_master_wait_event(I2C_MASTER_EVENT_RX_COMPLETE);
}

/*
 *[@name] write_reg_16bit
 *[@type] function
 *[@usage] 写入16位地址寄存器
 *[@argument] address, data
 *[@return] FSP error code
 */
fsp_err_t write_reg_16bit(uint16_t address, uint8_t data)
{
    uint8_t i2c_buffer[3] =
    {
        (uint8_t) (address >> 8),
        (uint8_t) address,
        data
    };
    i2c_event_reset();

#if BSP_CFG_DCACHE_ENABLED
    /*
     *[@usage] CPU写入发送缓冲区后，DTC读取前将Cache数据写回SRAM
     */
    SCB_CleanDCache_by_Addr((uint32_t *) i2c_buffer,
                            (int32_t) sizeof(i2c_buffer));
#endif

    fsp_err_t err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl,
                                       i2c_buffer,
                                       sizeof(i2c_buffer),
                                       false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return i2c_master_wait_event(I2C_MASTER_EVENT_TX_COMPLETE);
}

/*
 *[@name] read_reg_16bit
 *[@type] function
 *[@usage] 读取16位地址寄存器
 *[@argument] address, p_data
 *[@return] FSP error code
 */
fsp_err_t read_reg_16bit(uint16_t address, uint8_t * p_data)
{
    if (NULL == p_data)
    {
        return FSP_ERR_ASSERTION;
    }

    uint8_t i2c_buffer[2] =
    {
        (uint8_t) (address >> 8),
        (uint8_t) address
    };
    i2c_event_reset();

#if BSP_CFG_DCACHE_ENABLED
    /*
     *[@usage] CPU写入寄存器地址后，DTC读取前将Cache数据写回SRAM
     */
    SCB_CleanDCache_by_Addr((uint32_t *) i2c_buffer,
                            (int32_t) sizeof(i2c_buffer));
#endif

    fsp_err_t err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl,
                                       i2c_buffer,
                                       sizeof(i2c_buffer),
                                       true);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = i2c_master_wait_event(I2C_MASTER_EVENT_TX_COMPLETE);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    i2c_event_reset();
    err = R_IIC_MASTER_Read(&g_i2c_master_for_peripheral_ctrl, p_data, 1U, false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return i2c_master_wait_event(I2C_MASTER_EVENT_RX_COMPLETE);
}

/*
 *[@name] g_i2c_master_for_peripheral_callback
 *[@type] function
 *[@usage] IIC中断回调，仅记录传输事件
 *[@argument] p_args
 *[@return] none
 */
void g_i2c_master_for_peripheral_callback(i2c_master_callback_args_t * p_args)
{
    if (NULL != p_args)
    {
        g_i2c_event_for_peripheral = p_args->event;
    }
}
