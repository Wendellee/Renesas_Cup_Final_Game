#ifndef VEHICLE_PLATFORM_FSP_VEHICLE_ACTUATORS_H_
#define VEHICLE_PLATFORM_FSP_VEHICLE_ACTUATORS_H_

#include "Vehicle/contracts/vehicle_ports.h"

/** 返回由 GPT6/GPT7/GPT8 实现的车辆执行器端口。 */
void fsp_vehicle_actuator_port_get(vehicle_actuator_port_t * port);

#endif /* VEHICLE_PLATFORM_FSP_VEHICLE_ACTUATORS_H_ */
