
/*
** COPYRIGHT (c) 2023 by INGCHIPS
*/

#include "stdint.h"
#include "string.h"

#include "att_db.h"
#include "att_db_util.h"
#include "bluetooth.h"
#include "btstack_event.h"
#include "gatt_client.h"
#include "btstack_defines.h"
#include "sig_uuid.h"

#include "project_common.h"
#include "throughput_service.h"
#include "bt_cmd_data_uart_io_adp.h"
#include "low_power.h"

#if defined __cplusplus
    extern "C" {
#endif

typedef struct peer_slave_info
{
    uint16_t    conn_handle;
    gatt_client_service_t                   throughput_service;
    gatt_client_characteristic_t            throughput_write_notify_char;
    gatt_client_characteristic_descriptor_t throughput_notify_desc;
    gatt_client_characteristic_t            throughput_io_char;
    gatt_client_notification_t              throughput_notify;
} peer_slave_info_t;

//const static uint8_t throughput_service_uuid128[16] =     {0x00, 0x00, 0x00, 0x01, 0x49, 0x4E, 0x47, 0x43, 0x48, 0x49, 0x50, 0x53, 0x55, 0x55, 0x49, 0x44};
//const static uint8_t throughput_char_ctrl_uuid128[16] =   {0xBF, 0x83, 0xF3, 0xF1, 0x39, 0x9A, 0x41, 0x4D, 0x90, 0x35, 0xCE, 0x64, 0xCE, 0xB3, 0xFF, 0x67};
//const static uint8_t throughput_char_output_uuid128[16] = {0xBF, 0x83, 0xF3, 0xF2, 0x39, 0x9A, 0x41, 0x4D, 0x90, 0x35, 0xCE, 0x64, 0xCE, 0xB3, 0xFF, 0x67};
//const static uint8_t throughput_char_info_uuid128[16] =   {0x10, 0x00, 0x00, 0x01, 0x49, 0x4E, 0x47, 0x43, 0x48, 0x49, 0x50, 0x53, 0x55, 0x55, 0x49, 0x44};

const static uint8_t throughput_service_uuid128[16] =           {0x00,0x00,0xFF,0xE0,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5f,0x9b,0x34,0xfb};
const static uint8_t throughput_char_write_notify_uuid128[16] = {0x00,0x00,0xFF,0xE1,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5f,0x9b,0x34,0xfb};
const static uint8_t throughput_char_io_uuid128[16] =           {0x00,0x00,0xFF,0xE2,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5f,0x9b,0x34,0xfb};
const static uint8_t throughput_char_info_uuid128[16] =         {0x00,0x00,0xFF,0xE3,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5f,0x9b,0x34,0xfb};

peer_slave_info_t peer_slave = {
    .conn_handle = INVALID_HANDLE
};

static uint8_t throughput_char_info_data = 0x96;
static uint8_t throughput_notify_enable = 0;

static uint16_t throughput_con_handle = INVALID_HANDLE;

static uint16_t throughput_att_write_notify_handle = INVALID_HANDLE;
static uint16_t throughput_att_io_handle = INVALID_HANDLE;
void throughput_service_init(void)
{
    att_db_util_add_service_uuid128(throughput_service_uuid128);

    throughput_att_write_notify_handle = att_db_util_add_characteristic_uuid128(throughput_char_write_notify_uuid128,
        ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE | ATT_PROPERTY_DYNAMIC,
        0, 0);

    throughput_att_io_handle = att_db_util_add_characteristic_uuid128(throughput_char_io_uuid128,
        ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE | ATT_PROPERTY_DYNAMIC,
        0, 0);

    att_db_util_add_characteristic_uuid128(throughput_char_info_uuid128,
        ATT_PROPERTY_READ,
        &throughput_char_info_data, sizeof(throughput_char_info_data));

    log_printf("[service]: throughput wr:%d, io:%d\r\n", throughput_att_write_notify_handle, throughput_att_io_handle);

    return;
}

int16_t throughput_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset,
                                  uint8_t *buffer, uint16_t buffer_size)
{
    if (att_handle == throughput_att_write_notify_handle) {
        if (buffer) {
            *(int8_t *)buffer = 0xff;
            return buffer_size;
        } else {
            return 1;
        }
    } else if (att_handle == throughput_att_write_notify_handle + 1) {
        if (buffer) {
            *(int8_t *)buffer = throughput_notify_enable;
            return buffer_size;
        } else {
            return 1;
        }
    }else {
        return -1;
    }
}

