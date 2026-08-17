#ifndef VEHICLE_CONTRACTS_VEHICLE_COMMAND_H_
#define VEHICLE_CONTRACTS_VEHICLE_COMMAND_H_

#include "Vehicle/contracts/vehicle_types.h"

/** 命令来源便于未来实现“急停 > 手持遥控 > 自动控制”的仲裁。 */
typedef enum e_vehicle_command_source
{
    VEHICLE_COMMAND_SOURCE_RTT = 0,
    VEHICLE_COMMAND_SOURCE_NRF,
    VEHICLE_COMMAND_SOURCE_IPC,
} vehicle_command_source_t;

typedef enum e_vehicle_command_kind
{
    VEHICLE_COMMAND_MANUAL = 0,
    VEHICLE_COMMAND_SET_SUCTION,
    VEHICLE_COMMAND_SET_SPEED,
    VEHICLE_COMMAND_START_AUTOMATIC,
    VEHICLE_COMMAND_SET_MODE,
    VEHICLE_COMMAND_EMERGENCY_STOP,
} vehicle_command_kind_t;

/**
 * 输入适配器提交给 Vehicle Thread 的命令。
 * 该结构不包含指针，因此适合放入 FreeRTOS 队列或跨模块复制。
 */
typedef struct st_vehicle_command
{
    vehicle_command_source_t source;
    vehicle_command_kind_t kind;
    uint32_t sequence;
    uint32_t received_tick;
    vehicle_manual_command_t manual_action;
    vehicle_mode_t mode;
    uint8_t speed_percent;
    uint8_t suction_percent;
    bool suction_enable;
} vehicle_command_t;

#endif /* VEHICLE_CONTRACTS_VEHICLE_COMMAND_H_ */
