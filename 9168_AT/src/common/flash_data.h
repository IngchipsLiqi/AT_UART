#ifndef __FLASH_DATA_H__
#define __FLASH_DATA_H__

#include "../common/preh.h"

#include "eflash.h"


#if (INGCHIPS_FAMILY == INGCHIPS_FAMILY_918)
#define PRIVATE_FLASH_DATA_START_ADD 0x40000
#elif (INGCHIPS_FAMILY == INGCHIPS_FAMILY_916)
#define PRIVATE_FLASH_DATA_START_ADD 0x2141000
#else
    #error unknown or unsupported chip family
#endif

typedef struct {
    uint8_t flags[3];
    uint8_t local_name_len;
    uint8_t local_name_handle;
    uint8_t local_name[BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN];
} private_module_adv_data_t;


typedef struct {
    uint32_t data_init_flag;
    uint32_t data_len;
    
    char module_name[BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN + 1];
    uint8_t module_mac_address[BT_AT_CMD_TTM_MAC_ADDRESS_LEN];
    
    private_module_adv_data_t module_adv_data;
    
    uint8_t dev_id;
    uint8_t dev_type;
    
    uint8_t peer_mac_address[BT_AT_CMD_TTM_MAC_ADDRESS_LEN];
    
} private_flash_data_t;

void sdk_private_data_write_to_flash(void);

void sdk_load_private_flash_data(void);

extern private_flash_data_t g_power_off_save_data_in_ram;

#endif