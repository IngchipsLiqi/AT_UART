#ifndef __PREH_H__
#define __PREH_H__

//#define ROLE_MASTER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <platform_api.h>
#include "ota_service.h"

#define  LOG_LEVEL      3
#define  LOG_LEVEL_MSG  2
#if (LOG_LEVEL <= LOG_LEVEL_MSG)
#define LOG_MSG(...)   platform_printf( __VA_ARGS__)
#else
#define LOG_MSG(...)
#endif

typedef enum {
    BLE_DEV_TYPE_MASTER,
    BLE_DEV_TYPE_SLAVE,
    BLE_DEV_TYPE_NO_CONNECTION,
} bt_ble_dev_type_e;

#define PRIVATE_FlASH_DATA_IS_INIT 0xAABBCCDD
#define BT_AT_CMD_TTM_MODULE_NAME_MAX_LEN 16
#define BT_AT_CMD_TTM_MAC_ADDRESS_LEN 6

#define MAC_ADDR_LEN 6
#define UUID_SIZE_2 2
#define UUID_SIZE_16 16
//#define BLE_CONNECTION_MAX (20)

#define ADV_LED_OFF __NOP()
#define ADV_LED_ON __NOP()
#define LINK_LED_OFF __NOP()
#define LINK_LED_ON __NOP()

#define PRINT_PORT      APB_UART0

#ifndef MAX_CONN_AS_SLAVE
#define MAX_CONN_AS_SLAVE           2
#endif

#ifndef MAX_CONN_AS_MASTER
#define MAX_CONN_AS_MASTER          8
#endif

#define TOTAL_CONN_NUM              (MAX_CONN_AS_MASTER + MAX_CONN_AS_SLAVE)

#define BT_PRIVT_OK 0
#define BT_PRIVT_ERROR -1
#define BT_PRIVT_ENABLE 1
#define BT_PRIVT_DISABLE 0

#define INVALID_HANDLE 0xFFFF


#define RX_INPUT_BUFFER_CAPACITY 300
#define TX_OUTPUT_BUFFER_CAPACITY 300

#define BUFFER_THRESHOLD_TOP 800
#define BUFFER_THRESHOLD_BOTTOM 200

#define USER_MSG_SEND_DATA_RX_TO_BLE 0x01
#define USER_MSG_SEND_DATA_TO_BLE_SLAVE 0x02
#define USER_MSG_SEND_DATA_TO_BLE_MASTER 0x03
#define USER_MSG_CREATE_CONNECTION 0x04
#define USER_MSG_PROCESS_BLE_MASTER_DATA 0x05
#define USER_MSG_AT_RECV_CMD   0x06
#define USER_MSG_AT_TRANSPARENT_START_TIMER 0x07
#define USER_MSG_AT_RECV_TRANSPARENT_DATA 0x08

#define USER_MSG_START_SEND 0x09



#define FLOW_CONTROL_MASTER_SEND_DATA_TO_SLAVE 0x01
#define FLOW_CONTROL_SLAVE_SEND_DATA_TO_MASTER 0x02


#include "ingsoc.h"

#include "bluetooth.h"

typedef bd_addr_t mac_addr_t;

#endif