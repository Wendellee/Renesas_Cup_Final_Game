#ifndef VEHICLE_PLATFORM_FSP_VEHICLE_I2C_H_
#define VEHICLE_PLATFORM_FSP_VEHICLE_I2C_H_

#include "Vehicle/contracts/vehicle_ports.h"
#include "vehicle_thread.h"

typedef enum e_fsp_vehicle_i2c_error_stage
{
    FSP_VEHICLE_I2C_ERROR_STAGE_NONE = 0,
    FSP_VEHICLE_I2C_ERROR_STAGE_SELECT_SLAVE,
    FSP_VEHICLE_I2C_ERROR_STAGE_WRITE_START,
    FSP_VEHICLE_I2C_ERROR_STAGE_WRITE_WAIT,
    FSP_VEHICLE_I2C_ERROR_STAGE_READ_REGISTER_TX_START,
    FSP_VEHICLE_I2C_ERROR_STAGE_READ_REGISTER_TX_WAIT,
    FSP_VEHICLE_I2C_ERROR_STAGE_READ_DATA_START,
    FSP_VEHICLE_I2C_ERROR_STAGE_READ_DATA_WAIT,
} fsp_vehicle_i2c_error_stage_t;

typedef struct st_fsp_vehicle_i2c_error_snapshot
{
    fsp_vehicle_i2c_error_stage_t stage;
    fsp_err_t fsp_error;
    i2c_master_event_t expected_event;
    i2c_master_event_t actual_event;
    uint32_t error_count;
    uint32_t transfer_length;
    uint8_t slave_address;
    uint8_t register_address;
    bool timed_out;
} fsp_vehicle_i2c_error_snapshot_t;

/** 初始化 IIC0，并返回给 device 层使用的抽象端口。 */
bool fsp_vehicle_i2c_init(vehicle_i2c_port_t * port);

/** 获取最近一次 IIC0 失败现场；没有记录时返回 false。仅由 Vehicle Thread 调用。 */
bool fsp_vehicle_i2c_error_snapshot_get(fsp_vehicle_i2c_error_snapshot_t * snapshot);

/** FSP XML 中 g_i2c_master0 的 Callback 必须填写该函数名。 */
void imu_i2c0_callback(i2c_master_callback_args_t * args);

#endif /* VEHICLE_PLATFORM_FSP_VEHICLE_I2C_H_ */
