#include "Vehicle/platform/fsp_vehicle_factory.h"

#include "Vehicle/platform/fsp_vehicle_actuators.h"
#include "Vehicle/platform/fsp_vehicle_i2c.h"

#include <stddef.h>
#include <string.h>

bool fsp_vehicle_dependencies_create(vehicle_dependencies_t * dependencies)
{
    if (NULL == dependencies) return false;
    memset(dependencies, 0, sizeof(*dependencies));

    if (!fsp_vehicle_i2c_init(&dependencies->imu_i2c)) return false;
    fsp_vehicle_actuator_port_get(&dependencies->actuators);
    return true;
}
