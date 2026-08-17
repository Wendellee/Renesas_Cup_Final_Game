#ifndef VEHICLE_DEVICE_MPU6050_H_
#define VEHICLE_DEVICE_MPU6050_H_

#include "Vehicle/contracts/vehicle_ports.h"

/**
 * @file mpu6050.h
 * @brief MPU6050 设备模型；只依赖 I2C 抽象端口，不依赖 FSP。
 */

typedef struct st_mpu6050_sample
{
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float temperature_c;
} mpu6050_sample_t;

typedef struct st_mpu6050
{
    vehicle_i2c_port_t io;
    uint8_t address;
    uint8_t who_am_i;
    float gyro_bias_x_dps;
    float gyro_bias_y_dps;
    float gyro_bias_z_dps;
    bool initialized;
} mpu6050_t;

bool mpu6050_init(mpu6050_t * device, vehicle_i2c_port_t io, uint8_t address);
bool mpu6050_read(mpu6050_t * device, mpu6050_sample_t * sample);
bool mpu6050_calibrate_gyro(mpu6050_t * device,
                            uint16_t sample_count,
                            uint32_t sample_interval_ms);

#endif /* VEHICLE_DEVICE_MPU6050_H_ */
