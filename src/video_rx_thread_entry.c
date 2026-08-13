#include "video_rx_thread.h"

extern void AppRadioTask_Entry(void);

void video_rx_thread_entry(void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    AppRadioTask_Entry();
}
