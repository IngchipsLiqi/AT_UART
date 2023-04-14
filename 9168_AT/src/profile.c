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




uint8_t need_connection_ret = 0;

extern private_flash_data_t g_power_off_save_data_in_ram;

extern volatile uint8_t send_to_slave_data_to_be_continued;
extern volatile uint8_t send_to_master_data_to_be_continued;

extern uint8_t UUID_NORDIC_TPT[];
extern uint8_t UUID_NORDIC_CHAR_GEN_IN[];
extern uint8_t UUID_NORDIC_CHAR_GEN_OUT[];

//==============================================================================================================
//* Global variable
//==============================================================================================================
hci_con_handle_t master_connect_handle = INVALID_HANDLE;
hci_con_handle_t slave_connect_handle = INVALID_HANDLE;

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
        if (gatt_event_query_complete_parse(packet)->status != 0)
            break;

        if (slave_output_desc.handle != INVALID_HANDLE)
        {
            gatt_client_listen_for_characteristic_value_updates(&slave_output_notify, output_notification_handler,
                                                                slave_connect_handle, slave_output_char.value_handle);
                
            gatt_client_write_characteristic_descriptor_using_descriptor_handle(config_notify_callback, slave_connect_handle,
                slave_output_desc.handle, sizeof(char_config_notification),
                (uint8_t *)&char_config_notification);
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
            if (memcmp(UUID_NORDIC_CHAR_GEN_IN, result->characteristic.uuid128, 16) == 0)
            {
                slave_input_char = result->characteristic;
                LOG_MSG("input handle: %d", slave_input_char.value_handle);
            }
            else if (memcmp(UUID_NORDIC_CHAR_GEN_OUT, result->characteristic.uuid128, 16) == 0)
            {
                slave_output_char = result->characteristic;
                LOG_MSG("output handle: %d", slave_output_char.value_handle);
            }
        }
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        if (gatt_event_query_complete_parse(packet)->status != 0)
            break;

        if (INVALID_HANDLE == slave_input_char.value_handle ||
            INVALID_HANDLE == slave_output_char.value_handle)
        {
            LOG_MSG("characteristic not found, disc");
            gap_disconnect(slave_connect_handle);
        }
        else
        {
            gatt_client_discover_characteristic_descriptors(descriptor_discovery_callback, slave_connect_handle, &slave_output_char);
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
            
            if (memcmp(result->service.uuid128, UUID_NORDIC_TPT, 16) == 0)
            {
                slave_service = result->service;
                LOG_MSG("service handle: %d %d", slave_service.start_group_handle, slave_service.end_group_handle);
            }
        }
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        LOG_MSG("query complete\r\n");
    
        if (gatt_event_query_complete_parse(packet)->status != 0)
            break;
        if (slave_service.start_group_handle != INVALID_HANDLE)
        {
            gatt_client_discover_characteristics_for_service(characteristic_discovery_callback, slave_connect_handle,
                                                           slave_service.start_group_handle,
                                                           slave_service.end_group_handle);
        }
        else
        {
            LOG_MSG("service not found, disc");
            gap_disconnect(slave_connect_handle);
        }
        break;
    }
}


static void discovery_service()
{
    if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_MASTER) {
        gatt_client_discover_primary_services(service_discovery_callback, slave_connect_handle);
    }
}

//==============================================================================================================










//==============================================================================================================
//* Adv
//==============================================================================================================
const static uint8_t scan_data[] = { 0 };

const static ext_adv_set_en_t adv_sets_en[] = { {.handle = 0, .duration = 0, .max_events = 0} };

static void config_adv_and_set_interval(uint32_t intervel)
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
static void start_adv(void)
{
    gap_set_ext_adv_enable(1, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
    
    LOG_MSG("Start Adv\r\n");
}
static void stop_adv(void)
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

static void config_scan(void)
{
    gap_set_ext_scan_para(BD_ADDR_TYPE_LE_RANDOM, SCAN_ACCEPT_ALL_EXCEPT_NOT_DIRECTED,
                          sizeof(configs) / sizeof(configs[0]),
                          configs);
}

static void start_scan(void)
{
    gap_set_ext_scan_enable(1, 0, 0, 0);   // start continuous scanning
                          
    LOG_MSG("Start Scan\r\n");
}

static void stop_scan(void)
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





/**
 * Receive broadcast event callbacks.
 * If the device meets the requirements, send a BLE connection request
 */
void receive_advertising_report(const le_meta_event_ext_adv_report_t* report_complete)
{
    const le_ext_adv_report_t* report = report_complete->reports;
    
    
    uint16_t name_len = 0;
    const uint8_t* name = ad_data_from_type(report->data_len, (uint8_t*)report->data, 9, &name_len);
    if (name == NULL || name_len <= 0)
        return;
    
    if (memcmp(name, "ET07_BLE", 8) != 0) {
        return;
    }
    
    bd_addr_t peer_dev_addr;
    reverse_bd_addr(report->address, peer_dev_addr);
    
    // If the device is not in the device record table, recorded in the table
    if (!at_contains_device(peer_dev_addr)) {
        at_processor_add_scan_device(peer_dev_addr);
        LOG_MSG("dev addr: %02X%02X%02X%02X%02X%02X", peer_dev_addr[0], 
                                                              peer_dev_addr[1], 
                                                              peer_dev_addr[2], 
                                                              peer_dev_addr[3], 
                                                              peer_dev_addr[4], 
                                                              peer_dev_addr[5]);
    }
}

void create_connection(bd_addr_t address)
{
    gap_set_ext_scan_enable(0, 0, 0, 0);
    LOG_MSG("connecting ...");
    
    phy_configs[0].phy = PHY_1M;
    gap_ext_create_connection(        INITIATING_ADVERTISER_FROM_PARAM, // Initiator_Filter_Policy,
                                      BD_ADDR_TYPE_LE_RANDOM,           // Own_Address_Type,
                                      BD_ADDR_TYPE_LE_RANDOM,           // Peer_Address_Type,
                                      address,                          // Peer_Address,
                                      sizeof(phy_configs) / sizeof(phy_configs[0]),
                                      phy_configs);
}


void update_device_type()
{
    if (slave_connect_handle != INVALID_HANDLE) {
        g_power_off_save_data_in_ram.dev_type = BLE_DEV_TYPE_MASTER;
        LOG_MSG("Master.\r\n");
    } else if (master_connect_handle != INVALID_HANDLE) {
        g_power_off_save_data_in_ram.dev_type = BLE_DEV_TYPE_SLAVE;
        LOG_MSG("Slave.\r\n");
    } else {
        g_power_off_save_data_in_ram.dev_type = BLE_DEV_TYPE_NO_CONNECTION;
        LOG_MSG("No Connection.\r\n");
    }
}

void handle_can_send_now(void)
{
    LOG_MSG("Into Scan send now\r\n");
    if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_MASTER) {
        if (send_to_slave_data_to_be_continued == 1) {
            send_data_to_ble_slave();
        }
    } else if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_SLAVE) {
        if (send_to_master_data_to_be_continued == 1) {
            send_data_to_ble_master();
        }
    }
}

