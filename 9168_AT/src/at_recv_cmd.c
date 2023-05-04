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
#include "private_user_packet_handler.h"
#include "profile.h"
#include "at_cmd_task.h"
#include <stdbool.h>
#include <stdlib.h>
#include "btstack_event.h"

#include "at_profile_spss.h"
#include "at_profile_spsc.h"

#include "common/flash_data.h"
#include "util/circular_queue.h"

#include "gatt_client.h"
#include "att_dispatch.h"
#include "btstack_defines.h"

#ifndef TMR_CLK_FREQ
#define TMR_CLK_FREQ 112000000
#endif

#define TMR1_CLK_1S_COUNT 10000
#define TMR1_CLK_CMP (TMR_CLK_FREQ / TMR1_CLK_1S_COUNT)

uint32_t rx_num = 0;

#define AT_UART    APB_UART1

uint32_t timer_isr_count = 0;

struct at_env gAT_env = {0};

TimerHandle_t at_transparent_timer = 0;
TimerHandle_t at_exit_transparent_mode_timer = 0;


bool at_buffer_full()
{
    return (gAT_env.at_recv_buffer_wp + 1) % AT_RECV_MAX_LEN == gAT_env.at_recv_buffer_rp;
}
bool at_buffer_empty()
{
    return gAT_env.at_recv_buffer_wp == gAT_env.at_recv_buffer_rp;
}
uint16_t at_buffer_data_size()
{
    int32_t len = gAT_env.at_recv_buffer_wp - gAT_env.at_recv_buffer_rp;
    return len >= 0 ? len : len + AT_RECV_MAX_LEN;
}
void at_buffer_enqueue_data(uint8_t data)
{
    gAT_env.at_recv_buffer[gAT_env.at_recv_buffer_wp++] = data;
    
    if (gAT_env.at_recv_buffer_wp == AT_RECV_MAX_LEN)
        gAT_env.at_recv_buffer_wp = 0;
}
uint8_t at_buffer_dequeue_data(void)
{
    uint8_t data = gAT_env.at_recv_buffer[gAT_env.at_recv_buffer_rp++];
    
    if (gAT_env.at_recv_buffer_rp == AT_RECV_MAX_LEN)
        gAT_env.at_recv_buffer_rp = 0;
    return data;
}
uint8_t at_buffer_read_data(uint16_t index) 
{
    index += gAT_env.at_recv_buffer_rp;
    if (index >= AT_RECV_MAX_LEN)
        index -= AT_RECV_MAX_LEN;
    return gAT_env.at_recv_buffer[index];
}


/*********************************************************************
 * @fn      uart_put_data_noint
 *
 * @brief   uart send data
 *			
 *
 * @param   pBase 
 * @param   data 
 * @param   size 
 *       	 
 *
 * @return  None
 */
void uart_put_data_noint(UART_TypeDef* pBase, uint8_t* data, uint32_t size)
{
    for (uint32_t i = 0; i < size; ++i)
    {
        while (apUART_Check_TXFIFO_FULL(pBase) == 1){}
        UART_SendData(pBase, data[i]);
    }
}

/*********************************************************************
 * @fn      uart_finish_transfers
 *
 * @brief   wait for tx fifo empty.
 *
 * @param   uart_addr   - which uart will be checked, UART0 or UART1.
 *
 * @return  None.
 */
void uart_finish_transfers(UART_TypeDef* pBase)
{
    while (apUART_Check_TXFIFO_EMPTY(pBase) == 0){}
}

/*********************************************************************
 * @fn      uart_putc_noint
 *
 * @brief   uart send c
 *			
 *
 * @param   pBase 
 * @param   data 
 *       	 
 *
 * @return  None
 */
void uart_putc_noint(UART_TypeDef* pBase, uint8_t data)
{
    while (apUART_Check_TXFIFO_FULL(pBase) == 1){}
    UART_SendData(pBase, data);
}

/*********************************************************************
 * @fn      at_clr_uart_buff
 *
 * @brief   Reset uart char receive index to zero.
 *			
 *
 * @param   None
 *       	 
 *
 * @return  None
 */
