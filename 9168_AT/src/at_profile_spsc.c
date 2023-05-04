#include <stdint.h>
#include <string.h>

#include "at_profile_spsc.h"

#include "gatt_client.h"
#include "profile.h"
#include "at_recv_cmd.h"
#include "att_dispatch.h"
#include "btstack_defines.h"


at_recv_data_func_t spsc_recv_data_ind_func = NULL;

uint32_t send_num = 0;

extern struct at_env gAT_env;

bool can_send_now = true;

/*********************************************************************
 * @fn      at_spsc_send_data
 *
 * @brief   function to write date to peer. write without response
 *
 * @param   conidx - link  index
 *
 * @return  None
 */
void at_spsc_send_data(uint8_t conidx)
{
    if (!can_send_now)
        return;
    
    uint16_t send_packet_len , r, valid_data, this_time_send_len = 0;
    uint8_t* p_send_data = NULL;
    
    gatt_client_get_mtu(conidx, &send_packet_len);
    send_packet_len -= 3;
    
    while ((valid_data = at_buffer_data_size()) > 0) 
    {
        this_time_send_len = valid_data;
        if (this_time_send_len > send_packet_len)
            this_time_send_len = send_packet_len;
        if (gAT_env.at_recv_buffer_rp + this_time_send_len >= AT_RECV_MAX_LEN)
            this_time_send_len = AT_RECV_MAX_LEN - gAT_env.at_recv_buffer_rp;
        p_send_data = gAT_env.at_recv_buffer + gAT_env.at_recv_buffer_rp;
        
        if (this_time_send_len != 0)
        {
            r = gatt_client_write_value_of_characteristic_without_response(conidx, slave_input_char.value_handle, 
                                                                               this_time_send_len, p_send_data);
            if (r == 0)
            {
                gAT_env.at_recv_buffer_rp += this_time_send_len;
                if (gAT_env.at_recv_buffer_rp >= AT_RECV_MAX_LEN)
                    gAT_env.at_recv_buffer_rp = gAT_env.at_recv_buffer_rp - AT_RECV_MAX_LEN;
                send_num += this_time_send_len;
            }
            else if (r == BTSTACK_ACL_BUFFERS_FULL)
            {
                can_send_now = false;
                att_dispatch_client_request_can_send_now_event(conidx); // Request can send now
                break;
            }
        }
    }
}
