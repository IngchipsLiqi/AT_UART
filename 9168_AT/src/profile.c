#include "profile.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "gap.h"
#include "l2cap.h"
#include "att_dispatch.h"
#include "platform_api.h"
#include "btstack_event.h"
#include "btstack_defines.h"
#include "gatt_client.h"
#include "att_db.h"
#include "att_db_util.h"
#include "ad_parser.h"
#include "at_recv_cmd.h"

#include <stdio.h>
#include <string.h>

#include "service/transmission_service.h"
#include "service/transmission_callback.h"

#include "common/flash_data.h"
#include "at/at_parser.h"
#include "router.h"

#include "util/utils.h"

#include "at_recv_cmd.h"
#include "at_cmd_task.h"
#include "sm.h"

#include "at_profile_spsc.h"




uint8_t need_connection_ret = 0;

extern private_flash_data_t g_power_off_save_data_in_ram;

extern volatile uint8_t send_to_slave_data_to_be_continued;
extern volatile uint8_t send_to_master_data_to_be_continued;

extern uint8_t UUID_NORDIC_TPT[];
extern uint8_t UUID_NORDIC_CHAR_GEN_IN[];
extern uint8_t UUID_NORDIC_CHAR_GEN_OUT[];

extern bool gap_conn_table[];

//==============================================================================================================
//* Global variable
//==============================================================================================================

uint32_t rx_sum = 0;
uint32_t tx_sum = 0;
uint32_t receive_slave_sum = 0;
uint32_t receive_master_sum = 0;
uint32_t send_to_slave_sum = 0;
uint32_t send_to_master_sum = 0;



//==============================================================================================================
//* GATT read/write callback   Self
//==============================================================================================================
extern uint8_t notify_enable;
extern uint16_t g_ble_input_handle;
extern uint16_t g_ble_output_handle;
extern uint16_t g_ble_output_desc_handle;
//==============================================================================================================

//==============================================================================================================
//* GATT discovery             Peer
//==============================================================================================================
gatt_client_service_t slave_service;
gatt_client_characteristic_t slave_input_char;
gatt_client_characteristic_t slave_output_char;
gatt_client_characteristic_descriptor_t slave_output_desc;
gatt_client_notification_t slave_output_notify;

static uint16_t char_config_notification = GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;

//Variable to store at event callback functions
static at_cb_func_t at_cb_func[AT_GAP_CB_MAX] = {0};

/*********************************************************************
 * @fn      at_set_gap_cb_func
 *
 * @brief   Fucntion to set at event call back function
 *
 * @param   func_idx - at event idx 
 *       	func 	 - at event call back function 
 *
 * @return  None.
 */
void at_set_gap_cb_func(enum at_cb_func_idx func_idx,at_cb_func_t func)
{
    at_cb_func[func_idx] = func;
}

static void on_connect_end()
{
    gAT_ctrl_env.initialization_ongoing = false;

    if (at_cb_func[AT_GAP_CB_CONN_END]!=NULL)
        at_cb_func[AT_GAP_CB_CONN_END](NULL);
}

//==============================================================================================================
//* Receive Slave Data
//==============================================================================================================
static void output_notification_handler(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    const gatt_event_value_packet_t *value_packet;
    uint16_t value_size;
    switch (packet[0])
    {
    case GATT_EVENT_NOTIFICATION:
        value_packet = gatt_event_notification_parse(packet, size, &value_size);
    
        uart_io_send(value_packet->value, value_size);
    
        break;
    }
}

void config_notify_callback(uint8_t packet_type, uint16_t channel, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_QUERY_COMPLETE:
        if (gatt_event_query_complete_parse(packet)->status != 0)
            return;
        LOG_MSG("write output configuration enable cmpl");
        break;
    }
}





