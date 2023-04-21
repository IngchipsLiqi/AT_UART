#include "transmission_service.h"
#include "transmission_callback.h"

#include "../at_recv_cmd.h"//#include "at_recv_cmd.h"
#include <string.h>
#include "../private_user_packet_handler.h"
#include "../router.h"
#include "../at_recv_cmd.h"

static uint8_t att_db_storage[512];

uint8_t UUID_NORDIC_TPT[]            = {0x6e,0x40,0x00,0x01,0xb5,0xa3,0xf3,0x93,0xe0,0xa9,0xe5,0x0e,0x24,0xdc,0xca,0x9e};
uint8_t UUID_NORDIC_CHAR_GEN_IN[]    = {0x6e,0x40,0x00,0x02,0xb5,0xa3,0xf3,0x93,0xe0,0xa9,0xe5,0x0e,0x24,0xdc,0xca,0x9e};
uint8_t UUID_NORDIC_CHAR_GEN_OUT[]   = {0x6e,0x40,0x00,0x03,0xb5,0xa3,0xf3,0x93,0xe0,0xa9,0xe5,0x0e,0x24,0xdc,0xca,0x9e};


uint8_t notify_enable = 0;
uint16_t g_ble_input_handle = 0;
uint16_t g_ble_output_handle = 0;
uint16_t g_ble_output_desc_handle = 0;
uint16_t g_ble_flow_ctrl_handle = 0;

uint8_t send_to_master_flow_control_enable = BT_PRIVT_DISABLE;
uint8_t send_to_slave_flow_control_enable = BT_PRIVT_DISABLE;
uint8_t recv_from_master_flow_control_enable = BT_PRIVT_DISABLE;
uint8_t recv_from_slave_flow_control_enable = BT_PRIVT_DISABLE;

void init_tansmit_service(void)
{
    uint16_t att_value_handle;
    uint8_t g_value[GATT_CHARACTERISTIC_MAX_DATA_LEN]={0};

    att_db_util_add_service_uuid128(g_power_off_save_data_in_ram.serivce_uuid);
    
    LOG_MSG("ble gatt add transmission service.");
    
    att_value_handle = att_db_util_add_characteristic_uuid128(g_power_off_save_data_in_ram.characteristic_output_uuid, 
            ATT_PROPERTY_NOTIFY | ATT_PROPERTY_READ | ATT_PROPERTY_DYNAMIC,
            g_value, sizeof(g_value));
    g_ble_output_handle = att_value_handle;
    g_ble_output_desc_handle = att_value_handle + 1;
    module_att_write_callback_register(att_value_handle + 1, att_write_output_desc_callback);    //output desc write callback
    LOG_MSG("ble gatt add transmission characteristic output.");

    
    att_value_handle = att_db_util_add_characteristic_uuid128(g_power_off_save_data_in_ram.characteristic_input_uuid, 
            ATT_PROPERTY_DYNAMIC | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE, 
            g_value, sizeof(g_value));
    g_ble_input_handle = att_value_handle;
    module_att_write_callback_register(att_value_handle, att_write_input_callback);    //input write callback
    LOG_MSG("ble gatt add transmission characteristic input.");
}

int att_write_output_desc_callback(uint16_t offset, const uint8_t *buffer, uint16_t buffer_size)
{
    if(*(uint16_t *)buffer == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION) {
        LOG_MSG("configuration notification!\r\n");
        notify_enable = 1;
    } else {
        notify_enable = 0;
    }
    return 0;
}


//==============================================================================================================
//* Receive Master Data
//==============================================================================================================
int att_write_input_callback(uint16_t offset, const uint8_t *buffer, uint16_t buffer_size)
{
    LOG_MSG("into write input callback %d\r\n", buffer_size);
    
    //uart_io_send(buffer, buffer_size);

    uint8_t data[300] = {0};
    //platform_printf("%p %p %p\r\n", data, &data[0], &data[299]);
    
    memcpy(data, buffer, buffer_size);
    
    btstack_push_user_msg(USER_MSG_PROCESS_BLE_MASTER_DATA, data, buffer_size);
    
    //platform_printf("over\r\n");
    
    return 0;
}

void init_service()
{
    LOG_MSG("add gatt service \n");
    att_db_util_init(att_db_storage, sizeof(att_db_storage));

    init_tansmit_service();
}

