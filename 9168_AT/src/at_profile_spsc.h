#ifndef _AT_PROFILE_SPSC_H_
#define _AT_PROFILE_SPSC_H_

#include <stdint.h>


/*********************************************************************
 * @fn      at_spsc_send_data
 *
 * @brief   function to write date to peer. write without response
 *
 * @param   conidx - link  index
 *       	data   - pointer to data buffer 
 *       	len    - data len
 *
 * @return  None
 */
void at_spsc_send_data(uint8_t conidx, uint8_t *data, uint8_t len);

#endif