void descriptor_discovery_callback(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
        {
            const gatt_event_all_characteristic_descriptors_query_result_t *result =
                gatt_event_all_characteristic_descriptors_query_result_parse(packet);
            if (get_sig_short_uuid(result->descriptor.uuid128) ==
                SIG_UUID_DESCRIP_GATT_CLIENT_CHARACTERISTIC_CONFIGURATION)
            {
                slave_output_desc = result->descriptor;
                LOG_MSG("output desc: %d", slave_output_desc.handle);
            }
        }
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        {
            const gatt_event_query_complete_t *result =
                    gatt_event_query_complete_parse(packet);
            
            if (result->status != 0)
                break;

            if (slave_output_desc.handle != INVALID_HANDLE)
            {
                gatt_client_listen_for_characteristic_value_updates(&slave_output_notify, output_notification_handler,
                                                                    result->handle, slave_output_char.value_handle);
                    
                gatt_client_write_characteristic_descriptor_using_descriptor_handle(config_notify_callback, result->handle,
                    slave_output_desc.handle, sizeof(char_config_notification),
                    (uint8_t *)&char_config_notification);
            }
        }
        break;
    }
}

void characteristic_discovery_callback(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
        {
            const gatt_event_characteristic_query_result_t *result =
                gatt_event_characteristic_query_result_parse(packet);
            if (memcmp(g_power_off_save_data_in_ram.characteristic_input_uuid, result->characteristic.uuid128, 16) == 0)
            {
                slave_input_char = result->characteristic;
                LOG_MSG("input handle: %d", slave_input_char.value_handle);
            }
            else if (memcmp(g_power_off_save_data_in_ram.characteristic_output_uuid, result->characteristic.uuid128, 16) == 0)
            {
                slave_output_char = result->characteristic;
                LOG_MSG("output handle: %d", slave_output_char.value_handle);
            }
        }
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        {
            const gatt_event_query_complete_t *result =
                    gatt_event_query_complete_parse(packet);
            
            if (result->status != 0)
                break;

            if (INVALID_HANDLE == slave_input_char.value_handle ||
                INVALID_HANDLE == slave_output_char.value_handle)
            {
                LOG_MSG("characteristic not found, disc");
                gap_disconnect(result->handle);
            }
            else
            {
                gatt_client_discover_characteristic_descriptors(descriptor_discovery_callback, result->handle, &slave_output_char);
            }
        }
        break;
    }
}

void service_discovery_callback(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    LOG_MSG("discovered service %d\r\n", packet[0]);
    switch (packet[0])
    {
    case GATT_EVENT_SERVICE_QUERY_RESULT:
        {
            const gatt_event_service_query_result_t *result =
                    gatt_event_service_query_result_parse(packet);
            
            LOG_MSG("Service short UUID: %08X", get_sig_short_uuid(result->service.uuid128));
            
            if (memcmp(result->service.uuid128, g_power_off_save_data_in_ram.serivce_uuid, 16) == 0)
            {
                slave_service = result->service;
                LOG_MSG("service handle: %d %d", slave_service.start_group_handle, slave_service.end_group_handle);
            }
        }
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        {
            LOG_MSG("query complete\r\n");
        
            const gatt_event_query_complete_t *result =
                    gatt_event_query_complete_parse(packet);
        
            if (result->status != 0)
                break;
            if (slave_service.start_group_handle != INVALID_HANDLE)
            {
                gatt_client_discover_characteristics_for_service(characteristic_discovery_callback, result->handle,
                                                               slave_service.start_group_handle,
                                                               slave_service.end_group_handle);
            }
            else
            {
                LOG_MSG("service not found, disc");
                gap_disconnect(result->handle);
            }
        }
        break;
    }
}


static void discovery_service(uint16_t conn_handle)
{
    gatt_client_discover_primary_services(service_discovery_callback, conn_handle);
}

//==============================================================================================================










//==============================================================================================================
//* Adv
//==============================================================================================================
const static uint8_t scan_data[] = { 0 };

const static ext_adv_set_en_t adv_sets_en[] = { {.handle = 0, .duration = 0, .max_events = 0} };

