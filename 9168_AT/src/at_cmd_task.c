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
#include "ad_parser.h"
#include "at_profile_spss.h"
#include "at_profile_spsc.h"
#include "btstack_defines.h"

#include "gap.h"

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
    AT_CMD_IDX_SCAN_FILTER,
    AT_CMD_IDX_SCAN,
    AT_CMD_IDX_APP,
    AT_CMD_IDX_CONN_PHY,
    AT_CMD_IDX_CONN_PARAM,
    AT_CMD_IDX_CONNADD,
    AT_CMD_IDX_CONN,
    AT_CMD_IDX_SLEEP,
    AT_CMD_IDX_SHUTDOWN,
    AT_CMD_IDX_UUID,
    AT_CMD_IDX_DISCONN,
    AT_CMD_IDX_FLASH,
    AT_CMD_IDX_SEND,
    AT_CMD_IDX_TO,
    AT_CMD_IDX_TRANSPARENT,
    AT_CMD_IDX_AUTO_TRANSPARENT,
    AT_CMD_IDX_POWER,
    AT_CMD_IDX_ADVINT,
    AT_CMD_IDX_CLR_DFT,
    AT_CMD_IDX_RXNUM,
    AT_CMD_IDX_BLEGATTSRD,
    AT_CMD_IDX_BLEGATTSWR,
    AT_CMD_IDX_BLEGATTCRD,
    AT_CMD_IDX_BLEGATTCWR,
    AT_CMD_IDX_BLEGATTCSUB,
    AT_CMD_IDX_BLEGATTC,
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
    [AT_CMD_IDX_SCAN_FILTER] = "SCAN_FILTER",
    [AT_CMD_IDX_SCAN] = "SCAN",
    [AT_CMD_IDX_APP] = "ADP",
    [AT_CMD_IDX_CONN_PHY] = "CONN_PHY",
    [AT_CMD_IDX_CONN_PARAM] = "CONN_PARAM",
    [AT_CMD_IDX_CONNADD] = "CONNADD",
    [AT_CMD_IDX_CONN] = "CONN",
    [AT_CMD_IDX_SLEEP] = "SLEEP",
    [AT_CMD_IDX_SHUTDOWN] = "SHUTDOWN",
    [AT_CMD_IDX_UUID] = "UUID",
    [AT_CMD_IDX_DISCONN] = "DISCONN",
    [AT_CMD_IDX_FLASH] = "FLASH",
    [AT_CMD_IDX_SEND] = "SEND",
    [AT_CMD_IDX_TO] = "TO",
    [AT_CMD_IDX_TRANSPARENT] = "+++",
    [AT_CMD_IDX_AUTO_TRANSPARENT] = "AUTO+++",
    [AT_CMD_IDX_POWER] = "POWER",
    [AT_CMD_IDX_ADVINT] = "ADVINT",
    [AT_CMD_IDX_CLR_DFT] = "CLR_INFO",
    [AT_CMD_IDX_RXNUM] = "RXNUM",
    [AT_CMD_IDX_BLEGATTSRD] = "BLEGATTSRD",
    [AT_CMD_IDX_BLEGATTSWR] = "BLEGATTSWR",
    [AT_CMD_IDX_BLEGATTC] = "BLEGATTC",
    [AT_CMD_IDX_BLEGATTCRD] = "BLEGATTCRD",
    [AT_CMD_IDX_BLEGATTCWR] = "BLEGATTCWR",
    [AT_CMD_IDX_BLEGATTCSUB] = "BLEGATTCSUB",
};

// master role comes first; then slave role.
conn_info_t conn_infos[TOTAL_CONN_NUM] = {0};

// 26 is maximum number of connections supported by ING918.
uint8_t handle_2_id[26] = {0};

#define get_id_of_handle(handle)    (handle_2_id[handle])
#define get_handle_of_id(id)        (conn_infos[id].handle)

int initiating_index = -1;
int initiating_timeout = 30;

// print buff
char buffer[100] = {0};

// context
struct at_ctrl gAT_ctrl_env = {0};
struct at_buff_env gAT_buff_env = {0};

// AT POWER ADV_Interval
const int16_t rf_power_arr[6] = {5,2,0,-5,-10,-17}; //TODO 2.5dbm?arr[1]
const uint16_t adv_int_arr[6] = {80,160,320,800,1600,3200};

// Power save data
extern private_flash_data_t g_power_off_save_data_in_ram;

// FOTA
extern prog_ver_t prog_ver;

// RXNUM Enable
extern bool print_data_len_flag;
extern uint32_t receive_master_data_len;


/*********************************************************************
 * @fn      get_id_by_handle
 * @param   handle
 * @return  link_id
 */
uint8_t get_id_by_handle(uint16_t handle)
{
    return get_id_of_handle(handle);
}

/*********************************************************************
 * @fn      get_handle_by_id
 * @param   link_id
 * @return  handle
 */
uint16_t get_handle_by_id(uint8_t id) 
{
    return get_handle_of_id(id);
}

static void stack_cancel_create_conn(void *data, uint16_t index)
{
    gap_create_connection_cancel();
}

static void initiate_timeout(void)
{
    btstack_push_user_runnable(stack_cancel_create_conn, NULL, 0);
}

void uart_at_start()
{
    int i;
    for (i = 0; i < TOTAL_CONN_NUM; i++)
    {
        conn_info_t *p = conn_infos + i;
        p->handle = INVALID_HANDLE;
        p->peer_addr_type = BD_ADDR_TYPE_LE_RANDOM;
        p->min_interval = 350;
        p->max_interval = 350;
        p->timeout = 800;
    }
    
    gAT_ctrl_env.transparent_conidx = 0xFF;
    gAT_ctrl_env.transparent_notify_enable = false;
}

static char * append_hex_str(char *s, const uint8_t *data, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        sprintf(s, "%02X", data[i]);
        s += 2;
    }
    return s;
}









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
    //gap_set_ext_adv_enable(0, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);// stop adv 
    gap_set_ext_adv_data(0, sizeof(g_power_off_save_data_in_ram.module_adv_data), (uint8_t*)&g_power_off_save_data_in_ram.module_adv_data);
    //gap_set_ext_adv_enable(1, sizeof(adv_sets_en) / sizeof(adv_sets_en[0]), adv_sets_en);// start adv 
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


