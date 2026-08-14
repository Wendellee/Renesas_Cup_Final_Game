#include "command_tx_thread.h"

extern void AppCommandTxTask_Entry(void * pv_parameters);

void command_tx_thread_entry(void * pvParameters)
{
    AppCommandTxTask_Entry(pvParameters);
}
