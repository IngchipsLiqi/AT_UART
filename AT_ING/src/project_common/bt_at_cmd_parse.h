
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#ifndef __BT_AT_CMD_PARSE_H__
#define __BT_AT_CMD_PARSE_H__

#include <stdint.h>

#if defined __cplusplus
    extern "C" {
#endif

int baec_msg_handle(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength);
void bt_at_power_on_ack(void);
void bt_at_wake_up_ack(void);
void bt_at_connect_ack(void);
void bt_at_disconnect_ack(void);

#if defined __cplusplus
    }
#endif

#endif

