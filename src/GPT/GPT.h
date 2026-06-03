#ifndef __GPT_H__
#define __GPT_H__

#include "hal_data.h"
#include "common_utils.h"


fsp_err_t gpt_init(timer_instance_t g_gpt_instance);
fsp_err_t gpt_set_duty_cycle(timer_instance_t g_gpt_instance, uint32_t duty_cycle_percent, uint32_t pin);












#endif /* __GPT_H__ */