static void at_ok()
{
    uart_io_print("OK\n");
}
static void at_err()
{
    uart_io_print("ERR\n");
}
static uint8_t preprocess_at(uint8_t *at_cmd)
{
    uint8_t *p = at_cmd;
    uint8_t argc = 0;
    while (*p != '\r')
    {
        if (*p == ',')
        {
            *p = '\0';
            argc++;
        }
        p++;
    }
    *p = 0;
    return argc;
}
static uint8_t *find_next_arg(uint8_t *at_cmd_p)
{
    while (*at_cmd_p != '\0')
        at_cmd_p++;
    return at_cmd_p + 1;
}
static uint16_t get_arg_len(uint8_t *at_cmd_p)
{
    return strlen((char *)at_cmd_p);
}
static bool parse_arg_int(uint8_t *at_cmd_p, int* value)
{
    *value = atoi((char *)at_cmd_p);
    
    if (*value == 0) 
        return (*(at_cmd_p) == '0') && (*(at_cmd_p + 1) == '\0');
    else
        return true;
}
static bool parse_arg_str_normal(uint8_t *at_cmd_p, char* str)
{
    str = (char*)at_cmd_p;
    return true;
}
static bool parse_arg_str_hex_data(uint8_t *at_cmd_p, uint8_t* data)
{
    uint16_t len = get_arg_len(at_cmd_p);
    
    if (len % 2 == 1)
        return false;
    
    uint8_t *p = at_cmd_p;
    while (*p != '\0')
    {
        if (!((('0' <= *p) && (*p <= '9')) 
            ||(('A' <= *p) && (*p <= 'Z')) 
            ||(('a' <= *p) && (*p <= 'z'))))
            return false;
        p++;
    }
    str_to_hex_arr(at_cmd_p, data, len / 2);
    return true;
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
    return conn_infos[idx].handle != INVALID_HANDLE;
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
    for (int i = 0; i < TOTAL_CONN_NUM; ++i)
        if (conn_infos[i].handle != INVALID_HANDLE)
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
    uart_put_data_noint(UART0,(uint8_t *)"\r\n", 2);
    uart_put_data_noint(UART0,(uint8_t *)str, strlen(str));
    uart_put_data_noint(UART0,(uint8_t *)"\r\n", 2);
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
 * @param 	latency - indicate latency for patameter updation operation 
 *
 * @return  None.
 */
void at_con_param_update(uint8_t conidx, uint16_t latency)
{
    conn_info_t *p = &conn_infos[conidx];

    if(p->latency != latency)
    {
        gap_update_connection_parameters(conidx, 
            p->min_interval, p->max_interval, latency, p->timeout, 7, 7);
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
    
    
    uint16_t name_len = 0;
    const uint8_t* name = NULL;
    
    // filter name prefix
    if (strcmp(g_power_off_save_data_in_ram.scan_filter.name_prefix, "*") != 0) 
    {
        name = ad_data_from_type(report->data_len, (uint8_t*)report->data, 9, &name_len);
        uint32_t name_prefix_len = strlen(g_power_off_save_data_in_ram.scan_filter.name_prefix);
        
        if (name == NULL || name_len < name_prefix_len ||
            memcmp(name, g_power_off_save_data_in_ram.scan_filter.name_prefix, name_prefix_len) != 0) {
            goto _exit;
        }
    }
    // filter name suffix
    if (strcmp(g_power_off_save_data_in_ram.scan_filter.name_suffix, "*") != 0) 
    {
        name = ad_data_from_type(report->data_len, (uint8_t*)report->data, 9, &name_len);
        uint32_t name_suffix_len = strlen(g_power_off_save_data_in_ram.scan_filter.name_suffix);
        
        const uint8_t* compare_pointer = name + (name_len - name_suffix_len);
        
        if (name == NULL || name_len < name_suffix_len ||
            memcmp(compare_pointer, g_power_off_save_data_in_ram.scan_filter.name_suffix, name_suffix_len) != 0) {
            goto _exit;
        }
    }
    // filter service uuid16
    if (g_power_off_save_data_in_ram.scan_filter.enable_uuid_16_filter)
    {
        uint8_t uuid_16[2] = {0};
        uint16_t uuid16_len = 0;
        const uint8_t* uuid16_reverse = ad_data_from_type(report->data_len, (uint8_t*)report->data, 3, &uuid16_len);
        reverse(uuid_16, uuid16_reverse, UUID_SIZE_2);
        
        if (memcmp(uuid_16, g_power_off_save_data_in_ram.scan_filter.uuid_16, UUID_SIZE_2) != 0)
            goto _exit;
    }
    // filter service uuid128
    if (g_power_off_save_data_in_ram.scan_filter.enable_uuid_128_filter)
    {
        
        uint8_t uuid_128[2] = {0};
        uint16_t uuid128_len = 0;
        const uint8_t* uuid128_reverse = ad_data_from_type(report->data_len, (uint8_t*)report->data, 7, &uuid128_len);
        reverse(uuid_128, uuid128_reverse, UUID_SIZE_16);
        
        if (memcmp(uuid_128, g_power_off_save_data_in_ram.scan_filter.uuid_128, UUID_SIZE_16) != 0)
            goto _exit;
    }
    // filter rssi
    if (g_power_off_save_data_in_ram.scan_filter.enable_rssi_filter)
    {
        if (report->rssi < g_power_off_save_data_in_ram.scan_filter.rssi)
            goto _exit;
    }
    
    bd_addr_t peer_addr;
    reverse_bd_addr(report->address, peer_addr);
    
    uint8_t free_rpt_idx = 0xff;
    for(uint8_t idx = 0; idx<ADV_REPORT_NUM; idx++)
    {
        if(gAT_buff_env.adv_rpt[idx].evt_type == 0xff && free_rpt_idx == 0xff)
            free_rpt_idx = idx;
        if(memcmp(gAT_buff_env.adv_rpt[idx].adv_addr,peer_addr,sizeof(mac_addr_t)) == 0 )
            goto _exit;
    }
    
    gAT_buff_env.adv_rpt[free_rpt_idx].evt_type = report->evt_type;
    gAT_buff_env.adv_rpt[free_rpt_idx].adv_addr_type = report->addr_type;
    memcpy(gAT_buff_env.adv_rpt[free_rpt_idx].adv_addr,peer_addr,sizeof(mac_addr_t));
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
        if (gAT_buff_env.adv_rpt[idx].evt_type != 0xff)
        {
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
            uart_put_data_noint(UART0,(uint8_t *)at_rsp, strlen((const char *)at_rsp));
        }
        else
            break;
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
extern initiating_phy_config_t phy_configs[];

/*********************************************************************
 * @fn      stack_initiate
 *
 * @brief   Start a active connection opertaion
 *			
 *
 * @param   arg - reseved
 *       	
 *
 * @return  None
 */
static void stack_initiate(void *data, uint16_t index)
{
    conn_info_t *p = conn_infos + index;
    initiating_phy_config_t phy_configs[] =
    {
        {
            .phy = PHY_1M,
            .conn_param =
            {
                .scan_int = 150,
                .scan_win = 100,
                .interval_min = p->min_interval,
                .interval_max = p->max_interval,
                .latency = p->latency,
                .supervision_timeout = p->timeout,
                .min_ce_len = 7,
                .max_ce_len = 7
            }
        }
    };

    if (gap_ext_create_connection(
                INITIATING_ADVERTISER_FROM_PARAM,
                BD_ADDR_TYPE_LE_RANDOM,
                p->peer_addr_type,
                p->peer_addr,
                sizeof(phy_configs) / sizeof(phy_configs[0]),
                phy_configs) == 0)
    {
        gAT_ctrl_env.initialization_ongoing = true;
        initiating_index = index;
        platform_set_timer(initiate_timeout, initiating_timeout * 1600);
    }
    else
    {
        LOG_MSG("gap_ext_create_connection failed.\r\n");
        gAT_ctrl_env.initialization_ongoing = false;
        gAT_ctrl_env.async_evt_on_going = false;
    }
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
    if (gAT_ctrl_env.curr_adv_int != adv_int_arr[g_power_off_save_data_in_ram.default_info.adv_int])
        config_adv_and_set_interval(adv_int_arr[g_power_off_save_data_in_ram.default_info.adv_int]);
    start_adv();
    gAT_ctrl_env.adv_ongoing = true;
    ADV_LED_ON;
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
 * @fn      at_link_idle_status_hdl
 *
 * @brief   at event call back function, handle after all link is disconnected
 *			
 *
 * @param   arg - reserved
 *       	
 *
 * @return  None
 */
void at_link_idle_status_hdl(void *arg)
{
    if(gap_get_connect_num()==0)
    {
        LINK_LED_OFF;
        if(gAT_ctrl_env.async_evt_on_going)
        {
            uint8_t *at_rsp = malloc(150);
            sprintf((char *)at_rsp,"+DISCONN:A\r\nOK");
            at_send_rsp((char *)at_rsp);
            free(at_rsp);
            gAT_ctrl_env.async_evt_on_going = false;
        }
    }
}

/*********************************************************************
 * @fn      get_conn_by_addr
 *
 * @brief   get_conn_by_addr
 *			
 * @param[in]  type
 * @param[in]  addr
 *
 * @return  pointer of conn_info_t
 */
conn_info_t *get_conn_by_addr(bd_addr_type_t type, const uint8_t *addr)
{
    int i;
    for (i = 0; i < MAX_CONN_AS_MASTER; i++)
    {
        if ((conn_infos[i].peer_addr_type == type)
            && (memcmp(conn_infos[i].peer_addr, addr, sizeof(conn_infos[i].peer_addr)) == 0))
            return conn_infos + i;
    }
    return NULL;
}

static char *append_bd_addr(char *s, const uint8_t *addr)
{
    return s + sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0], addr[1], addr[2],
            addr[3], addr[4], addr[5]);
}

static void report_connected(uint8_t id)
{
    char *s  = buffer + sprintf(buffer, "+BLECONN:%d,", id);
    s = append_bd_addr(s, conn_infos[id].peer_addr);
    strcpy(s, "\n");
    at_send_rsp(buffer);
}

/*********************************************************************
 * @fn      at_on_connection_complete
 *
 * @brief   
 *			
 * @param[in]  complete connection data package
 *
 * @return  None.
 */
void at_on_connection_complete(const le_meta_event_enh_create_conn_complete_t *complete)
{
    conn_info_t *p = NULL;
    if (complete->role == HCI_ROLE_SLAVE)
    {
        if (complete->status != 0) return;

        int i;
        for (i = MAX_CONN_AS_MASTER; i < TOTAL_CONN_NUM; i++)
        {
            if (conn_infos[i].handle == INVALID_HANDLE)
            {
                p = conn_infos + i;
                break;
            }
        }
        if (p)
        {
            p->handle = complete->handle;
            handle_2_id[complete->handle] = p - conn_infos;

            p->peer_addr_type = complete->peer_addr_type;
            reverse_bd_addr(complete->peer_addr, p->peer_addr);
        }
        else
        {
            // too many connections
            gap_disconnect(complete->handle);
        }
    }
    else
    {
        platform_set_timer(initiate_timeout, 0);

        if (complete->status == 0)
        {
            bd_addr_t rev;
            reverse_bd_addr(complete->peer_addr, rev);
            p = get_conn_by_addr(complete->peer_addr_type, rev);
            if (p)
            {
                p->handle = complete->handle;
                handle_2_id[complete->handle] = p - conn_infos;

                //if (sec_auth_req & SM_AUTHREQ_BONDING)
                //    sm_request_pairing(complete->handle);
            }
            else
                gap_disconnect(complete->handle);
        }
        else
        {
            int len = sprintf(buffer, "+BLECONN:%d,-1\n", initiating_index);
            at_send_rsp(buffer);
        }
        initiating_index = -1;
    }

    if (p)
    {
        p->cur_interval = complete->interval;
        p->latency = complete->latency;
        p->timeout = complete->sup_timeout;

        report_connected(get_id_of_handle(complete->handle));
    }
}


/*********************************************************************
 * @fn      at_on_disconnect
 *
 * @brief   
 *			
 * @param[in]  complete disconnection data package
 *
 * @return  None.
 */
void at_on_disconnect(const event_disconn_complete_t *complete)
{
    int id = get_id_of_handle(complete->conn_handle);
    int len = sprintf(buffer, "+DISCONN:%d,%d\n", id, complete->status);
    at_send_rsp(buffer);
    conn_infos[id].handle = INVALID_HANDLE;
    get_id_of_handle(complete->conn_handle) = 0;

    conn_info_t *p = conn_infos + id;
    notification_handler_t *first = p->first_handler;
    while (first)
    {
        p->first_handler = first->next;
        free(first);
        first = p->first_handler;
    }
    
    // 允许接收新AT指令
    
    // 透传时断连需要重置透传上下文
    gAT_ctrl_env.async_evt_on_going = false;
    if (gap_get_connect_status(gAT_ctrl_env.transparent_conidx) == false)
    {
        at_clr_uart_buff();
        gAT_ctrl_env.transparent_start = false;
        gAT_ctrl_env.transparent_notify_enable = false;
        at_send_rsp("Exit transparent mode");
        
        if (gAT_ctrl_env.one_slot_send_start && gAT_ctrl_env.one_slot_send_len > 0)
        {
            gAT_ctrl_env.one_slot_send_start = false;
            gAT_ctrl_env.one_slot_send_len = 0;
            at_send_rsp("SEND FAIL");
        }
    }
    
    
    // 单次发送时断连需要重置单次发送上下文
    if(gAT_ctrl_env.one_slot_send_start && gAT_ctrl_env.one_slot_send_len > 0)
    {
        if(gap_get_connect_status(gAT_ctrl_env.transparent_conidx)==false)
        {
            at_clr_uart_buff();
            
        }
    }
}

/*********************************************************************
 * @fn      print_uuid
 *
 * @brief   
 *			
 * @param[out]  s
 * @param[in]   uuid    
 *
 * @return  sprintf
 */
static int print_uuid(char *s, const uint8_t *uuid)
{
    if (uuid_has_bluetooth_prefix(uuid))
    {
        return sprintf(s, "%04x", (uuid[2] << 8) | uuid[3]);
    }
    else
        return sprintf(s, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                        uuid[0], uuid[1], uuid[2], uuid[3],
                        uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9],
                        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}

/*********************************************************************
 * @fn      gatt_client_dump_profile
 *
 * @brief   
 *			
 *
 * @param[in]   first       service_node
 * @param[in]   user_data   user_data
 * @param[in]   err_code    err_code
 *
 * @return  None
 */
static void gatt_client_dump_profile(service_node_t *first, void *user_data, int err_code)
{
    service_node_t *s = first;
    conn_info_t *p = (conn_info_t *)user_data;
    int id = p - conn_infos;

    while (s)
    {
        char_node_t *c = s->chars;
        char *str = buffer + sprintf(buffer, "+BLEGATTCPRIMSRV:%d,%d,%d,",
            id, s->service.start_group_handle, s->service.end_group_handle);
        str += print_uuid(str, s->service.uuid128);
        strcpy(str, "\n");
        at_send_rsp(buffer);

        while (c)
        {
            str = buffer + sprintf(buffer, "+BLEGATTCCHAR:%d,%d,%d,%d,%d,",
                id, c->chara.start_handle, c->chara.end_handle, c->chara.value_handle,
                c->chara.properties);
            str += print_uuid(str, c->chara.uuid128);
            strcpy(str, "\n");
            at_send_rsp(buffer);

            desc_node_t *d = c->descs;
            while (d)
            {
                str = buffer + sprintf(buffer, "+BLEGATTCDESC:%d,%d,",
                    id, d->desc.handle);
                str += print_uuid(str, d->desc.uuid128);
                strcpy(str, "\n");
                at_send_rsp(buffer);

                d = d->next;
            }
            c = c->next;
        }
        s = s->next;
    }

    int len = sprintf(buffer, "+BLEGATTCC:%d,%d\n", id, err_code);
    at_send_rsp(buffer);
    gatt_client_util_free(p->discoverer);
    p->discoverer = NULL;
}

/*********************************************************************
 * @fn      stack_discover_all
 *
 * @brief   
 *			
 *
 * @param[in]   p       conn_info_t
 * @param[in]   value   conn_handle
 *       	
 *
 * @return  None
 */
static void stack_discover_all(void *p, uint16_t value)
{
    conn_info_t *conn = (conn_info_t *)p;
    conn->discoverer = gatt_client_util_discover_all(value, gatt_client_dump_profile, p);
}



/*********************************************************************
 * @fn      read_characteristic_value_callback
 *
 * @brief   gatt characteristic read callback
 *			
 * @param[in]   packet_type
 * @param[in]   channel
 * @param[in]   packet
 * @param[in]   size
 *
 * @return  None
 */
void read_characteristic_value_callback(uint8_t packet_type, uint16_t channel, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
        {
            uint16_t value_size;
            const gatt_event_value_packet_t *value =
                gatt_event_characteristic_value_query_result_parse(packet, size, &value_size);

            char *s = buffer + sprintf(buffer, "+BLEGATTCRD:%d,%d,0,", get_id_of_handle(channel), value->handle);
            s = append_hex_str(s, value->value, value_size);
            strcpy(s, "\n");
            at_send_rsp(buffer);
        }
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        {
            const gatt_event_query_complete_t *complete = gatt_event_query_complete_parse(packet);
            if (complete->status != 0)
            {
                int len = sprintf(buffer, "+BLEGATTCRD:%d,%d,%d\n", get_id_of_handle(channel), complete->handle, complete->status);
                at_send_rsp(buffer);
            }
            else;
        }
        break;
    }
}

/*********************************************************************
 * @fn      stack_read_char
 *
 * @brief   
 *			
 * @param[in]   p
 * @param[in]   value_handle
 *
 * @return  None
 */
static void stack_read_char(void *p, uint16_t value_handle)
{
    uint8_t r = gatt_client_read_value_of_characteristic_using_value_handle(
                read_characteristic_value_callback,
                (uint16_t)(uintptr_t)p,
                value_handle);
    if (0 == r)
        at_send_rsp("OK");
    else
        at_send_rsp("ERROR");
}


/*********************************************************************
 * @fn      stack_write_char
 *
 * @brief   
 *			
 * @param[in]   user_data
 * @param[in]   value_len
 *
 * @return  None
 */
static void stack_write_char(void *user_data, uint16_t value_len)
{
    conn_info_t *p = (conn_info_t *)user_data;

    uint8_t r = gatt_client_write_value_of_characteristic_without_response(
                //write_characteristic_value_callback,
                p->handle,
                p->write_char_info.value_handle,
                value_len,
                p->write_char_info.data);
    if (0 == r)
        at_send_rsp("OK");
    else
        at_send_rsp("ERROR");
}


/*********************************************************************
 * @fn      output_notification_handler
 *
 * @brief   
 *
 * @param[in]   packet_type
 * @param[in]   channel
 * @param[in]   packet
 * @param[in]   size
 *
 * @return  None
 */
static void output_notification_handler(uint8_t packet_type, uint16_t channel, const uint8_t *packet, uint16_t size)
{
    const gatt_event_value_packet_t *value;
    platform_printf("notify data\n");
    uint16_t value_size = 0;
    int len = 0;
    switch (packet[0])
    {
    case GATT_EVENT_NOTIFICATION:
        value = gatt_event_notification_parse(packet, size, &value_size);
        strcpy(buffer, "+BLEGATTCNOTI");
        len = 13;
        break;
    case GATT_EVENT_INDICATION:
        // TODO: indication
        break;
    }
    if (value_size < 1) return;
    char *s = buffer + len;
    s += sprintf(s, ":%d,%d,", get_id_of_handle(channel), value->handle);
    s = append_hex_str(s, value->value, value_size);
    strcpy(s, "\n");
    at_send_rsp(buffer);
}

/*********************************************************************
 * @fn      write_characteristic_descriptor_callback
 *
 * @brief   
 *
 * @param[in]   packet_type
 * @param[in]   channel
 * @param[in]   packet
 * @param[in]   size
 *
 * @return  None
 */
static void write_characteristic_descriptor_callback(uint8_t packet_type, uint16_t channel, const uint8_t *packet, uint16_t size)
{
    switch (packet[0])
    {
    case GATT_EVENT_QUERY_COMPLETE:
        {
            const gatt_event_query_complete_t *complete = gatt_event_query_complete_parse(packet);
            int len = sprintf(buffer, "+BLEGATTCSUB:%d,%d,%d\n", get_id_of_handle(channel), complete->handle, complete->status);
            at_send_rsp(buffer);
        }
        break;
    }
}


/*********************************************************************
 * @fn      stack_sub_char
 *
 * @brief   
 *			
 *
 * @param[in]   user_data
 * @param[in]   value_handle
 *       	
 *
 * @return  None
 */
static void stack_sub_char(void *user_data, uint16_t value_handle)
{
    conn_info_t *p = (conn_info_t *)user_data;

    notification_handler_t *first = p->first_handler;

    while (first)
    {
        if (first->value_handle == value_handle) break;
        first = first->next;
    }

    if (NULL == first) return;

    if (first->registered == 0)
    {
        platform_printf("listen %d %d\n", p->handle, first->value_handle);
        gatt_client_listen_for_characteristic_value_updates(
            &first->notification, output_notification_handler,
            p->handle, first->value_handle);
        first->registered = 1;
    }

    gatt_client_write_characteristic_descriptor_using_descriptor_handle(
        write_characteristic_descriptor_callback,
        p->handle,
        first->desc_handle,
        sizeof(first->config),
        (uint8_t *)&first->config);
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
    
    //uint8_t argc = preprocess_at(buff);
    //printf("argc = %d\n", argc);
    
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
                    //if (argc != 1)
                    //{
                    //    // argc error message
                    //    at_err();
                    //    goto _exit;
                    //}
                    //
                    //uint16_t len = get_arg_len(buff);
                    //char *name = NULL;
                    //parse_arg_str_normal(buff, name);
                    //
                    //if (len >= LOCAL_NAME_MAX_LEN)
                    //{
                    //    // name len overflow error message
                    //    at_err();
                    //    goto _exit;
                    //}
                    //
                    //if (local_name_len != len || memcmp(local_name, name, local_name_len) != 0)   
                    //{
                    //    //name is different, then set it
                    //    gap_set_dev_name(buff, len + 1);
                    //    at_init_adv_rsp_parameter();
                    //}
                    //sprintf((char *)at_rsp,"+NAME:%s\r\nOK",buff);
                    //at_send_rsp((char *)at_rsp);
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
                    mode_str[0] = 'I';              //idle
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

                        if(gAT_ctrl_env.adv_ongoing)
                        {
                            at_set_gap_cb_func(AT_GAP_CB_ADV_END,at_idle_status_hdl);
                            stop_adv();
                        }
                        if(gAT_ctrl_env.scan_ongoing)
                        {
                            at_set_gap_cb_func(AT_GAP_CB_SCAN_END,at_idle_status_hdl);
                            stop_scan();
                        }
                        if(gAT_ctrl_env.initialization_ongoing)
                        {
                            at_set_gap_cb_func(AT_GAP_CB_CONN_END,at_idle_status_hdl);
                            gap_disconnect_all();
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
                            at_start_advertising(NULL);
                            at_set_gap_cb_func(AT_GAP_CB_DISCONNECT,at_cb_disconnected);
                        }
                        sprintf((char *)at_rsp,"+MODE:B\r\nOK");
                        at_send_rsp((char *)at_rsp);
                    }
                    else if(*buff == 'M')
                    {
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
        
        case AT_CMD_IDX_SCAN_FILTER:
        {
            switch(*buff++)
            {
                case '?':
                {
                }
                break;
                case '=':
                {
                    uint8_t* p_name_prefix = buff;
                    
                    uint8_t* p_name_suffix = find_int_from_str(buff) + 1;
                    buff = p_name_suffix;
                    
                    uint8_t* p_uuid16 = find_int_from_str(buff) + 1;
                    buff = p_uuid16;
                    
                    uint8_t* p_uuid128 = find_int_from_str(buff) + 1;
                    buff = p_uuid128;
                    
                    uint8_t* p_rssi = find_int_from_str(buff) + 1;
                    buff = p_rssi;
                    
                    find_int_from_str(buff);
                    
                    strcpy(g_power_off_save_data_in_ram.scan_filter.name_prefix, (char*)p_name_prefix);
                    strcpy(g_power_off_save_data_in_ram.scan_filter.name_suffix, (char*)p_name_suffix);
                    
                    if (strcmp((char*)p_uuid16, "*") == 0 || strlen((char*)p_uuid16) != (UUID_SIZE_2 * 2)) {
                        g_power_off_save_data_in_ram.scan_filter.enable_uuid_16_filter = false;
                    } else {
                        g_power_off_save_data_in_ram.scan_filter.enable_uuid_16_filter = true;
                        str_to_hex_arr(p_uuid16, g_power_off_save_data_in_ram.scan_filter.uuid_16, UUID_SIZE_2);
                    }
                    
                    if (strcmp((char*)p_uuid128, "*") == 0 || strlen((char*)p_uuid128) != (UUID_SIZE_16 * 2)) {
                        g_power_off_save_data_in_ram.scan_filter.enable_uuid_128_filter = false;
                    } else {
                        g_power_off_save_data_in_ram.scan_filter.enable_uuid_128_filter = true;
                        str_to_hex_arr(p_uuid128, g_power_off_save_data_in_ram.scan_filter.uuid_128, UUID_SIZE_16);
                    }
                    
                    if (strcmp((char*)p_rssi, "*") == 0) {
                        g_power_off_save_data_in_ram.scan_filter.enable_rssi_filter = false;
                    } else {
                        g_power_off_save_data_in_ram.scan_filter.enable_rssi_filter = true;
                        g_power_off_save_data_in_ram.scan_filter.rssi = atoi((char*)p_rssi);
                    }
                }
                break;
            }
            
            char uuid16_buf[5] = {0};
            char uuid128_buf[33] = {0};
            char rssi_buf[5] = {0};
            
            if (g_power_off_save_data_in_ram.scan_filter.enable_uuid_16_filter) {
                hex_arr_to_str(g_power_off_save_data_in_ram.scan_filter.uuid_16, UUID_SIZE_2, (uint8_t*)uuid16_buf);
            } else {
                strcpy(uuid16_buf, "*");
            }
            if (g_power_off_save_data_in_ram.scan_filter.enable_uuid_128_filter) {
                hex_arr_to_str(g_power_off_save_data_in_ram.scan_filter.uuid_128, UUID_SIZE_16, (uint8_t*)uuid128_buf);
            } else {
                strcpy(uuid128_buf, "*");
            }
            if (g_power_off_save_data_in_ram.scan_filter.enable_rssi_filter) {
                sprintf(rssi_buf, "%d", g_power_off_save_data_in_ram.scan_filter.rssi);
            } else {
                strcpy(rssi_buf, "*");
            }
            
            sprintf((char *)at_rsp,"+SCAN_FILTER:%s,%s,%s,%s,%s\r\nOK", 
                g_power_off_save_data_in_ram.scan_filter.name_prefix, 
                g_power_off_save_data_in_ram.scan_filter.name_suffix,
                uuid16_buf,
                uuid128_buf,
                rssi_buf);
            at_send_rsp((char *)at_rsp);
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
                    int i;
                    for (i = 0; i < TOTAL_CONN_NUM; i++)
                    {
                        conn_info_t *p = conn_infos + i;
                        if (p->handle == INVALID_HANDLE) continue;
                        char *s = buffer + sprintf(buffer, "+BLECONN:%d,", i);
                        s = append_bd_addr(s, p->peer_addr);
                        strcpy(s, "\n");
                        s++;
                        at_send_rsp(buffer);
                    }
                    //at_tx_ok();
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
                            for(uint8_t i = 0; i<TOTAL_CONN_NUM; i++)
                            {
                                if(gap_get_connect_status(i))
                                    gap_disconnect(i);
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
                                gap_disconnect(get_handle_of_id(link_num));
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
                    sprintf((char *)at_rsp,"+VER:%d.%d.%d\r\nOK", prog_ver.major, prog_ver.minor, prog_ver.patch);
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
                    int stop_bits = convert_from_two_stop_bits(g_power_off_save_data_in_ram.uart_param.two_stop_bits);
                    
                    sprintf((char *)at_rsp,"+UART:%d,%d,%d,%d\r\nOK",
                        g_power_off_save_data_in_ram.uart_param.BaudRate,
                        databit,
                        pari,
                        stop_bits);
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
                    int pari = atoi((const char *)buff);
                    g_power_off_save_data_in_ram.uart_param.parity = convert_to_parity(pari);
                    
                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    int stop_bits = atoi((const char *)buff);
                    g_power_off_save_data_in_ram.uart_param.two_stop_bits = convert_to_two_stop_bits(stop_bits);

                    databit = convert_from_word_length(g_power_off_save_data_in_ram.uart_param.word_length);
                    pari = convert_from_parity(g_power_off_save_data_in_ram.uart_param.parity);
                    stop_bits = convert_from_two_stop_bits(g_power_off_save_data_in_ram.uart_param.two_stop_bits);
                    
                    sprintf((char *)at_rsp,"+UART:%d,%d,%d,%d\r\nOK",
                        g_power_off_save_data_in_ram.uart_param.BaudRate,
                        databit,
                        pari,
                        stop_bits);
                    at_send_rsp((char *)at_rsp);
                    
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    
                    apUART_Initialize(UART0, 
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
                        platform_config(PLATFORM_CFG_POWER_SAVING, PLATFORM_CFG_ENABLE);
                        g_power_off_save_data_in_ram.default_info.auto_sleep = true;
                        //set_sleep_flag_after_key_release(true);
                        for(uint8_t i=0; i< TOTAL_CONN_NUM; i++)
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
                        g_power_off_save_data_in_ram.default_info.auto_sleep = false;
                        //set_sleep_flag_after_key_release(false);
                        for(uint8_t i=0; i< TOTAL_CONN_NUM; i++)
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
        
        case AT_CMD_IDX_SHUTDOWN:
        {
            platform_shutdown(0, NULL, 0);
        }
        break;
        
        case AT_CMD_IDX_CONN_PHY:
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
                    uint32_t phy = atoi((const char *)buff);
                    
                    gap_set_phy(get_handle_of_id(conidx), 0, phy, phy, HOST_NO_PREFERRED_CODING);
                    
                    sprintf((char *)at_rsp,"\r\n+CONN_PHY:%d,%d\r\nOK", conidx, phy);
                    at_send_rsp((char *)at_rsp);
                }
                break;
            }
        }
        break;
        
        case AT_CMD_IDX_CONN_PARAM:
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
                    uint32_t interval = atoi((const char *)buff);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    uint32_t latency = atoi((const char *)buff);

                    buff = pos_int_end+1;
                    pos_int_end = find_int_from_str(buff);
                    uint32_t supervision_timeout = atoi((const char *)buff);
                    
                    gap_update_connection_parameters(get_handle_of_id(conidx), interval, interval, 
                                                     latency, 
                                                     supervision_timeout, 
                                                     phy_configs[0].conn_param.min_ce_len, 
                                                     phy_configs[0].conn_param.max_ce_len);
                    
                    sprintf((char *)at_rsp,"\r\n+CONN_PARAM:%d,%d,%d,%d\r\nOK", conidx, interval, latency, supervision_timeout);
                    at_send_rsp((char *)at_rsp);
                }
                break;
            }
        }
        break;
        
        case AT_CMD_IDX_CONNADD:
        {
            //uint8_t peer_mac_addr_str[MAC_ADDR_LEN*2+1];
            //switch(*buff++)
            //{
            //    case '?':
            //        break;
            //    case '=':
            //    {
            //        uint8_t *pos_int_end;
            //        str_to_hex_arr(buff,g_power_off_save_data_in_ram.master_peer_param.addr,MAC_ADDR_LEN);
            //        pos_int_end = find_int_from_str(buff);
            //        buff = pos_int_end+1;
            //        g_power_off_save_data_in_ram.master_peer_param.addr_type = atoi((const char *)buff);
            //        // if (g_power_off_save_data_in_ram.master_peer_param.addr_type > 1)
            //        //     g_power_off_save_data_in_ram.master_peer_param.addr_type = 0;
            //    }
            //    break;
            //}
            //hex_arr_to_str(g_power_off_save_data_in_ram.master_peer_param.addr,MAC_ADDR_LEN,peer_mac_addr_str);
            //peer_mac_addr_str[MAC_ADDR_LEN*2] = 0;
            //sprintf((char *)at_rsp,"\r\n+CONNADD:%s,%d\r\nOK",peer_mac_addr_str,g_power_off_save_data_in_ram.master_peer_param.addr_type);
            //at_send_rsp((char *)at_rsp);
        }
        break;
        case AT_CMD_IDX_CONN:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t id = atoi((const char *)buff);
                    
                    if(gAT_ctrl_env.initialization_ongoing == false
                        && (0 <= id) && (id < ADV_REPORT_NUM)
                        && gAT_buff_env.adv_rpt[id].evt_type != 0xFF)
                    {
                        gAT_ctrl_env.async_evt_on_going = true;
                        
                        conn_info_t *p = NULL;
                        
                        int i;
                        for (i = 0; i < MAX_CONN_AS_MASTER; i++)
                        {
                            if (conn_infos[i].handle == INVALID_HANDLE)
                            {
                                p = conn_infos + i;
                                break;
                            }
                        }
                        if (p)
                        {
                            memcpy(p->peer_addr, gAT_buff_env.adv_rpt[id].adv_addr, MAC_ADDR_LEN);
                            p->peer_addr_type = gAT_buff_env.adv_rpt[id].adv_addr_type;
                            
                            btstack_push_user_runnable(stack_initiate, NULL, (uint16_t)id);
                        }
                        else
                        {
                            sprintf((char *)at_rsp,"+CONN:%d\r\nERR", id);
                            at_send_rsp((char *)at_rsp);
                        }
                    }
                    else
                    {
                        sprintf((char *)at_rsp,"+CONN:%d\r\nERR", id);
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
        
        case AT_CMD_IDX_TO:
        {
            switch(*buff++)
            {
                case '?':
                {
                    if (gAT_ctrl_env.transparent_conidx == 0xFF)
                    {
                        sprintf((char *)at_rsp,"+TO\r\nNOT SET");
                        at_send_rsp((char *)at_rsp);
                    }
                    else
                    {
                        sprintf((char *)at_rsp,"+TO:%d\r\nOK", gAT_ctrl_env.transparent_conidx);
                        at_send_rsp((char *)at_rsp);
                    }
                }
                break;
                
                case '=':
                {
                    uint8_t *pos_int_end;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t conidx = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    if ((0 <= conidx) && (conidx < TOTAL_CONN_NUM)
                        && (gap_get_connect_status(conidx) == true))
                    {
                        if (conidx < MAX_CONN_AS_MASTER)
                            gAT_ctrl_env.transparent_method = TRANSMETHOD_M2S_W;
                        else
                            gAT_ctrl_env.transparent_method = TRANSMETHOD_S2M_N;
                        gAT_ctrl_env.transparent_conidx = conidx;
                        
                        sprintf((char *)at_rsp,"+TO:%d\r\nOK", gAT_ctrl_env.transparent_conidx);
                        at_send_rsp((char *)at_rsp);
                    }
                    else
                    {
                        sprintf((char *)at_rsp,"+TO:%d\r\nERR", gAT_ctrl_env.transparent_conidx);
                        at_send_rsp((char *)at_rsp);
                    }
                }
                break;
            }
        }
        break;
        
        case AT_CMD_IDX_TRANSPARENT:            //go to transparent transmit
        {
            if(gAT_ctrl_env.transparent_conidx != 0xFF 
                && gap_get_connect_status(gAT_ctrl_env.transparent_conidx) == true)
            {
                gAT_ctrl_env.transparent_start = true;
                at_clr_uart_buff();
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
                    if(g_power_off_save_data_in_ram.default_info.auto_transparent == true)
                        sprintf((char *)at_rsp,"+AUTO+++:Y\r\nOK");
                    else
                        sprintf((char *)at_rsp,"+AUTO+++:N\r\nOK");
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                    if(*buff == 'Y')
                    {
                        g_power_off_save_data_in_ram.default_info.auto_transparent = true;
                        sprintf((char *)at_rsp,"+AUTO+++:Y\r\nOK");
                    }
                    else if(*buff == 'N')
                    {
                        g_power_off_save_data_in_ram.default_info.auto_transparent = false;
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
                    sprintf((char *)at_rsp,"+POWER:%d\r\nOK",g_power_off_save_data_in_ram.default_info.rf_power);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                    g_power_off_save_data_in_ram.default_info.rf_power = atoi((const char *)buff);
                    if(g_power_off_save_data_in_ram.default_info.rf_power > 5)
                        sprintf((char *)at_rsp,"+POWER:%d\r\nERR",g_power_off_save_data_in_ram.default_info.rf_power);
                    else
                    {
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
        
        case AT_CMD_IDX_RXNUM:
        {
            switch(*buff++)
            {
                case '?':
                    sprintf((char *)at_rsp,"+RXNUM:%d\r\nOK",print_data_len_flag);
                    at_send_rsp((char *)at_rsp);
                    break;
                case '=':
                {
                    print_data_len_flag = atoi((const char *)buff);
                    
                    if (print_data_len_flag)
                    {
                        receive_master_data_len = 0;
                        sprintf((char *)at_rsp,"+RXNUM:enable\r\nOK");
                    }
                    else
                    {
                        sprintf((char *)at_rsp,"+RXNUM:disable\r\nOK");
                    }
                    at_send_rsp((char *)at_rsp);
                }
                break;
                default:
                    break;
            }
        }
        break;
        
        case AT_CMD_IDX_BLEGATTSRD:
        {
            
        }
        break;
        
        case AT_CMD_IDX_BLEGATTSWR:
        {
        }
        break;
        
        case AT_CMD_IDX_BLEGATTC:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t *pos_int_end;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t conidx = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    conn_info_t *p = &conn_infos[conidx];
                    
                    if (NULL == p)
                    {
                        sprintf((char *)at_rsp,"+AT_CMD_IDX_BLEGATTSRD:%d\r\nERR",conidx);
                        at_send_rsp((char *)at_rsp);
                    }
                    else
                    {
                        btstack_push_user_runnable(stack_discover_all, p, p->handle);
                    }
                }
                break;
            }
        }
        break;
        
        case AT_CMD_IDX_BLEGATTCRD:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t *pos_int_end;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t id = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    conn_info_t *p = conn_infos + id;
                    if (p->handle == INVALID_HANDLE)    //invalid id
                        break;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t value_handle = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    p->write_char_info.value_handle = value_handle;

                    btstack_push_user_runnable(stack_read_char, (void *)(uintptr_t)p->handle, value_handle);
                    return;
                }
                break;
            }
        }
        break;
        
        case AT_CMD_IDX_BLEGATTCWR:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t *pos_int_end;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t id = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    conn_info_t *p = conn_infos + id;
                    if (p->handle == INVALID_HANDLE)    //invalid id
                        break;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t value_handle = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    p->write_char_info.value_handle = value_handle;
                    
                    pos_int_end = find_int_from_str(buff);
                    
                    p->write_char_info.data = buff;
                    
                    uint16_t len = strlen((char *)buff) / 2;
                    
                    str_to_hex_arr(buff, buff, len);

                    btstack_push_user_runnable(stack_write_char, p, len);
                    
                    buff = pos_int_end + 1;
                }
                break;
            }
        }
        break;
        
        case AT_CMD_IDX_BLEGATTCSUB:
        {
            switch(*buff++)
            {
                case '=':
                {
                    uint8_t *pos_int_end;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t conidx = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t value_handle = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    pos_int_end = find_int_from_str(buff);
                    uint8_t config = atoi((const char *)buff);
                    buff = pos_int_end + 1;
                    
                    platform_printf("%d %d %d", conidx, value_handle, config);
                    
                    
                    conn_info_t *p = conn_infos + conidx;
                    if (p->handle == INVALID_HANDLE) break;

                    notification_handler_t *first = p->first_handler;

                    while (first)
                    {
                        if (first->value_handle == value_handle) break;
                        first = first->next;
                    }

                    if (NULL == first)
                    {
                        first = (notification_handler_t *)malloc(sizeof(notification_handler_t));
                        first->next = p->first_handler;
                        p->first_handler = first;

                        first->registered = 0;
                        first->value_handle = value_handle;
                        first->desc_handle = value_handle + 1;
                    }

                    first->config = config;

                    btstack_push_user_runnable(stack_sub_char, p, value_handle);
                }
                break;
            }
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
void auto_transparent_clr(void)
{
    if(gAT_ctrl_env.transparent_start)
    {
        gAT_ctrl_env.transparent_start = 0;
        at_clr_uart_buff();
    }
}
