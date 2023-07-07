#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "att_db.h"
#include "gap.h"
#include "btstack_event.h"
#include "btstack_defines.h"
#include "att_db.h"
#include "att_db_util.h"
#include "gatt_client.h"

#include "FreeRTOS.h"
#include "timers.h"

#include "platform_api.h"

#include "app.h"
#include "sdk_private_flash_data.h"
#include "./service/throughput_service.h"
#include "project_common.h"
#include "low_power.h"

const static uint8_t adv_data[] = {
    #include "../data/advertising.adv"
};

const static uint8_t scan_data[] = {
    #include "../data/scan_response.adv"
};

//// GATT characteristic handles
//#include "../data/gatt.const"
//const static uint8_t profile_data[] = {
//    #include "../data/gatt.profile"
//};

hci_con_handle_t handle_send = 0;
static uint8_t notify_enable = 0;

extern const char welcome_msg[];

static TimerHandle_t create_conn_timer = 0;
static uint16_t i_am_master_conn_handle = INVALID_HANDLE;
static uint16_t i_am_slave_conn_handle = INVALID_HANDLE;

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset,
                                  uint8_t * buffer, uint16_t buffer_size)
{
    int16_t ret;

    dbg_printf("[att]: read handle:%d size:%d\n", att_handle, buffer_size);

    ret = throughput_att_read_callback(connection_handle, att_handle, offset, buffer, buffer_size);
    if (ret != -1) {
        return ret;
    }

    switch (att_handle)
    {
    default:
        return 0;
    }
}

static btstack_packet_callback_registration_t hci_event_callback_registration;

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, const uint8_t *buffer, uint16_t buffer_size)
{
    int16_t ret;

    dbg_printf("[att]: write handle:%d size:%d\n", att_handle, buffer_size);

    ret = throughput_att_write_callback(connection_handle, att_handle, transaction_mode, offset, buffer, buffer_size);
    if (ret != -1) {
        return ret;
    }

    switch (att_handle)
    {
    default:
        return 0;
    }
}

static void stack_on_first_wake_up(void)
{
    btstack_push_user_msg(USER_MSG_ID_FIRST_WAKE_UP, NULL, 0);
}

static void stack_set_latency(int latency)
{
    ll_set_conn_latency(handle_send, latency);
}

const static ext_adv_set_en_t adv_sets_en[1] = {{.handle = 0, .duration = 0, .max_events = 0}};

#define INITIATING_OFF      0xff
#define INITIATING_AUTO     0xfe
#define INITIATING_SLAVE1   0x00
#define INITIATING_SLAVE2   0x01
#define INITIATING_SLAVE3   0x02
static int gap_is_connection_cancel = 0;
static uint8_t initiating_id = INITIATING_OFF;
static uint8_t cmd_scan_en_state = 0;
static uint8_t is_scanning = 0;

#define SCAN_INTERVAL 96 // * 0.625ms
#define SCAN_WINDOW 96 // * 0.625ms
#define I_AM_MASTER_CON_MIN_INTERVAL 24 // * 1.25ms
#define I_AM_MASTER_CON_MAX_INTERVAL 24 // * 1.25ms
#define I_AM_MASTER_TIMEOUT 400 // * 10ms
#define I_AM_MASTER_CE_LEN 96

const static initiating_phy_config_t phy_configs[] =
{
    {
        .phy = PHY_1M,
        .conn_param =
        {
            .scan_int = SCAN_INTERVAL,
            .scan_win = SCAN_WINDOW,
            .interval_min = I_AM_MASTER_CON_MIN_INTERVAL,
            .interval_max = I_AM_MASTER_CON_MAX_INTERVAL,
            .latency = 0,
            .supervision_timeout = I_AM_MASTER_TIMEOUT,
            .min_ce_len = I_AM_MASTER_CE_LEN,
            .max_ce_len = I_AM_MASTER_CE_LEN
        }
    }
};

static void initiating(uint8_t init_id)
{
    initiating_id = init_id;
    if (INITIATING_OFF == init_id) {
        return;
    }

    log_printf("[bt]: initiating%d\r\n", init_id);
    if (INITIATING_AUTO == init_id) {
        gap_ext_create_connection(INITIATING_ADVERTISER_FROM_LIST, // Initiator_Filter_Policy,
                                  BD_ADDR_TYPE_LE_RANDOM,          // Own_Address_Type,
                                  BD_ADDR_TYPE_LE_RANDOM,          // Peer_Address_Type,
                                  NULL,                            // Peer_Address,
                                  sizeof(phy_configs) / sizeof(phy_configs[0]),
                                  phy_configs);
    } else {
        gap_ext_create_connection(INITIATING_ADVERTISER_FROM_PARAM,              // Initiator_Filter_Policy,
                                  BD_ADDR_TYPE_LE_RANDOM,                        // Own_Address_Type,
                                  BD_ADDR_TYPE_LE_RANDOM,                        // Peer_Address_Type,
                                  g_power_off_save_data_in_ram.peer_mac_address,                                     // Peer_Address,
                                  sizeof(phy_configs) / sizeof(phy_configs[0]),
                                  phy_configs);
    }
    xTimerReset(create_conn_timer, portMAX_DELAY);
}

