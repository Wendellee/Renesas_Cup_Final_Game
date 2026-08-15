#ifndef VEHICLE_ADAPTERS_RTOS_VEHICLE_COMMAND_MAILBOX_H_
#define VEHICLE_ADAPTERS_RTOS_VEHICLE_COMMAND_MAILBOX_H_

#include "Vehicle/contracts/vehicle_command.h"

/** 由 Vehicle Thread 启动时调用一次。 */
bool vehicle_command_mailbox_init(void);

/**
 * 输入线程提交最新目标。NRF与IPC分别使用长度为1的最新值槽，避免互相覆盖。
 */
bool vehicle_command_mailbox_submit(vehicle_command_t const * command);

/** 只允许 Vehicle Thread 调用，非阻塞读取当前最新命令。 */
bool vehicle_command_mailbox_take(vehicle_command_t * command);

/** 只允许 Vehicle Thread 调用，非阻塞读取 M85 最新导航命令。 */
bool vehicle_command_mailbox_navigation_take(vehicle_command_t * command);

#endif /* VEHICLE_ADAPTERS_RTOS_VEHICLE_COMMAND_MAILBOX_H_ */
