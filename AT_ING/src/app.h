
/*
** COPYRIGHT (c) 2023 by INGCHIPS
*/

#ifndef __APP_H__
#define __APP_H__

#include "stdint.h"
#include "platform_api.h"

#if defined __cplusplus
    extern "C" {
#endif

// release DEBUG=0 RELEASE_VER=1
#define DEBUG 0
#define RELEASE_VER 0

#ifndef DEBUG
    #warning DEBUG should be defined 0 or 1
    #define DEBUG 0
#endif

#ifndef RELEASE_VER
    #warning RELEASE_VER should be defined 0 or 1
    #define RELEASE_VER 1
#endif

#if (DEBUG == 0)
    #define dbg_printf(...)
    #define debug_out(...)
#else
    #define dbg_printf(...) platform_printf(__VA_ARGS__)
    #define debug_out(...) platform_printf(__VA_ARGS__)
#endif

#if (RELEASE_VER == 1)
    #define log_printf(...)
#else
    #define log_printf(...) platform_printf(__VA_ARGS__)
#endif

#define COM_PORT APB_UART1

#define CMD_IO_PORT_BUF_LEN_COM_IN 800
#define CMD_IO_PORT_BUF_COM_RX_TO_BLE_SEND_THRESHOLD (CMD_IO_PORT_BUF_LEN_COM_IN / 4)

#define CMD_IO_PORT_BUF_LEN_BLE_IN 800

typedef enum {
    USER_MSG_ID_PROFILE                               = 0x90,
    USER_MSG_ID_FIRST_WAKE_UP                               ,
    USER_MSG_ID_START_CONNECTION                            ,
    USER_MSG_INITIATE_TIMOUT                                ,
    USER_MSG_ID_PROFILE_END                                 ,

    USER_MSG_ID_ADV                                   = 0xA0,
    USER_MSG_ID_START_ADV                                   ,
    USER_MSG_ID_ADV_END                                     ,

    USER_MSG_ID_HID                                   = 0xB0,
    USER_MSG_ID_HID_END                                     ,

    USER_MSG_ID_THROUGHPUT                            = 0xC0,
    USER_MSG_ID_THROUGHPUT_REQUEST_SEND                     ,
    USER_MSG_ID_UART_RX_TO_BLE_DATA_SERVICE                 ,
    USER_MSG_ID_SEND_DATA                                   ,
    USER_MSG_ID_THROUGHPUT_END                              ,

    USER_MSG_ID_BATTERY                               = 0xD0,
    USER_MSG_ID_BATTERY_END                                 ,

    USER_MSG_ID_SM                                    = 0xE0,
    USER_MSG_ID_SM_END                                      ,

    USER_MSG_ID_INGCHIPS_VOICE                        = 0xF0,
    USER_MSG_ID_INGCHIPS_VOICE_END                          ,

    USER_MSG_ID_MAX,
} user_msg_id_all_t;

#define PRIVATE_FlASH_DATA_IS_INIT     0xA55A0203
#define PRIVATE_FLASH_DATA_START_ADD   0x02040000

#define DB_FLASH_ADDRESS               0x02042000
#define COM_PORT_DEFAULT_BAUD_RATE     115200

void app_setup_peripherals(void);
void app_setup_peripherals_before_sleep(void);
void app_start(void);
void app_low_power_exit_callback(void);
void print_addr(uint8_t *addr);
void dump_ram_data_in_char(uint8_t *p_data, uint16_t data_len);

#if defined __cplusplus
    }
#endif

#endif

