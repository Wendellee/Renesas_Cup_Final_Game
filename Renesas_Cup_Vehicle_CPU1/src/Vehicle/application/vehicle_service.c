#include "Vehicle/application/vehicle_service.h"

#include "Vehicle/device/mpu6050.h"
#include "Vehicle/domain/heading_controller.h"

#include <stddef.h>
#include <string.h>

#define VEHICLE_SUCTION_STARTUP_TIME_MS       (2000U)
#define VEHICLE_AUTO_STRAIGHT_TIME_MS         (5000U)
#define VEHICLE_AUTO_STRAIGHT_PERCENT         (90U)
#define VEHICLE_AUTO_TURN_PERCENT             (90U)
#define VEHICLE_AUTO_RIGHT_TURN_TIME_MS        (750U)
#define VEHICLE_MPU6050_ADDRESS               (0x68U)
#define VEHICLE_GYRO_CALIBRATION_SAMPLES      (500U)
#define VEHICLE_GYRO_CALIBRATION_INTERVAL_MS  (2U)

typedef struct st_vehicle_service
{
    vehicle_dependencies_t dependencies;
    mpu6050_t imu;
    heading_controller_t heading;
    vehicle_status_t status;
    uint32_t suction_startup_elapsed_ms;
    uint32_t auto_elapsed_ms;
    uint32_t auto_turn_time_ms;
    uint8_t auto_straight_percent;
    uint8_t auto_turn_percent;
} vehicle_service_t;

static vehicle_service_t g_vehicle;

static float percent_to_duty(uint8_t percent)
{
    return (float) percent / 100.0F;
}

static vehicle_result_t set_error(vehicle_result_t result)
{
    g_vehicle.status.last_error = result;
    return result;
}

static vehicle_result_t write_wheels(float left, float right)
{
    if (!g_vehicle.dependencies.actuators.write_wheels(
            g_vehicle.dependencies.actuators.context, left, right))
    {
        g_vehicle.status.left_duty = 0.0F;
        g_vehicle.status.right_duty = 0.0F;
        return set_error(VEHICLE_RESULT_IO_ERROR);
    }
    g_vehicle.status.left_duty = left;
    g_vehicle.status.right_duty = right;
    return VEHICLE_RESULT_OK;
}

static void wheels_stop(void)
{
    heading_controller_stop(&g_vehicle.heading);
    g_vehicle.status.heading_enabled = false;
    (void) write_wheels(0.0F, 0.0F);
}

static vehicle_result_t straight_start(bool reverse, uint8_t pwm_percent)
{
    float duty;

    if (pwm_percent > 100U) return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    duty = percent_to_duty(pwm_percent);
    if (reverse) duty = -duty;
#if VEHICLE_MPU6050_ENABLE
    heading_controller_start(&g_vehicle.heading, duty);
    g_vehicle.status.heading_enabled = true;
#else
    heading_controller_stop(&g_vehicle.heading);
    g_vehicle.status.heading_enabled = false;
#endif
    return write_wheels(duty, duty);
}

static vehicle_result_t require_motion_ready(void)
{
    if ((!g_vehicle.status.initialized) || (!g_vehicle.status.suction_ready))
    {
        return set_error(VEHICLE_RESULT_NOT_READY);
    }
    return VEHICLE_RESULT_OK;
}

