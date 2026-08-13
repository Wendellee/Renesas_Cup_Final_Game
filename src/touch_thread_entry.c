#include "touch_thread.h"

extern void AppTouchTask_Entry(void * pv_parameters);

void touch_thread_entry(void * pvParameters)
{
    AppTouchTask_Entry(pvParameters);
}
