#ifndef _AT_CMD_TASK_H_
#define _AT_CMD_TASK_H_

#include "common/preh.h"
#include <stdint.h>
#include <stdbool.h>
#include "gap.h"
#include "btstack_event.h"
#include "gatt_client_util.h"

#define LOCAL_NAME_MAX_LEN      27
#define ADV_REPORT_NUM          10

struct recv_cmd_t
{
    uint16_t recv_length;
    uint8_t recv_data[];
};


/**************************************************************************************
 * RAM控制信息 at_ctrl
 * - 表示正在执行的BLE动作：adv/scan/initialization_ongoing
 * - 表示正在处理AT指令：async_evt_on_going，如果正在处理指令，则忽略期间产生的任何新AT指令
 * - 透传控制标志：transparent_start
 * - 单次透传控制标志：one_slot_send_start
 * - 透传目标链接索引号：transparent_conidx
 * at_buff_env
 * - 扫描到的广播report：at_adv_report
 **************************************************************************************/
#define TRANSMETHOD_M2S_W 0
#define TRANSMETHOD_S2M_N 1

struct at_ctrl
{
    bool adv_ongoing;   //record adv status
    bool scan_ongoing;  //record scan status
    bool initialization_ongoing;    //record init status
    bool async_evt_on_going;  //async event is on going, must wait util it end
    bool transparent_start;  //transparent flag
    bool one_slot_send_start;  //
    uint8_t curr_adv_int;  //current adv interval
    uint8_t transparent_conidx;  //
    uint8_t transparent_method;  // 0:master->slave write char, 1:slave->master notify
    bool transparent_notify_enable;  // true if enable
    uint32_t one_slot_send_len;  //
    uint16_t scan_duration; //uint: 10ms
};

struct at_adv_report
{
    uint8_t        evt_type;
    bd_addr_type_t adv_addr_type;
    bd_addr_t      adv_addr;
    uint8_t        data_len;
    uint8_t        data[0x1F];
    ///RSSI value for advertising packet (in dBm, between -127 and +20 dBm)
    int8_t         rssi;
};

struct at_buff_env
{
    struct at_adv_report adv_rpt[ADV_REPORT_NUM];
};
/**************************************************************************************/


/**************************************************************************************
 * 记录链接信息的结构 conn_info_t
 * - 链接参数：min_interval, max_interval, cur_interval, latency, timeout;
 * - GATT Server Profile：gatt_client_discoverer
 * - 每个链接都会有一个通知句柄链表：notification_handler_t
 * - 对端地址/地址类型
 **************************************************************************************/
typedef struct notification_handler
{
    struct notification_handler *next;
    uint8_t registered;
    uint16_t value_handle;
    uint16_t desc_handle;
    uint16_t config;
    gatt_client_notification_t notification;
} notification_handler_t;

struct write_char_info
{
    uint16_t value_handle;
    const uint8_t *data;
};

struct gatts_value_info
{
    uint16_t value_handle;
    const uint8_t *data;
};

typedef struct
{
    hci_con_handle_t handle;
    bd_addr_type_t peer_addr_type;
    bd_addr_t peer_addr;
    uint16_t min_interval, max_interval, cur_interval, latency, timeout;
    notification_handler_t *first_handler;
    struct gatt_client_discoverer *discoverer;

    struct write_char_info write_char_info;
    struct gatts_value_info gatts_value_info;
} conn_info_t;
/**************************************************************************************/


extern struct at_ctrl gAT_ctrl_env;
extern struct at_buff_env gAT_buff_env;
extern const int16_t rf_power_arr[];
extern const uint16_t adv_int_arr[];


/*********************************************************************
 * @fn      uart_at_start
 * @brief   init function
 */
void uart_at_start();


/*********************************************************************
 * @fn      get_id_by_handle
 * @param   handle
 * @return  link_id
 */
uint8_t get_id_by_handle(uint16_t handle);

/*********************************************************************
 * @fn      get_handle_by_id
 * @param   link_id
 * @return  handle
 */
uint16_t get_handle_by_id(uint8_t id) ;

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

/*********************************************************************
 * @fn      gap_get_connect_num
 *
 * @brief   get the connection num
 *			
 *
 * @param   None.
 *       	 
 *
 * @return  connect num
 */
int gap_get_connect_num();
 
/*********************************************************************
 * @fn      system_sleep_enable
 *
 * @brief   enable system enter deep sleep mode when all conditions are satisfied.
 *
 * @param   None.
 *
 * @return  None.
 */
void system_sleep_enable(void);

/*********************************************************************
 * @fn      system_sleep_disable
 *
 * @brief   disable system enter deep sleep mode.
 *
 * @param   None.
 *
 * @return  None.
 */
void system_sleep_disable(void);



/*********************************************************************
 * @fn      auto_transparent_clr
 *
 * @brief   Clear transparent mode flag and clear related profile data receive function. 
 *			
 *
 * @param   None
 *       	
 *
 * @return  None
 */
void auto_transparent_clr(void);


/*********************************************************************
 * @fn      at_on_connection_complete
 *
 * @brief   
 *			
 * @param[in]  complete connection data package
 *
 * @return  None.
 */
void at_on_connection_complete(const le_meta_event_enh_create_conn_complete_t *complete);


/*********************************************************************
 * @fn      at_on_disconnect
 *
 * @brief   
 *			
 * @param[in]  complete disconnection data package
 *
 * @return  None.
 */
void at_on_disconnect(const event_disconn_complete_t *complete);
#endif