void at_clr_uart_buff(void)
{
    gAT_env.at_recv_buffer_wp = gAT_env.at_recv_buffer_rp = 0;
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
    if( gAT_env.transparent_data_send_ongoing == 0 )
    {
        gAT_env.transparent_data_send_ongoing = 1;

        btstack_push_user_msg(USER_MSG_AT_RECV_TRANSPARENT_DATA, NULL, 0);
    }
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
   if( at_buffer_empty() )
   {
        xTimerStop(at_transparent_timer, portMAX_DELAY);
        gAT_ctrl_env.transparent_start = false;
        gAT_env.at_recv_buffer_wp = gAT_env.at_recv_buffer_rp = 0;
        //spss_recv_data_ind_func = NULL;
        //spsc_recv_data_ind_func = NULL;
        uint8_t at_rsp[] = "OK";
        at_send_rsp((char *)at_rsp);
   }
   else{
        uint8_t at_rsp[] = "ERROR";
        at_send_rsp((char *)at_rsp);
   }
}
void uart_init(uint32_t freq, uint32_t baud)
{
    //初始化串口1,置串口接收中断标志位
    apUART_Initialize(APB_UART1, &g_power_off_save_data_in_ram.uart_param, (1 << bsUART_RECEIVE_INTENAB) );//1 << bsUART_RECEIVE_INTENAB);
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

//void show_heap(void)
//{
//    static char buffer[200];
//    platform_heap_status_t status;
//    platform_get_heap_status(&status);
//    sprintf(buffer, "heap status:\n"
//                    "    free: %d B\n"
//                    "min free: %d B\n", status.bytes_free, status.bytes_minimum_ever_free);
//    LOG_MSG(buffer, strlen(buffer) + 1);
//}

int os_get_free_heap_size()
{
    platform_heap_status_t status;
    platform_get_heap_status(&status);
    return status.bytes_free;
}

/*********************************************************************
 * @fn      recv_transparent_data
 *
 * @brief   process transparent data
 *			
 *
 * @param   None
 *       	 
 *
 * @return  None
 */
void recv_transparent_data()
{
    xTimerStop(at_transparent_timer, portMAX_DELAY);
    
    //__disable_irq();
    if(gap_get_connect_status(gAT_ctrl_env.transparent_conidx))
    {
        if( os_get_free_heap_size()>2000)
        {
            if(g_power_off_save_data_in_ram.peer_param[gAT_ctrl_env.transparent_conidx].link_mode == SLAVE_ROLE)
                at_spss_send_data(gAT_ctrl_env.transparent_conidx);
            else if(g_power_off_save_data_in_ram.peer_param[gAT_ctrl_env.transparent_conidx].link_mode == MASTER_ROLE)
                at_spsc_send_data(gAT_ctrl_env.transparent_conidx);
        }
        else
            uart_putc_noint(UART1,'X');
    }
    //__enable_irq();
    
    gAT_env.transparent_data_send_ongoing = 0;
    
    if(gAT_ctrl_env.one_slot_send_start && gAT_ctrl_env.one_slot_send_len == 0)
    {
        gAT_ctrl_env.one_slot_send_start = false;
        gAT_ctrl_env.one_slot_send_len = 0;
        uint8_t at_rsp[] = "SEND OK";
        at_send_rsp((char *)at_rsp);
    }
}


void send_data(void)
{
    uint16_t send_packet_len = 0;
    uint8_t r;

    gatt_client_get_mtu(gAT_ctrl_env.transparent_conidx, &send_packet_len);
    send_packet_len -= 3;

    do
    {
        r = gatt_client_write_value_of_characteristic_without_response(gAT_ctrl_env.transparent_conidx,
                                                                       slave_input_char.value_handle,
                                                                       send_packet_len, (uint8_t *)0x4000);

        switch (r)
        {
            case 0:
                break;
            case BTSTACK_ACL_BUFFERS_FULL:
                att_dispatch_client_request_can_send_now_event(gAT_ctrl_env.transparent_conidx);
                break;
        }
    } while (0 == r);
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
        if(at_buffer_empty())
            btstack_push_user_msg(USER_MSG_AT_TRANSPARENT_START_TIMER, NULL, 0);
        
        rx_num++;
        at_buffer_enqueue_data(c);
 
        if(at_buffer_data_size() ==3)//连续收到3个+++，退出透传
        {
            
            if(    at_buffer_read_data(at_buffer_data_size() - 1) == '+'
                && at_buffer_read_data(at_buffer_data_size() - 2) == '+'
                && at_buffer_read_data(at_buffer_data_size() - 3) == '+')
            {
                BaseType_t xHigherPriorityTaskWoke = pdFALSE;
                xTimerStartFromISR(at_exit_transparent_mode_timer, &xHigherPriorityTaskWoke);
                if (xHigherPriorityTaskWoke == pdTRUE){}
            }
        }
        
        if( (at_buffer_data_size() >AT_TRANSPARENT_DOWN_LEVEL) && (gAT_env.transparent_data_send_ongoing == 0) )
        {
            gAT_env.transparent_data_send_ongoing = 1;
            btstack_push_user_msg(USER_MSG_AT_RECV_TRANSPARENT_DATA, NULL, 0);
        }
        
        if (at_buffer_full())
        {
            at_buffer_dequeue_data();   // discard.
        }
        goto _exit;
    }
    
    if(gAT_ctrl_env.one_slot_send_start)        // one shot send
    {
        if(gAT_ctrl_env.one_slot_send_len > 0)
        {
            if( at_buffer_empty() )
            {
                btstack_push_user_msg(USER_MSG_AT_TRANSPARENT_START_TIMER, NULL, 0);
            }
            if( at_buffer_data_size() < (AT_RECV_MAX_LEN-2) )
                at_buffer_enqueue_data(c);
            gAT_ctrl_env.one_slot_send_len--;

            if( ((at_buffer_data_size() >AT_TRANSPARENT_DOWN_LEVEL) && (gAT_env.transparent_data_send_ongoing == 0))
                || (gAT_ctrl_env.one_slot_send_len == 0)
              )
            {
                gAT_env.transparent_data_send_ongoing = 1;
                btstack_push_user_msg(USER_MSG_AT_RECV_TRANSPARENT_DATA, NULL, 0);
            }
        }
        goto _exit;
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
            at_buffer_enqueue_data(c);
        
            if(  (c == '\n') && (at_buffer_read_data(at_buffer_data_size() - 2) == '\r') )
            {
                struct recv_cmd_t *cmd = (struct recv_cmd_t *)malloc(sizeof(struct recv_cmd_t)+(at_buffer_data_size()+1));
                cmd->recv_length = at_buffer_data_size()+1;
                
                // 
                if (gAT_env.at_recv_buffer_rp <= gAT_env.at_recv_buffer_wp)
                {
                    memcpy(cmd->recv_data, gAT_env.at_recv_buffer + gAT_env.at_recv_buffer_rp, at_buffer_data_size());
                }
                else
                {
                    uint16_t part1_size = AT_RECV_MAX_LEN - gAT_env.at_recv_buffer_wp;
                    memcpy(cmd->recv_data, gAT_env.at_recv_buffer + gAT_env.at_recv_buffer_wp, part1_size);
                    memcpy(cmd->recv_data + part1_size, gAT_env.at_recv_buffer, gAT_env.at_recv_buffer_wp + 1);
                }
                
                memcpy(&cmd->recv_data[0], &gAT_env.at_recv_buffer[0], cmd->recv_length);
                
                btstack_push_user_msg(USER_MSG_AT_RECV_CMD, cmd, sizeof(struct recv_cmd_t) + cmd->recv_length);
                
                //free(cmd);
                gAT_env.at_recv_state = 0;
                gAT_env.at_recv_buffer_wp = gAT_env.at_recv_buffer_rp = 0;
            }
            else
            {
                if (at_buffer_full())
                {
                    LOG_MSG("buffer full\r\n");
                    uint8_t at_rsp[] = "buffer full";
                    at_send_rsp((char *)at_rsp);
                    gAT_env.at_recv_state = 0;
                    gAT_env.at_recv_buffer_wp = gAT_env.at_recv_buffer_rp = 0;
                }
            }
            break;
    }
_exit:
    ;
}

