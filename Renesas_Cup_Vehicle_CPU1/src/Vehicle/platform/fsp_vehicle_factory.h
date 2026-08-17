#ifndef VEHICLE_PLATFORM_FSP_VEHICLE_FACTORY_H_
#define VEHICLE_PLATFORM_FSP_VEHICLE_FACTORY_H_

#include "Vehicle/contracts/vehicle_ports.h"

/** 组装 CPU1 当前 FSP 外设，形成 application 层需要的依赖集合。 */
bool fsp_vehicle_dependencies_create(vehicle_dependencies_t * dependencies);

#endif /* VEHICLE_PLATFORM_FSP_VEHICLE_FACTORY_H_ */
