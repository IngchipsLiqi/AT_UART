#ifndef __USER_PACKET_HANDLER_H__
#define __USER_PACKET_HANDLER_H__

#include "btstack_event.h"

#include "common/preh.h"

#define MAX_LISTENER 10

typedef void (*btstack_event_state_listener_t)(void);
typedef void (*hci_subevent_le_extended_advertising_report_listener_t)(const le_meta_event_ext_adv_report_t* report);
typedef void (*hci_subevent_le_enhanced_connection_complete_listener_t)(const le_meta_event_enh_create_conn_complete_t* conn_cmpl);
typedef void (*hci_subevent_le_phy_update_complete_listener_t)(const le_meta_phy_update_complete_t *hci_le_cmpl);
typedef void (*hci_event_disconnection_complete_listener_t)(const event_disconn_complete_t * disconn_event);
typedef void (*att_event_can_send_now_listener_t)(void);
typedef uint16_t (*att_read_listener_t)(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset,
                                  uint8_t * buffer, uint16_t buffer_size);
typedef int (*att_write_listener_t)(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, const uint8_t *buffer, uint16_t buffer_size);
typedef void (*user_msg_listener_t)(uint32_t msg_id, void *data, uint16_t size);


typedef struct {
    uint32_t num;
    btstack_event_state_listener_t listener_list[MAX_LISTENER];
} btstack_event_state_listener_list_t;

typedef struct {
    uint32_t num;
    hci_subevent_le_extended_advertising_report_listener_t listener_list[MAX_LISTENER];
} hci_subevent_le_extended_advertising_report_listener_list_t;

typedef struct {
    uint32_t num;
    hci_subevent_le_enhanced_connection_complete_listener_t listener_list[MAX_LISTENER];
} hci_subevent_le_enhanced_connection_complete_listener_list_t;

typedef struct {
    uint32_t num;
    hci_subevent_le_phy_update_complete_listener_t listener_list[MAX_LISTENER];
} hci_subevent_le_phy_update_complete_listener_list_t;

typedef struct {
    uint32_t num;
    hci_event_disconnection_complete_listener_t listener_list[MAX_LISTENER];
} hci_event_disconnection_complete_listener_list_t;

typedef struct {
    uint32_t num;
    att_event_can_send_now_listener_t listener_list[MAX_LISTENER];
} att_event_can_send_now_listener_list_t;


typedef struct {
    uint32_t num;
    att_read_listener_t listener_list[MAX_LISTENER];
} att_read_listener_list_t;


typedef struct {
    uint32_t num;
    att_write_listener_t listener_list[MAX_LISTENER];
} att_write_listener_list_t;

typedef struct {
    uint32_t num;
    user_msg_listener_t listener_list[MAX_LISTENER];
} user_msg_listener_list_t;


typedef struct {
    btstack_event_state_listener_list_t                             btstack_event_state_listener_list;
    hci_subevent_le_extended_advertising_report_listener_list_t     hci_subevent_le_extended_advertising_report_listener_list;
    hci_subevent_le_enhanced_connection_complete_listener_list_t    hci_subevent_le_enhanced_connection_complete_listener_list;
    hci_subevent_le_phy_update_complete_listener_list_t             hci_subevent_le_phy_update_complete_listener_list;
    hci_event_disconnection_complete_listener_list_t                hci_event_disconnection_complete_listener_list;
    att_event_can_send_now_listener_list_t                          att_event_can_send_now_listener_list;
    att_read_listener_list_t                                        att_read_listener_list;
    att_write_listener_list_t                                       att_write_listener_list;
    user_msg_listener_list_t                                        user_msg_listener_list;
} listener_set_t;


void add_on_btstack_event_state_listener(btstack_event_state_listener_t fun);
void add_on_hci_subevent_le_extended_advertising_report_listener(hci_subevent_le_extended_advertising_report_listener_t fun);
void add_on_hci_subevent_le_enhanced_connection_complete_listener(hci_subevent_le_enhanced_connection_complete_listener_t fun);
void add_on_hci_subevent_le_phy_update_complete_listener(hci_subevent_le_phy_update_complete_listener_t fun);
void add_on_hci_event_disconnection_complete_listener(hci_event_disconnection_complete_listener_t fun);
void add_on_att_event_can_send_now_listener(att_event_can_send_now_listener_t fun);
void add_on_att_read_listener(att_read_listener_t fun);
void add_on_att_write_listener(att_write_listener_t fun);
void add_on_user_msg_listener(user_msg_listener_t fun);


void remove_on_btstack_event_state_listener(btstack_event_state_listener_t fun);
void remove_on_hci_subevent_le_extended_advertising_report_listener(hci_subevent_le_extended_advertising_report_listener_t fun);
void remove_on_hci_subevent_le_enhanced_connection_complete_listener(hci_subevent_le_enhanced_connection_complete_listener_t fun);
void remove_on_hci_subevent_le_phy_update_complete_listener(hci_subevent_le_phy_update_complete_listener_t fun);
void remove_on_hci_event_disconnection_complete_listener(hci_event_disconnection_complete_listener_t fun);
void remove_on_att_event_can_send_now_listener(att_event_can_send_now_listener_t fun);
void remove_on_att_read_listener(att_read_listener_t fun);
void remove_on_att_write_listener(att_write_listener_t fun);
void remove_on_user_msg_listener(user_msg_listener_t fun);


void on_btstack_event_state();
void on_hci_subevent_le_extended_advertising_report(const le_meta_event_ext_adv_report_t* report);
void on_hci_subevent_le_enhanced_connection_complete(const le_meta_event_enh_create_conn_complete_t* conn_cmpl);
void on_hci_subevent_le_phy_update_complete(const le_meta_phy_update_complete_t *hci_le_cmpl);
void on_hci_event_disconnection_complete(const event_disconn_complete_t * disconn_event);
void on_att_event_can_send_now();
uint16_t on_att_read(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
int on_att_write(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, const uint8_t *buffer, uint16_t buffer_size);
void on_user_msg(uint32_t msg_id, void *data, uint16_t size);

void init_user_packet_handler_manager();


#endif