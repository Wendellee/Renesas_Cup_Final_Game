#ifndef VEHICLE_DOMAIN_HEADING_CONTROLLER_H_
#define VEHICLE_DOMAIN_HEADING_CONTROLLER_H_

#include <stdbool.h>

/**
 * @file heading_controller.h
 * @brief 与硬件无关的航向控制算法。
 */

typedef struct st_heading_control_cfg
{
    float kp;
    float ki;
    float kd_rate;
    float integral_limit;
    float correction_limit;
    float output_limit;
    float gyro_deadband_dps;
} heading_control_cfg_t;

typedef struct st_heading_controller
{
    heading_control_cfg_t cfg;
    float target_heading_deg;
    float heading_deg;
    float integral;
    float base_duty;
    bool enabled;
} heading_controller_t;

typedef struct st_heading_output
{
    float left_duty;
    float right_duty;
} heading_output_t;

bool heading_controller_init(heading_controller_t * controller,
                             heading_control_cfg_t const * cfg);
void heading_controller_start(heading_controller_t * controller, float base_duty);
void heading_controller_stop(heading_controller_t * controller);
void heading_controller_set_base_duty(heading_controller_t * controller, float base_duty);

/** 更新姿态并返回本周期左右轮目标；此函数不直接访问电机硬件。 */
bool heading_controller_update(heading_controller_t * controller,
                               float gyro_z_dps,
                               float dt_s,
                               heading_output_t * output);
bool heading_controller_is_enabled(heading_controller_t const * controller);

#endif /* VEHICLE_DOMAIN_HEADING_CONTROLLER_H_ */
