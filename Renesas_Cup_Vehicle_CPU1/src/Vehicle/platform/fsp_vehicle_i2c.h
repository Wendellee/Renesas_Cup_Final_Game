#ifndef VEHICLE_PLATFORM_FSP_VEHICLE_I2C_H_
#define VEHICLE_PLATFORM_FSP_VEHICLE_I2C_H_

#include "Vehicle/contracts/vehicle_ports.h"
#include "vehicle_thread.h"

/** 初始化 IIC0，并返回给 device 层使用的抽象端口。 */
bool fsp_vehicle_i2c_init(vehicle_i2c_port_t * port);

/** FSP XML 中 g_i2c_master0 的 Callback 必须填写该函数名。 */
void imu_i2c0_callback(i2c_master_callback_args_t * args);

#endif /* VEHICLE_PLATFORM_FSP_VEHICLE_I2C_H_ */
