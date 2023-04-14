#include "at_recv_cmd.h"
#include "platform_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include <stdio.h>

#include "util/rtos_util.h"
#include "util/buffer.h"

#include "util/circular_queue.h"
#include "router.h"
#include "private_user_packet_handler.h"
#include "profile.h"
#include "at_cmd_task.h"
#include <stdbool.h>

#ifndef TMR_CLK_FREQ
#define TMR_CLK_FREQ 112000000
#endif

#define TMR1_CLK_1S_COUNT 5000
#define TMR1_CLK_FLOW_CONTROL_COUNT (TMR1_CLK_1S_COUNT / 1)
#define TMR1_CLK_CMP (TMR_CLK_FREQ / TMR1_CLK_1S_COUNT)

#define AT_UART    APB_UART1

#define AT_RECV_MAX_LEN     244
#define AT_TRANSPARENT_DOWN_LEVEL   (AT_RECV_MAX_LEN - 40)
struct at_env
{
    uint8_t at_recv_buffer[AT_RECV_MAX_LEN];
    uint8_t at_recv_index;
    uint8_t at_recv_state;
    uint16_t at_task_id;
    uint8_t transparent_data_send_ongoing;
    uint8_t upgrade_data_processing;
    uint16_t transparent_timer;       //recv timer out timer.    50ms
    uint16_t exit_transparent_mode_timer;     //send "+++" ,then 500ms later, exit transparent mode;
} gAT_env = {0};

static TimerHandle_t at_transparent_timer = 0;
static TimerHandle_t at_exit_transparent_mode_timer = 0;

extern circular_queue_t* buffer;


static void uart_handle_task_data_or_cmd_process()
{
    static uint32_t last_rx_input_buffer_data_len = 0;
    static uint32_t no_change_cnt = 0;
    
    uint32_t data_len = circular_queue_get_elem_num(buffer);
    
    if (data_len == 0)
        return;
    
    if (last_rx_input_buffer_data_len == data_len)
        no_change_cnt ++;
    else
        no_change_cnt = 0;
    
    last_rx_input_buffer_data_len = data_len;
    
    if (no_change_cnt >= (TMR1_CLK_1S_COUNT / 10)) {
        no_change_cnt = 0;
        process_rx_port_data();
    }
}
static void bt_cmd_data_uart_io_timer_init(void)
{

#if (INGCHIPS_FAMILY == INGCHIPS_FAMILY_918)
    SYSCTRL_ClearClkGateMulti((1 << SYSCTRL_ClkGate_APB_TMR1));
    TMR_SetCMP(APB_TMR1, TMR1_CLK_CMP);
    TMR_SetOpMode(APB_TMR1, TMR_CTL_OP_MODE_WRAPPING);
    TMR_IntEnable(APB_TMR1);
    TMR_Reload(APB_TMR1);
    TMR_Enable(APB_TMR1);
#elif (INGCHIPS_FAMILY == INGCHIPS_FAMILY_916)

    SYSCTRL_ClearClkGateMulti(  (1 << SYSCTRL_ClkGate_APB_TMR1));

    TMR_SetOpMode(APB_TMR1, 0, TMR_CTL_OP_MODE_32BIT_TIMER_x1, TMR_CLK_MODE_APB, 0);
    TMR_SetReload(APB_TMR1, 0, TMR1_CLK_CMP);
    TMR_Enable(APB_TMR1, 0, 0x1);
    TMR_IntEnable(APB_TMR1, 0, 0x1);
#else
    #error unknown or unsupported chip family
#endif
   
    return;
}
/*********************************************************************
 * @fn      transparent_timer_handler
 *
 * @brief   Timer handle function, when uart received char numbers less than AT_TRANSPARENT_DOWN_LEVEL for 50ms,
 *			transparent_timer will be timer out, and call this function to send uart buffed data immediately
 *
 * @param   arg - parameter for timer handle 
 *       	 
 *
 * @return  None
 */
