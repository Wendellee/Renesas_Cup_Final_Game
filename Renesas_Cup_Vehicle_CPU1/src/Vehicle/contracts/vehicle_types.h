#ifndef VEHICLE_CONTRACTS_VEHICLE_TYPES_H_
#define VEHICLE_CONTRACTS_VEHICLE_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @file vehicle_types.h
 * @brief 车辆子系统跨层共享的数据契约。
 *
 * 本文件只定义数据，不包含 FSP、FreeRTOS 或具体设备头文件，因此任意上层都可安全引用。
 */

typedef enum e_vehicle_result
{
    VEHICLE_RESULT_OK = 0,
    VEHICLE_RESULT_INVALID_ARGUMENT,
    VEHICLE_RESULT_NOT_READY,
    VEHICLE_RESULT_IO_ERROR,
    VEHICLE_RESULT_TIMEOUT,
    VEHICLE_RESULT_SENSOR_ERROR,
} vehicle_result_t;

typedef enum e_vehicle_mode
{
    VEHICLE_MODE_MANUAL = 0,
    VEHICLE_MODE_AUTOMATIC,
} vehicle_mode_t;

typedef enum e_vehicle_manual_command
{
    VEHICLE_MANUAL_STOP = 0,
    VEHICLE_MANUAL_FORWARD,
    VEHICLE_MANUAL_REVERSE,
    VEHICLE_MANUAL_TURN_LEFT,
    VEHICLE_MANUAL_TURN_RIGHT,
} vehicle_manual_command_t;

typedef enum e_vehicle_auto_state
{
    VEHICLE_AUTO_IDLE = 0,
    VEHICLE_AUTO_FORWARD_1,
    VEHICLE_AUTO_TURN_RIGHT,
    VEHICLE_AUTO_FORWARD_2,
    VEHICLE_AUTO_COMPLETE,
    VEHICLE_AUTO_ERROR,
} vehicle_auto_state_t;

/** 供诊断线程、NRF 状态回传使用的只读快照。 */
typedef struct st_vehicle_status
{
    vehicle_mode_t mode;
    vehicle_auto_state_t auto_state;
    bool initialized;
    bool suction_ready;
    bool heading_enabled;
    float heading_deg;
    float gyro_z_dps;
    float left_duty;
    float right_duty;
    float suction_duty;
    vehicle_result_t last_error;
} vehicle_status_t;

#endif /* VEHICLE_CONTRACTS_VEHICLE_TYPES_H_ */
