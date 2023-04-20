#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "at_cmd_task.h"
#include "at_recv_cmd.h"
#include "common/flash_data.h"
#include "util/utils.h"
#include "service/transmission_service.h"
#include "le_device_db.h"
#include "profile.h"

#include "gap.h"


#define LOCAL_NAME_MAX_LEN      (27)
struct at_ctrl gAT_ctrl_env = {0};
struct at_buff_env gAT_buff_env = {0};

enum
{
    AT_CMD_IDX_NAME,
    AT_CMD_IDX_MODE,
    AT_CMD_IDX_MAC,
    AT_CMD_IDX_CIVER,
    AT_CMD_IDX_UART,
    AT_CMD_IDX_Z,
    AT_CMD_IDX_CLR_BOND,
    AT_CMD_IDX_LINK,
    AT_CMD_IDX_ENC,
    AT_CMD_IDX_SCAN,
    AT_CMD_IDX_APP,
    AT_CMD_IDX_CONNADD,
    AT_CMD_IDX_CONN,
    AT_CMD_IDX_SLEEP,
    AT_CMD_IDX_UUID,
    AT_CMD_IDX_DISCONN,
    AT_CMD_IDX_FLASH,
    AT_CMD_IDX_SEND,
    AT_CMD_IDX_TRANSPARENT,
    AT_CMD_IDX_AUTO_TRANSPARENT,
    AT_CMD_IDX_POWER,
    AT_CMD_IDX_ADVINT,
    AT_CMD_IDX_CLR_DFT,
};
const char *cmds[] =
{
    [AT_CMD_IDX_NAME] = "NAME",
    [AT_CMD_IDX_MODE] = "MODE",
    [AT_CMD_IDX_MAC] = "MAC",
    [AT_CMD_IDX_CIVER] = "CIVER",
    [AT_CMD_IDX_UART] = "UART",
    [AT_CMD_IDX_Z] = "Z",
    [AT_CMD_IDX_CLR_BOND] = "CLR_BOND",
    [AT_CMD_IDX_LINK] = "LINK",
    [AT_CMD_IDX_ENC] = "ENC",
    [AT_CMD_IDX_SCAN] = "SCAN",
    [AT_CMD_IDX_APP] = "ADP",
    [AT_CMD_IDX_CONNADD] = "CONNADD",
    [AT_CMD_IDX_CONN] = "CONN",
    [AT_CMD_IDX_SLEEP] = "SLEEP",
    [AT_CMD_IDX_UUID] = "UUID",
    [AT_CMD_IDX_DISCONN] = "DISCONN",
    [AT_CMD_IDX_FLASH] = "FLASH",
    [AT_CMD_IDX_SEND] = "SEND",
    [AT_CMD_IDX_TRANSPARENT] = "+++",
    [AT_CMD_IDX_AUTO_TRANSPARENT] = "AUTO+++",
    [AT_CMD_IDX_POWER] = "POWER",
    [AT_CMD_IDX_ADVINT] = "ADVINT",
    [AT_CMD_IDX_CLR_DFT] = "CLR_INFO",
};

extern private_flash_data_t g_power_off_save_data_in_ram;

const int16_t rf_power_arr[6] = {5,2,0,-5,-10,-17}; //TODO 2.5dbm?arr[1]
const uint16_t adv_int_arr[6] = {80,160,320,800,1600,3200};

#define co_printf(...) platform_printf(__VA_ARGS__)

/*********************************************************************
 * @fn      at_init_adv_rsp_parameter
 *
 * @brief   Set advertising response data, only include local name element in rsp data
 *			
 *
 * @param   None
 *       	
 *
 * @return  None
 */