void transparent_timer_handler(TimerHandle_t xTimer)
{
    //LOG_INFO("r_Tout\r\n");
//    if( gAT_env.transparent_data_send_ongoing == 0 )
//    {
//        gAT_env.transparent_data_send_ongoing = 1;
//        os_event_t evt;
//        evt.event_id = AT_RECV_TRANSPARENT_DATA;
//        evt.param_len = 0;
//        evt.param = NULL;
//        evt.src_task_id = TASK_ID_NONE;
//        os_msg_post(gAT_env.at_task_id,&evt);
//    }
}

/*********************************************************************
 * @fn      exit_trans_tim_fn
 *
 * @brief   Timer handle function for gAT_env.exit_transparent_mode_timer, if +++ is received, and there is no char comming for 500ms,
 *			this function will be carried on, and transparent mode will end.
 *
 * @param   arg - parameter for timer handle 
 *       	 
 *
 * @return  None
 */
void exit_trans_tim_fn(TimerHandle_t xTimer)
{
//    os_timer_stop(&gAT_env.transparent_timer);
//    gAT_ctrl_env.transparent_start = 0;
//    gAT_env.at_recv_index = 0;
//    //spss_recv_data_ind_func = NULL;
//    //spsc_recv_data_ind_func = NULL;
//    uint8_t at_rsp[] = "OK";
//    at_send_rsp((char *)at_rsp);
}
void uart_init(uint32_t freq, uint32_t baud)
{
    UART_sStateStruct config;

    config.word_length       = UART_WLEN_8_BITS;
    config.parity            = UART_PARITY_NOT_CHECK;
    config.fifo_enable       = 1;
    config.two_stop_bits     = 0;
    config.receive_en        = 1;
    config.transmit_en       = 1;
    config.UART_en           = 1;
    config.cts_en            = 1;
    config.rts_en            = 1;
    config.rxfifo_waterlevel = 1;
    config.txfifo_waterlevel = 1;
    config.ClockFrequency    = freq;
    config.BaudRate          = baud;

    //初始化串口1,置串口接收中断标志位
    apUART_Initialize(APB_UART1, &config, (1 << bsUART_RECEIVE_INTENAB) );//1 << bsUART_RECEIVE_INTENAB);
}
void uart_io_print(const char* buf) 
{
    uint32_t index = 0;
    while (buf[index] != '\0') {
        while (apUART_Check_TXFIFO_FULL(APB_UART1) == 1){}
        UART_SendData(APB_UART1, buf[index]);
        index++;
    }
}
void uart_io_send(const uint8_t* buf, uint32_t size) 
{
    uint8_t i = 0;
    for (;i < size; ++i)
    {
        while (apUART_Check_TXFIFO_FULL(APB_UART1) == 1){}
        UART_SendData(APB_UART1, buf[i]);
    }
}



int at_task_func()
{




}
/*********************************************************************
 * @fn      app_at_recv_c
 *
 * @brief   process uart buffer further, this function is call in UART interruption
 *			
 *
 * @param   c - character received from uart FIFO
 *       	 
 *
 * @return  None
 */
