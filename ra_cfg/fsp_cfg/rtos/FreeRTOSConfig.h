#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "bsp_api.h"

#define configUSE_PREEMPTION                         1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION      0
#define configUSE_TICKLESS_IDLE                      0
#define configCPU_CLOCK_HZ                           SystemCoreClock
#define configTICK_RATE_HZ                           1000U
#define configMAX_PRIORITIES                         5U
#define configMINIMAL_STACK_SIZE                     128U
#define configMAX_TASK_NAME_LEN                      16U
#define configUSE_16_BIT_TICKS                       0
#define configIDLE_SHOULD_YIELD                      1
#define configUSE_TIME_SLICING                       1
#define configUSE_TASK_NOTIFICATIONS                 1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES        1
#define configUSE_MUTEXES                            0
#define configUSE_RECURSIVE_MUTEXES                  0
#define configUSE_COUNTING_SEMAPHORES                1
#define configUSE_QUEUE_SETS                         0
#define configQUEUE_REGISTRY_SIZE                    8U
#define configUSE_TRACE_FACILITY                     0
#define configUSE_STATS_FORMATTING_FUNCTIONS         0
#define configCHECK_FOR_STACK_OVERFLOW               2
#define configUSE_NEWLIB_REENTRANT                    0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS      0
#define configSTACK_DEPTH_TYPE                       uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE             size_t
#define configSUPPORT_STATIC_ALLOCATION              1
#define configSUPPORT_DYNAMIC_ALLOCATION             0
#define configTOTAL_HEAP_SIZE                        0U
#define configAPPLICATION_ALLOCATED_HEAP             0
#define configUSE_IDLE_HOOK                          0
#define configUSE_TICK_HOOK                          0
#define configUSE_MALLOC_FAILED_HOOK                 0
#define configUSE_DAEMON_TASK_STARTUP_HOOK           0
#define configUSE_TIMERS                             0
#define configTIMER_TASK_PRIORITY                    1U
#define configTIMER_QUEUE_LENGTH                     4U
#define configTIMER_TASK_STACK_DEPTH                 256U

/* FSP callbacks that use FreeRTOS FromISR APIs are configured at IPL 12. */
#define configPRIO_BITS                              __NVIC_PRIO_BITS
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 12U
#define configKERNEL_INTERRUPT_PRIORITY              (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY         (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))
#define configMAX_API_CALL_INTERRUPT_PRIORITY        configMAX_SYSCALL_INTERRUPT_PRIORITY

#define configENABLE_BACKWARD_COMPATIBILITY          0
#define configENABLE_MPU                             0
#define configUSE_CO_ROUTINES                        0
#define configMAX_CO_ROUTINE_PRIORITIES              2U
#define configGENERATE_RUN_TIME_STATS                0
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0
#define RM_FREERTOS_PORT_CFG_HW_STACK_MONITOR_ENABLE 0
#define INCLUDE_vTaskPrioritySet                     0
#define INCLUDE_uxTaskPriorityGet                    0
#define INCLUDE_vTaskDelete                          0
#define INCLUDE_vTaskSuspend                         0
#define INCLUDE_xResumeFromISR                       0
#define INCLUDE_vTaskDelayUntil                      0
#define INCLUDE_vTaskDelay                           1
#define INCLUDE_xTaskGetSchedulerState               0
#define INCLUDE_xTaskGetCurrentTaskHandle            1
#define INCLUDE_uxTaskGetStackHighWaterMark          0
#define INCLUDE_xTaskGetIdleTaskHandle               0
#define INCLUDE_eTaskGetState                        0
#define INCLUDE_xEventGroupSetBitFromISR             0
#define INCLUDE_xTimerPendFunctionCall               0
#define INCLUDE_xTaskAbortDelay                      0
#define INCLUDE_xTaskGetHandle                       0
#define INCLUDE_xTaskResumeFromISR                   0

#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0); } } while (0)

#endif /* FREERTOS_CONFIG_H */
