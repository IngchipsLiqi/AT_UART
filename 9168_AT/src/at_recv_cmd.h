#ifndef __AT_RECV_CMD_H__
#define __AT_RECV_CMD_H__

#include "common/preh.h"


#define USER_UART_AT_RX 10
#define USER_UART_AT_TX 9
#define USER_UART_AT_RTS 17
#define USER_UART_AT_CTS 18

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




uint32_t uart_isr(void *user_data);

void uart_io_print(const char* buf);
void uart_io_send(const uint8_t* buf, uint32_t size);

#endif