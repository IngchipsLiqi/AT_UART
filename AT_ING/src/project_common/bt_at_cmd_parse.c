
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#include <stdio.h>
#include <string.h>

#include "btstack_event.h"

#include "bt_at_cmd_parse.h"
#include "sdk_private_flash_data.h"
#include "project_common.h"

#if defined __cplusplus
    extern "C" {
#endif

typedef int (*pfunbt_at_cmd_parse)(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength);
static int bt_at_handle_cona(uint8_t const *in_buff, uint16_t inlength, uint8_t *out_buff, uint16_t *outlength);
static int bt_at_handle_role(uint8_t const *in_buff, uint16_t inlength, uint8_t *out_buff, uint16_t *outlength);
static int bt_at_handle_addr(uint8_t const *in_buff, uint16_t inlength, uint8_t *out_buff, uint16_t *outlength);

#define AT_INSTRUCTION_HEADER_LEN    3 // AT+
#define AT_INSTRUCTION_BODY_MAX_LEN 10 // CONA
#define AT_INSTRUCTION_PDU_MAX_LEN  20 // FA3456789300

typedef struct
{
    uint8_t at_body_str[AT_INSTRUCTION_BODY_MAX_LEN];
    uint8_t at_body_str_len;
    pfunbt_at_cmd_parse at_fun;
} at_instruction_set_body_t;

typedef struct
{
    uint8_t at_header[AT_INSTRUCTION_HEADER_LEN];
    at_instruction_set_body_t *at_body;
} at_instruction_set_t;

#define CMD_AT_STR_CONA  "CONA"
#define CMD_AT_STR_ROLE  "ROLE"
#define CMD_AT_STR_ADDR  "LADDR"

static at_instruction_set_body_t at_set_body[] =
{
    {CMD_AT_STR_CONA,    sizeof(CMD_AT_STR_CONA) - 1,    bt_at_handle_cona},
    {CMD_AT_STR_ROLE,    sizeof(CMD_AT_STR_ROLE) - 1,    bt_at_handle_role},
    {CMD_AT_STR_ADDR,    sizeof(CMD_AT_STR_ADDR) - 1,    bt_at_handle_addr},
};

const static at_instruction_set_t at_set =
{
    .at_header = {'A', 'T', '+'},
    .at_body = &at_set_body[0],
};
static int at_set_num = sizeof(at_set_body) / sizeof(at_set_body[0]);

static int bt_at_handle_cona(uint8_t const *in_buff, uint16_t inlength, uint8_t *out_buff, uint16_t *outlength)
{
    const uint8_t *pdu = &in_buff[AT_INSTRUCTION_HEADER_LEN + sizeof(CMD_AT_STR_CONA) - 1];
    int pdu_index;
    uint8_t pdu_data;
    uint8_t peer_addr[6];

    if (inlength < (AT_INSTRUCTION_HEADER_LEN + sizeof(CMD_AT_STR_CONA) - 1 + 12)) {
        return BT_PRIVT_AT_PDU_ERROR;
    }

    for (pdu_index = 0; pdu_index < 12; pdu_index++) {
        if ((pdu[pdu_index] >= '0') && (pdu[pdu_index] <= '9')) {
            pdu_data = pdu[pdu_index] - '0';
        } else if ((pdu[pdu_index] >= 'a') && (pdu[pdu_index] <= 'f')) {
            pdu_data = pdu[pdu_index] - 'a' + 10;
        } else if ((pdu[pdu_index] >= 'A') && (pdu[pdu_index] <= 'F')) {
            pdu_data = pdu[pdu_index] - 'A' + 10;
        } else {
            return BT_PRIVT_AT_PDU_ERROR;
        }
        if ((pdu_index % 2) == 0) {
            peer_addr[pdu_index / 2] = (pdu_data << 4);
        } else {
            peer_addr[pdu_index / 2] |= pdu_data;
        }
    }

    memcpy(g_power_off_save_data_in_ram.peer_mac_address, peer_addr, sizeof(g_power_off_save_data_in_ram.peer_mac_address));
    g_power_off_save_data_in_ram.dev_type = BLE_DEV_TYPE_MASTER;
    sdk_private_data_write_to_flash();
    print_addr(peer_addr);
    btstack_push_user_msg(USER_MSG_ID_START_CONNECTION, NULL, 0);
    sprintf(out_buff, "+OK");
    return BT_PRIVT_OK;
}
static int bt_at_handle_role(uint8_t const *in_buff, uint16_t inlength, uint8_t *out_buff, uint16_t *outlength)
{
    dbg_printf("[AT]: role\r\n");
    return BT_PRIVT_ERROR;
}
static int bt_at_handle_addr(uint8_t const *in_buff, uint16_t inlength, uint8_t *out_buff, uint16_t *outlength)
{
    const uint8_t *pdu = &in_buff[AT_INSTRUCTION_HEADER_LEN + sizeof(CMD_AT_STR_ADDR) - 1];
    int pdu_index;
    uint8_t pdu_data;
    uint8_t addr[6];

    if (inlength < (AT_INSTRUCTION_HEADER_LEN + sizeof(CMD_AT_STR_ADDR) - 1 + 12)) {
        print_addr(g_power_off_save_data_in_ram.module_mac_address);
        sprintf((char *)out_buff, "+LADDR=%02x%02x%02x%02x%02x%02x",
                g_power_off_save_data_in_ram.module_mac_address[0],
                g_power_off_save_data_in_ram.module_mac_address[1],
                g_power_off_save_data_in_ram.module_mac_address[2],
                g_power_off_save_data_in_ram.module_mac_address[3],
                g_power_off_save_data_in_ram.module_mac_address[4],
                g_power_off_save_data_in_ram.module_mac_address[5]);
        return BT_PRIVT_OK;
    }

    for (pdu_index = 0; pdu_index < 12; pdu_index++) {
        if ((pdu[pdu_index] >= '0') && (pdu[pdu_index] <= '9')) {
            pdu_data = pdu[pdu_index] - '0';
        } else if ((pdu[pdu_index] >= 'a') && (pdu[pdu_index] <= 'f')) {
            pdu_data = pdu[pdu_index] - 'a' + 10;
        } else if ((pdu[pdu_index] >= 'A') && (pdu[pdu_index] <= 'F')) {
            pdu_data = pdu[pdu_index] - 'A' + 10;
        } else {
            return BT_PRIVT_AT_PDU_ERROR;
        }
        if ((pdu_index % 2) == 0) {
            addr[pdu_index / 2] = (pdu_data << 4);
        } else {
            addr[pdu_index / 2] |= pdu_data;
        }
    }

    memcpy(g_power_off_save_data_in_ram.module_mac_address, addr, sizeof(g_power_off_save_data_in_ram.module_mac_address));
    g_power_off_save_data_in_ram.dev_type = BLE_DEV_TYPE_SLAVE;
    sdk_private_data_write_to_flash();
    print_addr(addr);
    btstack_push_user_msg(USER_MSG_ID_START_ADV, NULL, 0);
    sprintf(out_buff, "+OK");
    return BT_PRIVT_OK;
}

// AT+CONAFA3456789300
int baec_msg_handle(uint8_t const *in_buff, uint16_t inlength, uint8_t *out_buff, uint16_t *outlength)
{
    int in_buff_index;
    int at_set_index;
    int ret;

    dbg_printf("[AT]: %s\r\n", in_buff);

    for (in_buff_index = 0; in_buff_index < AT_INSTRUCTION_HEADER_LEN; in_buff_index++) {
        if (in_buff[in_buff_index] == at_set.at_header[in_buff_index]) {
            continue;
        } else {
            return BT_PRIVT_AT_HEADER_ERROR;
        }
    }

    ret = BT_PRIVT_AT_BODY_ERROR;
    for (at_set_index = 0; at_set_index < at_set_num; at_set_index++) {
        if (memcmp(&in_buff[in_buff_index], at_set.at_body[at_set_index].at_body_str, at_set.at_body[at_set_index].at_body_str_len) == 0) {
            ret = at_set.at_body[at_set_index].at_fun(in_buff, inlength, out_buff, outlength);
            break;
        } else {
            continue;
        }
    }
    return ret;
}

#if defined __cplusplus
    }
#endif

