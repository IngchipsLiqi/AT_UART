#include <stdint.h>
#include <string.h>

#include "at_profile_spsc.h"

#include "gatt_client.h"
#include "profile.h"




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
void at_spsc_send_data(uint8_t conidx,uint8_t *data, uint8_t len)
{
    //if(l2cm_get_nb_buffer_available() > 0)
    //{
    //    gatt_client_write_t write;
    //    write.conidx = conidx;
    //    write.client_id = spsc_client_id;
    //    write.att_idx = 1; //TX
    //    write.p_data = data;
    //    write.data_len = MIN(len,gatt_get_mtu(conidx) - 3);
    //    gatt_client_write_cmd(write);
    //} 
    
    uint16_t send_packet_len = 0;
    gatt_client_get_mtu(conidx, &send_packet_len);
    
    if (len > send_packet_len)
        len = send_packet_len;
    
    gatt_client_write_value_of_characteristic_without_response(conidx, slave_input_char.value_handle, 
                                                                       len, data);
}
