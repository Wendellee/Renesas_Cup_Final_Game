#include "command_tx_thread.h"

extern void AppCameraTask_Entry(void * pv_parameters);

void command_tx_thread_entry(void * pvParameters)
{
    AppCameraTask_Entry(pvParameters);
}
