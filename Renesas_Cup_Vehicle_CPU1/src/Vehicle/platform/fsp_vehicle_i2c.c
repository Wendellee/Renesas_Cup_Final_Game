#include "Vehicle/platform/fsp_vehicle_i2c.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <string.h>

#define VEHICLE_I2C_TIMEOUT_MS       (20U)
#define VEHICLE_I2C_WRITE_BUFFER_SIZE (16U)

static volatile i2c_master_event_t g_i2c_event = I2C_MASTER_EVENT_ABORTED;
static TaskHandle_t g_i2c_owner_task;

void imu_i2c0_callback(i2c_master_callback_args_t * args)
{
    if (NULL != args)
    {
        g_i2c_event = args->event;
        if (NULL != g_i2c_owner_task)
        {
            BaseType_t higher_priority_task_woken = pdFALSE;
            vTaskNotifyGiveFromISR(g_i2c_owner_task, &higher_priority_task_woken);
            portYIELD_FROM_ISR(higher_priority_task_woken);
        }
    }
}

static bool wait_event(i2c_master_event_t expected)
{
    if (0U == ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(VEHICLE_I2C_TIMEOUT_MS)))
    {
        (void) R_IIC_MASTER_Abort(&g_i2c_master0_ctrl);
        return false;
    }
    return g_i2c_event == expected;
}

static bool select_slave(uint8_t address)
{
    return FSP_SUCCESS == R_IIC_MASTER_SlaveAddressSet(&g_i2c_master0_ctrl,
                                                       address,
                                                       I2C_MASTER_ADDR_MODE_7BIT);
}

static bool port_write(void * context,
                       uint8_t address,
                       uint8_t register_address,
                       uint8_t const * data,
                       size_t length)
{
    uint8_t frame[VEHICLE_I2C_WRITE_BUFFER_SIZE];
    (void) context;

    if ((NULL == data) || (length > (sizeof(frame) - 1U)) || (!select_slave(address)))
    {
        return false;
    }

    frame[0] = register_address;
    memcpy(&frame[1], data, length);
    (void) ulTaskNotifyTake(pdTRUE, 0U);
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    if (FSP_SUCCESS != R_IIC_MASTER_Write(&g_i2c_master0_ctrl,
                                          frame,
                                          (uint32_t) length + 1U,
                                          false))
    {
        return false;
    }
    return wait_event(I2C_MASTER_EVENT_TX_COMPLETE);
}

static bool port_read(void * context,
                      uint8_t address,
                      uint8_t register_address,
                      uint8_t * data,
                      size_t length)
{
    (void) context;

    if ((NULL == data) || (0U == length) || (!select_slave(address)))
    {
        return false;
    }

    (void) ulTaskNotifyTake(pdTRUE, 0U);
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    if (FSP_SUCCESS != R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &register_address, 1U, true))
    {
        return false;
    }
    if (!wait_event(I2C_MASTER_EVENT_TX_COMPLETE)) return false;

    (void) ulTaskNotifyTake(pdTRUE, 0U);
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    if (FSP_SUCCESS != R_IIC_MASTER_Read(&g_i2c_master0_ctrl, data, (uint32_t) length, false))
    {
        return false;
    }
    return wait_event(I2C_MASTER_EVENT_RX_COMPLETE);
}

static void port_delay_ms(void * context, uint32_t delay_ms)
{
    (void) context;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

bool fsp_vehicle_i2c_init(vehicle_i2c_port_t * port)
{
    fsp_err_t err;

    if (NULL == port) return false;
    if (0U == g_i2c_master0_ctrl.open)
    {
        err = R_IIC_MASTER_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);
        if (FSP_SUCCESS != err) return false;
    }

    /* IIC0 只允许 Vehicle Thread 使用，回调可以准确唤醒唯一所有者。 */
    g_i2c_owner_task = xTaskGetCurrentTaskHandle();
    port->context = NULL;
    port->write = port_write;
    port->read = port_read;
    port->delay_ms = port_delay_ms;
    return true;
}