static void at_init_adv_rsp_parameter(void)
{
    const static ext_adv_set_en_t adv_sets_en[] = { {.handle = 0, .duration = 0, .max_events = 0} };
    gap_set_ext_adv_enable(0, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
    gap_set_ext_adv_data(0, sizeof(g_power_off_save_data_in_ram.module_adv_data), (uint8_t*)&g_power_off_save_data_in_ram.module_adv_data);
    gap_set_ext_adv_enable(1, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
}

/*********************************************************************
 * @fn      gap_set_dev_name
 *
 * @brief   set the device name
 *			
 *
 * @param   name
 * @param   size
 *       	 
 *
 * @return  None
 */
void gap_set_dev_name(uint8_t* name, uint32_t size)
{
    memcpy(g_power_off_save_data_in_ram.module_name, name, size);
    
    g_power_off_save_data_in_ram.module_adv_data.local_name_len = size; // -1 +1
    memcpy(g_power_off_save_data_in_ram.module_adv_data.local_name, name, size);
}

/*********************************************************************
 * @fn      gap_get_dev_name
 *
 * @brief   get the device name
 *			
 *
 * @param[out]  local_name  - out data name
 *       	 
 *
 * @return  name length
 */
int gap_get_dev_name(uint8_t* local_name) 
{
    int len = strlen(g_power_off_save_data_in_ram.module_name);
    
    strcpy((char*)local_name, g_power_off_save_data_in_ram.module_name);
    
    return len;
}

/*********************************************************************
 * @fn      gap_address_get
 *
 * @brief   get the address
 *			
 *
 * @param   addr    - out data 
 *       	 
 *
 * @return  None
 */
void gap_address_get(bd_addr_t addr)
{
    memcpy(addr, g_power_off_save_data_in_ram.module_mac_address, MAC_ADDR_LEN);
}

/*********************************************************************
 * @fn      gap_address_set
 *
 * @brief   set the address
 *			
 *
 * @param   addr
 *
 *
 * @return  None
 */
void gap_address_set(bd_addr_t addr)
{
    memcpy(g_power_off_save_data_in_ram.module_mac_address, addr, MAC_ADDR_LEN);
    
    const static ext_adv_set_en_t adv_sets_en[] = { {.handle = 0, .duration = 0, .max_events = 0} };
    gap_set_ext_adv_enable(0, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
    gap_set_adv_set_random_addr(0, g_power_off_save_data_in_ram.module_mac_address);
    gap_set_ext_adv_enable(1, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);
}

/*********************************************************************
 * @fn      hex_arr_to_str
 *
 * @brief   convert buffer data to hex str
 *			
 *
 * @param[in]   buff    - hex buffer
 * @param[in]   len     - hex buffer length
 * @param[out]  out_str - hex str
 *       	 
 *
 * @return  None
 */
void hex_arr_to_str(uint8_t* buff, uint16_t len, uint8_t* out_str)
{
    hex2str(buff, len, out_str);
}

/*********************************************************************
 * @fn      str_to_hex_arr
 *
 * @brief   convert hex str to data
 *			
 *
 * @param   in_str  - hex str
 * @param   buff    - buffer
 * @param   len     - buffer length
 *       	 
 *
 * @return  1 if has connection, otherwise 0
 */
void str_to_hex_arr(uint8_t* in_str, uint8_t* buff, uint16_t len)
{
    str2hex((char*)in_str, len * 2, buff, len);
}

bool gap_conn_table[BLE_CONNECTION_MAX] = {0}; //

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
int gap_get_connect_status(int idx)
{   
    return gap_conn_table[idx];
}


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
int gap_get_connect_num()
{
    int num = 0;
    for (int i = 0; i < BLE_CONNECTION_MAX; ++i)
        if (gap_conn_table[i])
            num++;
    return num;
}


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
void at_send_rsp(char *str)
{
    uart_put_data_noint(UART1,(uint8_t *)"\r\n", 2);
    uart_put_data_noint(UART1,(uint8_t *)str, strlen(str));
    uart_put_data_noint(UART1,(uint8_t *)"\r\n", 2);
}

/*********************************************************************
 * @fn      find_int_from_str
 *
 * @brief   Misc function, search and strim an integer number string from a string buffer, 
 *			
 *
 * @param   buff - pointer to string buffer, which will be searched.  
 *       	 
 *
 * @return  buffer start address of next string.
 */
static uint8_t *find_int_from_str(uint8_t *buff)
{
    uint8_t *pos = buff;
    while(1)
    {
        if(*pos == ',' || *pos == '\r')
        {
            *pos = 0;
            break;
        }
        pos++;
    }
    return pos;
}

/*********************************************************************
 * @fn      system_sleep_enable
 *
 * @brief   enable system enter deep sleep mode when all conditions are satisfied.
 *
 * @param   None.
 *
 * @return  None.
 */
void system_sleep_enable(void)
{
    platform_config(PLATFORM_CFG_POWER_SAVING, PLATFORM_CFG_ENABLE);
}

/*********************************************************************
 * @fn      system_sleep_disable
 *
 * @brief   disable system enter deep sleep mode.
 *
 * @param   None.
 *
 * @return  None.
 */
void system_sleep_disable(void)
{
    platform_config(PLATFORM_CFG_POWER_SAVING, PLATFORM_CFG_DISABLE);
}

/*********************************************************************
 * @fn      at_con_param_update
 *
 * @brief   function to update link parameters.
 *
 * @param   conidx  - indicate which link idx will do patameter updation 
 *       	latency - indicate latency for patameter updation operation 
 *
 * @return  None.
 */
void at_con_param_update(uint8_t conidx,uint16_t latency)
{
    conn_para_t *con_param = &g_power_off_save_data_in_ram.peer_param[conidx].conn_param;
    
    int current_con_interval = 15;  // TODO

    if( con_param->latency != latency || con_param->interval_min > current_con_interval || con_param->supervision_timeout != 400 )
    {
        gap_update_connection_parameters(conidx, 
            current_con_interval, 
            current_con_interval, 
            latency, 
            400, 
            con_param->min_ce_len, 
            con_param->max_ce_len);
    }
}

/*********************************************************************
 * @fn      gap_bond_manager_delete_all
 *
 * @brief   Erase all bond information.
 *
 * @param   None.
 *
 * @return  None.
 */
void gap_bond_manager_delete_all(void)
{
    le_device_memory_db_iter_t device_db_iter;
    le_device_db_iter_init(&device_db_iter);
    while (le_device_db_iter_next(&device_db_iter))
        le_device_db_remove_key(device_db_iter.key);
    platform_write_persistent_reg(1);
    platform_reset();
}

/*********************************************************************
 * @fn      convert_to_word_length
 *
 * @brief   convert at databit to word length.
 *
 * @param   databit.
 *
 * @return  enum word length.
 */
UART_eWLEN convert_to_word_length(int databit)
{
    switch (databit) 
    {
        case 8: 
            return UART_WLEN_8_BITS;
        case 7: 
            return UART_WLEN_7_BITS;
        case 6: 
            return UART_WLEN_6_BITS;
        case 5: 
            return UART_WLEN_5_BITS;
        default:
            return UART_WLEN_8_BITS;
    }
}

/*********************************************************************
 * @fn      convert_from_word_length
 *
 * @brief   convert word length to at databit.
 *
 * @param   word length.
 *
 * @return  databit.
 */
int convert_from_word_length(UART_eWLEN word_length)
{
    switch (word_length) 
    {
        case UART_WLEN_8_BITS: 
            return 8;
        case UART_WLEN_7_BITS: 
            return 7;
        case UART_WLEN_6_BITS: 
            return 6;
        case UART_WLEN_5_BITS: 
            return 5;
        default:
            return 8;
    }
}

/*********************************************************************
 * @fn      convert_to_parity
 *
 * @brief   convert at pari to uart parity
 *
 * @param   pari.
 *
 * @return  enum parity.
 */
UART_ePARITY convert_to_parity(int pari)
{
    switch (pari) 
    {
        case 0: 
            return UART_PARITY_NOT_CHECK;
        case 1: 
            return UART_PARITY_ODD_PARITY;
        case 2: 
            return UART_PARITY_EVEN_PARITY;
        default:
            return UART_PARITY_NOT_CHECK;
    }
}

/*********************************************************************
 * @fn      convert_to_parity
 *
 * @brief   convert uart parity to at pari
 *
 * @param   parity.
 *
 * @return  pari.
 */
int convert_from_parity(UART_ePARITY parity)
{
    switch (parity) 
    {
        case UART_PARITY_NOT_CHECK: 
            return 0;
        case UART_PARITY_ODD_PARITY: 
            return 1;
        case UART_PARITY_EVEN_PARITY: 
            return 2;
        default:
            return 0;
    }
}
int convert_to_two_stop_bits(int stop_bits)
{
    return stop_bits == 2 ? 1 : 0;
}
int convert_from_two_stop_bits(int two_stop_bits)
{
    return two_stop_bits == 1 ? 2 : 1;
}




/***********SCAN Handle***************/

/*********************************************************************
 * @fn      at_get_adv
 *
 * @brief   at event call back function, handle after advertising report is posted
 *			
 *
 * @param   arg  - pointer to advertising report buffer
 *       	
 *
 * @return   None
 */
void at_get_adv(void *arg)
{
    const le_meta_event_ext_adv_report_t *param = (const le_meta_event_ext_adv_report_t *)arg;
    const le_ext_adv_report_t* report = param->reports;
    
    
    uint8_t free_rpt_idx = 0xff;
    for(uint8_t idx = 0; idx<ADV_REPORT_NUM; idx++)
    {
        if(gAT_buff_env.adv_rpt[idx].evt_type == 0xff && free_rpt_idx == 0xff)
            free_rpt_idx = idx;
        if(memcmp(gAT_buff_env.adv_rpt[idx].adv_addr,report->address,sizeof(mac_addr_t)) == 0 )
            goto _exit;
    }
    
    gAT_buff_env.adv_rpt[free_rpt_idx].evt_type = report->evt_type;
    gAT_buff_env.adv_rpt[free_rpt_idx].adv_addr_type = report->addr_type;
    memcpy(gAT_buff_env.adv_rpt[free_rpt_idx].adv_addr,report->address,sizeof(mac_addr_t));
    gAT_buff_env.adv_rpt[free_rpt_idx].rssi = report->rssi;
    gAT_buff_env.adv_rpt[free_rpt_idx].data_len = report->data_len;
    memcpy(gAT_buff_env.adv_rpt[free_rpt_idx].data,report->data,report->data_len);
_exit:
    ;
}

/*********************************************************************
 * @fn      at_scan_done
 *
 * @brief   at event call back function, handle after scan operation end
 *			
 *
 * @param   arg  - pointer to buffer, which store scan done status
 *       	
 *
 * @return  None
 */
void at_scan_done(void *arg)
{
    uint8_t *at_rsp = malloc(150);
    uint8_t *addr_str = malloc(MAC_ADDR_LEN*2+1);
    uint8_t *rsp_data_str = malloc(0x1F*2+1);        //adv data len

    sprintf((char *)at_rsp,"+SCAN:ON\r\nOK");
    at_send_rsp((char *)at_rsp);

    for(uint8_t idx = 0; idx<ADV_REPORT_NUM; idx++)
    {
        //if(gAT_buff_env.adv_rpt[idx].evt_type ==0 || gAT_buff_env.adv_rpt[idx].evt_type ==2 || gAT_buff_env.adv_rpt[idx].evt_type ==8)
        //{
            hex_arr_to_str(gAT_buff_env.adv_rpt[idx].adv_addr,MAC_ADDR_LEN,addr_str);
            addr_str[MAC_ADDR_LEN * 2] = 0;

            if(gAT_buff_env.adv_rpt[idx].data_len != 0)
            {
                hex_arr_to_str(gAT_buff_env.adv_rpt[idx].data,gAT_buff_env.adv_rpt[idx].data_len,rsp_data_str);
                rsp_data_str[gAT_buff_env.adv_rpt[idx].data_len * 2] = 0;
            }
            else
                memcpy(rsp_data_str,"NONE",sizeof("NONE"));

            sprintf((char *)at_rsp,"\n\nNo: %d Addr:%s Type:%d Rssi:%ddBm\n\n\r\nAdv data: \r\n %s\r\n",idx
                    ,addr_str
                    ,gAT_buff_env.adv_rpt[idx].adv_addr_type
                    ,(signed char)gAT_buff_env.adv_rpt[idx].rssi
                    ,rsp_data_str);
            uart_put_data_noint(UART1,(uint8_t *)at_rsp, strlen((const char *)at_rsp));
        //}
        //else
        //    break;
    }
    free(rsp_data_str);
    free(addr_str);
    free(at_rsp);
    gAT_ctrl_env.async_evt_on_going = false;
}

/*********************************************************************
 * @fn      at_start_scan
 *
 * @brief   Start a active scan opertaion, duration is deceided by gAT_ctrl_env.scan_duration. Or 10s if gAT_ctrl_env.scan_duration is 0
 *			
 *
 * @param   None
 *       	
 *
 * @return  None
 */
void at_start_scan(void)
{
    uint16_t duration = 1000;
    if (gAT_ctrl_env.scan_duration != 0)
        duration = gAT_ctrl_env.scan_duration;
    start_continuous_scan(duration);
    
    gAT_ctrl_env.scan_ongoing = true;
}
/***********SCAN Handle***************/

/***********CONNECTION Handle***************/

/*********************************************************************
 * @fn      at_start_connecting
 *
 * @brief   Start a active connection opertaion
 *			
 *
 * @param   arg - reseved
 *       	
 *
 * @return  None
 */
void at_start_connecting(void *arg)
{
    bd_addr_t addr;
    memcpy(addr, g_power_off_save_data_in_ram.master_peer_param.addr, MAC_ADDR_LEN);
    uint8_t addr_type = g_power_off_save_data_in_ram.master_peer_param.addr_type;
    

    initiating_phy_config_t phy_config =
    {
        .phy = PHY_1M,
        .conn_param =
        {
            .scan_int = 200,
            .scan_win = 180,
            .interval_min = 16,
            .interval_max = 16,
            .latency = 0,
            .supervision_timeout = 400,
            .min_ce_len = 80,
            .max_ce_len = 80
        }
    };
    gap_ext_create_connection(INITIATING_ADVERTISER_FROM_PARAM, // Initiator_Filter_Policy,
                              BD_ADDR_TYPE_LE_RANDOM,           // Own_Address_Type,
                              addr_type,                        // Peer_Address_Type,
                              addr,                             // Peer_Address,
                              sizeof(phy_config),
                              &phy_config);
    
    gAT_ctrl_env.initialization_ongoing = true;
}
/***********CONNECTION Handle***************/

/***********ADVERTISIING Handle***************/

/*********************************************************************
 * @fn      at_start_advertising
 *
 * @brief   Start an advertising action 
 *			
 *
 * @param   arg - reserved
 *       	
 *
 * @return  None
 */
void at_start_advertising(void *arg)
{
    if (gAT_ctrl_env.curr_adv_int != adv_int_arr[gAT_buff_env.default_info.adv_int])
        config_adv_and_set_interval(adv_int_arr[gAT_buff_env.default_info.adv_int]);
    start_adv();
    gAT_ctrl_env.adv_ongoing = true;
    ADV_LED_ON;
}

/*********************************************************************
 * @fn      at_cb_adv_end
 *
 * @brief   at event call back function, handle after adv action end.
 *			
 *
 * @param   arg - reserved
 *       	
 *
 * @return  None
 */
void at_cb_adv_end(void *arg)
{
    if(gAT_buff_env.default_info.role & SLAVE_ROLE)     //B mode
        at_start_advertising(NULL);
    else
        ADV_LED_OFF;
}
/***********ADVERTISIING Handle***************/

/*********************************************************************
 * @fn      at_idle_status_hdl
 *
 * @brief   at event call back function, handle after all adv actions stop, include advtertising, scan and conn actions.
 *			
 *
 * @param   arg - reserved
 *       	
 *
 * @return  None
 */
void at_idle_status_hdl(void *arg)
{
    if(gAT_ctrl_env.async_evt_on_going
       && gAT_ctrl_env.adv_ongoing == false
       && gAT_ctrl_env.scan_ongoing == false
       && gAT_ctrl_env.initialization_ongoing == false)
    {
        uint8_t *at_rsp = malloc(150);
        sprintf((char *)at_rsp,"+MODE:I\r\nOK");
        at_send_rsp((char *)at_rsp);
        free(at_rsp);
        gAT_ctrl_env.async_evt_on_going = false;
    }
}

/*********************************************************************
 * @fn      at_recv_cmd_handler
 *
 * @brief   Handle at commands , this function is called in at_task when a whole AT cmd is detected
 *			
 *
 * @param   param - pointer to at command data buffer
 *       	
 *
 * @return  None
 */
void at_recv_cmd_handler(struct recv_cmd_t *param)
{
    uint8_t *buff;      //cmd buff
    uint8_t index;
    uint8_t *at_rsp;    //scan rsp buff

    buff = param->recv_data;
    if(gAT_ctrl_env.async_evt_on_going)
        goto _out;

    for(index = 0; index < (sizeof(cmds) / 4); index ++)
    {
        if(memcmp(buff,cmds[index],strlen(cmds[index])) == 0)
        {
            buff += strlen(cmds[index]);
            break;
        }
    }
    at_rsp = malloc(150);

    switch(index)
    {
        case AT_CMD_IDX_NAME:
        {
            uint8_t local_name[LOCAL_NAME_MAX_LEN];
            uint8_t local_name_len = gap_get_dev_name(local_name);
            local_name[local_name_len] = 0;
            switch(*buff++)
            {
                case '?':
                {
                    sprintf((char *)at_rsp,"+NAME:%s\r\nOK",local_name);
                    at_send_rsp((char *)at_rsp);
                }
                break;
                case '=':
                {
                    uint8_t idx = 0;
                    for(; idx<LOCAL_NAME_MAX_LEN; idx++)
                    {
                        if(*(buff+idx) == '\r')
                            break;
                    }
                    if(idx>=LOCAL_NAME_MAX_LEN)
                    {
                        co_printf("ERR,name_len:%d >=%d",idx,LOCAL_NAME_MAX_LEN);
                        *(buff+idx) = 0x0;
                        sprintf((char *)at_rsp,"+NAME:%s\r\nERR",buff);
                        at_send_rsp((char *)at_rsp);
                        goto _exit;
                    }
                    *(buff+idx) = 0;
                    if(memcmp(local_name,buff,local_name_len)!=0 
                        || local_name_len != strlen((const char *)buff))   //name is different,the set it
                    {
                        gap_set_dev_name(buff,strlen((const char *)buff)+1);
                        at_init_adv_rsp_parameter();
                    }
                    sprintf((char *)at_rsp,"+NAME:%s\r\nOK",buff);
                    at_send_rsp((char *)at_rsp);
                }
                break;
                default:
                    break;
            }
        }
        break;

        case AT_CMD_IDX_MODE:
        {
            switch(*buff++)
            {
                case '?':
                {
                    uint8_t mode_str[3];
                    uint8_t idx = 0;
                    if(gAT_ctrl_env.upgrade_start)
                        mode_str[0] = 'U';         //upgrade
                    else
                        mode_str[0] = 'I';          //idle
                    if(gAT_ctrl_env.adv_ongoing)
                        mode_str[idx++] = 'B';
                    if(gAT_ctrl_env.scan_ongoing)
                        mode_str[idx++] = 'S';
                    if(gAT_ctrl_env.initialization_ongoing)
                        mode_str[idx++] = 'C';

                    if(idx == 1 || idx == 0)
                        sprintf((char *)at_rsp,"+MODE:%c\r\nOK",mode_str[0]);
                    else if(idx == 2)
                        sprintf((char *)at_rsp,"+MODE:%c %c\r\nOK",mode_str[0],mode_str[1]);
                    else if(idx == 3)
                        sprintf((char *)at_rsp,"+MODE:%c %c %c\r\nOK",mode_str[0],mode_str[1],mode_str[2]);
                    at_send_rsp((char *)at_rsp);
                }
                break;
                case '=':
                    if(*buff == 'I')
                    {
                        gAT_ctrl_env.async_evt_on_going = true;
                        gAT_buff_env.default_info.role = IDLE_ROLE;

                        if(gAT_ctrl_env.adv_ongoing)
                        {
                            stop_adv();
                            at_set_gap_cb_func(AT_GAP_CB_ADV_END,at_idle_status_hdl);
                        }
                        if(gAT_ctrl_env.scan_ongoing)
                        {
                            stop_scan();
                            at_set_gap_cb_func(AT_GAP_CB_SCAN_END,at_idle_status_hdl);
                        }
                        if(gAT_ctrl_env.initialization_ongoing)
                        {
                            gap_disconnect_all();
                            at_set_gap_cb_func(AT_GAP_CB_CONN_END,at_idle_status_hdl);
                        }
                        at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);

                        sprintf((char *)at_rsp,"+MODE:I\r\nOK");
                        at_send_rsp((char *)at_rsp);

                        gAT_ctrl_env.async_evt_on_going = false;
                    }
                    else if(*buff == 'B')
                    {
                        if(gAT_ctrl_env.adv_ongoing == false)
                        {
                            gAT_buff_env.default_info.role |= SLAVE_ROLE;
                            at_start_advertising(NULL);
                            at_set_gap_cb_func(AT_GAP_CB_ADV_END,at_cb_adv_end);
                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);
                        }
                        sprintf((char *)at_rsp,"+MODE:B\r\nOK");
                        at_send_rsp((char *)at_rsp);
                    }
                    else if(*buff == 'M')
                    {
                        uint8_t i=0;
                        for(; i<BLE_CONNECTION_MAX; i++)
                        {
                            if(gap_get_connect_status(i) && gAT_buff_env.peer_param[i].link_mode ==MASTER_ROLE)
                                break;
                        }
                        if(i >= BLE_CONNECTION_MAX ) //no master link
                        {
                            gAT_buff_env.default_info.role |= MASTER_ROLE;
                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_start_connecting);
                            //gAT_ctrl_env.async_evt_on_going = true;
                            at_start_connecting(NULL);
                            sprintf((char *)at_rsp,"+MODE:M\r\nOK");
                        }
                        else
                            sprintf((char *)at_rsp,"+MODE:M\r\nERR");
                        at_send_rsp((char *)at_rsp);
                    }
                    else if(*buff == 'U')
                    {
                        sprintf((char *)at_rsp,"+MODE:U\r\nERR");
                        at_send_rsp((char *)at_rsp);
                    }
                    break;
                default:
                    break;
            }
        }
        break;
        case AT_CMD_IDX_SCAN:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t scan_time = atoi((const char *)buff);
                    if(scan_time > 0 && scan_time < 100)
                        gAT_ctrl_env.scan_duration = scan_time*100;
                }
                break;
            }
            at_start_scan();

            at_set_gap_cb_func(AT_GAP_CB_SCAN_END,at_scan_done);
            at_set_gap_cb_func(AT_GAP_CB_ADV_RPT,at_get_adv);
            memset(&(gAT_buff_env.adv_rpt[0]),0xff,sizeof(struct at_adv_report)*ADV_REPORT_NUM);
            gAT_ctrl_env.async_evt_on_going = true;
        }
        break;
        case AT_CMD_IDX_LINK:
        {
            switch(*buff++)
            {
                case '?':
                {
                    uint8_t mac_str[MAC_ADDR_LEN*2+1];
                    uint8_t link_mode;
                    uint8_t encryption = 'N';
                    sprintf((char *)at_rsp,"+LINK\r\nOK");
                    at_send_rsp((char *)at_rsp);
                    for(uint8_t i=0; i< BLE_CONNECTION_MAX; i++)
                    {
                        if(gap_get_connect_status(i))
                        {
                            if(gAT_buff_env.peer_param[i].link_mode == SLAVE_ROLE)
                                link_mode = 'S';
                            else
                                link_mode = 'M';
                            if(gAT_buff_env.peer_param[i].encryption)
                                encryption = 'Y';
                            else
                                encryption = 'N';

                            hex_arr_to_str(g_power_off_save_data_in_ram.peer_param[i].addr,MAC_ADDR_LEN,mac_str);
                            mac_str[MAC_ADDR_LEN*2] = 0;
                            sprintf((char *)at_rsp,"Link_ID: %d LinkMode:%c Enc:%c PeerAddr:%s\r\n",i,link_mode,encryption,mac_str);
                            uart_put_data_noint(UART0,(uint8_t *)at_rsp, strlen((const char *)at_rsp));
                        }
                        else
                        {
                            encryption = 'N';
                            link_mode = 'N';
                        }
                    }
                }
                break;
            }
        }
        break;
        /*
        case AT_CMD_IDX_ENC:
        {
            switch(*buff++)
            {
                case '?':
                {
                    if(gAT_buff_env.default_info.encryption_link == 'M'
                       || gAT_buff_env.default_info.encryption_link == 'B')
                        sprintf((char *)at_rsp,"+ENC:%c\r\nOK",gAT_buff_env.default_info.encryption_link);
                    else
                        sprintf((char *)at_rsp,"+ENC:N\r\nOK");
                    at_send_rsp((char *)at_rsp);
                }
                break;
                case '=':
                {
                    if(*buff == 'M' || *buff == 'B')
                    {
                        gAT_buff_env.default_info.encryption_link = *buff;
                        sprintf((char *)at_rsp,"+ENC:%c\r\nOK",*buff);
                    }
                    else
                    {
                        gAT_buff_env.default_info.encryption_link = 0;
                        sprintf((char *)at_rsp,"+ENC:N\r\nOK");
                    }
                    at_send_rsp((char *)at_rsp);
                }
                break;
            }
        }
        break;
        case AT_CMD_IDX_DISCONN:
        {
            switch(*buff++)
            {
                case '=':
                {
                    if(*buff == 'A')
                    {
                        if(gap_get_connect_num()>0)
                        {
                            gAT_ctrl_env.async_evt_on_going = true;
                            for(uint8_t i = 0; i<BLE_CONNECTION_MAX; i++)
                            {
                                if(gap_get_connect_status(i))
                                    gap_disconnect_req(i);
                            }
                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_link_idle_status_hdl);
                        }
                    }
                    else
                    {
                        uint8_t link_num = atoi((const char *)buff);

                        if(gap_get_connect_status(link_num))
                        {
                            gAT_ctrl_env.async_evt_on_going = true;
                            if(gap_get_connect_status(link_num))
                                gap_disconnect_req(link_num);
                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);
                        }
                        else
                        {
                            sprintf((char *)at_rsp,"+DISCONN:%d\r\nERR",link_num);
                            at_send_rsp((char *)at_rsp);
                        }
                    }
                }
                break;
            }
        }
        break;
        */
        case AT_CMD_IDX_MAC:
        {
            uint8_t mac_str[MAC_ADDR_LEN*2+1];
            
            bd_addr_t addr;
            switch(*buff++)
            {
                case '?':
                    gap_address_get(addr);
                    hex_arr_to_str(addr,MAC_ADDR_LEN,mac_str);
                    mac_str[MAC_ADDR_LEN*2] = 0;

                    sprintf((char *)at_rsp,"+MAC:%s\r\nOK",mac_str);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                {
                    str_to_hex_arr(buff,addr,MAC_ADDR_LEN);
                    gap_address_set(addr);
                    hex_arr_to_str(addr,MAC_ADDR_LEN,mac_str);
                    mac_str[MAC_ADDR_LEN*2] = 0;

                    sprintf((char *)at_rsp,"+MAC:%s\r\nOK",mac_str);
                    at_send_rsp((char *)at_rsp);
                }
                break;
                default:
                    break;
            }
        }
        break;
        case AT_CMD_IDX_CIVER:
        {
            switch(*buff++)
            {
                case '?':
                    sprintf((char *)at_rsp,"+VER:%d\r\nOK",AT_MAIN_VER);
                    at_send_rsp((char *)at_rsp);
                    break;
                default:
                    break;
            }
        }
        break;
        case AT_CMD_IDX_UART:
        {
            switch(*buff++)
            {
                case '?':
                {
                    int databit = convert_from_word_length(g_power_off_save_data_in_ram.uart_param.word_length);
                    int pari = convert_from_parity(g_power_off_save_data_in_ram.uart_param.parity);
                    int stop = convert_from_two_stop_bits(g_power_off_save_data_in_ram.uart_param.two_stop_bits);
                    
                    sprintf((char *)at_rsp,"+UART:%d,%d,%d,%d\r\nOK",
                        g_power_off_save_data_in_ram.uart_param.BaudRate,
                        databit,
                        pari,
                        stop);
                    at_send_rsp((char *)at_rsp);
                }
                break;
                case '=':
                {
                    uint8_t *pos_int_end;
                    pos_int_end = find_int_from_str(buff);
                    g_power_off_save_data_in_ram.uart_param.BaudRate = atoi((const char *)buff);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    int databit = atoi((const char *)buff);
                    g_power_off_save_data_in_ram.uart_param.word_length = convert_to_word_length(databit);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    int parity = atoi((const char *)buff);
                    g_power_off_save_data_in_ram.uart_param.parity = convert_to_parity(parity);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    
                    int stop_bits = atoi((const char *)buff);
                    g_power_off_save_data_in_ram.uart_param.two_stop_bits = convert_to_two_stop_bits(stop_bits);
                    //at_store_info_to_flash();

                    sprintf((char *)at_rsp,"+UART:%d,%d,%d,%d\r\nOK",
                            g_power_off_save_data_in_ram.uart_param.BaudRate,
                            g_power_off_save_data_in_ram.uart_param.word_length,
                            g_power_off_save_data_in_ram.uart_param.parity,
                            g_power_off_save_data_in_ram.uart_param.two_stop_bits);
                    at_send_rsp((char *)at_rsp);
                    //uart_init(UART0,find_uart_idx_from_baudrate(gAT_buff_env.uart_param.baud_rate));
  
                    apUART_Initialize(APB_UART1, 
                            &g_power_off_save_data_in_ram.uart_param, (1 << bsUART_RECEIVE_INTENAB) );
                }
                break;
                default:
                    break;
            }
        }
        break;
        case AT_CMD_IDX_Z:
        {
            sprintf((char *)at_rsp,"+Z\r\nOK");
            at_send_rsp((char *)at_rsp);
            uart_finish_transfers(UART0);
            //NVIC_SystemReset();
            platform_reset();
        }
        break;
        case AT_CMD_IDX_CLR_BOND:
        {
            gap_bond_manager_delete_all();
            sprintf((char *)at_rsp,"+CLR_BOND\r\nOK");
            at_send_rsp((char *)at_rsp);
        }
        break;
        case AT_CMD_IDX_SLEEP:
        {
            switch(*buff++)
            {
                case '?':
                    if(g_power_off_save_data_in_ram.default_info.auto_sleep)
                        sprintf((char *)at_rsp,"+SLEEP:S\r\nOK");
                    else
                        sprintf((char *)at_rsp,"+SLEEP:E\r\nOK");
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                    if(*buff == 'S')
                    {
                        system_sleep_enable();
                        gAT_buff_env.default_info.auto_sleep = true;
                        //set_sleep_flag_after_key_release(true);
                        for(uint8_t i=0; i< BLE_CONNECTION_MAX; i++)
                        {
                            if(gap_get_connect_status(i))
                                at_con_param_update(i,15);
                        }
                        sprintf((char *)at_rsp,"+SLEEP:S\r\nOK");
                        at_send_rsp((char *)at_rsp);
                    }
                    else if(*buff == 'E')
                    {
                        system_sleep_disable();
                        gAT_buff_env.default_info.auto_sleep = false;
                        //set_sleep_flag_after_key_release(false);
                        for(uint8_t i=0; i< BLE_CONNECTION_MAX; i++)
                        {
                            if(gap_get_connect_status(i))
                                at_con_param_update(i,0);
                        }
                        sprintf((char *)at_rsp,"+SLEEP:E\r\nOK");
                        at_send_rsp((char *)at_rsp);
                    }
                    break;
                default:
                    break;
            }
        }
        break;
        case AT_CMD_IDX_CONNADD:
        {
            uint8_t peer_mac_addr_str[MAC_ADDR_LEN*2+1];
            switch(*buff++)
            {
                case '?':
                    break;
                case '=':
                {
                    uint8_t *pos_int_end;
                    str_to_hex_arr(buff,g_power_off_save_data_in_ram.master_peer_param.addr,MAC_ADDR_LEN);
                    pos_int_end = find_int_from_str(buff);
                    buff = pos_int_end+1;
                    g_power_off_save_data_in_ram.master_peer_param.addr_type = atoi((const char *)buff);
                    // if (g_power_off_save_data_in_ram.master_peer_param.addr_type > 1)
                    //     g_power_off_save_data_in_ram.master_peer_param.addr_type = 0;
                }
                break;
            }
            hex_arr_to_str(g_power_off_save_data_in_ram.master_peer_param.addr,MAC_ADDR_LEN,peer_mac_addr_str);
            peer_mac_addr_str[MAC_ADDR_LEN*2] = 0;
            sprintf((char *)at_rsp,"\r\n+CONNADD:%s,%d\r\nOK",peer_mac_addr_str,g_power_off_save_data_in_ram.master_peer_param.addr_type);
            at_send_rsp((char *)at_rsp);
        }
        break;
        /*
        case AT_CMD_IDX_CONN:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t connect_idx = atoi((const char *)buff);

                    if(gAT_ctrl_env.initialization_ongoing == false) //no master link
                    {
                        memcpy(gAT_buff_env.master_peer_param.conn_param.peer_addr.addr, gAT_buff_env.adv_rpt[connect_idx].adv_addr.addr, MAC_ADDR_LEN);
                        gAT_buff_env.master_peer_param.conn_param.addr_type = gAT_buff_env.adv_rpt[connect_idx].adv_addr_type;
                        gAT_buff_env.default_info.role |= MASTER_ROLE;
                        at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_start_connecting);
                        gAT_ctrl_env.async_evt_on_going = true;
                        at_start_connecting(NULL);
                    }
                    else
                    {
                        sprintf((char *)at_rsp,"+CONN:%d\r\nERR",connect_idx);
                        at_send_rsp((char *)at_rsp);
                    }
                }
                break;
            }
        }
        break;
        */
        case AT_CMD_IDX_UUID:
        {
            uint8_t uuid_str_svc[UUID_SIZE_16*2+1];
            uint8_t uuid_str_tx[UUID_SIZE_16*2+1];
            uint8_t uuid_str_rx[UUID_SIZE_16*2+1];
            switch(*buff++)
            {
                case '?':
                    hex_arr_to_str(g_power_off_save_data_in_ram.serivce_uuid,UUID_SIZE_16,uuid_str_svc);
                    uuid_str_svc[UUID_SIZE_16*2] = 0;
                    hex_arr_to_str(g_power_off_save_data_in_ram.characteristic_output_uuid,UUID_SIZE_16,uuid_str_tx);
                    uuid_str_tx[UUID_SIZE_16*2] = 0;
                    hex_arr_to_str(g_power_off_save_data_in_ram.characteristic_input_uuid,UUID_SIZE_16,uuid_str_rx);
                    uuid_str_rx[UUID_SIZE_16*2] = 0;

                    sprintf((char *)at_rsp,"+%s:\r\nDATA:UUID\r\n\r\n+%s:\r\nDATA:UUID\r\n\r\n+%s:\r\nDATA:UUID\r\n\r\nOK"
                            ,uuid_str_svc,uuid_str_tx,uuid_str_rx);
                    at_send_rsp((char *)at_rsp);
                    break;

                case '=':
                {
                    if( *buff == 'A' && *(buff+1) == 'A')
                    {
                        str_to_hex_arr(buff+3,g_power_off_save_data_in_ram.serivce_uuid,UUID_SIZE_16);
                    }
                    else if( *buff == 'B' && *(buff+1) == 'B')
                    {
                        str_to_hex_arr(buff+3,g_power_off_save_data_in_ram.characteristic_output_uuid,UUID_SIZE_16);
                    }
                    else if( *buff == 'C' && *(buff+1) == 'C')
                    {
                        str_to_hex_arr(buff+3,g_power_off_save_data_in_ram.characteristic_input_uuid,UUID_SIZE_16);
                    }
                    init_service();
                    *(buff+3 + UUID_SIZE_16*2) = 0;

                    sprintf((char *)at_rsp,"+%s:\r\nDATA:UUID\r\n\r\nsuccessful",buff+3);
                    at_send_rsp((char *)at_rsp);
                    break;
                }
                default:
                    break;
            }
        }
        break;
        case AT_CMD_IDX_FLASH:            //store param
        {
            at_store_info_to_flash();
            sprintf((char *)at_rsp,"+FLASH\r\nOK");
            at_send_rsp((char *)at_rsp);
        }
        break;
        case AT_CMD_IDX_SEND:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t *pos_int_end;
                    pos_int_end = find_int_from_str(buff);
                    uint8_t conidx = atoi((const char *)buff);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    uint32_t len = atoi((const char *)buff);

                    if(gap_get_connect_status(conidx) && gAT_ctrl_env.one_slot_send_start == false)
                    {
                        gAT_ctrl_env.transparent_conidx = conidx;
                        gAT_ctrl_env.one_slot_send_len = len;
                        gAT_ctrl_env.one_slot_send_start = true;
                        at_clr_uart_buff();
                        sprintf((char *)at_rsp,">");
                    }
                    else
                        sprintf((char *)at_rsp,"+SEND\r\nERR");

                    at_send_rsp((char *)at_rsp);
                }
                break;
            }
        }
        break;
        
        /*
        case AT_CMD_IDX_TRANSPARENT:            //go to transparent transmit
        {
            if(gap_get_connect_num()==1)
            {
                //printf("%d,%d\r\n",app_env.conidx,gAT_buff_env.peer_param[app_env.conidx].link_mode);
                gAT_ctrl_env.transparent_start = true;
                at_clr_uart_buff();

                uint8_t i;
                //find which conidx is connected
                for(i = 0; i<BLE_CONNECTION_MAX; i++)
                {
                    if(gap_get_connect_status(i))
                        break;
                }

                gAT_ctrl_env.transparent_conidx = i;
                if(gAT_buff_env.peer_param[i].link_mode == SLAVE_ROLE)
                    spss_recv_data_ind_func = at_spss_recv_data_ind_func;
                else if(gAT_buff_env.peer_param[i].link_mode == MASTER_ROLE)
                    spsc_recv_data_ind_func = at_spsc_recv_data_ind_func;
                //app_spss_send_ble_flowctrl(160);
                sprintf((char *)at_rsp,"+++\r\nOK");
            }
            else
                sprintf((char *)at_rsp,"+++\r\nERR");
            at_send_rsp((char *)at_rsp);
        }
        break;
        case AT_CMD_IDX_AUTO_TRANSPARENT:            //go to transparent transmit
        {
            switch(*buff++)
            {
                case '?':
                    if(gAT_buff_env.default_info.auto_transparent == true)
                        sprintf((char *)at_rsp,"+AUTO+++:Y\r\nOK");
                    else
                        sprintf((char *)at_rsp,"+AUTO+++:N\r\nOK");
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                    if(*buff == 'Y')
                    {
                        gAT_buff_env.default_info.auto_transparent = true;
                        sprintf((char *)at_rsp,"+AUTO+++:Y\r\nOK");
                    }
                    else if(*buff == 'N')
                    {
                        gAT_buff_env.default_info.auto_transparent = false;
                        sprintf((char *)at_rsp,"+AUTO+++:N\r\nOK");
                    }
                    at_send_rsp((char *)at_rsp);
                    break;
                default:
                    break;
            }
        }
        break;
        */
        case AT_CMD_IDX_POWER:            //rf_power set/req
        {
            switch(*buff++)
            {
                case '?':
                    sprintf((char *)at_rsp,"+POWER:%d\r\nOK",g_power_off_save_data_in_ram.default_info.rf_power);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                    g_power_off_save_data_in_ram.default_info.rf_power = atoi((const char *)buff);
                    if(g_power_off_save_data_in_ram.default_info.rf_power > 5)
                        sprintf((char *)at_rsp,"+POWER:%d\r\nERR",g_power_off_save_data_in_ram.default_info.rf_power);
                    else
                    {
                        //rf_set_tx_power(rf_power_arr[gAT_buff_env.default_info.rf_power]);
                        sprintf((char *)at_rsp,"+POWER:%d\r\nOK",g_power_off_save_data_in_ram.default_info.rf_power);
                    }
                    at_send_rsp((char *)at_rsp);
                    break;
                default:
                    break;
            }
        }
        break;
        case AT_CMD_IDX_ADVINT:            //rf_power set/req
        {
            switch(*buff++)
            {
                case '?':
                    sprintf((char *)at_rsp,"+ADVINT:%d\r\nOK",g_power_off_save_data_in_ram.default_info.adv_int);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                {
                    uint8_t tmp = g_power_off_save_data_in_ram.default_info.adv_int;
                    g_power_off_save_data_in_ram.default_info.adv_int = atoi((const char *)buff);
                    if(g_power_off_save_data_in_ram.default_info.adv_int > 5)
                    {
                        sprintf((char *)at_rsp,"+ADVINT:%d\r\nERR",g_power_off_save_data_in_ram.default_info.adv_int);
                        g_power_off_save_data_in_ram.default_info.adv_int = tmp;
                    }
                    else
                    {
                        sprintf((char *)at_rsp,"+ADVINT:%d\r\nOK",g_power_off_save_data_in_ram.default_info.adv_int);
                    }
                    at_send_rsp((char *)at_rsp);
                }
                break;
                default:
                    break;
            }
        }
        break;
        /*
        case AT_CMD_IDX_CLR_DFT:
        {
            sprintf((char *)at_rsp,"+CLR_INFO\r\nOK");
            at_send_rsp((char *)at_rsp);
            uart_finish_transfers(UART0);
            at_clr_flash_info();
        }
        break;
        */
        default:
            break;
    }
_exit:
    free(at_rsp);
_out:
    ;
}