static void start_scan(void)
{
    is_scanning = 1;
    return;
}

static void auto_connect(void)
{
    const uint8_t *addr;

    if (is_scanning) {
        gap_set_ext_scan_enable(0, 0, 0, 0);
        is_scanning = 0;
    }

    gap_clear_white_lists();

    addr = g_power_off_save_data_in_ram.peer_mac_address;
    if (i_am_master_conn_handle != INVALID_HANDLE) {
        gap_disconnect(i_am_master_conn_handle);
        gap_add_whitelist(addr, BD_ADDR_TYPE_LE_RANDOM);
    } else {
        gap_add_whitelist(addr, BD_ADDR_TYPE_LE_RANDOM);
    }

    initiating(INITIATING_SLAVE1);
    return;
}

static void cancel_initiating(void)
{
    if (initiating_id != INITIATING_OFF)
    {
        gap_create_connection_cancel();

        if (gap_is_connection_cancel == 1) {
            gap_is_connection_cancel = 0;
            initiating_id = INITIATING_OFF;
            auto_connect();
        } else {
            gap_is_connection_cancel = 1;
        }
    }
}

static void profile_user_msg_handler(uint32_t msg_id, void *data, uint16_t size)
{
    switch (msg_id)
    {
    case USER_MSG_ID_FIRST_WAKE_UP:
    {
        platform_32k_rc_auto_tune();
        platform_config(PLATFORM_CFG_32K_CALI_PERIOD, 120);
        if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_MASTER) {
            auto_connect();
            log_printf("[bt]: start connect\r\n");
        } else if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_SLAVE) {
            gap_set_ext_adv_enable(1, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
            log_printf("[bt]: start adv\r\n");
        }
        low_power_exit_saving(LOW_POWER_EXIT_REASON_SYS_START);
        break;
    }
    case USER_MSG_INITIATE_TIMOUT:
    {
        log_printf("initiate timeout %x\n", initiating_id);
        if (initiating_id != INITIATING_OFF) {
            cancel_initiating();
        }
        break;
    }
    default:
        break;
    }
    return;
}

static void adv_user_msg_handler(uint32_t msg_id, void *data, uint16_t size)
{
    switch (msg_id)
    {
    case USER_MSG_ID_START_ADV:
    {
        break;
    }
    default:
        break;
    }
    return;
}

static void user_msg_handler(uint32_t msg_id, void *data, uint16_t size)
{
    dbg_printf("[msg]: id:0x%x, size:%d\r\n", msg_id, size);

    if ((msg_id >= USER_MSG_ID_PROFILE) && (msg_id <= USER_MSG_ID_PROFILE_END)) {
        profile_user_msg_handler(msg_id, data, size);
    } else if ((msg_id >= USER_MSG_ID_ADV) && (msg_id <= USER_MSG_ID_ADV_END)) {
        adv_user_msg_handler(msg_id, data, size);
    } else if ((msg_id >= USER_MSG_ID_THROUGHPUT) && (msg_id <= USER_MSG_ID_THROUGHPUT_END)) {
        throughput_user_msg_handler(msg_id, data, size);
    } else {
    }
    return;
}

extern int adv_tx_power;

static void setup_adv()
{
    gap_set_adv_set_random_addr(0, g_power_off_save_data_in_ram.module_mac_address);
    gap_set_ext_adv_para(0,
                            CONNECTABLE_ADV_BIT | SCANNABLE_ADV_BIT | LEGACY_PDU_BIT, //LEGACY_PDU_BIT, //CONNECTABLE_ADV_BIT | SCANNABLE_ADV_BIT | LEGACY_PDU_BIT,
                            160 * 5, 160 * 5,                  // Primary_Advertising_Interval_Min, Primary_Advertising_Interval_Max
                            PRIMARY_ADV_ALL_CHANNELS,  // Primary_Advertising_Channel_Map
                            BD_ADDR_TYPE_LE_RANDOM,    // Own_Address_Type
                            BD_ADDR_TYPE_LE_PUBLIC,    // Peer_Address_Type (ignore)
                            NULL,                      // Peer_Address      (ignore)
                            ADV_FILTER_ALLOW_ALL,      // Advertising_Filter_Policy
                            100,                       // Advertising_Tx_Power
                            PHY_1M,                    // Primary_Advertising_PHY
                            0,                         // Secondary_Advertising_Max_Skip
                            PHY_1M,                    // Secondary_Advertising_PHY
                            0x00,                      // Advertising_SID
                            0x00);                     // Scan_Request_Notification_Enable
    gap_set_ext_adv_data(0, sizeof(adv_data), (uint8_t*)adv_data);
    gap_set_ext_scan_response_data(0, sizeof(scan_data), (uint8_t*)scan_data);
}

