#include "Vehicle/device/mpu6050.h"

#include <stddef.h>
#include <string.h>

#define MPU6050_REG_SMPLRT_DIV    (0x19U)
#define MPU6050_REG_CONFIG        (0x1AU)
#define MPU6050_REG_GYRO_CONFIG   (0x1BU)
#define MPU6050_REG_ACCEL_CONFIG  (0x1CU)
#define MPU6050_REG_ACCEL_XOUT_H  (0x3BU)
#define MPU6050_REG_PWR_MGMT_1    (0x6BU)
#define MPU6050_REG_WHO_AM_I      (0x75U)

static bool write_u8(mpu6050_t * device, uint8_t reg, uint8_t value)
{
    return device->io.write(device->io.context, device->address, reg, &value, 1U);
}

static int16_t be_i16(uint8_t const * data)
{
    return (int16_t) (((uint16_t) data[0] << 8U) | (uint16_t) data[1]);
}

bool mpu6050_init(mpu6050_t * device, vehicle_i2c_port_t io, uint8_t address)
{
    uint8_t who_am_i = 0U;

    if ((NULL == device) || (NULL == io.read) || (NULL == io.write) || (NULL == io.delay_ms))
    {
        return false;
    }

    memset(device, 0, sizeof(*device));
    device->io = io;
    device->address = address;

    if ((!io.read(io.context, address, MPU6050_REG_WHO_AM_I, &who_am_i, 1U)) ||
        ((who_am_i & 0x7EU) != 0x68U))
    {
        return false;
    }
    device->who_am_i = who_am_i;

    /* 100 Hz 采样、约 42 Hz DLPF、陀螺仪 ±250 dps、加速度计 ±2 g。 */
    if (!write_u8(device, MPU6050_REG_PWR_MGMT_1, 0x01U)) return false;
    io.delay_ms(io.context, 100U);
    if (!write_u8(device, MPU6050_REG_CONFIG, 0x03U)) return false;
    if (!write_u8(device, MPU6050_REG_SMPLRT_DIV, 0x09U)) return false;
    if (!write_u8(device, MPU6050_REG_GYRO_CONFIG, 0x00U)) return false;
    if (!write_u8(device, MPU6050_REG_ACCEL_CONFIG, 0x00U)) return false;

    device->initialized = true;
    return true;
}

bool mpu6050_read(mpu6050_t * device, mpu6050_sample_t * sample)
{
    uint8_t raw[14];

    if ((NULL == device) || (!device->initialized) || (NULL == sample) ||
        (!device->io.read(device->io.context,
                          device->address,
                          MPU6050_REG_ACCEL_XOUT_H,
                          raw,
                          sizeof(raw))))
    {
        return false;
    }

    sample->accel_x_g = (float) be_i16(&raw[0]) / 16384.0F;
    sample->accel_y_g = (float) be_i16(&raw[2]) / 16384.0F;
    sample->accel_z_g = (float) be_i16(&raw[4]) / 16384.0F;
    sample->temperature_c = ((float) be_i16(&raw[6]) / 340.0F) + 36.53F;
    sample->gyro_x_dps = ((float) be_i16(&raw[8]) / 131.0F) - device->gyro_bias_x_dps;
    sample->gyro_y_dps = ((float) be_i16(&raw[10]) / 131.0F) - device->gyro_bias_y_dps;
    sample->gyro_z_dps = ((float) be_i16(&raw[12]) / 131.0F) - device->gyro_bias_z_dps;
    return true;
}

bool mpu6050_calibrate_gyro(mpu6050_t * device,
                            uint16_t sample_count,
                            uint32_t sample_interval_ms)
{
    mpu6050_sample_t sample;
    float sum_x = 0.0F;
    float sum_y = 0.0F;
    float sum_z = 0.0F;

    if ((NULL == device) || (0U == sample_count)) return false;

    device->gyro_bias_x_dps = 0.0F;
    device->gyro_bias_y_dps = 0.0F;
    device->gyro_bias_z_dps = 0.0F;
    for (uint16_t i = 0U; i < sample_count; ++i)
    {
        if (!mpu6050_read(device, &sample)) return false;
        sum_x += sample.gyro_x_dps;
        sum_y += sample.gyro_y_dps;
        sum_z += sample.gyro_z_dps;
        device->io.delay_ms(device->io.context, sample_interval_ms);
    }

    device->gyro_bias_x_dps = sum_x / (float) sample_count;
    device->gyro_bias_y_dps = sum_y / (float) sample_count;
    device->gyro_bias_z_dps = sum_z / (float) sample_count;
    return true;
}
