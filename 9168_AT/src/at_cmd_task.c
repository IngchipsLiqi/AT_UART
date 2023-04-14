#include <stdint.h>
#include <string.h>
#include "at_cmd_task.h"


#define LOCAL_NAME_MAX_LEN      (27)


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

struct at_ctrl gAT_ctrl_env = {0};
struct at_buff_env gAT_buff_env = {0};

void at_send_rsp(char *str)
{
    uart_io_send((uint8_t *)"\r\n", 2);
    uart_io_send((uint8_t *)str, strlen(str));
    uart_io_send((uint8_t *)"\r\n", 2);
}

void gap_set_dev_name(uint8_t *p_name,uint8_t len)
{


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
            uint8_t local_name_len;//= gap_get_dev_name(local_name);
            //local_name[local_name_len] = 0;
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
                        //co_printf("ERR,name_len:%d >=%d",idx,LOCAL_NAME_MAX_LEN);
                        *(buff+idx) = 0x0;
                        sprintf((char *)at_rsp,"+NAME:%s\r\nERR",buff);
                        at_send_rsp((char *)at_rsp);
                        goto _exit;
                    }
                    *(buff+idx) = 0;
                    if(memcmp(local_name,buff,local_name_len)!=0)   //name is different,the set it
                    {
                        //gap_set_dev_name(buff,strlen((const char *)buff)+1);
                        //at_init_adv_rsp_parameter();
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
//                        gAT_ctrl_env.async_evt_on_going = true;
//                        gAT_buff_env.default_info.role = IDLE_ROLE;

//                        if(gAT_ctrl_env.adv_ongoing)
//                        {
//                            gap_stop_advertising();
//                            at_set_gap_cb_func(AT_GAP_CB_ADV_END,at_idle_status_hdl);
//                        }
//                        if(gAT_ctrl_env.scan_ongoing)
//                        {
//                            gap_stop_scan();
//                            at_set_gap_cb_func(AT_GAP_CB_SCAN_END,at_idle_status_hdl);
//                        }
//                        if(gAT_ctrl_env.initialization_ongoing)
//                        {
//                            gap_stop_conn();
//                            at_set_gap_cb_func(AT_GAP_CB_CONN_END,at_idle_status_hdl);
//                        }
//                        at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);

                        sprintf((char *)at_rsp,"+MODE:I\r\nOK");
                        at_send_rsp((char *)at_rsp);

                        gAT_ctrl_env.async_evt_on_going = false;
                        if(gAT_ctrl_env.upgrade_start == true)
                        {
                            gAT_ctrl_env.upgrade_start = false;
                            //os_free("1st_pkt_buff\r\n"); todo
                        }
//                        if( gAT_buff_env.default_info.auto_sleep)
//                            system_sleep_enable();
//                        else
//                            system_sleep_disable();
                    }
                    else if(*buff == 'B')
                    {
                        if(gAT_ctrl_env.adv_ongoing == false)
                        {
//                            gAT_buff_env.default_info.role |= SLAVE_ROLE;
//                            at_start_advertising(NULL);
//                            at_set_gap_cb_func(AT_GAP_CB_ADV_END,at_cb_adv_end);
//                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);

//                            if( gAT_buff_env.default_info.auto_sleep)
//                                system_sleep_enable();
//                            else
//                                system_sleep_disable();
                        }
                        sprintf((char *)at_rsp,"+MODE:B\r\nOK");
                        at_send_rsp((char *)at_rsp);
                    }
                    else if(*buff == 'M')
                    {
//                        uint8_t i=0;
//                        for(; i<BLE_CONNECTION_MAX; i++)
//                        {
//                            if(gap_get_connect_status(i) && gAT_buff_env.peer_param[i].link_mode ==MASTER_ROLE)
//                                break;
//                        }
//                        if(i >= BLE_CONNECTION_MAX ) //no master link
//                        {
//                            gAT_buff_env.default_info.role |= MASTER_ROLE;
//                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_start_connecting);
//                            //gAT_ctrl_env.async_evt_on_going = true;
//                            at_start_connecting(NULL);
//                            sprintf((char *)at_rsp,"+MODE:M\r\nOK");
//                        }
//                        else
//                            sprintf((char *)at_rsp,"+MODE:M\r\nERR");
                        at_send_rsp((char *)at_rsp);
                    }
                    else if(*buff == 'U')
                    {
        
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
//                case '=':
//                {
//                    uint8_t scan_time = atoi((const char *)buff);
//                    if(scan_time > 0 && scan_time < 100)
//                        gAT_ctrl_env.scan_duration = scan_time*100;
//                }
                break;
            }
            //at_start_scan();

//            at_set_gap_cb_func(AT_GAP_CB_SCAN_END,at_scan_done);
//            at_set_gap_cb_func(AT_GAP_CB_ADV_RPT,at_get_adv);
//            memset(&(gAT_buff_env.adv_rpt[0]),0xff,sizeof(struct at_adv_report)*ADV_REPORT_NUM);
//            gAT_ctrl_env.async_evt_on_going = true;
        }
        break;
        case AT_CMD_IDX_LINK:
        {

        }
        break;
        case AT_CMD_IDX_ENC:
        {

        }
        break;
        case AT_CMD_IDX_DISCONN:
        {
    
        }
        break;
        case AT_CMD_IDX_MAC:
        {

        }
        break;
        case AT_CMD_IDX_CIVER:
        {

        }
        break;
        case AT_CMD_IDX_UART:
        {

        }
        break;
        case AT_CMD_IDX_Z:
        {
     
        }
        break;
        case AT_CMD_IDX_CLR_BOND:
        {

        }
        break;
        case AT_CMD_IDX_SLEEP:
        {

        }
        break;
        case AT_CMD_IDX_CONNADD:
        {

        }
        break;
        case AT_CMD_IDX_CONN:
        {

        }
        break;

        case AT_CMD_IDX_UUID:
        {

        }
        break;
        case AT_CMD_IDX_FLASH:            //store param
        {

        }
        break;
        case AT_CMD_IDX_SEND:
        {

        }
        break;
        case AT_CMD_IDX_TRANSPARENT:            //go to transparent transmit
        {

        }
        break;
        case AT_CMD_IDX_AUTO_TRANSPARENT:            //go to transparent transmit
        {
     
        }
        break;
        case AT_CMD_IDX_POWER:            //rf_power set/req
        {
      
        }
        break;
        case AT_CMD_IDX_ADVINT:            //rf_power set/req
        {

        }
        break;
        case AT_CMD_IDX_CLR_DFT:
        {

        }
        break;
        default:
            break;
        
    }
_exit:
    free(at_rsp);
_out:
    ;
}