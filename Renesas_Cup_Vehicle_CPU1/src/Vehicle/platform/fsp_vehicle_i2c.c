#include "Vehicle/platform/fsp_vehicle_i2c.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <string.h>

#define VEHICLE_I2C_TIMEOUT_MS       (20U)
#define VEHICLE_I2C_WRITE_BUFFER_SIZE (16U)

static volatile i2c_master_event_t g_i2c_event = I2C_MASTER_EVENT_ABORTED;
static TaskHandle_t g_i2c_owner_task;
static fsp_vehicle_i2c_error_snapshot_t g_i2c_error_snapshot;

static void error_snapshot_set(fsp_vehicle_i2c_error_stage_t stage,
                               fsp_err_t fsp_error,
                               i2c_master_event_t expected_event,
                               i2c_master_event_t actual_event,
                               bool timed_out,
                               uint8_t slave_address,
                               uint8_t register_address,
                               size_t transfer_length)
{
    g_i2c_error_snapshot.stage = stage;
    g_i2c_error_snapshot.fsp_error = fsp_error;
    g_i2c_error_snapshot.expected_event = expected_event;
    g_i2c_error_snapshot.actual_event = actual_event;
    g_i2c_error_snapshot.timed_out = timed_out;
    g_i2c_error_snapshot.slave_address = slave_address;
    g_i2c_error_snapshot.register_address = register_address;
    g_i2c_error_snapshot.transfer_length = (uint32_t) transfer_length;
    ++g_i2c_error_snapshot.error_count;
}

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

static bool wait_event(i2c_master_event_t expected,
                       fsp_vehicle_i2c_error_stage_t stage,
                       uint8_t slave_address,
                       uint8_t register_address,
                       size_t transfer_length)
{
    if (0U == ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(VEHICLE_I2C_TIMEOUT_MS)))
    {
        /* Abort 可能触发回调并覆盖 g_i2c_event，必须先冻结超时现场。 */
        error_snapshot_set(stage,
                           FSP_SUCCESS,
                           expected,
                           g_i2c_event,
                           true,
                           slave_address,
                           register_address,
                           transfer_length);
        (void) R_IIC_MASTER_Abort(&g_i2c_master0_ctrl);
        return false;
    }
    if (g_i2c_event != expected)
    {
        error_snapshot_set(stage,
                           FSP_SUCCESS,
                           expected,
                           g_i2c_event,
                           false,
                           slave_address,
                           register_address,
                           transfer_length);
        return false;
    }
    return true;
}

static bool select_slave(uint8_t address, uint8_t register_address, size_t transfer_length)
{
    fsp_err_t const err = R_IIC_MASTER_SlaveAddressSet(&g_i2c_master0_ctrl,
                                                       address,
                                                       I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS != err)
    {
        error_snapshot_set(FSP_VEHICLE_I2C_ERROR_STAGE_SELECT_SLAVE,
                           err,
                           I2C_MASTER_EVENT_ABORTED,
                           g_i2c_event,
                           false,
                           address,
                           register_address,
                           transfer_length);
        return false;
    }
    return true;
}

static bool port_write(void * context,
                       uint8_t address,
                       uint8_t register_address,
                       uint8_t const * data,
                       size_t length)
{
    uint8_t frame[VEHICLE_I2C_WRITE_BUFFER_SIZE];
    (void) context;

    if ((NULL == data) || (length > (sizeof(frame) - 1U)))
    {
        return false;
    }
    if (!select_slave(address, register_address, length + 1U)) return false;

    frame[0] = register_address;
    memcpy(&frame[1], data, length);
    (void) ulTaskNotifyTake(pdTRUE, 0U);
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    fsp_err_t const err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl,
                                             frame,
                                             (uint32_t) length + 1U,
                                             false);
    if (FSP_SUCCESS != err)
    {
        error_snapshot_set(FSP_VEHICLE_I2C_ERROR_STAGE_WRITE_START,
                           err,
                           I2C_MASTER_EVENT_TX_COMPLETE,
                           g_i2c_event,
                           false,
                           address,
                           register_address,
                           length + 1U);
        return false;
    }
    return wait_event(I2C_MASTER_EVENT_TX_COMPLETE,
                      FSP_VEHICLE_I2C_ERROR_STAGE_WRITE_WAIT,
                      address,
                      register_address,
                      length + 1U);
}

static bool port_read(void * context,
                      uint8_t address,
                      uint8_t register_address,
                      uint8_t * data,
                      size_t length)
{
    (void) context;

    if ((NULL == data) || (0U == length))
    {
        return false;
    }
    if (!select_slave(address, register_address, length)) return false;

    (void) ulTaskNotifyTake(pdTRUE, 0U);
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    fsp_err_t err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &register_address, 1U, true);
    if (FSP_SUCCESS != err)
    {
        error_snapshot_set(FSP_VEHICLE_I2C_ERROR_STAGE_READ_REGISTER_TX_START,
                           err,
                           I2C_MASTER_EVENT_TX_COMPLETE,
                           g_i2c_event,
                           false,
                           address,
                           register_address,
                           1U);
        return false;
    }
    if (!wait_event(I2C_MASTER_EVENT_TX_COMPLETE,
                    FSP_VEHICLE_I2C_ERROR_STAGE_READ_REGISTER_TX_WAIT,
                    address,
                    register_address,
                    1U)) return false;

    (void) ulTaskNotifyTake(pdTRUE, 0U);
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, data, (uint32_t) length, false);
    if (FSP_SUCCESS != err)
    {
        error_snapshot_set(FSP_VEHICLE_I2C_ERROR_STAGE_READ_DATA_START,
                           err,
                           I2C_MASTER_EVENT_RX_COMPLETE,
                           g_i2c_event,
                           false,
                           address,
                           register_address,
                           length);
        return false;
    }
    return wait_event(I2C_MASTER_EVENT_RX_COMPLETE,
                      FSP_VEHICLE_I2C_ERROR_STAGE_READ_DATA_WAIT,
                      address,
                      register_address,
                      length);
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

bool fsp_vehicle_i2c_error_snapshot_get(fsp_vehicle_i2c_error_snapshot_t * snapshot)
{
    if ((NULL == snapshot) ||
        (FSP_VEHICLE_I2C_ERROR_STAGE_NONE == g_i2c_error_snapshot.stage))
    {
        return false;
    }

    *snapshot = g_i2c_error_snapshot;
    return true;
}
