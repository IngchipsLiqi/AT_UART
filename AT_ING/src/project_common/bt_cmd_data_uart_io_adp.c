
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "platform_api.h"
#include "btstack_event.h"
#include "att_db.h"

#include "ingsoc.h"

#include "project_common.h"
#include "bt_cmd_data_uart_io_adp.h"
#include "bt_at_cmd_parse.h"
#include "app.h"
#include "low_power.h"

#if defined __cplusplus
    extern "C" {
#endif

#define TMR_CLK_1S_COUNT 20

//uint16_t g_my_peer_master_dev_con_handle = INVALID_HANDLE;
//uint16_t g_bt_bm_module_uart_rx_to_ble_handle = 0;

static bt_cmd_data_port_com_in_buf_t com_buf = {0};
static bt_cmd_data_port_ble_in_buf_t ble_buf = {0};

typedef enum {
    CMD_DATA_PORT_PROCESS_IDLE                = 0, // 0
    CMD_DATA_PORT_PROCESS_START_PARSE_CMD        ,
    CMD_DATA_PORT_PROCESS_START_SEND_BY_BLE      ,
    CMD_DATA_PORT_PROCESS_START_SEND_BY_COM      ,
    CMD_DATA_PORT_PROCESSING_PARSE_CMD           ,
    CMD_DATA_PORT_PROCESSING_SEND_BY_BLE         ,
    CMD_DATA_PORT_PROCESSING_SEND_BY_COM         ,
    CMD_DATA_PORT_PROCESS_END                = 30,
} bt_cmd_data_port_process_state_e;

static void bt_cmd_data_print_buf_state(void)
{
    log_printf("[com_buf]: [%d] [%d] [%d] [%d]", com_buf.buf_index_top, com_buf.buf_index_bottom, com_buf.buf_data_top_count, com_buf.buf_data_bottom_count);
    log_printf("[ble_buf]: [%d] [%d] [%d] [%d]", ble_buf.buf_index_top, ble_buf.buf_index_bottom, ble_buf.buf_data_top_count, ble_buf.buf_data_bottom_count);
}

void bt_cmd_data_uart_out(const uint8_t *data, int data_len)
{
    int data_index;

    if (data == BT_PRIVT_NULL) {
        return;
    }

    for (data_index = 0; data_index < data_len; data_index++) {
        while (apUART_Check_TXFIFO_FULL(COM_PORT) == 1);
        UART_SendData(COM_PORT, data[data_index]);
    }

    return;
}

static void give_sem_send_data_by_ble(void);
#define CMD_IO_PORT_MAX_CMD_LEN 64
static uint8_t cmd_out_buf[CMD_IO_PORT_MAX_CMD_LEN] = {0};
static void bt_cmd_data_uart_rx_cmd_process()
{
    unsigned int buf_data_count = com_buf.buf_data_bottom_count - com_buf.buf_data_top_count;
    uint16_t out_data_len = CMD_IO_PORT_MAX_CMD_LEN;
    int str_len;
    int i;
    int ret;
    com_buf.in_buf[com_buf.buf_index_bottom] = '\0';

    ret = baec_msg_handle((uint8_t *)(&com_buf.in_buf[com_buf.buf_index_top]),
                          buf_data_count,
                          cmd_out_buf,
                          &out_data_len);
    if (ret == BT_PRIVT_OK) {
        str_len = strlen((char *)cmd_out_buf);
        if (out_data_len > str_len)
        {
            out_data_len = str_len;
        }
        bt_cmd_data_uart_out((char *)cmd_out_buf, out_data_len);
        com_buf.buf_index_bottom = 0;
        com_buf.buf_data_bottom_count = 0;
        com_buf.buf_index_top = 0;
        com_buf.buf_data_top_count = 0;
    } else {
        log_printf("[com]: data pass\r\n");
        bt_cmd_data_print_buf_state();
        give_sem_send_data_by_ble();
    }

}

static void bt_cmd_data_uart_rx_data_fun(char data)
{
    if (com_buf.buf_data_bottom_count >= 0xEFFFFFFF &&
        com_buf.buf_data_bottom_count == com_buf.buf_data_top_count) {
        com_buf.buf_data_bottom_count = 0;
        com_buf.buf_data_top_count = 0;
    }

    com_buf.in_buf[com_buf.buf_index_bottom] = data;

    com_buf.buf_index_bottom++;
    com_buf.buf_data_bottom_count++;
    if (com_buf.buf_index_bottom >= CMD_IO_PORT_BUF_LEN_COM_IN) {
        com_buf.buf_index_bottom -= CMD_IO_PORT_BUF_LEN_COM_IN;
    }

    return;
}

int bt_cmd_data_com_buf_pop_num(int mtu)
{
    int ret;

    ret = com_buf.buf_data_bottom_count - com_buf.buf_data_top_count;
    if (ret == 0) {
        return 0;
    }

    if (ret > mtu) {
        ret = mtu;
    }

    if ((com_buf.buf_index_top + ret) > CMD_IO_PORT_BUF_LEN_COM_IN) {
        ret = CMD_IO_PORT_BUF_LEN_COM_IN - com_buf.buf_index_top;
    }

    return ret;
}

uint8_t *bt_cmd_data_com_buf_top_pos(void)
{
    return &com_buf.in_buf[com_buf.buf_index_top];
}

void bt_cmd_data_com_buf_pop_data(uint16_t pop_num)
{
    com_buf.buf_index_top += pop_num;
    com_buf.buf_data_top_count += pop_num;
    if (com_buf.buf_index_top >= CMD_IO_PORT_BUF_LEN_COM_IN) {
        com_buf.buf_index_top -= CMD_IO_PORT_BUF_LEN_COM_IN;
    }
    return;
}

static void give_sem_send_data_by_com(void);
void bt_cmd_data_ble_recv_data(const uint8_t *data, int data_len)
{
    //bt_cmd_data_uart_out(data, data_len);
    if (ble_buf.buf_data_bottom_count >= 0xEFFFFFFF &&
        ble_buf.buf_data_bottom_count == ble_buf.buf_data_top_count) {
        ble_buf.buf_data_bottom_count = 0;
        ble_buf.buf_data_top_count = 0;
    }

    if (ble_buf.buf_index_bottom + data_len <= CMD_IO_PORT_BUF_LEN_BLE_IN) {
        memcpy(&ble_buf.in_buf[ble_buf.buf_index_bottom], data, data_len);
    } else {
        int left_len = CMD_IO_PORT_BUF_LEN_BLE_IN - ble_buf.buf_index_bottom;
        memcpy(&ble_buf.in_buf[ble_buf.buf_index_bottom], data,            left_len);
        memcpy(&ble_buf.in_buf[0],                        &data[left_len], data_len - left_len);
    }

    ble_buf.buf_index_bottom += data_len;
    ble_buf.buf_data_bottom_count += data_len;
    if (ble_buf.buf_index_bottom >= CMD_IO_PORT_BUF_LEN_BLE_IN) {
        ble_buf.buf_index_bottom -= CMD_IO_PORT_BUF_LEN_BLE_IN;
    }
    give_sem_send_data_by_com();
    return;
}

static void bt_cmd_data_uart_handle_task_send_data(void) // --test
{
    while (ble_buf.buf_data_top_count < ble_buf.buf_data_bottom_count) {
        while (apUART_Check_TXFIFO_FULL(COM_PORT) == 1) {};
        UART_SendData(COM_PORT, ble_buf.in_buf[ble_buf.buf_index_top]);
        ble_buf.buf_index_top++;
        ble_buf.buf_data_top_count++;
        if (ble_buf.buf_index_top >= CMD_IO_PORT_BUF_LEN_BLE_IN) {
            ble_buf.buf_index_top -= CMD_IO_PORT_BUF_LEN_BLE_IN;
        }
    }
    return;
}

static SemaphoreHandle_t sem_cmd_process = NULL;
static void give_sem_send_data_by_ble(void)
{
    BaseType_t xHigherPriorityTaskCmdProcess;

    com_buf.process_state |= (0x1 << CMD_DATA_PORT_PROCESS_START_SEND_BY_BLE);
    xHigherPriorityTaskCmdProcess = pdFALSE;
    xSemaphoreGiveFromISR(sem_cmd_process, &xHigherPriorityTaskCmdProcess);
    return;
}

static void give_sem_send_data_by_com(void)
{
    BaseType_t xHigherPriorityTaskCmdProcess;

    ble_buf.process_state |= (0x1 << CMD_DATA_PORT_PROCESS_START_SEND_BY_COM);
    xHigherPriorityTaskCmdProcess = pdFALSE;
    xSemaphoreGiveFromISR(sem_cmd_process, &xHigherPriorityTaskCmdProcess);

    return;
}

static void give_sem_process_cmd(void)
{
    BaseType_t xHigherPriorityTaskCmdProcess;

    com_buf.process_state |= (0x1 << CMD_DATA_PORT_PROCESS_START_PARSE_CMD);
    xHigherPriorityTaskCmdProcess = pdFALSE;
    xSemaphoreGiveFromISR(sem_cmd_process, &xHigherPriorityTaskCmdProcess);
    return;
}

void bt_cmd_data_uart_recv_data(uart_read_trig_source_t trig_source)
{
    uint8_t rx_fifo_have_data = 0;
    static uint8_t have_data_no_process = 0;
    static uart_read_trig_source_t trig_source_last = UART_READ_TRIG_SOURCE_TIMER;
    static uint32_t trig_timer_cnt = 0;

    while (apUART_Check_RXFIFO_EMPTY(COM_PORT) != 1) {
        char c = COM_PORT->DataRead;
        bt_cmd_data_uart_rx_data_fun(c);
        rx_fifo_have_data = 1;
    }

    if (trig_source == UART_READ_TRIG_SOURCE_TIMER) {
        if (trig_timer_cnt <= TMR_CLK_1S_COUNT * 100) {
            trig_timer_cnt++;
        }
    } else {
        trig_timer_cnt = 0;
    }

    if (rx_fifo_have_data) { // trig by isr or timer
        if (trig_source == UART_READ_TRIG_SOURCE_ISR) {
            give_sem_send_data_by_ble();
        } else {
            if (trig_source_last == UART_READ_TRIG_SOURCE_ISR) {
                give_sem_send_data_by_ble();
            } else {
                have_data_no_process = 1;
            }
        }
    } else { // trig by timer
        if (have_data_no_process) {
            if (trig_timer_cnt >= TMR_CLK_1S_COUNT) {
                give_sem_process_cmd();
            } else {
                give_sem_send_data_by_ble();
            }
            have_data_no_process = 0;
        }
    }

    trig_source_last = trig_source;

    return;
}

static void main_cmd_data_timer_task(TimerHandle_t xTimer)
{
    bt_cmd_data_uart_recv_data(UART_READ_TRIG_SOURCE_TIMER);

    return;
}

static void bt_cmd_data_port_check_buf_into_lowpower(void)
{
     dbg_printf("a ");
     if (ble_buf.buf_data_bottom_count != ble_buf.buf_data_top_count) {
         //give_sem_send_data_by_com();
         dbg_printf("1 ");
         return;
     }
     if ((apUART_Check_RXFIFO_EMPTY(COM_PORT) != 1)
         || (com_buf.buf_data_bottom_count != com_buf.buf_data_top_count)
         ) {
         dbg_printf("2 ");
         return;
     }
     while (apUART_Check_TXFIFO_EMPTY(COM_PORT) != 1) {};
     g_sys_work_status_bits = g_sys_work_status_bits & (0x1 << LOW_POWER_EXIT_REASON_COM_TX_DATA);
     dbg_printf("3 ");
     return;
}

static void cmd_process_task(void *pdata)
{
    for (;;)
    {
        BaseType_t r = xSemaphoreTake(sem_cmd_process,  portMAX_DELAY);
        if (pdTRUE != r) continue;

        dbg_printf("[dt_pcs]: 0x%x 0x%x\r\n", com_buf.process_state, ble_buf.process_state);

        if ((com_buf.process_state == 0) && (ble_buf.process_state == 0)) {
            bt_cmd_data_port_check_buf_into_lowpower();
            continue;
        }

        if (com_buf.process_state & (0x1 << CMD_DATA_PORT_PROCESS_START_PARSE_CMD)) {
            bt_cmd_data_uart_rx_cmd_process();
            com_buf.process_state &= (~(0x1 << CMD_DATA_PORT_PROCESS_START_PARSE_CMD));
        }

        if (com_buf.process_state & (0x1 << CMD_DATA_PORT_PROCESS_START_SEND_BY_BLE)) {
            btstack_push_user_msg(USER_MSG_ID_UART_RX_TO_BLE_DATA_SERVICE, BT_PRIVT_NULL, 0);
            com_buf.process_state &= (~(0x1 << CMD_DATA_PORT_PROCESS_START_SEND_BY_BLE));
            //dbg_printf("[dt_pcs]: send by ble\r\n");
        }

        if (ble_buf.process_state & (0x1 << CMD_DATA_PORT_PROCESS_START_SEND_BY_COM)) {
            bt_cmd_data_uart_handle_task_send_data();
            ble_buf.process_state &= (~(0x1 << CMD_DATA_PORT_PROCESS_START_SEND_BY_COM));
        }
    }
}

static TimerHandle_t m_main_cmd_data_timer = 0;
static int m_main_cmd_data_timer_state = BT_PRIVT_DISNABLE;
void timer_start_main_cmd_data(void)
{
    if (m_main_cmd_data_timer_state != BT_PRIVT_ENABLE) {
        xTimerStart(m_main_cmd_data_timer, portMAX_DELAY);
        m_main_cmd_data_timer_state = BT_PRIVT_ENABLE;
    }

    return;
}

void timer_stop_main_cmd_data(void)
{
    if (m_main_cmd_data_timer_state != BT_PRIVT_DISNABLE) {
        xTimerStop(m_main_cmd_data_timer, portMAX_DELAY);
        m_main_cmd_data_timer_state = BT_PRIVT_DISNABLE;
    }

    return;
}

void bt_cmd_data_process_init(void)
{
    m_main_cmd_data_timer = xTimerCreate("main_cmd_data_task", pdMS_TO_TICKS(1000 / TMR_CLK_1S_COUNT), pdTRUE, NULL, main_cmd_data_timer_task);

    sem_cmd_process = xSemaphoreCreateBinary();
    xTaskCreate(cmd_process_task, "b", 150, NULL, (configMAX_PRIORITIES - configMAX_PRIORITIES + 3), NULL);

    com_buf.process_state = 0;
    ble_buf.process_state = 0;

    return;
}

#if defined __cplusplus
    }
#endif

