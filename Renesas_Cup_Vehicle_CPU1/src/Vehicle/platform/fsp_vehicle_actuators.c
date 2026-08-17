#include "Vehicle/platform/fsp_vehicle_actuators.h"

#include "vehicle_thread.h"

#include <stddef.h>

typedef enum e_wheel_direction
{
    WHEEL_STOP = 0,
    WHEEL_FORWARD,
    WHEEL_REVERSE,
} wheel_direction_t;

static bool timer_open_start_safe(timer_instance_t const * timer)
{
    fsp_err_t err;

    if (NULL == timer) return false;
    err = timer->p_api->open(timer->p_ctrl, timer->p_cfg);
    if ((FSP_SUCCESS != err) && (FSP_ERR_ALREADY_OPEN != err)) return false;

    /* 无论 XML 初始 duty 如何，开放输出前都先强制两路关闭。 */
    (void) R_GPT_OutputDisable(timer->p_ctrl, GPT_IO_PIN_GTIOCA);
    (void) R_GPT_OutputDisable(timer->p_ctrl, GPT_IO_PIN_GTIOCB);
    return FSP_SUCCESS == timer->p_api->start(timer->p_ctrl);
}

static bool set_duty(timer_instance_t const * timer, uint32_t pin, float duty)
{
    timer_info_t info = {(timer_direction_t) 0, 0, 0};
    uint32_t counts;

    if ((NULL == timer) || (duty < 0.0F) || (duty > 1.0F)) return false;
    if (FSP_SUCCESS != timer->p_api->infoGet(timer->p_ctrl, &info)) return false;
    counts = (uint32_t) (((float) info.period_counts * duty) + 0.5F);
    return FSP_SUCCESS == timer->p_api->dutyCycleSet(timer->p_ctrl, counts, pin);
}

static bool set_wheel(timer_instance_t const * timer, float signed_duty)
{
    float magnitude;
    wheel_direction_t direction;

    if (signed_duty > 1.0F) signed_duty = 1.0F;
    if (signed_duty < -1.0F) signed_duty = -1.0F;
    direction = (signed_duty > 0.0F) ? WHEEL_FORWARD :
                ((signed_duty < 0.0F) ? WHEEL_REVERSE : WHEEL_STOP);
    magnitude = (signed_duty < 0.0F) ? -signed_duty : signed_duty;

    /* 先关闭两路，再改变占空比和方向，避免正反转瞬间两路同时导通。 */
    (void) R_GPT_OutputDisable(timer->p_ctrl, GPT_IO_PIN_GTIOCA);
    (void) R_GPT_OutputDisable(timer->p_ctrl, GPT_IO_PIN_GTIOCB);
    if ((!set_duty(timer, GPT_IO_PIN_GTIOCA, magnitude)) ||
        (!set_duty(timer, GPT_IO_PIN_GTIOCB, magnitude)))
    {
        return false;
    }

    if (WHEEL_FORWARD == direction)
    {
        return FSP_SUCCESS == R_GPT_OutputEnable(timer->p_ctrl, GPT_IO_PIN_GTIOCA);
    }
    if (WHEEL_REVERSE == direction)
    {
        return FSP_SUCCESS == R_GPT_OutputEnable(timer->p_ctrl, GPT_IO_PIN_GTIOCB);
    }
    return true;
}

static bool actuator_init(void * context)
{
    (void) context;

    if (!timer_open_start_safe(&g_left_wheel)) return false;
    if (!timer_open_start_safe(&g_right_wheel)) return false;
    if (!timer_open_start_safe(&g_fan)) return false;
    return true;
}

static bool wheels_write(void * context, float left_duty, float right_duty)
{
    (void) context;

    if (!set_wheel(&g_left_wheel, left_duty)) return false;
    if (!set_wheel(&g_right_wheel, right_duty))
    {
        (void) set_wheel(&g_left_wheel, 0.0F);
        return false;
    }
    return true;
}

static bool suction_write(void * context, float duty)
{
    (void) context;

    if (duty > 1.0F) duty = 1.0F;
    if (duty < 0.0F) duty = 0.0F;
    (void) R_GPT_OutputDisable(g_fan.p_ctrl, GPT_IO_PIN_GTIOCA);
    (void) R_GPT_OutputDisable(g_fan.p_ctrl, GPT_IO_PIN_GTIOCB);
    if ((!set_duty(&g_fan, GPT_IO_PIN_GTIOCA, duty)) ||
        (!set_duty(&g_fan, GPT_IO_PIN_GTIOCB, 0.0F)))
    {
        return false;
    }
    return (0.0F == duty) ||
           (FSP_SUCCESS == R_GPT_OutputEnable(g_fan.p_ctrl, GPT_IO_PIN_GTIOCA));
}

void fsp_vehicle_actuator_port_get(vehicle_actuator_port_t * port)
{
    if (NULL == port) return;
    port->context = NULL;
    port->init = actuator_init;
    port->write_wheels = wheels_write;
    port->write_suction = suction_write;
}
