#include "Vehicle/adapters/rtos/vehicle_command_mailbox.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <stddef.h>

static StaticQueue_t g_command_queue_control;
static uint8_t g_command_queue_storage[sizeof(vehicle_command_t)];
static QueueHandle_t g_command_queue;
static StaticQueue_t g_navigation_queue_control;
static uint8_t g_navigation_queue_storage[sizeof(vehicle_command_t)];
static QueueHandle_t g_navigation_queue;

bool vehicle_command_mailbox_init(void)
{
    if(NULL == g_command_queue)
    {
        g_command_queue = xQueueCreateStatic(1U,
                                             sizeof(vehicle_command_t),
                                             g_command_queue_storage,
                                             &g_command_queue_control);
    }
    if(NULL == g_navigation_queue)
    {
        g_navigation_queue = xQueueCreateStatic(1U,
                                                sizeof(vehicle_command_t),
                                                g_navigation_queue_storage,
                                                &g_navigation_queue_control);
    }
    return (NULL != g_command_queue) && (NULL != g_navigation_queue);
}

bool vehicle_command_mailbox_submit(vehicle_command_t const * command)
{
    QueueHandle_t queue;

    if (NULL == command) return false;
    queue = (VEHICLE_COMMAND_SOURCE_IPC == command->source) ?
            g_navigation_queue : g_command_queue;
    if (NULL == queue) return false;
    return pdPASS == xQueueOverwrite(queue, command);
}

bool vehicle_command_mailbox_take(vehicle_command_t * command)
{
    if ((NULL == command) || (NULL == g_command_queue)) return false;
    return pdPASS == xQueueReceive(g_command_queue, command, 0U);
}

bool vehicle_command_mailbox_navigation_take(vehicle_command_t * command)
{
    if ((NULL == command) || (NULL == g_navigation_queue)) return false;
    return pdPASS == xQueueReceive(g_navigation_queue, command, 0U);
}
