#include "user_packet_handler.h"

#include <string.h>

listener_set_t listener_set;

void add_on_btstack_event_state_listener(btstack_event_state_listener_t fun)
{
    btstack_event_state_listener_list_t* list = &listener_set.btstack_event_state_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}

void add_on_hci_subevent_le_extended_advertising_report_listener(hci_subevent_le_extended_advertising_report_listener_t fun)
{
    hci_subevent_le_extended_advertising_report_listener_list_t* list = 
            &listener_set.hci_subevent_le_extended_advertising_report_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}

void add_on_hci_subevent_le_enhanced_connection_complete_listener(hci_subevent_le_enhanced_connection_complete_listener_t fun)
{
    hci_subevent_le_enhanced_connection_complete_listener_list_t* list = 
            &listener_set.hci_subevent_le_enhanced_connection_complete_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}

void add_on_hci_subevent_le_phy_update_complete_listener(hci_subevent_le_phy_update_complete_listener_t fun)
{
    hci_subevent_le_phy_update_complete_listener_list_t* list = 
            &listener_set.hci_subevent_le_phy_update_complete_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}

void add_on_hci_event_disconnection_complete_listener(hci_event_disconnection_complete_listener_t fun)
{
    hci_event_disconnection_complete_listener_list_t* list = 
            &listener_set.hci_event_disconnection_complete_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}

void add_on_att_event_can_send_now_listener(att_event_can_send_now_listener_t fun)
{
    att_event_can_send_now_listener_list_t* list = 
            &listener_set.att_event_can_send_now_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}

void add_on_att_read_listener(att_read_listener_t fun)
{
    att_read_listener_list_t* list = 
            &listener_set.att_read_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}

void add_on_att_write_listener(att_write_listener_t fun)
{
    att_write_listener_list_t* list = 
            &listener_set.att_write_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}
void add_on_user_msg_listener(user_msg_listener_t fun)
{
    user_msg_listener_list_t* list = 
            &listener_set.user_msg_listener_list;
    
    if (list->num >= MAX_LISTENER)
        return;
    list->listener_list[list->num++] = fun;
}


void remove_on_btstack_event_state_listener(btstack_event_state_listener_t fun)
{
    btstack_event_state_listener_list_t* list = &listener_set.btstack_event_state_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(btstack_event_state_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_hci_subevent_le_extended_advertising_report_listener(hci_subevent_le_extended_advertising_report_listener_t fun)   
{
    hci_subevent_le_extended_advertising_report_listener_list_t* list = 
            &listener_set.hci_subevent_le_extended_advertising_report_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(hci_subevent_le_extended_advertising_report_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_hci_subevent_le_enhanced_connection_complete_listener(hci_subevent_le_enhanced_connection_complete_listener_t fun)
{
    hci_subevent_le_enhanced_connection_complete_listener_list_t* list = 
            &listener_set.hci_subevent_le_enhanced_connection_complete_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(hci_subevent_le_enhanced_connection_complete_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_hci_subevent_le_phy_update_complete_listener(hci_subevent_le_phy_update_complete_listener_t fun)
{
    hci_subevent_le_phy_update_complete_listener_list_t* list = 
            &listener_set.hci_subevent_le_phy_update_complete_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(hci_subevent_le_phy_update_complete_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_hci_event_disconnection_complete_listener(hci_event_disconnection_complete_listener_t fun)
{
    hci_event_disconnection_complete_listener_list_t* list = 
            &listener_set.hci_event_disconnection_complete_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(hci_event_disconnection_complete_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_att_event_can_send_now_listener(att_event_can_send_now_listener_t fun)
{
    att_event_can_send_now_listener_list_t* list = &listener_set.att_event_can_send_now_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(att_event_can_send_now_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_att_read_listener(att_read_listener_t fun)
{
    att_read_listener_list_t* list = &listener_set.att_read_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(att_read_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_att_write_listener(att_write_listener_t fun)
{
    att_write_listener_list_t* list = &listener_set.att_write_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(att_write_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}
void remove_on_user_msg_listener(user_msg_listener_t fun)
{
    user_msg_listener_list_t* list = &listener_set.user_msg_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i) {
        if (list->listener_list[i] == fun) {
            memcpy(list->listener_list + i, list->listener_list + i + 1, 
                    sizeof(user_msg_listener_t) * (list->num - i - 1));
            list->num -= 1;
            break;
        }
    }
}

void on_btstack_event_state()
{
    btstack_event_state_listener_list_t* list = &listener_set.btstack_event_state_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i)
    {
        list->listener_list[i]();
    }
}
void on_hci_subevent_le_extended_advertising_report(const le_meta_event_ext_adv_report_t* report)
{
    hci_subevent_le_extended_advertising_report_listener_list_t* list = 
            &listener_set.hci_subevent_le_extended_advertising_report_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i)
    {
        list->listener_list[i](report);
    }
}
void on_hci_subevent_le_enhanced_connection_complete(const le_meta_event_enh_create_conn_complete_t* conn_cmpl)
{
    hci_subevent_le_enhanced_connection_complete_listener_list_t* list = 
            &listener_set.hci_subevent_le_enhanced_connection_complete_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i)
    {
        list->listener_list[i](conn_cmpl);
    }
}

void on_hci_subevent_le_phy_update_complete(const le_meta_phy_update_complete_t *hci_le_cmpl)
{
    hci_subevent_le_phy_update_complete_listener_list_t* list = 
            &listener_set.hci_subevent_le_phy_update_complete_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i)
    {
        list->listener_list[i](hci_le_cmpl);
    }
}

void on_hci_event_disconnection_complete(const event_disconn_complete_t * disconn_event)
{
    hci_event_disconnection_complete_listener_list_t* list = 
            &listener_set.hci_event_disconnection_complete_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i)
    {
        list->listener_list[i](disconn_event);
    }
}
void on_att_event_can_send_now()
{
    att_event_can_send_now_listener_list_t* list = 
            &listener_set.att_event_can_send_now_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i)
    {
        list->listener_list[i]();
    }
}

uint16_t on_att_read(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{
    att_read_listener_list_t* list = 
            &listener_set.att_read_listener_list;
    
    return list->listener_list[0](connection_handle, att_handle, offset, buffer, buffer_size);
}

int on_att_write(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, const uint8_t *buffer, uint16_t buffer_size)
{
    att_write_listener_list_t* list = 
            &listener_set.att_write_listener_list;
    
    return list->listener_list[0](connection_handle, att_handle, transaction_mode, offset, buffer, buffer_size);
}


void on_user_msg(uint32_t msg_id, void *data, uint16_t size)
{
    user_msg_listener_list_t* list = 
            &listener_set.user_msg_listener_list;
    
    for (uint32_t i = 0; i < list->num; ++i)
    {
        list->listener_list[0](msg_id, data, size);
    }
}


void init_user_packet_handler_manager()
{
    memset(&listener_set, 0, sizeof(listener_set));
}



