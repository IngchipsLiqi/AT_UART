#ifndef _AT_CMD_TASK_H_
#define _AT_CMD_TASK_H_


#include <stdint.h>
#include <stdbool.h>
#include "gap.h"

#define ADV_REPORT_NUM  10
#define BLE_CONNECTION_MAX (20)

struct at_ctrl
{
    bool adv_ongoing;   //record adv status
    bool scan_ongoing;  //record scan status
    bool initialization_ongoing;    //record init status
    bool upgrade_start;
    bool async_evt_on_going;  //async event is on going, must wait util it end
    bool transparent_start;  //transparent flag
    bool one_slot_send_start;  //
    uint8_t curr_adv_int;  //current adv interval
    uint8_t transparent_conidx;  //
    uint32_t one_slot_send_len;  //
    uint16_t scan_duration; //uint: 10ms
};

#define IDLE_ROLE    (0)
#define SLAVE_ROLE   (1<<0)
#define MASTER_ROLE  (1<<1)
typedef struct at_defualt_info
{
    uint8_t role;    //0 = idle; 1 = salve; 2 = master;
    bool auto_transparent;   
    uint8_t rf_power;
    uint8_t adv_int;
    bool auto_sleep;
    uint8_t encryption_link;    //0 = idle; 'S' = salve enc; 'M' = master enc, 'B' = master bond;
} default_info_t;

struct at_conn_peer_param
{
    uint8_t encryption;
    uint8_t link_mode;
    conn_para_t conn_param;
};
struct recv_cmd_t
{
    uint16_t recv_length;
    uint8_t recv_data[];
};

struct at_adv_report
{
    uint8_t        evt_type;
    uint8_t        adv_addr_type;
    bd_addr_t      adv_addr;
    uint8_t        data_len;
    uint8_t        data[0x1F];
    ///RSSI value for advertising packet (in dBm, between -127 and +20 dBm)
    int8_t         rssi;
};

struct at_buff_env
{
    uint16_t flash_page_idx;
    uint16_t flash_write_cnt;
    default_info_t default_info;
    // uart_param_t uart_param;
    struct at_conn_peer_param master_peer_param;
    struct at_conn_peer_param peer_param[BLE_CONNECTION_MAX];
    struct at_adv_report adv_rpt[ADV_REPORT_NUM];
};

extern struct at_ctrl gAT_ctrl_env;
extern struct at_buff_env gAT_buff_env;

/*********************************************************************
 * @fn      at_recv_cmd_handler
 *
 * @brief   
 *			
 *
 * @param   
 *       	 
 *
 * @return  None
 */
void at_recv_cmd_handler(struct recv_cmd_t *param);

/*********************************************************************
 * @fn      at_send_rsp
 *
 * @brief   Common function for at command execution result response sending 
 *			
 *
 * @param   str - AT command execution result string. 
 *       	 
 *
 * @return  None
 */
void at_send_rsp(char *str);

/*********************************************************************
 * @fn      gap_get_connect_status
 *
 * @brief   get the connection status of the specified index
 *			
 *
 * @param   index of connection 
 *       	 
 *
 * @return  1 if has connection, otherwise 0
 */
int gap_get_connect_status(int idx);
 
#endif
