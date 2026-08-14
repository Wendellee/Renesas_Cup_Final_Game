#ifndef BSP_PRINT_H
#define BSP_PRINT_H

#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"

#define BUFFER_INDEX 0

static inline void g_printf(const char * sFormat, ...)
{
    va_list ParamList;
    va_start(ParamList, sFormat);
    SEGGER_RTT_vprintf(BUFFER_INDEX, sFormat, &ParamList);
    va_end(ParamList);
}

#endif


