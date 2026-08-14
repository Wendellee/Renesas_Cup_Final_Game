#include <vehicle_thread.h>
#include "app_runtime.h"
#include "Vehicle/adapters/rtos/vehicle_command_mailbox.h"
#include "Vehicle/application/vehicle_service.h"
#include "Vehicle/platform/fsp_vehicle_factory.h"
#include "SEGGER_RTT/bsp_print.h"

#define VEHICLE_CONTROL_PERIOD_MS        (10U)
#define VEHICLE_COMMAND_TIMEOUT_MS       (200U)
#define VEHICLE_STARTUP_SUCTION_PERCENT  (80U)

/*
 * 车轮直行验证开关：
 *   1：上电完成初始化后，以 90% 速度直行 3 秒并自动停车；
 *   0：关闭上电自检，直接进入正常遥控流程。
 *
 * 首次测试时请架空车轮。确认左右轮方向正确后，再把车辆放到地面测试。
 */
#define VEHICLE_WHEEL_STRAIGHT_TEST_ENABLE       (0U)
#define VEHICLE_WHEEL_TEST_START_DELAY_MS        (2500U)
#define VEHICLE_WHEEL_TEST_RUN_TIME_MS           (3000U)
#define VEHICLE_WHEEL_TEST_SPEED_PERCENT         (90U)

typedef enum e_vehicle_wheel_test_state
{
    VEHICLE_WHEEL_TEST_WAITING = 0,
    VEHICLE_WHEEL_TEST_RUNNING,
    VEHICLE_WHEEL_TEST_COMPLETE,
} vehicle_wheel_test_state_t;

static void execute_command(vehicle_command_t const * command)
{
    switch (command->kind)
    {
        case VEHICLE_COMMAND_MANUAL:
            (void) vehicle_service_manual_command(command->manual_action,
                                                  command->speed_percent);
            break;
        case VEHICLE_COMMAND_SET_SUCTION:
            (void) vehicle_service_suction_set(command->suction_enable,
                                               command->suction_percent);
            break;
        case VEHICLE_COMMAND_SET_SPEED:
            (void) vehicle_service_automatic_speed_set(command->speed_percent);
            break;
        case VEHICLE_COMMAND_START_AUTOMATIC:
            (void) vehicle_service_automatic_speed_set(command->speed_percent);
            (void) vehicle_service_automatic_start();
            break;
        case VEHICLE_COMMAND_SET_MODE:
            (void) vehicle_service_mode_set(command->mode);
            break;
        case VEHICLE_COMMAND_EMERGENCY_STOP:
            vehicle_service_emergency_stop();
            break;
        default:
            break;
    }
}

#if VEHICLE_WHEEL_STRAIGHT_TEST_ENABLE
static bool command_requests_stop(vehicle_command_t const * command)
{
    return (VEHICLE_COMMAND_EMERGENCY_STOP == command->kind) ||
           ((VEHICLE_COMMAND_MANUAL == command->kind) &&
            (VEHICLE_MANUAL_STOP == command->manual_action));
}
#endif

/**
 * @brief 底盘硬件的唯一所有者。
 *
 * Command RX 等输入线程只向 mailbox 提交目标，本线程在固定 10 ms 周期中执行目标、
 * 读取 IMU、运行控制器并写 GPT，从结构上避免多个线程同时操作电机。
 */
