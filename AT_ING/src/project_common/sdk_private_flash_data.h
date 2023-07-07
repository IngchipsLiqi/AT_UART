
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#ifndef __SDK_PRIVATE_FLASH_H__
#define __SDK_PRIVATE_FLASH_H__

#include <stdint.h>

#if defined __cplusplus
    extern "C" {
#endif

#define BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN 14
#define BT_AT_CMD_TTM_MODULE_MANU_DATA_LEN (18 - BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN)

#define BT_AT_CMD_TTM_MAC_ADDRESS_LEN 6
#define BT_AT_CMD_TTM_MAC_ADDRESS_STR_LEN (BT_AT_CMD_TTM_MAC_ADDRESS_LEN * 2)
#define GATT_CHARACTERISTIC_BLE_TO_TX_DATA_BUF_LEN 20

typedef enum {
    BLE_DEV_TYPE_MASTER,
    BLE_DEV_TYPE_SLAVE,
    BLE_DEV_TYPE_MASTER_AND_SLAVE,
    BLE_DEV_TYPE_NO_CONNECTION,
} bt_ble_dev_type_e;

typedef struct {
    uint8_t flags[3];
    uint8_t service_uuid16[4];
    uint8_t local_name_len;
    uint8_t local_name_handle;
    uint8_t local_name[BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN];
    uint8_t manufacturer_len;
    uint8_t manufacturer_handle;
    uint8_t manufacturer_id[2];
    uint8_t manufacturer_data[BT_AT_CMD_TTM_MODULE_MANU_DATA_LEN];
} private_module_adv_data_t;

typedef struct {
    uint32_t data_init_flag;
    uint32_t data_len;
    unsigned int access_address;
    int8_t phy_channel_id;
    uint8_t phy;
    char module_name[BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN + 1];
    uint32_t module_uart_baud_rate;
    uint8_t module_mac_address[BT_AT_CMD_TTM_MAC_ADDRESS_LEN];
    uint32_t module_adv_period;
    private_module_adv_data_t module_adv_data;
    uint32_t adv_interval;
    uint16_t con_interval;
    uint8_t adv_data[31];
    uint8_t adv_data_len;
    uint8_t scan_response_data[31];
    uint8_t scan_response_data_len;
    uint8_t module_product_id[2];
    uint8_t dev_id;
    uint8_t dev_type;
    uint8_t re_connect;
    uint8_t peer_mac_address[BT_AT_CMD_TTM_MAC_ADDRESS_LEN];
    uint8_t flow_control;
    uint8_t adv_mode;
    uint8_t adv_type;
#if (CONNECT_DEVICE_DIRECT_RECONNECT == 1)
		uint8_t peer_addr_type;
#endif
    uint32_t data_end_init_flag;
} private_flash_data_t;

#define COMPLETE_128_SERVICE_UUID               0x79, 0x41, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e
#define COMPLETE_128_CHARACTERISTIC_WRITE_UUID  0x79, 0x41, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e
#define COMPLETE_128_CHARACTERISTIC_NOTIFY_UUID 0x79, 0x41, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e

extern private_flash_data_t g_power_off_save_data_in_ram;
int sdk_load_private_flash_data(void);
void sdk_private_data_write_to_flash(void);
void sdk_private_data_back_to_default(void);

#if defined __cplusplus
    }
#endif

#endif

