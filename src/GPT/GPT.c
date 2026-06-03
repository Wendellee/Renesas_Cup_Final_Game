

#include "GPT.h"

fsp_err_t gpt_init(timer_instance_t g_gpt_instance)
{
    fsp_err_t err = g_gpt_instance.p_api->open(g_gpt_instance.p_ctrl, g_gpt_instance.p_cfg);
    if (FSP_SUCCESS == err)
    {
        err = g_gpt_instance.p_api->start(g_gpt_instance.p_ctrl);
    }
    return err;
}
fsp_err_t gpt_deinit(timer_instance_t g_gpt_instance)
{
    fsp_err_t err = g_gpt_instance.p_api->stop(g_gpt_instance.p_ctrl);
    if (FSP_SUCCESS == err)
    {
        err = g_gpt_instance.p_api->close(g_gpt_instance.p_ctrl);
    }
    return err;
}




fsp_err_t gpt_set_duty_cycle(timer_instance_t g_gpt_instance, uint32_t duty_cycle_percent, uint32_t pin)
{
    fsp_err_t err                           = FSP_SUCCESS;
    uint32_t duty_cycle_counts              = 0;
    uint32_t current_period_counts          = 0;
    timer_info_t info                       = {(timer_direction_t)0, 0, 0};

    err = g_gpt_instance.p_api->infoGet(g_gpt_instance.p_ctrl, &info);
    if (FSP_SUCCESS != err)
    {
        /* GPT Timer InfoGet Failure message */
        APP_PRINT ("\r\n** R_GPT_InfoGet API failed **\r\n");
    }
    else//successfully got the timer info, set the duty cycle
    {
        current_period_counts = info.period_counts;
        duty_cycle_counts = (uint32_t) ((uint64_t) (current_period_counts * duty_cycle_percent) / 100);
        err = g_gpt_instance.p_api->dutyCycleSet(g_gpt_instance.p_ctrl, duty_cycle_counts, pin);
        if (FSP_SUCCESS != err)
        {
            /* GPT Timer duty cycle set failure message */
            /* In case of GPT_open is successful and duty cycle set fails, requires a immediate cleanup.
             * Since, cleanup for GPT open is done in timer_duty_cycle_set, hence cleanup is not required */
            APP_PRINT ("\r\n** R_GPT_DutyCycleSet API failed **\r\n");
        }
    }
    return err;
}