vehicle_result_t vehicle_service_init(vehicle_dependencies_t const * dependencies)
{
    static heading_control_cfg_t const heading_cfg =
    {
        .kp = 0.020F,
        .ki = 0.003F,
        .kd_rate = 0.004F,
        .integral_limit = 20.0F,
        .correction_limit = 0.35F,
        .output_limit = 1.0F,
        .gyro_deadband_dps = 0.10F,
    };

    if ((NULL == dependencies) ||
        (NULL == dependencies->actuators.init) ||
        (NULL == dependencies->actuators.write_wheels) ||
        (NULL == dependencies->actuators.write_suction))
    {
        return VEHICLE_RESULT_INVALID_ARGUMENT;
    }

    memset(&g_vehicle, 0, sizeof(g_vehicle));
    g_vehicle.dependencies = *dependencies;
    g_vehicle.auto_turn_time_ms = VEHICLE_AUTO_RIGHT_TURN_TIME_MS;
    g_vehicle.auto_straight_percent = VEHICLE_AUTO_STRAIGHT_PERCENT;
    g_vehicle.auto_turn_percent = VEHICLE_AUTO_TURN_PERCENT;
    g_vehicle.status.mode = VEHICLE_MODE_MANUAL;
    g_vehicle.status.auto_state = VEHICLE_AUTO_IDLE;

    if (!g_vehicle.dependencies.actuators.init(g_vehicle.dependencies.actuators.context))
    {
        return set_error(VEHICLE_RESULT_IO_ERROR);
    }
#if VEHICLE_MPU6050_ENABLE
    if (!mpu6050_init(&g_vehicle.imu,
                      g_vehicle.dependencies.imu_i2c,
                      VEHICLE_MPU6050_ADDRESS))
    {
        return set_error(VEHICLE_RESULT_SENSOR_ERROR);
    }

    /* 校准必须在风机和车轮启动前进行，避免机械振动污染陀螺仪零偏。 */
    if (!mpu6050_calibrate_gyro(&g_vehicle.imu,
                                VEHICLE_GYRO_CALIBRATION_SAMPLES,
                                VEHICLE_GYRO_CALIBRATION_INTERVAL_MS))
    {
        return set_error(VEHICLE_RESULT_SENSOR_ERROR);
    }
#endif
    if (!heading_controller_init(&g_vehicle.heading, &heading_cfg))
    {
        return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    }

    wheels_stop();
    if (!g_vehicle.dependencies.actuators.write_suction(
            g_vehicle.dependencies.actuators.context,
            0.0F))
    {
        return set_error(VEHICLE_RESULT_IO_ERROR);
    }

    /* 启动门禁释放前保持车轮和吸附输出为零，业务命令放行后再显式启动吸附。 */
    g_vehicle.status.suction_duty = 0.0F;
    g_vehicle.status.suction_ready = false;
    g_vehicle.status.initialized = true;
    return set_error(VEHICLE_RESULT_OK);
}

vehicle_result_t vehicle_service_step(float elapsed_seconds)
{
    mpu6050_sample_t sample;
    heading_output_t output;
    uint32_t elapsed_ms;
    vehicle_result_t result = VEHICLE_RESULT_OK;

    if ((!g_vehicle.status.initialized) || (elapsed_seconds <= 0.0F) || (elapsed_seconds > 0.1F))
    {
        return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    }
    elapsed_ms = (uint32_t) ((elapsed_seconds * 1000.0F) + 0.5F);

    if ((!g_vehicle.status.suction_ready) && (g_vehicle.status.suction_duty > 0.0F))
    {
        g_vehicle.suction_startup_elapsed_ms += elapsed_ms;
        if (g_vehicle.suction_startup_elapsed_ms >= VEHICLE_SUCTION_STARTUP_TIME_MS)
        {
            g_vehicle.status.suction_ready = true;
        }
    }

    if (heading_controller_is_enabled(&g_vehicle.heading))
    {
        if (!mpu6050_read(&g_vehicle.imu, &sample))
        {
            wheels_stop();
            return set_error(VEHICLE_RESULT_SENSOR_ERROR);
        }
        g_vehicle.status.gyro_z_dps = sample.gyro_z_dps;
        if ((!heading_controller_update(&g_vehicle.heading,
                                        sample.gyro_z_dps,
                                        elapsed_seconds,
                                        &output)) ||
            (VEHICLE_RESULT_OK != write_wheels(output.left_duty, output.right_duty)))
        {
            wheels_stop();
            return set_error(VEHICLE_RESULT_IO_ERROR);
        }
        g_vehicle.status.heading_deg = g_vehicle.heading.heading_deg;
    }

    if (VEHICLE_MODE_AUTOMATIC != g_vehicle.status.mode) return VEHICLE_RESULT_OK;
    g_vehicle.auto_elapsed_ms += elapsed_ms;

    switch (g_vehicle.status.auto_state)
    {
        case VEHICLE_AUTO_FORWARD_1:
            if (g_vehicle.auto_elapsed_ms >= VEHICLE_AUTO_STRAIGHT_TIME_MS)
            {
                wheels_stop();
                g_vehicle.auto_elapsed_ms = 0U;
                g_vehicle.status.auto_state = VEHICLE_AUTO_TURN_RIGHT;
                result = write_wheels(percent_to_duty(g_vehicle.auto_turn_percent),
                                      -percent_to_duty(g_vehicle.auto_turn_percent));
            }
            break;

        case VEHICLE_AUTO_TURN_RIGHT:
            if (g_vehicle.auto_elapsed_ms >= g_vehicle.auto_turn_time_ms)
            {
                wheels_stop();
                g_vehicle.auto_elapsed_ms = 0U;
                g_vehicle.status.auto_state = VEHICLE_AUTO_FORWARD_2;
                result = straight_start(false, g_vehicle.auto_straight_percent);
            }
            break;

        case VEHICLE_AUTO_FORWARD_2:
            if (g_vehicle.auto_elapsed_ms >= VEHICLE_AUTO_STRAIGHT_TIME_MS)
            {
                wheels_stop();
                g_vehicle.status.auto_state = VEHICLE_AUTO_COMPLETE;
            }
            break;

        case VEHICLE_AUTO_IDLE:
        case VEHICLE_AUTO_COMPLETE:
        case VEHICLE_AUTO_ERROR:
        default:
            break;
    }

    if (VEHICLE_RESULT_OK != result)
    {
        g_vehicle.status.auto_state = VEHICLE_AUTO_ERROR;
        wheels_stop();
    }
    return set_error(result);
}

