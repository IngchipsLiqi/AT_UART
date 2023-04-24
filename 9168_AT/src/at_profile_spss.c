#include <stdint.h>
#include <string.h>

#include "at_profile_spss.h"

#include "att_db.h"
#include "service/transmission_service.h"

at_recv_data_func_t spss_recv_data_ind_func = NULL;

extern struct at_env gAT_env;

/*********************************************************************
 * @fn      at_spss_send_data
 *
 * @brief   function to notification date to peer.
 *
 * @param   conidx - link  index
 *
 * @return  None
 */
void at_spss_send_data(uint8_t conidx)
{
    //TODO
}