static void app_at_recv_c(uint8_t c)
{
    //AT_LOG("%02x ",c);
    if(gAT_ctrl_env.transparent_start)
    {
        if(gAT_env.at_recv_index == 0)
        {

        }
        if( gAT_env.at_recv_index < (AT_RECV_MAX_LEN-2) )
            gAT_env.at_recv_buffer[gAT_env.at_recv_index++] = c;



        if( (gAT_env.at_recv_index >AT_TRANSPARENT_DOWN_LEVEL) && (gAT_env.transparent_data_send_ongoing == 0) )
        {

        }
        goto _exit;
    }

    if(gAT_ctrl_env.one_slot_send_start)        // one slot send
    {

    }

    if(gAT_ctrl_env.upgrade_start == true)
    {
        /*
        if( gAT_env.upgrade_data_processing == 0 )
        {
            if( gAT_env.at_recv_index < (AT_RECV_MAX_LEN-2) )
            {
                gAT_env.at_recv_buffer[gAT_env.at_recv_index++] = c;
                uint8_t chk_ret = check_whole_pkt_in_upgrade_mode();
                if( chk_ret > 0 )
                {
                    if(chk_ret == 0xff)
                    {
                        gAT_env.at_recv_index = 0;
                        goto _exit;
                    }
                    gAT_env.upgrade_data_processing = 1;
                    os_event_t evt;
                    evt.event_id = AT_RECV_UPGRADE_DATA;
                    evt.param_len = 0;
                    evt.param = NULL;
                    if( chk_ret == 1 )
                        evt.src_task_id = (TASK_ID_NONE-1);
                    else if (chk_ret == 2)
                        evt.src_task_id = TASK_ID_NONE;
                    os_msg_post(gAT_env.at_task_id,&evt);
                }
            }
            else
                gAT_env.at_recv_index = 0;
        }
        goto _exit;
        */
    }

    switch(gAT_env.at_recv_state)
    {
        case 0:
            if(c == 'A')
            {
                gAT_env.at_recv_state++;
                //system_prevent_sleep_set();
            }
            break;
        case 1:
            if(c == 'T')
                gAT_env.at_recv_state++;
            else
                gAT_env.at_recv_state = 0;
            break;
        case 2:
            if(c == '+')
                gAT_env.at_recv_state++;
            else
                gAT_env.at_recv_state = 0;
            break;
        case 3:
            gAT_env.at_recv_buffer[gAT_env.at_recv_index] = c;
            if(  (c == '\n') && (gAT_env.at_recv_buffer[gAT_env.at_recv_index-1] == '\r') )
            {

            }
            else
            {
                gAT_env.at_recv_index++;
                if(gAT_env.at_recv_index >= AT_RECV_MAX_LEN)
                {
                    gAT_env.at_recv_state = 0;
                    gAT_env.at_recv_index = 0;
                }
            }
            break;
    }
_exit:
    ;
}

//串口1的中断
uint32_t uart_isr(void *user_data)
{
    uint32_t status;

    while(1)
    {
        status = apUART_Get_all_raw_int_stat(AT_UART);
        if (status == 0)
            break;

        AT_UART->IntClear = status;

        // rx int
        if (status & (1 << bsUART_RECEIVE_INTENAB))
        {
            while (apUART_Check_RXFIFO_EMPTY(AT_UART) != 1)
            {
                //char c = AT_UART->DataRead;
                app_at_recv_c(AT_UART->DataRead);
            }
        }
    }
    return 0;
}
void at_init(void)
{  
    //1.配置串口的IO口 
    SYSCTRL_ClearClkGateMulti(  (1 << SYSCTRL_ITEM_APB_UART1));
    PINCTRL_SetPadMux(USER_UART_AT_RX, IO_SOURCE_GENERAL);
    PINCTRL_SetPadMux(USER_UART_AT_TX, IO_SOURCE_UART1_TXD);
    PINCTRL_SetPadMux(USER_UART_AT_RTS, IO_SOURCE_UART1_RTS);
    
    PINCTRL_Pull(USER_UART_AT_RX, PINCTRL_PULL_UP);
    PINCTRL_SelUartRxdIn(UART_PORT_1, USER_UART_AT_RX);
    
    PINCTRL_Pull(USER_UART_AT_CTS, PINCTRL_PULL_DOWN);
    PINCTRL_SelUartCtsIn(UART_PORT_1, USER_UART_AT_CTS);
    
    //2.UART的串口参数初始化
    uart_init(OSC_CLK_FREQ, 115200);
    platform_set_irq_callback(PLATFORM_CB_IRQ_UART1, uart_isr, NULL);
    
    at_transparent_timer = xTimerCreate("t1",
            pdMS_TO_TICKS(50),
            pdTRUE,
            NULL,
            transparent_timer_handler);
            
    at_exit_transparent_mode_timer = xTimerCreate("t2",
        pdMS_TO_TICKS(500),
        pdTRUE,
        NULL,
        exit_trans_tim_fn);
}