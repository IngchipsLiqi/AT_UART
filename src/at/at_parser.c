#include "at_parser.h"

#include <stdlib.h>

#include "btstack_event.h"

#include "../util/utils.h"
#include "../common/flash_data.h"
#include "../at_recv_cmd.h"


extern private_flash_data_t g_power_off_save_data_in_ram;
extern uint8_t need_connection_ret;
extern initiating_phy_config_t phy_configs[];

uint32_t cmd_list_size;
char cmd_list[CMD_LIST_CAPACITY][CMD_MAX_SIZE + 1];

uint32_t device_list_size;
char device_list[DEVICE_LIST_CAPACITY][BT_AT_CMD_TTM_MAC_ADDRESS_LEN];

#define STATE_READ_CMD_START 0
#define STATE_READ_CMD_END 1

static void at_store_cmd(uint8_t* buf, uint32_t begin, uint32_t end)
{
    if (cmd_list_size == CMD_LIST_CAPACITY)
        return;
    
    uint32_t cmd_size = end - begin + 1;
    
    if (cmd_size > CMD_MAX_SIZE)
        return;
    
    memcpy(cmd_list[cmd_list_size], buf + begin, cmd_size);
    *(cmd_list[cmd_list_size] + cmd_size) = '\0';
    cmd_list_size += 1;
}


uint32_t at_contains_device(uint8_t* le_mac_addr)
{
    for (uint32_t i = 0; i < device_list_size; ++i) {
        if (memcmp(device_list[i], le_mac_addr, 6) == 0)
            return 1;
    }
    return 0;
}

void at_processor_add_scan_device(uint8_t* le_mac_addr) 
{
    if (device_list_size == DEVICE_LIST_CAPACITY)
        return;
    
    if (at_contains_device(le_mac_addr))
        return;
    
    memcpy(device_list[device_list_size], le_mac_addr, 6);
    device_list_size ++;
}



static void at_process_command_le_scan() 
{
    char buffer[256] = {0};
    
    if (device_list_size <= 0) {
        sprintf(buffer, "None");
        uart_io_print(buffer);
        return;
    }
    
    sprintf(buffer, "Scan Device Total:%d\r\n", device_list_size);
    uart_io_print(buffer);
    for (uint32_t i = 0; i < device_list_size; ++i) {
        sprintf(buffer, "Device%d:%02X%02X%02X%02X%02X%02X ", i, device_list[i][0]
                                                        , device_list[i][1]
                                                        , device_list[i][2]
                                                        , device_list[i][3]
                                                        , device_list[i][4]
                                                        , device_list[i][5]);
        uart_io_print(buffer);
    }
}

static void at_process_command_le_scan_connect(char* mac)
{
    uint8_t le_mac[6] = {0};
    
    LOG_MSG("<mac>:%s\r\n", mac);
    
    str2hex(mac, 12, le_mac, 6);
    
    memcpy(g_power_off_save_data_in_ram.peer_mac_address, le_mac, 6);
    
    //sdk_private_data_write_to_flash();
    
    btstack_push_user_msg(USER_MSG_CREATE_CONNECTION, g_power_off_save_data_in_ram.peer_mac_address, 6);
    
    char buffer[24] = {0};
    sprintf(buffer, "connecting...\r\n");
    uart_io_print(buffer);
    
    need_connection_ret = 1;
    
    device_list_size = 0;  // Clear
}

static uint8_t num_buf[10] = {0};

static void at_process_command_set_conn_interval(char* cmd, int len)
{
    memcpy(num_buf, cmd, len);
    num_buf[len] = '\0';
    
    int ms = atoi((const char*)num_buf);
    
    int interval = ms * 0.625f;
    
    LOG_MSG("interval:%d\r\n", interval);
    
    phy_configs[0].conn_param.interval_min = interval;
    phy_configs[0].conn_param.interval_max = interval;
    
    uart_io_print("+READY\r\n");
}


static void at_process_command(char* cmd)
{
    LOG_MSG("%s\r\n", cmd);
    uint32_t len = strlen(cmd);
    
    if (strcmp(cmd, "AT+LESCAN?\r\n") == 0) {
        at_process_command_le_scan();
    } else if (memcmp(cmd, "AT+LESCAN=", 10) == 0) {
        at_process_command_le_scan_connect(cmd + 10);
    } else if (memcmp(cmd, "AT+CIT=", 7) == 0) {
        at_process_command_set_conn_interval(cmd + 7, len - 9);
    }
}

static void at_process_buffer()
{
    for (uint32_t i = 0; i < cmd_list_size; ++i) {
        at_process_command(cmd_list[i]);
    }
    cmd_list_size = 0; // Clear
}


void init_at_processor()
{
    cmd_list_size = 0;
    memset(cmd_list, 0, sizeof(cmd_list));
    device_list_size = 0;
    memset(device_list, 0, sizeof(device_list));
}

void at_processor_start(uint8_t* buf, uint32_t size)
{
    uint32_t i = 2; 
    
    uint8_t state = STATE_READ_CMD_START;
    
    uint32_t cmd_start_index = 0;       // -> 'A' 
    uint32_t cmd_end_index = 0;         // -> '\n'
    
    LOG_MSG("start parse\r\n");
    
    while(i < size)
    {
        if (state == STATE_READ_CMD_START) {
            if (buf[i - 2] == 'A' && buf[i - 1] == 'T' && buf[i] == '+') {
                
                cmd_start_index = i - 2;
                
                state = STATE_READ_CMD_END;
            }
        } else if (state == STATE_READ_CMD_END) {
            if (buf[i - 1] == '\r' && buf[i] == '\n') {
                
                cmd_end_index = i;
                
                at_store_cmd(buf, cmd_start_index, cmd_end_index);
                
                state = STATE_READ_CMD_START;
            }
        }
        i++;
    }
    
    LOG_MSG("at_cmd count:%d\r\n", cmd_list_size);
    
    if (cmd_list_size > 0) {
        at_process_buffer();
    }
}









