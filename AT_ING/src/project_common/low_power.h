
/*
** COPYRIGHT (c) 2023 by INGCHIPS
*/

#ifndef __LOW_POWER_H__
#define __LOW_POWER_H__

#include "stdint.h"
#include "FreeRTOS.h"
#include "semphr.h"

#if defined __cplusplus
    extern "C" {
#endif

typedef enum {
    LOW_POWER_EXIT_REASON_COM_RX_DATA     =  0,
    LOW_POWER_EXIT_REASON_COM_TX_DATA         ,
    LOW_POWER_EXIT_REASON_GPIO                ,
    LOW_POWER_EXIT_REASON_SYS_START           ,
    LOW_POWER_EXIT_REASON_MAX             = 15,
} low_power_exit_reason_t;

extern uint16_t g_connection_interval;
extern uint16_t g_sys_work_status_bits;

void low_power_exit_saving(low_power_exit_reason_t reason);
void low_power_create_timer(void);

#if defined __cplusplus
    }
#endif

#endif

