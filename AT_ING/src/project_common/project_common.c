
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#include <stdio.h>
#include <string.h>

#include "project_common.h"

#if defined __cplusplus
    extern "C" {
#endif

int wake_up_by_ext_int(void)
{
    while((((*((uint32_t *)0x40040004)) >> 4) & 0xF) != 0x2);
    if (((*((uint32_t *)0x40090640)) & 0x10) == 0) {
        return 0;
    } else {
        return 1;
    }
}

#if defined __cplusplus
    }
#endif

