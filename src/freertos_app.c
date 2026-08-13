#include "freertos_app.h"

#include "hal_data.h"
#include "FreeRTOS.h"
#include "task.h"

static volatile bool g_application_initialized;

typedef struct st_app_fault_info
{
    uint32_t exception;
    uint32_t exc_return;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t dfsr;
    uint32_t afsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t shcsr;
} app_fault_info_t;

/* Inspect this object in the debugger if execution ever stops in
 * AppFault_Handler_C.  Unlike the weak startup aliases it preserves the actual
 * fault type, stacked PC and Cortex-M fault status registers. */
volatile app_fault_info_t g_app_fault_info;

void AppFault_Handler_C(uint32_t exception, uint32_t * p_stack, uint32_t exc_return);

#define APP_FAULT_WRAPPER(handler_name, exception_number)            \
    __attribute__((naked)) void handler_name(void)                    \
    {                                                                 \
        __asm volatile (                                              \
            "movs r0, %0\n"                                          \
            "mov r2, lr\n"                                           \
            "tst lr, #4\n"                                           \
            "ite eq\n"                                               \
            "mrseq r1, msp\n"                                        \
            "mrsne r1, psp\n"                                        \
            "b AppFault_Handler_C\n"                                 \
            : : "I" (exception_number) : "r0", "r1", "r2", "memory"); \
    }

APP_FAULT_WRAPPER(HardFault_Handler,   3)
APP_FAULT_WRAPPER(MemManage_Handler,   4)
APP_FAULT_WRAPPER(BusFault_Handler,    5)
APP_FAULT_WRAPPER(UsageFault_Handler,  6)

void AppFault_Handler_C(uint32_t exception, uint32_t * p_stack, uint32_t exc_return)
{
    g_app_fault_info.exception = exception;
    g_app_fault_info.exc_return = exc_return;
    g_app_fault_info.r0         = p_stack[0];
    g_app_fault_info.r1         = p_stack[1];
    g_app_fault_info.r2         = p_stack[2];
    g_app_fault_info.r3         = p_stack[3];
    g_app_fault_info.r12        = p_stack[4];
    g_app_fault_info.lr         = p_stack[5];
    g_app_fault_info.pc         = p_stack[6];
    g_app_fault_info.xpsr       = p_stack[7];
    g_app_fault_info.cfsr       = SCB->CFSR;
    g_app_fault_info.hfsr       = SCB->HFSR;
    g_app_fault_info.dfsr       = SCB->DFSR;
    g_app_fault_info.afsr       = SCB->AFSR;
    g_app_fault_info.mmfar      = SCB->MMFAR;
    g_app_fault_info.bfar       = SCB->BFAR;
    g_app_fault_info.shcsr      = SCB->SHCSR;
    __DMB();

    taskDISABLE_INTERRUPTS();
    while (1)
    {
        __NOP();
    }
}

void FreeRtosApp_NotifyInitialized(void)
{
    g_application_initialized = true;
    __DMB();
}

void FreeRtosApp_WaitInitialized(void)
{
    while (!g_application_initialized)
    {
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
    __DMB();
}

void vApplicationStackOverflowHook(TaskHandle_t task, char * p_task_name)
{
    FSP_PARAMETER_NOT_USED(task);
    FSP_PARAMETER_NOT_USED(p_task_name);
    taskDISABLE_INTERRUPTS();
    BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
}