vehicle_result_t vehicle_service_manual_command(vehicle_manual_command_t command,
                                                uint8_t pwm_percent)
{
    float duty;
    vehicle_result_t ready;

    /* 停车命令在未完成吸附、传感器异常等任何状态下都必须有效。 */
    if (VEHICLE_MANUAL_STOP == command)
    {
        wheels_stop();
        g_vehicle.status.mode = VEHICLE_MODE_MANUAL;
        g_vehicle.status.auto_state = VEHICLE_AUTO_IDLE;
        return set_error(VEHICLE_RESULT_OK);
    }

    ready = require_motion_ready();
    if (VEHICLE_RESULT_OK != ready) return ready;
    if (pwm_percent > 100U) return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);

    wheels_stop();
    g_vehicle.status.mode = VEHICLE_MODE_MANUAL;
    g_vehicle.status.auto_state = VEHICLE_AUTO_IDLE;
    duty = percent_to_duty(pwm_percent);

    switch (command)
    {
        case VEHICLE_MANUAL_FORWARD:    return set_error(straight_start(false, pwm_percent));
        case VEHICLE_MANUAL_REVERSE:    return set_error(straight_start(true, pwm_percent));
        case VEHICLE_MANUAL_TURN_LEFT:  return set_error(write_wheels(-duty, duty));
        case VEHICLE_MANUAL_TURN_RIGHT: return set_error(write_wheels(duty, -duty));
        case VEHICLE_MANUAL_STOP:       return set_error(VEHICLE_RESULT_OK);
        default:                        return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    }
}

vehicle_result_t vehicle_service_navigation_command(vehicle_manual_command_t command,
                                                     uint8_t pwm_percent)
{
    float duty;
    vehicle_result_t ready;

    if (VEHICLE_MODE_AUTOMATIC != g_vehicle.status.mode)
    {
        return set_error(VEHICLE_RESULT_NOT_READY);
    }
    if (VEHICLE_MANUAL_STOP == command)
    {
        wheels_stop();
        g_vehicle.status.auto_state = VEHICLE_AUTO_IDLE;
        return set_error(VEHICLE_RESULT_OK);
    }

    ready = require_motion_ready();
    if (VEHICLE_RESULT_OK != ready) return ready;
    if (pwm_percent > 100U) return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);

    wheels_stop();
    g_vehicle.status.auto_state = VEHICLE_AUTO_IDLE;
    duty = percent_to_duty(pwm_percent);

    switch (command)
    {
        case VEHICLE_MANUAL_FORWARD:    return set_error(straight_start(false, pwm_percent));
        case VEHICLE_MANUAL_REVERSE:    return set_error(straight_start(true, pwm_percent));
        case VEHICLE_MANUAL_TURN_LEFT:  return set_error(write_wheels(-duty, duty));
        case VEHICLE_MANUAL_TURN_RIGHT: return set_error(write_wheels(duty, -duty));
        case VEHICLE_MANUAL_STOP:       return set_error(VEHICLE_RESULT_OK);
        default:                        return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    }
}

