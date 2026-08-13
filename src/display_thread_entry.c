#include "display_thread.h"

extern void AppGuiTask_Entry(void * pv_parameters);

void display_thread_entry(void * pvParameters)
{
    AppGuiTask_Entry(pvParameters);
}