/*********************************************************************
 * @fn      at_store_info_to_flash
 *
 * @brief   Store AT infomations to flash, AT+FLASH cmd will carry out this function
 *			
 *
 * @param   None 
 *       	 
 *
 * @return  None
 */
void at_store_info_to_flash(void)
{
    sdk_private_data_write_to_flash();
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
                app_at_recv_c(AT_UART->DataRead);
            }
        }
    }
    return 0;
}

extern uint32_t send_num;
uint32_t timer_isr_counter2 = 0;

// Timer 中断
static uint32_t bt_cmd_data_timer_isr(void *user_data)
{
    TMR_IntClr(APB_TMR1, 0, 0x1);
    
    //// uart rx fifo >>> gAT_env.at_recv_buffer
    //while (apUART_Check_RXFIFO_EMPTY(APB_UART1) != 1) 
    //{
    //    if (at_buffer_data_size() >= AT_RECV_MAX_LEN)
    //        break;
    //    app_at_recv_c(APB_UART1->DataRead);
    //}
    
    timer_isr_count ++;
    timer_isr_counter2 ++;
    if (timer_isr_counter2 >= 1)
    {
        timer_isr_counter2 = 0;
        LOG_MSG("rx_num:%d, send_num:%d\n", rx_num, send_num);
        rx_num = 0;
        send_num = 0;
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
    platform_set_irq_callback(PLATFORM_CB_IRQ_UART1, uart_isr, NULL); //暂不采用串口中断的方式
    
    //3.用于透传的控制
    if (at_transparent_timer == 0) 
        at_transparent_timer = xTimerCreate("t1",
                pdMS_TO_TICKS(50),
                pdFALSE,
                NULL,
                transparent_timer_handler);
    
    //
    if (at_exit_transparent_mode_timer == 0)
        at_exit_transparent_mode_timer = xTimerCreate("t2",
            pdMS_TO_TICKS(500),
            pdFALSE,
            NULL,
            exit_trans_tim_fn);
    
    //4.Timer查询串口数据
    SYSCTRL_ClearClkGateMulti(  (1 << SYSCTRL_ClkGate_APB_TMR1));
    TMR_SetOpMode(APB_TMR1, 0, TMR_CTL_OP_MODE_32BIT_TIMER_x1, TMR_CLK_MODE_APB, 0);
    TMR_SetReload(APB_TMR1, 0, TMR_CLK_FREQ);//TMR1_CLK_CMP);
    TMR_Enable(APB_TMR1, 0, 0x1);
    TMR_IntEnable(APB_TMR1, 0, 0x1);
    platform_set_irq_callback(PLATFORM_CB_IRQ_TIMER1, bt_cmd_data_timer_isr, NULL);
    
}