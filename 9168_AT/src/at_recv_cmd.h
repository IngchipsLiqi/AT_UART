#ifndef __AT_RECV_CMD_H__
#define __AT_RECV_CMD_H__

#include "common/preh.h"
#include "FreeRTOS.h"
#include "timers.h"

#define UART0 APB_UART0

#define AT_MAIN_VER (0)


#define AT_RECV_MAX_LEN     4000
#define AT_TRANSPARENT_DOWN_LEVEL   205
struct at_env
{
    uint8_t at_recv_buffer[AT_RECV_MAX_LEN];
    uint16_t at_recv_buffer_rp;
    uint16_t at_recv_buffer_wp;
    
    //uint8_t at_recv_index;
    uint8_t at_recv_state;
    uint16_t at_task_id;
    uint8_t transparent_data_send_ongoing;
    uint8_t upgrade_data_processing;
    uint16_t transparent_timer;       //recv timer out timer.    50ms
    uint16_t exit_transparent_mode_timer;     //send "+++" ,then 500ms later, exit transparent mode;
};

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
void uart_putc_noint(UART_TypeDef* pBase, uint8_t data);

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
void uart_put_data_noint(UART_TypeDef* pBase, uint8_t* data, uint32_t size);
 
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
void recv_transparent_data();
 
/*********************************************************************
 * @fn      at_init
 *
 * @brief   Initializate gAT_env elements and assign UART0 pins
 *			
 *
 * @param   None 
 *       	 
 *
 * @return  None
 */
void at_init(void);

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
void at_clr_uart_buff(void);


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
void at_store_info_to_flash(void);


/*********************************************************************
 * @fn      uart_finish_transfers
 *
 * @brief   wait for tx fifo empty.
 *
 * @param   uart_addr   - which uart will be checked, UART0 or UART1.
 *
 * @return  None.
 */
void uart_finish_transfers(UART_TypeDef* pBase);


int os_get_free_heap_size();

uint32_t uart_isr(void *user_data);

void uart_io_print(const char* buf);
void uart_io_send(const uint8_t* buf, uint32_t size);

extern TimerHandle_t at_transparent_timer;
extern TimerHandle_t at_exit_transparent_mode_timer;


bool at_buffer_full();
bool at_buffer_empty();
uint16_t at_buffer_data_size();
void at_buffer_enqueue_data(uint8_t data);
uint8_t at_buffer_dequeue_data(void);

void send_data(void);
#endif