void config_adv_and_set_interval(uint32_t intervel)
{
    gap_set_adv_set_random_addr(0, g_power_off_save_data_in_ram. module_mac_address);
    gap_set_ext_adv_para(0,
                            CONNECTABLE_ADV_BIT | SCANNABLE_ADV_BIT | LEGACY_PDU_BIT,
                            intervel, intervel,        // Primary_Advertising_Interval_Min, Primary_Advertising_Interval_Max
                            PRIMARY_ADV_ALL_CHANNELS,  // Primary_Advertising_Channel_Map
                            BD_ADDR_TYPE_LE_RANDOM,    // Own_Address_Type
                            BD_ADDR_TYPE_LE_PUBLIC,    // Peer_Address_Type (ignore)
                            NULL,                      // Peer_Address      (ignore)
                            ADV_FILTER_ALLOW_ALL,      // Advertising_Filter_Policy
                            0x00,                      // Advertising_Tx_Power
                            PHY_1M,                    // Primary_Advertising_PHY
                            0,                         // Secondary_Advertising_Max_Skip
                            PHY_1M,                    // Secondary_Advertising_PHY
                            0x00,                      // Advertising_SID
                            0x00);                     // Scan_Request_Notification_Enable
    gap_set_ext_adv_data(0, sizeof(g_power_off_save_data_in_ram.module_adv_data), (uint8_t*)&g_power_off_save_data_in_ram.module_adv_data);
    gap_set_ext_scan_response_data(0, sizeof(scan_data), (uint8_t*)scan_data);
}
void start_adv(void)
{
    gap_set_ext_adv_enable(1, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
    
    LOG_MSG("Start Adv\r\n");
}
void stop_adv(void)
{
    gap_set_ext_adv_enable(0, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
    
    LOG_MSG("Stop Adv\r\n");
}
//==============================================================================================================
















//==============================================================================================================
//* Scan
//==============================================================================================================
static const scan_phy_config_t configs[1] =
{
    {
        .phy = PHY_1M,
        .type = SCAN_PASSIVE,
        .interval = 200,
        .window = 50
    }
};

void config_scan(void)
{
    gap_set_ext_scan_para(BD_ADDR_TYPE_LE_RANDOM, SCAN_ACCEPT_ALL_EXCEPT_NOT_DIRECTED,
                          sizeof(configs) / sizeof(configs[0]),
                          configs);
}

void start_continuous_scan(uint16_t duration)
{
    gap_set_ext_scan_enable(1, 0, duration, 0);   // start continuous scanning
                          
    LOG_MSG("Start Scan\r\n");
}

void start_scan(void)
{
    gap_set_ext_scan_enable(1, 0, 0, 0);   // start continuous scanning
                          
    LOG_MSG("Start Scan\r\n");
}

void stop_scan(void)
{
    gap_set_ext_scan_enable(1, 0, 0, 0);   // stop scanning
                          
    LOG_MSG("Start Scan\r\n");
}

//==============================================================================================================











//==============================================================================================================
//* Connection Param
//==============================================================================================================


initiating_phy_config_t phy_configs[] =
{
    {
        .phy = PHY_1M,
        .conn_param =
        {
            .scan_int = 200,
            .scan_win = 180,
            .interval_min = 16,
            .interval_max = 16,
            .latency = 0,
            .supervision_timeout = 600,
            .min_ce_len = 80,
            .max_ce_len = 80
        }
    }
};
//==============================================================================================================



/*********************************************************************
 * @fn      at_slave_encrypted
 *
 * @brief   at event call back function, handle after link is lost
 *
 * @param   arg - point to buff of gap_evt_disconnect_t struct type
 *       
 *
 * @return  None.
 */
void at_cb_disconnected(void *arg)
{
    event_disconn_complete_t *param = (event_disconn_complete_t *)arg;
    platform_printf("at_disconnect[%d]\r\n",param->conn_handle);

    //TODO: auto transparent off

    if( (gAT_ctrl_env.async_evt_on_going)
        || (gAT_ctrl_env.async_evt_on_going == false && gAT_ctrl_env.transparent_start == false)    //passive disconnect,no transparant, then report to host
      )
    {
        uint8_t at_rsp[30];
        sprintf((char *)at_rsp,"+DISCONN:%d\r\nOK",param->conn_handle);
        at_send_rsp((char *)at_rsp);
        
        gAT_ctrl_env.async_evt_on_going = false;
    }

    // one slot send, but link loss
    if(gAT_ctrl_env.one_slot_send_start && gAT_ctrl_env.one_slot_send_len > 0)
    {
        if(gap_get_connect_status(gAT_ctrl_env.transparent_conidx)==false)
        {
            at_clr_uart_buff();
            gAT_ctrl_env.one_slot_send_start = false;
            gAT_ctrl_env.one_slot_send_len = 0;
            
            uint8_t at_rsp[] = "SEND FAIL";
            at_send_rsp((char *)at_rsp);
        }
    }
}

void handle_can_send_now(void)
{
    LOG_MSG("Into Scan send now\r\n");
}


void show_heap(void)
{
    static char buffer[200];
    platform_heap_status_t status;
    platform_get_heap_status(&status);
    sprintf(buffer, "heap status:\n"
                    "    free: %d B\n"
                    "min free: %d B\n", status.bytes_free, status.bytes_minimum_ever_free);
    LOG_MSG(buffer, strlen(buffer) + 1);
}


static void timer_task(TimerHandle_t xTimer)
{
    if (g_power_off_save_data_in_ram.default_info.role & MASTER_ROLE)
    {
        LOG_MSG("send_to_slave:%d\r\n", send_to_slave_sum);
    }
    else if (g_power_off_save_data_in_ram.default_info.role & SLAVE_ROLE)
    {
        LOG_MSG("receive_from_master_sum:%d\r\n", receive_master_sum);
    }
    else
    {
        LOG_MSG("asd");
    }

}


void init_slave_and_master_port_task()
{
    TimerHandle_t timer = xTimerCreate("state_print_task", pdMS_TO_TICKS(10000), pdTRUE, NULL, timer_task);
    xTimerStart(timer, portMAX_DELAY);
}

//==============================================================================================================
//* GATT read/write callback
//==============================================================================================================

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset,
                                  uint8_t * buffer, uint16_t buffer_size)
{
    return module_handle_att_read_callback(connection_handle, att_handle, offset, buffer, buffer_size);
}

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, const uint8_t *buffer, uint16_t buffer_size)
{
    return module_handle_att_write_callback(connection_handle, att_handle, transaction_mode, offset, buffer, buffer_size);
}
//==============================================================================================================
void user_msg_handler(uint32_t msg_id, void *data, uint16_t size)
{
    switch (msg_id)
    {
    case USER_MSG_PROCESS_BLE_MASTER_DATA:     
        uart_io_send(data, size);
        break;
    case USER_MSG_AT_RECV_CMD: //TODO  
        at_recv_cmd_handler(data);
        break;
    case USER_MSG_AT_TRANSPARENT_START_TIMER:
        xTimerStart(at_transparent_timer, portMAX_DELAY);
        break;
    case USER_MSG_AT_RECV_TRANSPARENT_DATA:
        recv_transparent_data();
        break;
    case USER_MSG_AT_QUENEUE_FULL_CMD: //TODO  
        {
            uint8_t at_rsp[] = "QUENEUE";
            at_send_rsp((char *)at_rsp);
        }
        break;   
         
    default:
        break;
    }
}


