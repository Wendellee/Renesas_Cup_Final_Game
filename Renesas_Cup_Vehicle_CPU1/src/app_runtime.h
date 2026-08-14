#ifndef APP_RUNTIME_H_
#define APP_RUNTIME_H_

#include "FreeRTOS.h"
#include "event_groups.h"

#include <stdbool.h>

/* System state belongs in the EventGroup; business payloads stay in queues/mailboxes. */
#define SYSTEM_EVENT_APP_START_ALLOWED      ((EventBits_t) (1UL << 0))
#define SYSTEM_EVENT_WIFI_ONLINE            ((EventBits_t) (1UL << 1))
#define SYSTEM_EVENT_WIFI_FRONTEND_READY    ((EventBits_t) (1UL << 2))
#define SYSTEM_EVENT_NRF_VIDEO_ONLINE       ((EventBits_t) (1UL << 3))
#define SYSTEM_EVENT_NRF_COMMAND_ONLINE     ((EventBits_t) (1UL << 4))
#define SYSTEM_EVENT_EMERGENCY_STOP         ((EventBits_t) (1UL << 5))
#define SYSTEM_EVENT_SYSTEM_DEGRADED        ((EventBits_t) (1UL << 6))

bool app_runtime_init(void);
void app_runtime_wait_for_start(void);
void app_runtime_allow_start_degraded(void);
void app_runtime_wifi_frontend_set(bool online);

#endif /* APP_RUNTIME_H_ */
