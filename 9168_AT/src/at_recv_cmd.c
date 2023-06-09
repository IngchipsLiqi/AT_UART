#include "at_recv_cmd.h"
#include "platform_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>


#include "profile.h"
#include "at_cmd_task.h"
#include <stdbool.h>
#include <stdlib.h>
#include "btstack_event.h"

#include "common/flash_data.h"

#include "gatt_client.h"
#include "att_dispatch.h"
#include "att_db.h"
#include "btstack_defines.h"

struct at_env gAT_env = {0};

bool can_send_now = true;

uint32_t timer_isr_count = 0;

extern uint16_t g_ble_output_handle;

/*********************************************************************
 * at_buffer
 * - 循环队列，连续内存空间（数组式）
 * - 存在一个读指针和一个写指针
 * - 写入数据时写指针往后移动
 * - 读取输入时读指针往后移动
 * - 读/写指针移动到数组长度减 1 的位置时，下一次移动会回到 0 的位置
 * - 读指针与写指针重合时，表示缓冲队列为空
 * - 读指针等于写指针加 1 时，表示缓冲队列为满
 * 
**********************************************************************/
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
/*********************************************************************/


/*********************************************************************
 * @fn      uart_io_print
 * @return  None.
 */
void uart_io_print(const char* buf) 
{
    uint32_t index = 0;
    while (buf[index] != '\0') {
        while (apUART_Check_TXFIFO_FULL(APB_UART0) == 1){}
        UART_SendData(APB_UART0, buf[index]);
        index++;
    }
}


/*********************************************************************
 * @fn      uart_io_send
 * @return  None.
 */
void uart_io_send(const uint8_t* buf, uint32_t size) 
{
    uint8_t i = 0;
    for (;i < size; ++i)
    {
        while (apUART_Check_TXFIFO_FULL(APB_UART0) == 1){}
        UART_SendData(APB_UART0, buf[i]);
    }
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
        xTimerStop(gAT_env.transparent_timer, portMAX_DELAY);
        gAT_ctrl_env.transparent_start = false;
        gAT_env.at_recv_buffer_wp = gAT_env.at_recv_buffer_rp = 0;
        uint8_t at_rsp[] = "OK";
        at_send_rsp((char *)at_rsp);
   }
   else{
        uint8_t at_rsp[] = "ERROR";
        at_send_rsp((char *)at_rsp);
   }
}

/*********************************************************************
 * @fn      os_get_free_heap_size
 * @return  free heap size
 */
int os_get_free_heap_size()
{
    platform_heap_status_t status;
    platform_get_heap_status(&status);
    return status.bytes_free;
}


/*********************************************************************
 * @fn      at_spss_send_data
 *
 * @brief   function to write date to peer. notification
 *
 * @param   conidx - link  index
 *
 * @return  None
 */
void at_spss_send_data(uint8_t conn_handle)
{
    // can_send_now为false代表蓝牙协议栈的发送缓冲以满
    // 需要等待协议栈处理缓冲中的数据
    if (!can_send_now)
        return;
    
    // notify_enable表示主机有无订阅(使能)透传服务的notify
    if (!gAT_ctrl_env.transparent_notify_enable)
        return;
    
    uint16_t send_packet_len = 0;
    uint16_t status = 0;
    uint16_t this_time_send_len = 0;
    uint8_t* p_send_data = NULL;
    
    // 查询MTU
    send_packet_len = att_server_get_mtu(conn_handle) - 3;
    
    // 查询协议栈如果可以发包
    while (att_server_can_send_packet_now(conn_handle))
    {
        // 计算本次传输长度
        this_time_send_len = at_buffer_data_size();
        
        // 如果数据全部发完了，退出函数
        if (this_time_send_len == 0)
            goto exit;
        
        if (this_time_send_len > send_packet_len)
            this_time_send_len = send_packet_len;
        if (gAT_env.at_recv_buffer_rp + this_time_send_len >= AT_RECV_MAX_LEN)
            this_time_send_len = AT_RECV_MAX_LEN - gAT_env.at_recv_buffer_rp;
        
        // 待传输数据的源地址
        p_send_data = gAT_env.at_recv_buffer + gAT_env.at_recv_buffer_rp;
        
        // 协议栈notify API
        status = att_server_notify(conn_handle, g_ble_output_handle, p_send_data, this_time_send_len);
        
        if (status == 0)
        {
            // 成功就将数据从缓冲队列中弹出
            gAT_env.at_recv_buffer_rp += this_time_send_len;
            if (gAT_env.at_recv_buffer_rp >= AT_RECV_MAX_LEN)
                gAT_env.at_recv_buffer_rp = gAT_env.at_recv_buffer_rp - AT_RECV_MAX_LEN;
        }
        else
        {
            LOG_MSG("Error:att_server_notify:%d\n", status);
        }
    }

    // 蓝牙协议栈缓冲满了，需要请求CAN_SEND_NOW事件
    // CAN_SEND_NOW事件会在发送缓冲有余的情况下通知应用层可以继续发送了
    att_server_request_can_send_now_event(conn_handle);
    
exit:
    ;
}