extern circular_queue_t* buffer;
uint8_t temp_buffer[1000] = { 0 };

void process_rx_port_data()
{
    if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_NO_CONNECTION) {
        
        // cmd parse
        uint32_t len = circular_queue_get_elem_num(buffer);
        circular_queue_dequeue_batch(buffer, temp_buffer, len);
        at_processor_start(temp_buffer, len);
        
    } else if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_MASTER) {
        send_data_to_ble_slave_start();
    } else if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_SLAVE) {
        send_data_to_ble_master_start();
    }
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
    //show_heap();

    if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_NO_CONNECTION) {
        LOG_MSG("cmd processor\r\n");
    } else if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_MASTER) {
        LOG_MSG("rx_sum:%d tx_sum:%d receive_slave_sum:%d send_to_slave_sum:%d\r\n", rx_sum, tx_sum, receive_slave_sum, send_to_slave_sum);
    } else if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_SLAVE) {
        LOG_MSG("rx_sum:%d tx_sum:%d receive_master_sum:%d send_to_master_sum:%d\r\n", rx_sum, tx_sum, receive_master_sum, send_to_master_sum);
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
    case USER_MSG_SEND_DATA_TO_BLE_SLAVE:
        send_data_to_ble_slave();
        break;
    case USER_MSG_SEND_DATA_TO_BLE_MASTER:
        send_data_to_ble_master();
        break;
    case USER_MSG_CREATE_CONNECTION:
        create_connection(data);
        break;
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
    const le_meta_event_enh_create_conn_complete_t *conn_complete;
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
        
        config_adv_and_set_interval(50);
        start_adv();
        
//        config_scan();
//        start_scan();
 //       uart_io_print("Start Adv and Scan\r\n");
        break;

    case HCI_EVENT_LE_META:
        switch (hci_event_le_meta_get_subevent_code(packet))
        {
        case HCI_SUBEVENT_LE_ADVERTISING_SET_TERMINATED:
            platform_printf("advertising set terminated");
            
            break;
            
        case HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT:
            report_complete = decode_hci_le_meta_event(packet, le_meta_event_ext_adv_report_t);
            receive_advertising_report(report_complete);
            break;
        
        case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
            conn_complete = decode_hci_le_meta_event(packet, le_meta_event_enh_create_conn_complete_t);
            LOG_MSG("Connected!\r\n");
            if (need_connection_ret) {
                uart_io_print("Connected\r\n");
                need_connection_ret = 0;
            }
            
            hint_ce_len(conn_complete->interval);
            
            att_set_db(conn_complete->handle, att_db_util_get_address());
            
            if (conn_complete->role == HCI_ROLE_SLAVE) {
                stop_scan();
                master_connect_handle = conn_complete->handle;
            } else {
                stop_adv();
                slave_connect_handle = conn_complete->handle;
            }
            update_device_type();
            
            gap_read_remote_used_features(conn_complete->handle);
            
            discovery_service();
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
        LOG_MSG("Disconnected!! %d\r\n", disconn_event->reason);
    
        
    
        uart_io_print("Disconnected!!\r\n");
    
        // TODO send disconnected info to AT console
        
        if (disconn_event->conn_handle == slave_connect_handle) {
            slave_connect_handle = INVALID_HANDLE;
        } else if (disconn_event->conn_handle == master_connect_handle) {
            master_connect_handle = INVALID_HANDLE;
        }
        update_device_type();
        
        
        if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_NO_CONNECTION) 
        {
            stop_adv();
            config_adv_and_set_interval(50);
            start_adv();
            start_scan();
        }
        break;

    case ATT_EVENT_CAN_SEND_NOW:
        LOG_MSG("Into Can Send Now\r\n");
        handle_can_send_now();
        break;

    case L2CAP_EVENT_CAN_SEND_NOW:
        LOG_MSG("Into Can Send Now\r\n");
        handle_can_send_now();
        break;

    case BTSTACK_EVENT_USER_MSG:
        p_user_msg = hci_event_packet_get_user_msg(packet);
        user_msg_handler(p_user_msg->msg_id, p_user_msg->data, p_user_msg->len);
        break;

    default:
        //LOG_MSG("event:%d\r\n", event);
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