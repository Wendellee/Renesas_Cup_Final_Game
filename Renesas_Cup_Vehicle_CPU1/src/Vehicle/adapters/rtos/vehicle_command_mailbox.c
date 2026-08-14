#include "Vehicle/adapters/rtos/vehicle_command_mailbox.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <stddef.h>

static StaticQueue_t g_command_queue_control;
static uint8_t g_command_queue_storage[sizeof(vehicle_command_t)];
static QueueHandle_t g_command_queue;

bool vehicle_command_mailbox_init(void)
{
    if(NULL == g_command_queue)
    {
        g_command_queue = xQueueCreateStatic(1U,
                                             sizeof(vehicle_command_t),
                                             g_command_queue_storage,
                                             &g_command_queue_control);
    }
    return NULL != g_command_queue;
}

bool vehicle_command_mailbox_submit(vehicle_command_t const * command)
{
    if ((NULL == command) || (NULL == g_command_queue)) return false;
    return pdPASS == xQueueOverwrite(g_command_queue, command);
}

bool vehicle_command_mailbox_take(vehicle_command_t * command)
{
    if ((NULL == command) || (NULL == g_command_queue)) return false;
    return pdPASS == xQueueReceive(g_command_queue, command, 0U);
}
