#include <stdint.h>
#include <string.h>

#include "at_profile_spss.h"

#include "att_db.h"
#include "service/transmission_service.h"

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
void at_spss_send_data(uint8_t conidx, uint8_t *data, uint8_t len)
{
    //if(l2cm_get_nb_buffer_available() > 0)
    //{
    //    if(ntf_enable_flag[conidx])
    //    {
    //        gatt_ntf_t ntf_att;
    //        ntf_att.att_idx = 2;
    //        ntf_att.conidx = conidx;
    //        ntf_att.svc_id = spss_svc_id;
    //        ntf_att.data_len = MIN(len,gatt_get_mtu(conidx) - 3);
    //        ntf_att.p_data = data;
    //        gatt_notification(ntf_att);
    //    }
    //}
    
    att_server_notify(conidx, g_ble_output_handle, data, len);
}