int16_t throughput_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, const uint8_t *buffer, uint16_t buffer_size)
{
    if (att_handle == throughput_att_write_notify_handle) {
        low_power_exit_saving(LOW_POWER_EXIT_REASON_COM_TX_DATA);
        bt_cmd_data_ble_recv_data(buffer, buffer_size);
        return 0;
    } else if (att_handle == (throughput_att_write_notify_handle + 1)) {
        if(*(uint16_t *)buffer == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION) {
            throughput_notify_enable = 1;
        } else {
            throughput_notify_enable = 0;
        }
        return 0;
    } else {
        return -1;
    }
}

void throughput_event_connected(const le_meta_event_enh_create_conn_complete_t *cmpl)
{
    throughput_con_handle = cmpl->handle;
    return;
}

void throughput_event_disconnect(const event_disconn_complete_t *cmpl)
{
    throughput_con_handle = INVALID_HANDLE;
    peer_slave.conn_handle = INVALID_HANDLE;
    throughput_notify_enable = 0;
    return;
}

void throughput_i_am_slave_send_data(void)
{
    uint16_t mtu = 0;
    uint16_t this_time_send_len;
    uint8_t *p_data = NULL;
    int ret;

    mtu = att_server_get_mtu(throughput_con_handle);
    mtu -= 3;

    while (1) {
        this_time_send_len = bt_cmd_data_com_buf_pop_num(mtu);
        if (this_time_send_len == 0) {
            return;
        }

        if (!att_server_can_send_packet_now(throughput_con_handle)) {
            att_server_request_can_send_now_event(throughput_con_handle);
            return;
        }

        p_data = bt_cmd_data_com_buf_top_pos();
        ret = att_server_notify(throughput_con_handle, throughput_att_write_notify_handle, p_data, this_time_send_len);
        if (ret == 0) {
            dbg_printf("[tpt]: slv_sd %d\r\n", this_time_send_len);
            bt_cmd_data_com_buf_pop_data(this_time_send_len);
        } else {
            continue;
        }

    }
    return;
}

static uint32_t i_am_master_send_data_num = 0;
void throughput_i_am_master_send_data(void)
{
    uint16_t mtu = 0;
    uint16_t this_time_send_len;
    uint8_t *p_data = NULL;
    int ret;

    gatt_client_get_mtu(throughput_con_handle, &mtu);
    mtu -= 3;

    while (1) {
        this_time_send_len = bt_cmd_data_com_buf_pop_num(mtu);
        if (this_time_send_len == 0) {
            return;
        }

        p_data = bt_cmd_data_com_buf_top_pos();

        ret = gatt_client_write_value_of_characteristic_without_response(
            throughput_con_handle,
            peer_slave.throughput_write_notify_char.value_handle,
            this_time_send_len,
            p_data);
        if (ret == 0) {
            i_am_master_send_data_num += this_time_send_len;
            dbg_printf("[tpt]: mst_sd %d %d\r\n", i_am_master_send_data_num, this_time_send_len);
            dump_ram_data_in_char(p_data, this_time_send_len);
            bt_cmd_data_com_buf_pop_data(this_time_send_len);
        } else {
            att_dispatch_client_request_can_send_now_event(throughput_con_handle);
            return;
        }
    }
    return;
}

static void throughput_clear_send_data(void)
{
    uint16_t mtu = 255;
    uint16_t this_time_send_len;
    while (1) {
        this_time_send_len = bt_cmd_data_com_buf_pop_num(mtu);
        if (this_time_send_len == 0) {
            return;
        }

        dbg_printf("[tpt]: clr_m%u c%u ", mtu, this_time_send_len);

        bt_cmd_data_com_buf_pop_data(this_time_send_len);
    }
    return;
}

void throughput_user_msg_handler(uint32_t msg_id, void *data, uint16_t size)
{
    switch (msg_id)
    {
    case USER_MSG_ID_THROUGHPUT_REQUEST_SEND:
        break;
    case USER_MSG_ID_UART_RX_TO_BLE_DATA_SERVICE:
    {
        if (throughput_notify_enable) {
            throughput_i_am_slave_send_data();
        } else if ((peer_slave.conn_handle != INVALID_HANDLE) && (peer_slave.throughput_write_notify_char.value_handle != INVALID_HANDLE)) {
            throughput_i_am_master_send_data();
        } else {
            throughput_clear_send_data();
        }
        break;
    }
    default:
        break;
    }
    return;
}

static void throughput_notification_listen_handler(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    const gatt_event_value_packet_t *value_packet;
    uint16_t value_size;
    switch (packet[0])
    {
    case GATT_EVENT_NOTIFICATION:
        value_packet = gatt_event_notification_parse(packet, size, &value_size);
        // value_packet->value value_size
        low_power_exit_saving(LOW_POWER_EXIT_REASON_COM_TX_DATA);
        bt_cmd_data_ble_recv_data(value_packet->value, value_size);
        break;
    }
}

