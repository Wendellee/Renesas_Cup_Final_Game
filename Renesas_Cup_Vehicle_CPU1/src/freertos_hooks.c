/*
 * FreeRTOS application-wide runtime support and hooks.
 *
 * This is user code and is not overwritten by FSP regeneration.
 */

#include "app_runtime.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"
#include "SEGGER_RTT/bsp_print.h"

#define APP_STARTUP_WIFI_DEADLINE_MS    (30000U)

static StaticEventGroup_t g_system_event_group_storage;
static EventGroupHandle_t g_system_event_group;
static StaticTimer_t g_startup_deadline_timer_storage;
static TimerHandle_t g_startup_deadline_timer;
static bool g_startup_deadline_started;

static void app_runtime_startup_deadline_callback(TimerHandle_t timer)
{
    (void) timer;

    if(NULL != g_system_event_group)
    {
        EventBits_t const bits = xEventGroupGetBits(g_system_event_group);
        if(0U == (bits & SYSTEM_EVENT_APP_START_ALLOWED))
        {
            g_printf("[SYSTEM][WARN] Wi-Fi startup deadline reached; releasing M33 business tasks.\r\n");
            (void) xEventGroupSetBits(g_system_event_group,
                                      SYSTEM_EVENT_SYSTEM_DEGRADED |
                                      SYSTEM_EVENT_APP_START_ALLOWED);
        }
    }
}

bool app_runtime_init(void)
{
    bool start_timer = false;

    taskENTER_CRITICAL();
    if(NULL == g_system_event_group)
    {
        g_system_event_group = xEventGroupCreateStatic(&g_system_event_group_storage);
    }

    if((NULL != g_system_event_group) && (NULL == g_startup_deadline_timer))
    {
        g_startup_deadline_timer = xTimerCreateStatic(
            "WiFiStartDeadline",
            pdMS_TO_TICKS(APP_STARTUP_WIFI_DEADLINE_MS),
            pdFALSE,
            NULL,
            app_runtime_startup_deadline_callback,
            &g_startup_deadline_timer_storage);
    }
    start_timer = (NULL != g_startup_deadline_timer) &&
                  !g_startup_deadline_started;
    taskEXIT_CRITICAL();

    if(start_timer)
    {
        if(pdPASS != xTimerStart(g_startup_deadline_timer, 0U))
        {
            return false;
        }
        g_startup_deadline_started = true;
    }

    return (NULL != g_system_event_group) &&
           (NULL != g_startup_deadline_timer) &&
           g_startup_deadline_started;
}

void app_runtime_wait_for_start(void)
{
    if(NULL != g_system_event_group)
    {
        (void) xEventGroupWaitBits(g_system_event_group,
                                   SYSTEM_EVENT_APP_START_ALLOWED,
                                   pdFALSE,
                                   pdTRUE,
                                   portMAX_DELAY);
    }
}

void app_runtime_allow_start_degraded(void)
{
    if(NULL != g_system_event_group)
    {
        (void) xEventGroupSetBits(g_system_event_group,
                                  SYSTEM_EVENT_SYSTEM_DEGRADED |
                                  SYSTEM_EVENT_APP_START_ALLOWED);
    }
}

void app_runtime_wifi_frontend_set(bool online)
{
    if(NULL == g_system_event_group)
    {
        return;
    }

    if(online)
    {
        if(NULL != g_startup_deadline_timer)
        {
            (void) xTimerStop(g_startup_deadline_timer, 0U);
        }

        (void) xEventGroupSetBits(g_system_event_group,
                                  SYSTEM_EVENT_WIFI_ONLINE |
                                  SYSTEM_EVENT_WIFI_FRONTEND_READY |
                                  SYSTEM_EVENT_APP_START_ALLOWED);
    }
    else
    {
        (void) xEventGroupClearBits(g_system_event_group,
                                    SYSTEM_EVENT_WIFI_ONLINE |
                                    SYSTEM_EVENT_WIFI_FRONTEND_READY);
    }
}

/*
 * 栈溢出后可在调试器 Watch 窗口查看这两个变量，定位发生故障的线程。
 * 变量刻意保留为全局 volatile，避免编译器优化掉故障现场信息。
 */
volatile TaskHandle_t g_stack_overflow_task;
char const * volatile g_stack_overflow_task_name;

void vApplicationStackOverflowHook(TaskHandle_t x_task, char * p_task_name)
{
    g_stack_overflow_task      = x_task;
    g_stack_overflow_task_name = p_task_name;

    /*
     * 此时当前线程的栈已经不可信，不能继续调用 printf、申请内存或
     * 执行其他依赖任务栈的恢复逻辑。停止系统以保护执行器状态和现场。
     */
    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        __asm volatile ("nop");
    }
}