//==============================================================================================================
//* Event packet handler
//==============================================================================================================


static void user_packet_handler(uint8_t packet_type, uint16_t channel, const uint8_t *packet, uint16_t size)
{
    const le_meta_event_ext_adv_report_t *report_complete;
    const le_meta_event_create_conn_complete_t *conn_complete;
    const le_meta_event_enh_create_conn_complete_t *enh_conn_complete;
    const event_disconn_complete_t *disconn_event;
    
    uint8_t event = hci_event_packet_get_type(packet);
    const btstack_user_msg_t *p_user_msg;
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (event)
    {
    case BTSTACK_EVENT_STATE:
        
        if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING)
            break;
        LOG_MSG("Ble stack init ok\r\n");
        gap_set_random_device_address(g_power_off_save_data_in_ram.module_mac_address);
        
        config_adv_and_set_interval(adv_int_arr[g_power_off_save_data_in_ram.default_info.adv_int]);
        config_scan();
        
        if (g_power_off_save_data_in_ram.default_info.role & SLAVE_ROLE)
            start_adv();
        if (g_power_off_save_data_in_ram.default_info.role & MASTER_ROLE)
            start_scan();
        
        //TODO: auto transparent mode
        
        break;

    case HCI_EVENT_LE_META:
        switch (hci_event_le_meta_get_subevent_code(packet))
        {
        case HCI_SUBEVENT_LE_SCAN_TIMEOUT:
            LOG_MSG("scan terminated\r\n");
            gAT_ctrl_env.scan_ongoing = false;
        
            if (at_cb_func[AT_GAP_CB_SCAN_END]!=NULL)
                at_cb_func[AT_GAP_CB_SCAN_END](NULL);
            break;
        case HCI_SUBEVENT_LE_ADVERTISING_SET_TERMINATED:
            LOG_MSG("stop adv\r\n");
            gAT_ctrl_env.adv_ongoing = false;
        
            if (at_cb_func[AT_GAP_CB_ADV_END]!=NULL)
                at_cb_func[AT_GAP_CB_ADV_END](NULL);
            
            break;
            
        case HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT:
            report_complete = decode_hci_le_meta_event(packet, le_meta_event_ext_adv_report_t);
        
            if (at_cb_func[AT_GAP_CB_ADV_RPT] != NULL)
                at_cb_func[AT_GAP_CB_ADV_RPT]((void*)report_complete);
            
            break;
        case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
            LOG_MSG("Connected!\r\n");
        
            gAT_ctrl_env.initialization_ongoing = false;
        
            enh_conn_complete = decode_hci_le_meta_event(packet, le_meta_event_enh_create_conn_complete_t);
            
            ll_set_conn_tx_power(enh_conn_complete->handle, rf_power_arr[g_power_off_save_data_in_ram.default_info.rf_power]);
            
            hint_ce_len(enh_conn_complete->interval);
            
            att_set_db(enh_conn_complete->handle, att_db_util_get_address());
        
            gap_conn_table[enh_conn_complete->handle] = true;
            
            if (enh_conn_complete->role == HCI_ROLE_SLAVE) {
                stop_adv();
                
                reverse_bd_addr(enh_conn_complete->peer_addr, g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].addr);
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].addr_type = enh_conn_complete->peer_addr_type;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].link_mode = SLAVE_ROLE;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].encryption = false;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].phy = phy_configs[0].phy;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.interval_min = enh_conn_complete->interval;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.interval_max = enh_conn_complete->interval;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.latency = enh_conn_complete->latency;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.supervision_timeout = enh_conn_complete->sup_timeout;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.scan_int = phy_configs[0].conn_param.scan_int;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.scan_win = phy_configs[0].conn_param.scan_win;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.min_ce_len = phy_configs[0].conn_param.min_ce_len;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.max_ce_len = phy_configs[0].conn_param.max_ce_len;
                
                if (gAT_ctrl_env.transparent_start == false)
                {
                    uint8_t at_rsp[30];
                    sprintf((char *)at_rsp,"+CONN:%d\r\nOK",enh_conn_complete->handle);
                    at_send_rsp((char *)at_rsp);
                }
                
                if(gap_get_connect_num() == 1)
                    auto_transparent_set();
                else
                    auto_transparent_clr();
            } else {
                stop_scan();
                
                reverse_bd_addr(enh_conn_complete->peer_addr, g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].addr);
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].addr_type = enh_conn_complete->peer_addr_type;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].link_mode = MASTER_ROLE;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].encryption = false;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].phy = phy_configs[0].phy;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.interval_min = enh_conn_complete->interval;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.interval_max = enh_conn_complete->interval;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.latency = enh_conn_complete->latency;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.supervision_timeout = enh_conn_complete->sup_timeout;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.scan_int = phy_configs[0].conn_param.scan_int;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.scan_win = phy_configs[0].conn_param.scan_win;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.min_ce_len = phy_configs[0].conn_param.min_ce_len;
                g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle].conn_param.max_ce_len = phy_configs[0].conn_param.max_ce_len;
                
                memcpy(&g_power_off_save_data_in_ram.master_peer_param,
                    &g_power_off_save_data_in_ram.peer_param[enh_conn_complete->handle], sizeof(private_module_conn_peer_param_t));
                
                LOG_MSG("at_master_connected\r\n");
                            
                if(gAT_ctrl_env.transparent_start == false)         //if(gAT_ctrl_env.async_evt_on_going)
                {
                    uint8_t at_rsp[30];
                    sprintf((char *)at_rsp,"+CONN:%d\r\nOK", enh_conn_complete->handle);
                    at_send_rsp((char *)at_rsp);
                    gAT_ctrl_env.async_evt_on_going = false;
                }
                
                discovery_service(enh_conn_complete->handle);
                
                if(gap_get_connect_num() == 1)
                    auto_transparent_set();
                else
                    auto_transparent_clr();
            }
            
            gap_read_remote_used_features(enh_conn_complete->handle);
            
            on_connect_end();
            break;
        
        case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
            {
                const le_meta_phy_update_complete_t* hci_le_cmpl = decode_hci_le_meta_event(packet, le_meta_phy_update_complete_t);
                LOG_MSG("PHY updated: Rx %d, Tx %d\r\n", hci_le_cmpl->rx_phy, hci_le_cmpl->tx_phy);
            }
            break;
        
        case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
            {
                const le_meta_event_conn_update_complete_t *cmpl = decode_hci_le_meta_event(packet, le_meta_event_conn_update_complete_t);
                LOG_MSG("CONN updated: interval %.2f ms", cmpl->interval * 1.25);
            }
            break;
            
        default:
            break;
        }

        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE:
        disconn_event = decode_hci_event_disconn_complete(packet);
    
        gap_conn_table[disconn_event->conn_handle] = false;
    
        if (at_cb_func[AT_GAP_CB_DISCONNECT] != NULL)
            at_cb_func[AT_GAP_CB_DISCONNECT]((void*)disconn_event);
    
        LOG_MSG("Disconnected!! %d\r\n", disconn_event->reason);
        break;

    case ATT_EVENT_CAN_SEND_NOW:
        LOG_MSG("Into Can Send Now\r\n");
        handle_can_send_now();
        break;

    case L2CAP_EVENT_CAN_SEND_NOW:
        LOG_MSG("Into Can Send Now\r\n");
        at_spsc_send_data(gAT_ctrl_env.transparent_conidx);
        break;

    case BTSTACK_EVENT_USER_MSG:
        p_user_msg = hci_event_packet_get_user_msg(packet);
        user_msg_handler(p_user_msg->msg_id, p_user_msg->data, p_user_msg->len);
        break;
    
    case HCI_EVENT_COMMAND_STATUS:
        {
            uint8_t status = hci_event_command_status_get_status(packet);
            
            //uint8_t at_rsp[30];
            //sprintf((char *)at_rsp,"status:%d %d %d\r\n", status, gAT_ctrl_env.async_evt_on_going, gAT_ctrl_env.initialization_ongoing);
            //at_send_rsp((char *)at_rsp);
            
            
            if (status == 0x07) 
            {
                if (gAT_ctrl_env.async_evt_on_going && gAT_ctrl_env.initialization_ongoing)
                {
                    //sprintf((char *)at_rsp,"+CONN:%d\r\nScheduling failed", enh_conn_complete->handle);
                    //at_send_rsp((char *)at_rsp);
                    gAT_ctrl_env.async_evt_on_going = false;
                    
                    on_connect_end();
                }
            }
        }
        break;
        
    default:
        LOG_MSG("event:%d\r\n", event);
        break;
    }
}
//==============================================================================================================

static btstack_packet_callback_registration_t hci_event_callback_registration;



uint32_t setup_profile(void *data, void *user_data)
{
    LOG_MSG("setup profile\n");
    init_service();
    init_slave_and_master_port_task();
    
    att_server_init(att_read_callback, att_write_callback);
    hci_event_callback_registration.callback = &user_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    att_server_register_packet_handler(&user_packet_handler);
    
    return 0;
}