static uint32_t get_sig_short_uuid(const uint8_t *uuid128)
{
    return uuid_has_bluetooth_prefix(uuid128) ? big_endian_read_32(uuid128, 0) : 0;
}

static void btstack_callback(uint8_t packet_type, uint16_t channel, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_QUERY_COMPLETE:
        if (gatt_event_query_complete_parse(packet)->status != 0)
            return;
        log_printf("[tpt]: cmpl\r\n");
        bt_cmd_data_uart_out((uint8_t *)"+CONNECT", sizeof("+CONNECT"));
        break;
    }
}

static uint16_t char_config_notification = GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
static void throughput_descriptor_discovery_callback(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
    {
        const gatt_event_all_characteristic_descriptors_query_result_t *result = gatt_event_all_characteristic_descriptors_query_result_parse(packet);
        if (get_sig_short_uuid(result->descriptor.uuid128) == SIG_UUID_DESCRIP_GATT_CLIENT_CHARACTERISTIC_CONFIGURATION) {
            peer_slave.throughput_notify_desc = result->descriptor;
            log_printf("output desc: %d", peer_slave.throughput_notify_desc.handle);
        }
        break;
    }
    case GATT_EVENT_QUERY_COMPLETE:
    {
        if (gatt_event_query_complete_parse(packet)->status != 0)
            break;

        if (peer_slave.throughput_notify_desc.handle != INVALID_HANDLE)
        {
            gatt_client_listen_for_characteristic_value_updates(&peer_slave.throughput_notify, throughput_notification_listen_handler,
                                                                peer_slave.conn_handle, peer_slave.throughput_write_notify_char.value_handle);
            gatt_client_write_characteristic_descriptor_using_descriptor_handle(
                btstack_callback,
                peer_slave.conn_handle,
                peer_slave.throughput_notify_desc.handle,
                sizeof(char_config_notification),
                (uint8_t *)&char_config_notification);
        }
        break;
    }
    default:
        break;
    }
}

static void throughput_characteristic_discovery_callback(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
    {
        const gatt_event_characteristic_query_result_t *result = gatt_event_characteristic_query_result_parse(packet);
        if (memcmp(result->characteristic.uuid128, throughput_char_write_notify_uuid128, sizeof(throughput_char_write_notify_uuid128)) == 0) {
            peer_slave.throughput_write_notify_char = result->characteristic;
            log_printf("write handle: %d\r\n", peer_slave.throughput_write_notify_char.value_handle);
        } else if (memcmp(result->characteristic.uuid128, throughput_char_io_uuid128, sizeof(throughput_char_io_uuid128)) == 0) {
            peer_slave.throughput_io_char = result->characteristic;
            log_printf("io handle: %d", peer_slave.throughput_io_char.value_handle);
        } else {
        }
        break;
    }
    case GATT_EVENT_QUERY_COMPLETE:
    {
        if (gatt_event_query_complete_parse(packet)->status != 0) {
            break;
        }

        if (INVALID_HANDLE == peer_slave.throughput_write_notify_char.value_handle) {
            log_printf("characteristic not found, disc");
            gap_disconnect(peer_slave.conn_handle);
        } else {
            gatt_client_discover_characteristic_descriptors(throughput_descriptor_discovery_callback, peer_slave.conn_handle, &peer_slave.throughput_write_notify_char);
        }
        break;
    }
    default:
        break;
    }
}

static void throughput_service_discovery_callback(uint8_t packet_type, uint16_t _, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_SERVICE_QUERY_RESULT:
    {
        peer_slave.throughput_service = gatt_event_service_query_result_parse(packet)->service;
        log_printf("service handle: %d %d", peer_slave.throughput_service.start_group_handle, peer_slave.throughput_service.end_group_handle);
        break;
    }
    case GATT_EVENT_QUERY_COMPLETE:
    {
        if (gatt_event_query_complete_parse(packet)->status != 0) {
            break;
        }
        if (peer_slave.throughput_service.start_group_handle != INVALID_HANDLE) {
            gatt_client_discover_characteristics_for_service(throughput_characteristic_discovery_callback,
                                                             peer_slave.conn_handle,
                                                             peer_slave.throughput_service.start_group_handle,
                                                             peer_slave.throughput_service.end_group_handle);
        } else {
            log_printf("service not found, disc");
            gap_disconnect(peer_slave.conn_handle);
        }
        break;
    }
    default:
        break;
    }
}

void throughput_discover_services(const le_meta_event_enh_create_conn_complete_t *cmpl)
{
    peer_slave.conn_handle = cmpl->handle;
    gatt_client_discover_primary_services_by_uuid128(throughput_service_discovery_callback,
                                                         cmpl->handle, throughput_service_uuid128);
    return;
}

#if defined __cplusplus
    }
#endif

