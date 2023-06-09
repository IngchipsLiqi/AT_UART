#include "flash_data.h"

#include <string.h>
#include <stdio.h>
#include "../service/transmission_service.h"
#include "../util/utils.h"
#include "btstack_util.h"

#define PRIVATE_FlASH_INIT_DEV_NAME "9168_AT"
private_flash_data_t g_power_off_save_data_in_ram = {
    .data_init_flag = PRIVATE_FlASH_DATA_IS_INIT,
    .data_len = sizeof(private_flash_data_t),
    .module_name = PRIVATE_FlASH_INIT_DEV_NAME,
    .module_mac_address = {0xF1, 0xF2, 0x28, 0x07, 0x07, 0x07},
    .module_adv_data = {
        .flags = {2, 0x01, 0x06},
        .local_name_len = BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN + 1,
        .local_name_handle = 0x09,
        .local_name = PRIVATE_FlASH_INIT_DEV_NAME,
    },
    .dev_id = 1,
    .dev_type = BLE_DEV_TYPE_NO_CONNECTION,
    .default_info = {
        .auto_transparent   = false,
        .auto_sleep         = false,
        .wakeup_source      = GIO_GPIO_5,
        .wakeup_level       = 1,
        .rf_power           = 2,
        .adv_int            = 1,
    },
    .uart_param = {
        .word_length        = UART_WLEN_8_BITS,
        .parity             = UART_PARITY_NOT_CHECK,
        .fifo_enable        = 1,
        .two_stop_bits      = 0,
        .receive_en         = 1,
        .transmit_en        = 1,
        .UART_en            = 1,
        .cts_en             = 1,
        .rts_en             = 1,
        .rxfifo_waterlevel  = 1,
        .txfifo_waterlevel  = 1,
        .ClockFrequency     = OSC_CLK_FREQ,
        .BaudRate           = 115200,
    },
    .scan_filter = {
        .name_prefix = "9168_AT",
        .name_suffix = "*",
        .uuid_16 = { 0 },
        .uuid_128 = { 0 },
        .rssi = -100,
        .enable_uuid_16_filter = false,
        .enable_uuid_128_filter = false,
        .enable_rssi_filter = false,
    },
    .serivce_uuid = {0},
    .characteristic_input_uuid = {0},
    .characteristic_output_uuid = {0},
};
static private_flash_data_t *p_power_off_save_data_in_flash = (private_flash_data_t *)PRIVATE_FLASH_DATA_START_ADD;

void sdk_private_data_write_to_flash(void)
{
    program_flash((uint32_t)p_power_off_save_data_in_flash, (uint8_t *)(&g_power_off_save_data_in_ram), sizeof(g_power_off_save_data_in_ram));
    return;
}

extern uint8_t UUID_NORDIC_TPT[];
extern uint8_t UUID_NORDIC_CHAR_GEN_IN[];
extern uint8_t UUID_NORDIC_CHAR_GEN_OUT[];

void sdk_load_private_flash_data(void)
{
    unsigned int rand_temp;

    if ((p_power_off_save_data_in_flash->data_init_flag != g_power_off_save_data_in_ram.data_init_flag) ||
        (p_power_off_save_data_in_flash->data_len != g_power_off_save_data_in_ram.data_len)) {
        rand_temp = platform_rand();

        g_power_off_save_data_in_ram.dev_id = rand_temp;
        g_power_off_save_data_in_ram.module_mac_address[5] = rand_temp;
        g_power_off_save_data_in_ram.module_mac_address[4] = rand_temp >> 8;
        g_power_off_save_data_in_ram.module_mac_address[3] = rand_temp >> 16;
        g_power_off_save_data_in_ram.module_mac_address[2] = rand_temp >> 24;
        
        sprintf(g_power_off_save_data_in_ram.module_name, "9168_AT%02x%02x%02x", 
            g_power_off_save_data_in_ram.module_mac_address[5],
            g_power_off_save_data_in_ram.module_mac_address[4],
            g_power_off_save_data_in_ram.module_mac_address[3]);
            
        memcpy(g_power_off_save_data_in_ram.module_adv_data.local_name, 
            g_power_off_save_data_in_ram.module_name, sizeof(g_power_off_save_data_in_ram.module_name));

        LOG_MSG("Device name:%s", g_power_off_save_data_in_ram.module_name);
            
        memcpy(g_power_off_save_data_in_ram.serivce_uuid, UUID_NORDIC_TPT, UUID_SIZE_16);
        memcpy(g_power_off_save_data_in_ram.characteristic_input_uuid, UUID_NORDIC_CHAR_GEN_IN, UUID_SIZE_16);
        memcpy(g_power_off_save_data_in_ram.characteristic_output_uuid, UUID_NORDIC_CHAR_GEN_OUT, UUID_SIZE_16);
            
        sdk_private_data_write_to_flash();
    } else {
        memcpy(&g_power_off_save_data_in_ram, p_power_off_save_data_in_flash, sizeof(g_power_off_save_data_in_ram));
    }
    g_power_off_save_data_in_ram.dev_type = BLE_DEV_TYPE_NO_CONNECTION;
    return;
}