/*********************************************************************
 * @fn      at_spsc_send_data
 *
 * @brief   function to write date to peer. write without response
 *
 * @param   conidx - link  index
 *
 * @return  None
 */
void at_spsc_send_data(uint8_t conn_handle)
{
    // can_send_now为false代表蓝牙协议栈的发送缓冲以满
    // 需要等待协议栈处理缓冲中的数据
    
    if (!can_send_now)
        return;
    
    uint16_t send_packet_len = 0;
    uint16_t status = 0;
    uint16_t valid_data = 0;
    uint16_t this_time_send_len = 0;
    uint8_t* p_send_data = NULL;
    
    // 查询MTU
    gatt_client_get_mtu(conn_handle, &send_packet_len);
    send_packet_len -= 3;
    
    while ((valid_data = at_buffer_data_size()) > 0) 
    {
        // 计算本次传输长度
        this_time_send_len = valid_data;
        if (this_time_send_len > send_packet_len)
            this_time_send_len = send_packet_len;
        if (gAT_env.at_recv_buffer_rp + this_time_send_len >= AT_RECV_MAX_LEN)
            this_time_send_len = AT_RECV_MAX_LEN - gAT_env.at_recv_buffer_rp;
        
        // 待传输数据的源地址
        p_send_data = gAT_env.at_recv_buffer + gAT_env.at_recv_buffer_rp;
        
        if (this_time_send_len != 0)
        {
            // GATT发送API
            status = gatt_client_write_value_of_characteristic_without_response(conn_handle, 
                                                                                slave_input_char.value_handle, 
                                                                                this_time_send_len, p_send_data);

            // 成功添加到协议栈发送缓冲
            if (status == 0)
            {
                gAT_env.at_recv_buffer_rp += this_time_send_len;
                if (gAT_env.at_recv_buffer_rp >= AT_RECV_MAX_LEN)
                    gAT_env.at_recv_buffer_rp = gAT_env.at_recv_buffer_rp - AT_RECV_MAX_LEN;
            }
            // 蓝牙协议栈缓冲满了，需要请求CAN_SEND_NOW事件
            // CAN_SEND_NOW事件会在发送缓冲有余的情况下通知应用层可以继续发送了
            else if (status == BTSTACK_ACL_BUFFERS_FULL)
            {
                can_send_now = false;
                att_dispatch_client_request_can_send_now_event(conn_handle); // Request can send now
                break;
            }
        }
    }
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
    xTimerStop(gAT_env.transparent_timer, portMAX_DELAY);
    
    __disable_irq();
    
    //if( os_get_free_heap_size()>2000)
    //{
    //}
    //else
    //    uart_putc_noint(UART0,'X');
    
    if (get_handle_by_id(gAT_ctrl_env.transparent_conidx) != INVALID_HANDLE)
    {
        if (gAT_ctrl_env.transparent_method == TRANSMETHOD_M2S_W)
            at_spsc_send_data(get_handle_by_id(gAT_ctrl_env.transparent_conidx));
        else
            at_spss_send_data(get_handle_by_id(gAT_ctrl_env.transparent_conidx));
    }
    else
    {
        at_clr_uart_buff();
    }
    __enable_irq();
    
    gAT_env.transparent_data_send_ongoing = 0;
    
    if(gAT_ctrl_env.one_slot_send_start && gAT_ctrl_env.one_slot_send_len == 0)
    {
        gAT_ctrl_env.one_slot_send_start = false;
        gAT_ctrl_env.one_slot_send_len = 0;
        uint8_t at_rsp[] = "SEND OK";
        at_send_rsp((char *)at_rsp);
    }
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
    // 透传模式
    if(gAT_ctrl_env.transparent_start)
    {
        // c存入时缓冲为空，延迟触发此次传输
        if(at_buffer_empty())
            btstack_push_user_msg(USER_MSG_AT_TRANSPARENT_START_TIMER, NULL, 0);
        
        // c存入缓冲
        at_buffer_enqueue_data(c);
 
        // 连续收到3个+，退出透传
        if(at_buffer_data_size() ==3)
        {
            if(    at_buffer_read_data(at_buffer_data_size() - 1) == '+'
                && at_buffer_read_data(at_buffer_data_size() - 2) == '+'
                && at_buffer_read_data(at_buffer_data_size() - 3) == '+')
            {
                
                
                BaseType_t xHigherPriorityTaskWoke = pdFALSE;
                xTimerStartFromISR(gAT_env.exit_transparent_mode_timer, &xHigherPriorityTaskWoke);
                if (xHigherPriorityTaskWoke == pdTRUE){}
            }
        }
        
        // c存入时，缓冲数据达到低水位，并且没有正在进行的传输时，立即触发传输
        if( (at_buffer_data_size() >AT_TRANSPARENT_DOWN_LEVEL) && (gAT_env.transparent_data_send_ongoing == 0) )
        {
            gAT_env.transparent_data_send_ongoing = 1;
            btstack_push_user_msg(USER_MSG_AT_RECV_TRANSPARENT_DATA, NULL, 0);
        }
        
        // c存入时，缓冲数据满了，立马丢弃缓冲队列头部的一个字节数据
        if (at_buffer_full())
            at_buffer_dequeue_data();
        goto _exit;
    }
    
    // 单次传输指定长度数据
    if(gAT_ctrl_env.one_slot_send_start)
    {
        if(gAT_ctrl_env.one_slot_send_len > 0)
        {
            // c存入时缓冲为空，延迟触发此次传输
            if( at_buffer_empty() )
                btstack_push_user_msg(USER_MSG_AT_TRANSPARENT_START_TIMER, NULL, 0);
            
            // c存入缓冲
            if( at_buffer_data_size() < (AT_RECV_MAX_LEN-2) )
                at_buffer_enqueue_data(c);
            gAT_ctrl_env.one_slot_send_len--;

            // c存入时，缓冲数据达到低水位，并且没有正在进行的传输时，立即触发传输
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

    // AT指令处理
    switch(gAT_env.at_recv_state)
    {
        case 0:
            if(c == 'A')
            {
                gAT_env.at_recv_state++;
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
                // 将AT指令封装成结构体recv_cmd_t，注意内存释放的时机
                struct recv_cmd_t *cmd = (struct recv_cmd_t *)malloc(sizeof(struct recv_cmd_t)+(at_buffer_data_size()+1));
                cmd->recv_length = at_buffer_data_size()+1;
                
                if (gAT_env.at_recv_buffer_rp <= gAT_env.at_recv_buffer_wp)
                {
                    memcpy(cmd->recv_data, gAT_env.at_recv_buffer + gAT_env.at_recv_buffer_rp, at_buffer_data_size());
                }
                else
                {
                    // 循环队列头尾数据拼接
                    uint16_t part1_size = AT_RECV_MAX_LEN - gAT_env.at_recv_buffer_wp;
                    memcpy(cmd->recv_data, gAT_env.at_recv_buffer + gAT_env.at_recv_buffer_wp, part1_size);
                    memcpy(cmd->recv_data + part1_size, gAT_env.at_recv_buffer, gAT_env.at_recv_buffer_wp + 1);
                }
                
                memcpy(&cmd->recv_data[0], &gAT_env.at_recv_buffer[0], cmd->recv_length);
                
                // 包装成用户消息交给蓝牙协议栈处理
                btstack_push_user_msg(USER_MSG_AT_RECV_CMD, cmd, sizeof(struct recv_cmd_t) + cmd->recv_length);
                
                gAT_env.at_recv_state = 0;
                gAT_env.at_recv_buffer_wp = gAT_env.at_recv_buffer_rp = 0;
            }
            else
            {
                // 缓冲满了直接清空
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

// Timer 中断
static uint32_t bt_cmd_data_timer_isr(void *user_data)
{
    TMR_IntClr(APB_TMR1, 0, 0x1);
    
    // UART rx fifo >>> gAT_env.at_recv_buffer
    while (apUART_Check_RXFIFO_EMPTY(UART0) != 1) 
    {
        if (at_buffer_data_size() >= AT_RECV_MAX_LEN)
            break;
        app_at_recv_c(UART0->DataRead);
    }
    
    timer_isr_count ++;
    
    return 0;
}

void at_uart_init(void)
{  
    //1.UART的串口参数初始化
    apUART_Initialize(APB_UART0, &g_power_off_save_data_in_ram.uart_param, 0);//1 << bsUART_RECEIVE_INTENAB);

    
    //2.透传-延时触发传输
    if (gAT_env.transparent_timer == 0) 
        gAT_env.transparent_timer = xTimerCreate("t1",
            pdMS_TO_TICKS(50),
            pdFALSE,
            NULL,
            transparent_timer_handler);
    
    //3.透传-退出透传
    if (gAT_env.exit_transparent_mode_timer == 0)
        gAT_env.exit_transparent_mode_timer = xTimerCreate("t2",
            pdMS_TO_TICKS(500),
            pdFALSE,
            NULL,
            exit_trans_tim_fn);
    
    //4.硬件Timer接收串口数据
    SYSCTRL_ClearClkGateMulti(  (1 << SYSCTRL_ClkGate_APB_TMR1));
    TMR_SetOpMode(APB_TMR1, 0, TMR_CTL_OP_MODE_32BIT_TIMER_x1, TMR_CLK_MODE_APB, 0);
    TMR_SetReload(APB_TMR1, 0, TMR1_CLK_CMP);
    TMR_Enable(APB_TMR1, 0, 0x1);
    TMR_IntEnable(APB_TMR1, 0, 0x1);
    platform_set_irq_callback(PLATFORM_CB_IRQ_TIMER1, bt_cmd_data_timer_isr, NULL);
    
}