
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#ifndef __BT_CMD_PARSE_H__
#define __BT_CMD_PARSE_H__

#include "project_common.h"

#if defined __cplusplus
    extern "C" {
#endif

#define RES_FUNC 0

typedef int (*pfunbt_at_cmd_parse)(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength);

typedef struct {
    uint16_t cmd_code;
    pfunbt_at_cmd_parse cmd_handle;
} bt_hex_cmd_parse_fun_t;

int baec_msg_handle(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength);

#if defined __cplusplus
    }
#endif

#endif

