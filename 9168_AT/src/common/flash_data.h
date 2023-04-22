#ifndef __FLASH_DATA_H__
#define __FLASH_DATA_H__

#include "../common/preh.h"

#include "eflash.h"
#include "gap.h"


#if (INGCHIPS_FAMILY == INGCHIPS_FAMILY_918)
#define PRIVATE_FLASH_DATA_START_ADD 0x40000
#elif (INGCHIPS_FAMILY == INGCHIPS_FAMILY_916)
#define PRIVATE_FLASH_DATA_START_ADD 0x2141000
#else
    #error unknown or unsupported chip family
#endif

typedef struct {
    bd_addr_t addr;
    bd_addr_type_t addr_type;
    uint8_t encryption;
    uint8_t link_mode;
    phy_type_t phy;
    conn_para_t conn_param;
} private_module_conn_peer_param_t;

typedef struct {
    uint8_t flags[3];
    uint8_t local_name_len;
    uint8_t local_name_handle;
    uint8_t local_name[BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN];
} private_module_adv_data_t;

#define IDLE_ROLE    (0)
#define SLAVE_ROLE   (1<<0)
#define MASTER_ROLE  (1<<1)
typedef struct {
    uint8_t role;               //0 = idle; 1 = salve; 2 = master;
    bool auto_sleep;
    int wakeup_source;
    int wakeup_level;
    uint16_t rf_power;
    uint32_t adv_int;
    uint8_t encryption_link;    //0 = idle; 'S' = salve enc; 'M' = master enc, 'B' = master bond;
    //...
    //...
} private_module_default_info_t;

typedef struct
{
    uint32_t baud_rate;     //uart baudrate
    uint8_t data_bit_num;   //payload data len, valid num:5,6,7,8
    uint8_t pari;           //0 = parity is disable,  1 = odd parity is enable, 2 = even parity is enable,
    uint8_t stop_bit;       //1 = stop bit is 1,  2 = stop bit is 1.5 for 5bit data OR stop bit is 2 for 6,7,8 bit data,
} uart_param_t;

#define ADV_DATA_MAC_LEN 32
typedef struct 
{
    char name_prefix[ADV_DATA_MAC_LEN];
    char name_suffix[ADV_DATA_MAC_LEN];
    uint8_t uuid_16[UUID_SIZE_2];
    uint8_t uuid_128[UUID_SIZE_16];
    int8_t rssi;
    bool enable_uuid_16_filter;
    bool enable_uuid_128_filter;
    bool enable_rssi_filter;
} scan_filter_t;

typedef struct {
    uint32_t data_init_flag;
    uint32_t data_len;
    
    char module_name[BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN + 1];
    uint8_t module_mac_address[BT_AT_CMD_TTM_MAC_ADDRESS_LEN];
    
    private_module_adv_data_t module_adv_data;
    
    uint8_t dev_id;
    uint8_t dev_type;
    
    private_module_default_info_t default_info;
    
    UART_sStateStruct uart_param;
    scan_filter_t scan_filter;
    
    uint8_t peer_mac_address[BT_AT_CMD_TTM_MAC_ADDRESS_LEN];
    
    private_module_conn_peer_param_t master_peer_param;
    private_module_conn_peer_param_t peer_param[BLE_CONNECTION_MAX];
    
    uint8_t serivce_uuid[UUID_SIZE_16];
    uint8_t characteristic_input_uuid[UUID_SIZE_16];
    uint8_t characteristic_output_uuid[UUID_SIZE_16];
    
} private_flash_data_t;

void sdk_private_data_write_to_flash(void);

void sdk_load_private_flash_data(void);

extern private_flash_data_t g_power_off_save_data_in_ram;

#endif