void vehicle_thread_entry(void * pvParameters)
{
    vehicle_dependencies_t dependencies;
    vehicle_command_t command;
    vehicle_result_t init_result;
    TickType_t wake_tick;
    TickType_t last_command_tick;
    bool remote_command_seen = false;
#if VEHICLE_WHEEL_STRAIGHT_TEST_ENABLE
    vehicle_wheel_test_state_t wheel_test_state = VEHICLE_WHEEL_TEST_WAITING;
    TickType_t wheel_test_tick;
#endif

    FSP_PARAMETER_NOT_USED(pvParameters);

    if(!app_runtime_init())
    {
        g_printf("[SYSTEM][FATAL] runtime EventGroup/timer initialization failed.\r\n");
        vTaskSuspend(NULL);
    }

    if(!vehicle_command_mailbox_init())
    {
        vehicle_service_emergency_stop();
        g_printf("[VEHICLE][FATAL] command mailbox initialization failed.\r\n");
        vTaskSuspend(NULL);
    }

    if(!fsp_vehicle_dependencies_create(&dependencies))
    {
        vehicle_service_emergency_stop();
        g_printf("[VEHICLE][FATAL] dependency initialization failed; check IIC0 open.\r\n");
        vTaskSuspend(NULL);
    }

    init_result = vehicle_service_init(&dependencies);
    if(VEHICLE_RESULT_OK != init_result)
    {
        vehicle_service_emergency_stop();
        g_printf("[VEHICLE][FATAL] service initialization failed: %u.\r\n",
                 (unsigned int) init_result);
        vTaskSuspend(NULL);
    }

    g_printf("[VEHICLE] safe outputs established; waiting for startup gate.\r\n");
    app_runtime_wait_for_start();
    g_printf("[VEHICLE] startup gate released.\r\n");

    init_result = vehicle_service_suction_set(true,
                                              VEHICLE_STARTUP_SUCTION_PERCENT);
    if(VEHICLE_RESULT_OK != init_result)
    {
        vehicle_service_emergency_stop();
        g_printf("[VEHICLE][FATAL] startup suction failed: %u.\r\n",
                 (unsigned int) init_result);
        vTaskSuspend(NULL);
    }
    g_printf("[VEHICLE] suction starting at %u%%.\r\n",
             (unsigned int) VEHICLE_STARTUP_SUCTION_PERCENT);

    wake_tick = xTaskGetTickCount();
    last_command_tick = wake_tick;
#if VEHICLE_WHEEL_STRAIGHT_TEST_ENABLE
    wheel_test_tick = wake_tick;
    g_printf("[VEHICLE][TEST] wheel test waiting for suction\r\n");
#endif
    for (;;)
    {
        if (vehicle_command_mailbox_take(&command))
        {
#if VEHICLE_WHEEL_STRAIGHT_TEST_ENABLE
            /* 自检期间接受急停和普通停车，确保手持控制器始终可以停止车轮。 */
            if ((VEHICLE_WHEEL_TEST_COMPLETE == wheel_test_state) ||
                command_requests_stop(&command))
#endif
            {
                execute_command(&command);
                last_command_tick = xTaskGetTickCount();
                remote_command_seen = VEHICLE_COMMAND_SOURCE_NRF == command.source;
            }
#if VEHICLE_WHEEL_STRAIGHT_TEST_ENABLE
            if (command_requests_stop(&command))
            {
                wheel_test_state = VEHICLE_WHEEL_TEST_COMPLETE;
                g_printf("[VEHICLE][TEST] aborted by stop command\r\n");
            }
#endif
        }

#if VEHICLE_WHEEL_STRAIGHT_TEST_ENABLE
        if (VEHICLE_WHEEL_TEST_WAITING == wheel_test_state)
        {
            if ((xTaskGetTickCount() - wheel_test_tick) >=
                pdMS_TO_TICKS(VEHICLE_WHEEL_TEST_START_DELAY_MS))
            {
                if (VEHICLE_RESULT_OK ==
                    vehicle_service_manual_command(VEHICLE_MANUAL_FORWARD,
                                                   VEHICLE_WHEEL_TEST_SPEED_PERCENT))
                {
                    wheel_test_state = VEHICLE_WHEEL_TEST_RUNNING;
                    wheel_test_tick = xTaskGetTickCount();
                    g_printf("[VEHICLE][TEST] forward, speed=%u%%\r\n",
                             VEHICLE_WHEEL_TEST_SPEED_PERCENT);
                }
                else
                {
                    /* 吸附尚未建立时稍后重试，不绕过车辆服务的安全检查。 */
                    wheel_test_tick = xTaskGetTickCount() -
                                      pdMS_TO_TICKS(VEHICLE_WHEEL_TEST_START_DELAY_MS - 100U);
                }
            }
        }
        else if ((VEHICLE_WHEEL_TEST_RUNNING == wheel_test_state) &&
                 ((xTaskGetTickCount() - wheel_test_tick) >=
                  pdMS_TO_TICKS(VEHICLE_WHEEL_TEST_RUN_TIME_MS)))
        {
            (void) vehicle_service_manual_command(VEHICLE_MANUAL_STOP, 0U);
            wheel_test_state = VEHICLE_WHEEL_TEST_COMPLETE;
            remote_command_seen = false;
            g_printf("[VEHICLE][TEST] complete, wheels stopped\r\n");
        }
#endif

        /* 手持遥控曾接管后，超过 200 ms 未收到新命令则自主停车。 */
        if (remote_command_seen &&
            ((xTaskGetTickCount() - last_command_tick) >
             pdMS_TO_TICKS(VEHICLE_COMMAND_TIMEOUT_MS)))
        {
            (void) vehicle_service_manual_command(VEHICLE_MANUAL_STOP, 0U);
            remote_command_seen = false;
        }

        if (VEHICLE_RESULT_OK !=
            vehicle_service_step((float) VEHICLE_CONTROL_PERIOD_MS / 1000.0F))
        {
            vehicle_service_emergency_stop();
        }
        vTaskDelayUntil(&wake_tick, pdMS_TO_TICKS(VEHICLE_CONTROL_PERIOD_MS));
    }
}
