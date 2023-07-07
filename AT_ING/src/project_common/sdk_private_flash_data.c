
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform_api.h"
#include "eflash.h"

#include "sdk_private_flash_data.h"

#include "app.h"

#if defined __cplusplus
    extern "C" {
#endif

typedef enum {
    BLE_DEV_ADV_STOP,
    BLE_DEV_ADV_GEN,
    BLE_DEV_ADV_IBEACON,
} bt_ble_adv_mode_e;

private_flash_data_t g_power_off_save_data_in_ram = {
    .data_init_flag = PRIVATE_FlASH_DATA_IS_INIT,
    .data_len = sizeof(private_flash_data_t),
    .access_address = 0x789ACDEF,
    .phy_channel_id = 24,
    .phy = 3,
    .module_name = "I",
    .module_uart_baud_rate = COM_PORT_DEFAULT_BAUD_RATE,
    .module_mac_address = {0xF1, 0xF2, 0x28, 0x07, 0x07, 0x07},
    .module_adv_period = 2,
    .module_adv_data = {
        .flags = {2, 0x01, 0x06},
        #define UUID 0xFFF1
        .service_uuid16 = {3, 0x03, (UUID & 0xFF), ((UUID >> 8) & 0xFF)},
        .local_name_len = BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN + 1,
        .local_name_handle = 0x09,
        .local_name = "I",
        .manufacturer_len = BT_AT_CMD_TTM_MODULE_MANU_DATA_LEN + 3,
        .manufacturer_handle = 0xFF,
        .manufacturer_id = {0xAC, 0x06},
        .manufacturer_data = {0x00},
    },
    .adv_interval = 500,
    .con_interval = 500,

    .adv_data = {
        2, 0x01,
            0x06,
        8, 0x09,
            // 0x09 - «Complete Local Name»
            'W', 'M', '2', '5', '0', 'L', 'C',
        17, 0x07,
            // 0x07 - «Complete List of 128-bit Service Class UUIDs»
            COMPLETE_128_SERVICE_UUID,
    },
    .adv_data_len = 4 + 2 + 8 + 17,

    .scan_response_data = {
        5, 0xFF,
            // 0xFF - «Manufacturer Specific Data»
            0xAC, 0x06, 0x34, 0x12,
    },
    .scan_response_data_len = 1 + 5,

    .module_product_id = {0xCD, 0xA3},
    .dev_id = 1,
    .dev_type = BLE_DEV_TYPE_SLAVE,
    .re_connect = 0,
    .peer_mac_address = {0xCD, 0xA3, 0x3A, 0xE6, 0xA7, 0xA5},
    .flow_control = 0,
    .adv_mode = BLE_DEV_ADV_GEN,
    //.adv_type = CONNECTABLE_ADV_BIT | SCANNABLE_ADV_BIT | LEGACY_PDU_BIT,
    .adv_type = 0,
    .data_end_init_flag = PRIVATE_FlASH_DATA_IS_INIT,
};

static private_flash_data_t *p_power_off_save_data_in_flash = (private_flash_data_t *)PRIVATE_FLASH_DATA_START_ADD;

void sdk_private_data_write_to_flash(void)
{
    //log_printf("sdk_private_data_write_to_flash\r\n");
    program_flash((uint32_t)p_power_off_save_data_in_flash, (uint8_t *)(&g_power_off_save_data_in_ram), sizeof(g_power_off_save_data_in_ram));
    //write_flash((uint32_t)p_power_off_save_data_in_flash, (uint8_t *)(&g_power_off_save_data_in_ram), sizeof(g_power_off_save_data_in_ram));
    return;
}

void sdk_private_data_back_to_default(void)
{
    g_power_off_save_data_in_ram.data_init_flag = 0;
    sdk_private_data_write_to_flash();
    return;
}

int sdk_load_private_flash_data(void)
{
    unsigned int rand_temp;
    char name[32];
    int ret = 0;

    if ((p_power_off_save_data_in_flash->data_init_flag != g_power_off_save_data_in_ram.data_init_flag) ||
        (p_power_off_save_data_in_flash->data_end_init_flag != g_power_off_save_data_in_ram.data_end_init_flag) ||
        (p_power_off_save_data_in_flash->data_len != g_power_off_save_data_in_ram.data_len)) {
        rand_temp = platform_rand();

        g_power_off_save_data_in_ram.dev_id = rand_temp;

        g_power_off_save_data_in_ram.module_mac_address[5] = rand_temp;
        rand_temp = rand_temp >> 8;
        g_power_off_save_data_in_ram.module_mac_address[4] = rand_temp;
        rand_temp = rand_temp >> 8;
        g_power_off_save_data_in_ram.module_mac_address[3] = rand_temp;
        rand_temp = rand_temp >> 8;
        g_power_off_save_data_in_ram.module_mac_address[2] = rand_temp;

        g_power_off_save_data_in_ram.module_mac_address[1] = 0x71;
        g_power_off_save_data_in_ram.module_mac_address[0] = 0xFB;

        sprintf(name, "LVS-Lush%03u", rand_temp);

        strncpy((char *)g_power_off_save_data_in_ram.module_adv_data.local_name, name,
            sizeof(g_power_off_save_data_in_ram.module_adv_data.local_name));
        strncpy(g_power_off_save_data_in_ram.module_name, name,
            sizeof(g_power_off_save_data_in_ram.module_name));

        sdk_private_data_write_to_flash();
        ret = 1;
    } else {
        memcpy(&g_power_off_save_data_in_ram, p_power_off_save_data_in_flash, sizeof(g_power_off_save_data_in_ram));
        ret = 2;
    }

    return ret;
}

#if defined __cplusplus
    }
#endif

