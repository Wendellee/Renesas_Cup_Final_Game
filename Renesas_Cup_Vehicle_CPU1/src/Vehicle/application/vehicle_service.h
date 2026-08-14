#ifndef VEHICLE_APPLICATION_VEHICLE_SERVICE_H_
#define VEHICLE_APPLICATION_VEHICLE_SERVICE_H_

#include "Vehicle/contracts/vehicle_ports.h"
#include "Vehicle/contracts/vehicle_types.h"

/**
 * @file vehicle_service.h
 * @brief 车辆用例编排入口；必须只由 Vehicle Thread 调用。
 *
 * NRF、IPC、RTT 等输入源不能直接操作 GPT，应先把命令交给 Vehicle Thread，
 * 再由该线程调用本服务，从而保证所有底盘硬件只有一个所有者。
 */

vehicle_result_t vehicle_service_init(vehicle_dependencies_t const * dependencies);
vehicle_result_t vehicle_service_step(float elapsed_seconds);

vehicle_result_t vehicle_service_manual_command(vehicle_manual_command_t command,
                                                uint8_t pwm_percent);
vehicle_result_t vehicle_service_automatic_start(void);
vehicle_result_t vehicle_service_automatic_speed_set(uint8_t pwm_percent);
vehicle_result_t vehicle_service_automatic_turn_time_set(uint32_t turn_time_ms);
vehicle_result_t vehicle_service_suction_set(bool enabled, uint8_t pwm_percent);
vehicle_result_t vehicle_service_mode_set(vehicle_mode_t mode);
void vehicle_service_emergency_stop(void);

void vehicle_service_status_get(vehicle_status_t * status);
uint8_t vehicle_service_automatic_speed_get(void);
uint32_t vehicle_service_automatic_turn_time_get(void);

#endif /* VEHICLE_APPLICATION_VEHICLE_SERVICE_H_ */
