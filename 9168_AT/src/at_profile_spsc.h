#ifndef _AT_PROFILE_SPSC_H_
#define _AT_PROFILE_SPSC_H_

#include <stdint.h>
#include "at_profile_spss.h"

extern at_recv_data_func_t spsc_recv_data_ind_func;

/*********************************************************************
 * @fn      at_spsc_send_data
 *
 * @brief   function to write date to peer. write without response
 *
 * @param   conidx - link  index
 *
 * @return  None
 */
void at_spsc_send_data(uint8_t conn_handle);

#endif