vehicle_result_t vehicle_service_automatic_start(void)
{
    vehicle_result_t ready = require_motion_ready();
    if (VEHICLE_RESULT_OK != ready) return ready;
    wheels_stop();
    g_vehicle.status.mode = VEHICLE_MODE_AUTOMATIC;
    g_vehicle.status.auto_state = VEHICLE_AUTO_FORWARD_1;
    g_vehicle.auto_elapsed_ms = 0U;
    return set_error(straight_start(false, g_vehicle.auto_straight_percent));
}

vehicle_result_t vehicle_service_automatic_speed_set(uint8_t pwm_percent)
{
    if (pwm_percent > 100U) return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    g_vehicle.auto_straight_percent = pwm_percent;
    g_vehicle.auto_turn_percent = pwm_percent;
    heading_controller_set_base_duty(&g_vehicle.heading, percent_to_duty(pwm_percent));
    return set_error(VEHICLE_RESULT_OK);
}

vehicle_result_t vehicle_service_automatic_turn_time_set(uint32_t turn_time_ms)
{
    if ((turn_time_ms < 50U) || (turn_time_ms > 10000U))
    {
        return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    }
    g_vehicle.auto_turn_time_ms = turn_time_ms;
    return set_error(VEHICLE_RESULT_OK);
}

vehicle_result_t vehicle_service_suction_set(bool enabled, uint8_t pwm_percent)
{
    float duty;
    if ((!g_vehicle.status.initialized) || (pwm_percent > 100U))
    {
        return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    }

    duty = enabled ? percent_to_duty(pwm_percent) : 0.0F;
    if (!g_vehicle.dependencies.actuators.write_suction(
            g_vehicle.dependencies.actuators.context, duty))
    {
        return set_error(VEHICLE_RESULT_IO_ERROR);
    }
    g_vehicle.status.suction_duty = duty;
    /* 关闭吸附后车辆不可运动；开启后由 step() 等待建立吸附力再置 ready。 */
    g_vehicle.status.suction_ready = false;
    g_vehicle.suction_startup_elapsed_ms = 0U;
    return set_error(VEHICLE_RESULT_OK);
}

vehicle_result_t vehicle_service_mode_set(vehicle_mode_t mode)
{
    if ((VEHICLE_MODE_MANUAL != mode) && (VEHICLE_MODE_AUTOMATIC != mode))
    {
        return set_error(VEHICLE_RESULT_INVALID_ARGUMENT);
    }
    wheels_stop();
    g_vehicle.status.mode = mode;
    g_vehicle.status.auto_state = VEHICLE_AUTO_IDLE;
    g_vehicle.auto_elapsed_ms = 0U;
    return set_error(VEHICLE_RESULT_OK);
}

void vehicle_service_emergency_stop(void)
{
    wheels_stop();
    if (NULL != g_vehicle.dependencies.actuators.write_suction)
    {
        (void) g_vehicle.dependencies.actuators.write_suction(
            g_vehicle.dependencies.actuators.context, 0.0F);
    }
    g_vehicle.status.suction_duty = 0.0F;
    g_vehicle.status.suction_ready = false;
    g_vehicle.status.auto_state = VEHICLE_AUTO_ERROR;
}

void vehicle_service_status_get(vehicle_status_t * status)
{
    if (NULL != status) *status = g_vehicle.status;
}

uint8_t vehicle_service_automatic_speed_get(void)
{
    return g_vehicle.auto_straight_percent;
}

uint32_t vehicle_service_automatic_turn_time_get(void)
{
    return g_vehicle.auto_turn_time_ms;
}
