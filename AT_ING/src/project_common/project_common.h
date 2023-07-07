
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#ifndef __PROJECT_COMMON_H__
#define __PROJECT_COMMON_H__

#include <stdint.h>
#include "app.h"

#if defined __cplusplus
    extern "C" {
#endif

#define BT_PRIVT_OK 0
#define BT_PRIVT_ERROR -1
#define BT_PRIVT_NO_CMD -2
#define BT_PRIVT_TIMEOUT -3
#define BT_PRIVT_PARA_ERROR -4
#define BT_PRIVT_AT_HEADER_ERROR -5
#define BT_PRIVT_AT_BODY_ERROR -6
#define BT_PRIVT_AT_PDU_ERROR -7

#define BT_PRIVT_TRUE 1
#define BT_PRIVT_FALSE 0

#define BT_PRIVT_ENABLE 1
#define BT_PRIVT_DISNABLE 0

#define BT_PRIVT_NULL 0

#define INVALID_HANDLE (0xffff)

#if defined __cplusplus
    }
#endif

#endif