static void i_am_slave_connected_master(const le_meta_event_enh_create_conn_complete_t *conn_complete)
{
    i_am_slave_conn_handle = conn_complete->handle;
    return;
}

static void i_am_master_connected_slave(const le_meta_event_enh_create_conn_complete_t *conn_complete)
{
    i_am_master_conn_handle = conn_complete->handle;
    xTimerStop(create_conn_timer, portMAX_DELAY);
    throughput_discover_services(conn_complete);
    return;
}

static void i_am_slave_disconnected_master(const event_disconn_complete_t *disconn_complete)
{
    i_am_slave_conn_handle = INVALID_HANDLE;
    setup_adv();
    gap_set_ext_adv_enable(1, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
    return;
}

static void i_am_master_disconnected_slave(const event_disconn_complete_t *disconn_complete)
{
    i_am_master_conn_handle = INVALID_HANDLE;
    auto_connect();
    return;
}

static const scan_phy_config_t configs[2] =
{
    {
        .phy = PHY_1M,
        .type = SCAN_PASSIVE,
        .interval = 200,
        .window = 150
    },
};

static void user_packet_handler(uint8_t packet_type, uint16_t channel, const uint8_t *packet, uint16_t size)
{
    uint8_t event = hci_event_packet_get_type(packet);
    const btstack_user_msg_t *p_user_msg;
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (event)
    {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING)
            break;
        platform_config(PLATFORM_CFG_LL_LEGACY_ADV_INTERVAL, (1250 << 16) | 750);
        setup_adv();
        gap_set_random_device_address(g_power_off_save_data_in_ram.module_mac_address);
        gap_set_ext_scan_para(BD_ADDR_TYPE_LE_RANDOM, SCAN_ACCEPT_ALL_EXCEPT_NOT_DIRECTED,
                              sizeof(configs) / sizeof(configs[0]),
                              configs);
        platform_set_timer(stack_on_first_wake_up, 200);
        break;

    case HCI_EVENT_LE_META:
        switch (hci_event_le_meta_get_subevent_code(packet))
        {
        case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
        {
            const le_meta_event_enh_create_conn_complete_t *cmpl = decode_hci_le_meta_event(packet, le_meta_event_enh_create_conn_complete_t);
            if (0 != cmpl->status) {
                auto_connect();
                break;
            }
            log_printf("[bt]: con\r\n");
            print_addr(cmpl->peer_addr);
            gatt_client_is_ready(cmpl->handle);
            gap_read_remote_used_features(cmpl->handle);

            platform_calibrate_32k();

            if (HCI_ROLE_SLAVE == cmpl->role) { // i am slave
                att_set_db(cmpl->handle, att_db_util_get_address());
                ll_hint_on_ce_len(cmpl->handle, 10, 15);
                i_am_slave_connected_master(cmpl);
            } else { // i am master
                i_am_master_connected_slave(cmpl);
            }
            throughput_event_connected(cmpl);
            break;
        }
        default:
            break;
        }

        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE:
    {
        const event_disconn_complete_t *cmpl = decode_hci_event_disconn_complete(packet);

        log_printf("[bt]: discon\r\n");
        if (cmpl->conn_handle == i_am_master_conn_handle) {
            i_am_master_disconnected_slave(cmpl);
        } else if (cmpl->conn_handle == i_am_slave_conn_handle) {
            i_am_slave_disconnected_master(cmpl);
        } else {
            i_am_master_conn_handle = INVALID_HANDLE;
            i_am_slave_conn_handle = INVALID_HANDLE;
        }

        throughput_event_disconnect(cmpl);
        break;
    }

    case ATT_EVENT_CAN_SEND_NOW:
        throughput_i_am_slave_send_data();
        break;

    case L2CAP_EVENT_CAN_SEND_NOW:
        throughput_i_am_master_send_data();
        break;

    case BTSTACK_EVENT_USER_MSG:
        p_user_msg = hci_event_packet_get_user_msg(packet);
        user_msg_handler(p_user_msg->msg_id, p_user_msg->data, p_user_msg->len);
        break;

    default:
        break;
    }
}

static void conn_timer_callback(TimerHandle_t xTimer)
{
    btstack_push_user_msg(USER_MSG_INITIATE_TIMOUT, NULL, 0);
}

static uint8_t att_db_storage[500];
uint32_t setup_profile(void *data, void *user_data)
{
    log_printf("setup profile\n");
    create_conn_timer = xTimerCreate("a",
                            pdMS_TO_TICKS(5000),
                            pdTRUE,
                            NULL,
                            conn_timer_callback);

    att_server_init(att_read_callback, att_write_callback);
    hci_event_callback_registration.callback = &user_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    att_server_register_packet_handler(&user_packet_handler);

    att_db_util_init(att_db_storage, sizeof(att_db_storage));
    throughput_service_init();

    app_start();
    return 0;
}

