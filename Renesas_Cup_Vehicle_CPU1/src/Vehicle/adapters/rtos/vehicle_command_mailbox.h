#ifndef VEHICLE_ADAPTERS_RTOS_VEHICLE_COMMAND_MAILBOX_H_
#define VEHICLE_ADAPTERS_RTOS_VEHICLE_COMMAND_MAILBOX_H_

#include "Vehicle/contracts/vehicle_command.h"

/** 由 Vehicle Thread 启动时调用一次。 */
bool vehicle_command_mailbox_init(void);

/**
 * 输入线程提交最新目标。队列长度为 1，新命令覆盖旧命令，避免执行过期摇杆操作。
 */
bool vehicle_command_mailbox_submit(vehicle_command_t const * command);

/** 只允许 Vehicle Thread 调用，非阻塞读取当前最新命令。 */
bool vehicle_command_mailbox_take(vehicle_command_t * command);

#endif /* VEHICLE_ADAPTERS_RTOS_VEHICLE_COMMAND_MAILBOX_H_ */
