#include "Vehicle/domain/heading_controller.h"

#include <stddef.h>
#include <string.h>

static float clamp_float(float value, float minimum, float maximum)
{
    return (value < minimum) ? minimum : ((value > maximum) ? maximum : value);
}

static float wrap_degrees(float angle)
{
    while (angle > 180.0F) angle -= 360.0F;
    while (angle < -180.0F) angle += 360.0F;
    return angle;
}

bool heading_controller_init(heading_controller_t * controller,
                             heading_control_cfg_t const * cfg)
{
    if ((NULL == controller) || (NULL == cfg) ||
        (cfg->output_limit <= 0.0F) || (cfg->correction_limit < 0.0F))
    {
        return false;
    }

    memset(controller, 0, sizeof(*controller));
    controller->cfg = *cfg;
    return true;
}

void heading_controller_start(heading_controller_t * controller, float base_duty)
{
    if (NULL == controller) return;
    controller->base_duty = clamp_float(base_duty,
                                        -controller->cfg.output_limit,
                                        controller->cfg.output_limit);
    controller->target_heading_deg = controller->heading_deg;
    controller->integral = 0.0F;
    controller->enabled = true;
}

void heading_controller_stop(heading_controller_t * controller)
{
    if (NULL == controller) return;
    controller->enabled = false;
    controller->integral = 0.0F;
}

void heading_controller_set_base_duty(heading_controller_t * controller, float base_duty)
{
    if (NULL != controller)
    {
        controller->base_duty = clamp_float(base_duty,
                                            -controller->cfg.output_limit,
                                            controller->cfg.output_limit);
    }
}

bool heading_controller_update(heading_controller_t * controller,
                               float gyro_z_dps,
                               float dt_s,
                               heading_output_t * output)
{
    float absolute_rate;
    float error;
    float candidate_integral;
    float unsaturated;
    float correction;

    if ((NULL == controller) || (NULL == output) || (dt_s <= 0.0F) || (dt_s > 0.1F))
    {
        return false;
    }

    absolute_rate = (gyro_z_dps < 0.0F) ? -gyro_z_dps : gyro_z_dps;
    if (absolute_rate < controller->cfg.gyro_deadband_dps) gyro_z_dps = 0.0F;
    controller->heading_deg = wrap_degrees(controller->heading_deg + gyro_z_dps * dt_s);

    if (!controller->enabled)
    {
        output->left_duty = 0.0F;
        output->right_duty = 0.0F;
        return true;
    }

    error = wrap_degrees(controller->target_heading_deg - controller->heading_deg);
    candidate_integral = clamp_float(controller->integral + (error * dt_s),
                                     -controller->cfg.integral_limit,
                                     controller->cfg.integral_limit);
    unsaturated = (controller->cfg.kp * error) +
                  (controller->cfg.ki * candidate_integral) -
                  (controller->cfg.kd_rate * gyro_z_dps);
    correction = clamp_float(unsaturated,
                             -controller->cfg.correction_limit,
                             controller->cfg.correction_limit);

    /* 抗积分饱和：只有未饱和或误差推动输出回到线性区时才接受本次积分。 */
    if (((unsaturated >= -controller->cfg.correction_limit) &&
         (unsaturated <= controller->cfg.correction_limit)) ||
        ((unsaturated > correction) && (error < 0.0F)) ||
        ((unsaturated < correction) && (error > 0.0F)))
    {
        controller->integral = candidate_integral;
    }

    output->left_duty = clamp_float(controller->base_duty - correction,
                                    -controller->cfg.output_limit,
                                    controller->cfg.output_limit);
    output->right_duty = clamp_float(controller->base_duty + correction,
                                     -controller->cfg.output_limit,
                                     controller->cfg.output_limit);
    return true;
}

bool heading_controller_is_enabled(heading_controller_t const * controller)
{
    return (NULL != controller) && controller->enabled;
}
