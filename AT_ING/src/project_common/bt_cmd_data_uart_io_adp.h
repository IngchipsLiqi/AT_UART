
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#ifndef __BT_CMD_DATA_UART_IO_ADP_H__
#define __BT_CMD_DATA_UART_IO_ADP_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "app.h"

#if defined __cplusplus
    extern "C" {
#endif

typedef struct
{
    int process_state;

    int buf_index_top;
    unsigned int buf_data_top_count;

    int buf_index_bottom;
    unsigned int buf_data_bottom_count;

    unsigned char in_buf[CMD_IO_PORT_BUF_LEN_COM_IN + 1];
} bt_cmd_data_port_com_in_buf_t;

typedef struct
{
    int process_state;

    int buf_index_top;
    unsigned int buf_data_top_count;

    int buf_index_bottom;
    unsigned int buf_data_bottom_count;

    unsigned char in_buf[CMD_IO_PORT_BUF_LEN_BLE_IN + 1];
} bt_cmd_data_port_ble_in_buf_t;

typedef enum
{
    UART_READ_TRIG_SOURCE_ISR,
    UART_READ_TRIG_SOURCE_TIMER,
} uart_read_trig_source_t;

void bt_cmd_data_uart_recv_data(uart_read_trig_source_t trig_source);
void bt_cmd_data_com_buf_pop_data(uint16_t pop_num);
int bt_cmd_data_com_buf_pop_num(int mtu);
uint8_t *bt_cmd_data_com_buf_top_pos(void);

void bt_cmd_data_ble_recv_data(const uint8_t *data, int data_len);
void bt_cmd_data_uart_out(const uint8_t *data, int data_len);

void timer_start_main_cmd_data(void);
void timer_stop_main_cmd_data(void);

#if defined __cplusplus
    }
#endif

#endif

