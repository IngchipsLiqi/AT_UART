#ifndef __AT_RECV_CMD_H__
#define __AT_RECV_CMD_H__

#include "common/preh.h"
#include "FreeRTOS.h"
#include "timers.h"

#define UART0 APB_UART0
#define UART1 APB_UART1

#define USER_UART_AT_RX 10
#define USER_UART_AT_TX 9
#define USER_UART_AT_RTS 17
#define USER_UART_AT_CTS 18


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



uint32_t uart_isr(void *user_data);

void uart_io_print(const char* buf);
void uart_io_send(const uint8_t* buf, uint32_t size);

extern TimerHandle_t at_transparent_timer;

#endif