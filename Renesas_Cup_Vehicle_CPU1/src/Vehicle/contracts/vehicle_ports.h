#ifndef VEHICLE_CONTRACTS_VEHICLE_PORTS_H_
#define VEHICLE_CONTRACTS_VEHICLE_PORTS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @file vehicle_ports.h
 * @brief 上层需要的硬件能力接口（端口），具体实现由 platform 层注入。
 *
 * 这是依赖倒置的核心：应用层依赖“能力”，而不是依赖 g_left_wheel、IIC0 等具体对象。
 */

typedef bool (*vehicle_i2c_mem_write_fn_t)(void * context,
                                           uint8_t device_address,
                                           uint8_t register_address,
                                           uint8_t const * data,
                                           size_t length);
typedef bool (*vehicle_i2c_mem_read_fn_t)(void * context,
                                          uint8_t device_address,
                                          uint8_t register_address,
                                          uint8_t * data,
                                          size_t length);
typedef void (*vehicle_delay_ms_fn_t)(void * context, uint32_t delay_ms);

typedef struct st_vehicle_i2c_port
{
    void * context;
    vehicle_i2c_mem_write_fn_t write;
    vehicle_i2c_mem_read_fn_t read;
    vehicle_delay_ms_fn_t delay_ms;
} vehicle_i2c_port_t;

/** duty 范围为 [-1, 1]；符号表示方向，绝对值表示 PWM 比例。 */
typedef bool (*vehicle_wheels_write_fn_t)(void * context, float left_duty, float right_duty);
typedef bool (*vehicle_suction_write_fn_t)(void * context, float duty);
typedef bool (*vehicle_actuator_init_fn_t)(void * context);

typedef struct st_vehicle_actuator_port
{
    void * context;
    vehicle_actuator_init_fn_t init;
    vehicle_wheels_write_fn_t write_wheels;
    vehicle_suction_write_fn_t write_suction;
} vehicle_actuator_port_t;

typedef struct st_vehicle_dependencies
{
    vehicle_i2c_port_t imu_i2c;
    vehicle_actuator_port_t actuators;
} vehicle_dependencies_t;

#endif /* VEHICLE_CONTRACTS_VEHICLE_PORTS_H_ */
