/* Keep the FSP-generated RTOS feature selections, but reserve enough dynamic
 * heap for LVGL's FreeRTOS OS layer.  lv_init() creates recursive mutexes and
 * an 8 KiB software-render task, so the generated 1 KiB default cannot work. */
#ifndef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE (64U * 1024U)
#endif

#ifndef configUSE_MALLOC_FAILED_HOOK
#define configUSE_MALLOC_FAILED_HOOK (1)
#endif

#ifndef configCHECK_FOR_STACK_OVERFLOW
#define configCHECK_FOR_STACK_OVERFLOW (2)
#endif

#include "../ra_cfg/aws/FreeRTOSConfig.h"
