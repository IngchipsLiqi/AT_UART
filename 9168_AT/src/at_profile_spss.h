#ifndef _AT_PROFILE_SPSS_H_
#define _AT_PROFILE_SPSS_H_

#include <stdint.h>

/*********************************************************************
 * @fn      at_spss_send_data
 *
 * @brief   function to notification date to peer.
 *
 * @param   conidx - link  index
 *       	data   - pointer to data buffer 
 *       	len    - data len
 *
 * @return  None
 */
void at_spss_send_data(uint8_t conidx, uint8_t *data, uint8_t len);

#endif
