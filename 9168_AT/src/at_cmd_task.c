#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "at_cmd_task.h"
#include "at_recv_cmd.h"
#include "common/flash_data.h"
#include "util/utils.h"

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
 * @param   out data name
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
 * @param[addr] address, out data 
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
 * @param[addr] address
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
 * @param[buff]     hex buffer
 * @param[len]      hex buffer length
 * @param[out_str]  hex str, out data
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
 * @param[in_str]   hex str
 * @param[buff]     buffer
 * @param[len]      buffer length
 *       	 
 *
 * @return  1 if has connection, otherwise 0
 */
void str_to_hex_arr(uint8_t* in_str, uint8_t* buff, uint16_t len)
{
    str2hex((char*)in_str, len * 2, buff, len);
}

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
    return 1;
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

        /*
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
                            gap_stop_advertising();
                            at_set_gap_cb_func(AT_GAP_CB_ADV_END,at_idle_status_hdl);
                        }
                        if(gAT_ctrl_env.scan_ongoing)
                        {
                            gap_stop_scan();
                            at_set_gap_cb_func(AT_GAP_CB_SCAN_END,at_idle_status_hdl);
                        }
                        if(gAT_ctrl_env.initialization_ongoing)
                        {
                            gap_stop_conn();
                            at_set_gap_cb_func(AT_GAP_CB_CONN_END,at_idle_status_hdl);
                        }
                        at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);

                        sprintf((char *)at_rsp,"+MODE:I\r\nOK");
                        at_send_rsp((char *)at_rsp);

                        gAT_ctrl_env.async_evt_on_going = false;
                        if(gAT_ctrl_env.upgrade_start == true)
                        {
                            gAT_ctrl_env.upgrade_start = false;
                            //os_free("1st_pkt_buff\r\n"); todo
                        }
                        if( gAT_buff_env.default_info.auto_sleep)
                            system_sleep_enable();
                        else
                            system_sleep_disable();
                    }
                    else if(*buff == 'B')
                    {
                        if(gAT_ctrl_env.adv_ongoing == false)
                        {
                            gAT_buff_env.default_info.role |= SLAVE_ROLE;
                            at_start_advertising(NULL);
                            at_set_gap_cb_func(AT_GAP_CB_ADV_END,at_cb_adv_end);
                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);

                            if( gAT_buff_env.default_info.auto_sleep)
                                system_sleep_enable();
                            else
                                system_sleep_disable();
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
                        if(gap_get_connect_num()==0)
                        {
                            //upgrade mode, stop sleep
                            system_sleep_disable();
                            //set_sleep_flag_after_key_release(false);
                            gAT_ctrl_env.upgrade_start = true;
                            //at_ota_init();
                            sprintf((char *)at_rsp,"+MODE:U\r\nOK");
                        }
                        else
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

                            hex_arr_to_str(gAT_buff_env.peer_param[i].conn_param.peer_addr.addr,MAC_ADDR_LEN,mac_str);
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
        /*
        case AT_CMD_IDX_UART:
        {
            switch(*buff++)
            {
                case '?':
                    sprintf((char *)at_rsp,"+UART:%d,%d,%d,%d\r\nOK",gAT_buff_env.uart_param.baud_rate,gAT_buff_env.uart_param.data_bit_num
                            ,gAT_buff_env.uart_param.pari,gAT_buff_env.uart_param.stop_bit);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                {
                    uint8_t *pos_int_end;
                    pos_int_end = find_int_from_str(buff);
                    gAT_buff_env.uart_param.baud_rate = atoi((const char *)buff);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    gAT_buff_env.uart_param.data_bit_num = atoi((const char *)buff);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    gAT_buff_env.uart_param.pari = atoi((const char *)buff);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    gAT_buff_env.uart_param.stop_bit = atoi((const char *)buff);
                    //at_store_info_to_flash();

                    sprintf((char *)at_rsp,"+UART:%d,%d,%d,%d\r\nOK",gAT_buff_env.uart_param.baud_rate,
                            gAT_buff_env.uart_param.data_bit_num,gAT_buff_env.uart_param.pari,gAT_buff_env.uart_param.stop_bit);
                    at_send_rsp((char *)at_rsp);
                    //uart_init(UART0,find_uart_idx_from_baudrate(gAT_buff_env.uart_param.baud_rate));
  
                    uart_param_t param =
                    {
                        .baud_rate = gAT_buff_env.uart_param.baud_rate,
                        .data_bit_num = gAT_buff_env.uart_param.data_bit_num,
                        .pari = gAT_buff_env.uart_param.pari,
                        .stop_bit = gAT_buff_env.uart_param.stop_bit,
                    };
                    uart_init1(UART0,param);
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
            platform_reset_patch(0);
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
                    if(gAT_buff_env.default_info.auto_sleep)
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
                    str_to_hex_arr(buff,gAT_buff_env.master_peer_param.conn_param.peer_addr.addr,MAC_ADDR_LEN);
                    pos_int_end = find_int_from_str(buff);
                    buff = pos_int_end+1;
                    gAT_buff_env.master_peer_param.conn_param.addr_type = atoi((const char *)buff);
                    if(gAT_buff_env.master_peer_param.conn_param.addr_type > 1)
                        gAT_buff_env.master_peer_param.conn_param.addr_type = 0;
                }
                break;
            }
            hex_arr_to_str(gAT_buff_env.master_peer_param.conn_param.peer_addr.addr,MAC_ADDR_LEN,peer_mac_addr_str);
            peer_mac_addr_str[MAC_ADDR_LEN*2] = 0;
            sprintf((char *)at_rsp,"\r\n+CONNADD:%s,%d\r\nOK",peer_mac_addr_str,gAT_buff_env.master_peer_param.conn_param.addr_type );
            at_send_rsp((char *)at_rsp);
        }
        break;
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
        case AT_CMD_IDX_UUID:
        {
            uint8_t uuid_str_svc[UUID_SIZE_16*2+1];
            uint8_t uuid_str_tx[UUID_SIZE_16*2+1];
            uint8_t uuid_str_rx[UUID_SIZE_16*2+1];
            switch(*buff++)
            {
                case '?':
                    hex_arr_to_str(spss_uuids,UUID_SIZE_16,uuid_str_svc);
                    uuid_str_svc[UUID_SIZE_16*2] = 0;
                    hex_arr_to_str(spss_uuids + UUID_SIZE_16,UUID_SIZE_16,uuid_str_tx);
                    uuid_str_tx[UUID_SIZE_16*2] = 0;
                    hex_arr_to_str(spss_uuids + UUID_SIZE_16*2,UUID_SIZE_16,uuid_str_rx);
                    uuid_str_rx[UUID_SIZE_16*2] = 0;

                    sprintf((char *)at_rsp,"+%s:\r\nDATA:UUID\r\n\r\n+%s:\r\nDATA:UUID\r\n\r\n+%s:\r\nDATA:UUID\r\n\r\nOK"
                            ,uuid_str_svc,uuid_str_tx,uuid_str_rx);
                    at_send_rsp((char *)at_rsp);
                    break;

                case '=':
                {
                    svc_change_t svc_change =
                    {
                        .svc_id = spss_svc_id,
                        .type = SVC_CHANGE_UUID,
                        .param.new_uuid.size = UUID_SIZE_16,
                    };
                    if( *buff == 'A' && *(buff+1) == 'A')
                    {
                        str_to_hex_arr(buff+3,spss_uuids,UUID_SIZE_16);
                        svc_change.att_idx = 0;
                        memcpy(svc_change.param.new_uuid.p_uuid,spss_uuids,UUID_SIZE_16);
                        gatt_change_svc(svc_change);
                    }
                    else if( *buff == 'B' && *(buff+1) == 'B')
                    {
                        str_to_hex_arr(buff+3,spss_uuids + UUID_SIZE_16,UUID_SIZE_16);
                        svc_change.att_idx = 2;
                        memcpy(svc_change.param.new_uuid.p_uuid,spss_uuids + UUID_SIZE_16,UUID_SIZE_16);
                        gatt_change_svc(svc_change);
                    }
                    else if( *buff == 'C' && *(buff+1) == 'C')
                    {
                        str_to_hex_arr(buff+3,spss_uuids + UUID_SIZE_16*2,UUID_SIZE_16);
                        svc_change.att_idx = 6;
                        memcpy(svc_change.param.new_uuid.p_uuid,spss_uuids + UUID_SIZE_16*2,UUID_SIZE_16);
                        gatt_change_svc(svc_change);
                    }
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
        */
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
        case AT_CMD_IDX_POWER:            //rf_power set/req
        {
            switch(*buff++)
            {
                case '?':
                    sprintf((char *)at_rsp,"+POWER:%d\r\nOK",gAT_buff_env.default_info.rf_power);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                    gAT_buff_env.default_info.rf_power = atoi((const char *)buff);
                    if(gAT_buff_env.default_info.rf_power > 5)
                        sprintf((char *)at_rsp,"+POWER:%d\r\nERR",gAT_buff_env.default_info.rf_power);
                    else
                    {
                        //rf_set_tx_power(rf_power_arr[gAT_buff_env.default_info.rf_power]);
                        sprintf((char *)at_rsp,"+POWER:%d\r\nOK",gAT_buff_env.default_info.rf_power);
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
                    sprintf((char *)at_rsp,"+ADVINT:%d\r\nOK",gAT_buff_env.default_info.adv_int);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                {
                    uint8_t tmp = gAT_buff_env.default_info.adv_int;
                    gAT_buff_env.default_info.adv_int = atoi((const char *)buff);
                    if(gAT_buff_env.default_info.adv_int > 5)
                    {
                        sprintf((char *)at_rsp,"+ADVINT:%d\r\nERR",gAT_buff_env.default_info.adv_int);
                        gAT_buff_env.default_info.adv_int = tmp;
                    }
                    else
                    {
                        sprintf((char *)at_rsp,"+ADVINT:%d\r\nOK",gAT_buff_env.default_info.adv_int);
                    }
                    at_send_rsp((char *)at_rsp);
                }
                break;
                default:
                    break;
            }
        }
        break;
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