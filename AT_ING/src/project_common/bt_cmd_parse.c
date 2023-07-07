
/*
** COPYRIGHT (c) 2020 by INGCHIPS
*/

#include <stdio.h>
#include <string.h>

#include "btstack_event.h"

#include "project_common.h"
#include "bt_cmd_parse.h"

#include "crc.h"
#include "att_db.h"

#if defined __cplusplus
    extern "C" {
#endif

int cmd_root_num = 0;

void baec_msg_init(void)
{
    crcInit( );
}

int msg_get_temperature(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_delete_all_temperature_record(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_set_power_mode(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_get_real_time_temperature(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}

int msg_get_calendar(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    log_printf("msg_get_calendar 0x%02x_%02x\r\n", in_buff[0], in_buff[1]);
    return BT_PRIVT_FALSE;
}
int msg_set_calendar(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_get_sample_time(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_set_sample_time(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_get_temperture_calibration(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_set_temperture_calibration(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_set_device_name(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_get_device_name(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}
int msg_get_board_info(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength)
{
    return BT_PRIVT_FALSE;
}

static bt_hex_cmd_parse_fun_t cmd_point[] = {
    {0x0182, &msg_get_temperature},                /* 读取历史体温 */
    {0x018C, &msg_delete_all_temperature_record},  /* 删除所有温度记录 */
    {0x018E, &msg_set_power_mode},                 /* 系统模式控制 */
    {0x0190, &msg_get_real_time_temperature},      /* 读取实时体温 */
    {0x0192, &msg_get_calendar},                   /* 获取时间 */
    {0x0194, &msg_set_calendar},                   /* 设置时间 */
    {0x0196, &msg_get_sample_time},                /* 获取温度采样间隔时间 */
    {0x0198, &msg_set_sample_time},                /* 设置温度采样间隔时间 */
    {0x01A0, &msg_get_temperture_calibration},     /* 读取温度质保系数 */
    {0x01A2, &msg_set_temperture_calibration},     /* 设置温度质保系数 */
    {0x01A4, &msg_set_device_name},                /* 更改蓝牙广播名 */
    {0x01A6, &msg_get_device_name},                /* 获取蓝牙广播名 */
    {0x01A8, &msg_get_board_info}                  /* 获取硬件版本信息、采样间隔、日历、校准值、设备名称、ID、软件版本号、充电状态，软件名称 */
};
static int NUMBER_CMD = sizeof(cmd_point) / sizeof(cmd_point[0]);

int baec_msg_handle(uint8_t const * in_buff, uint16_t inlength, uint8_t * out_buff, uint16_t * outlength) // --test
{
    int cmd_index;
    uint8_t msg_head;
    uint16_t msg_len;
    uint16_t msg_cmd;
    uint16_t msg_crc;
    uint8_t msg_end;
    uint16_t msg_result;
    uint16_t crc;
    int i;

    cmd_index = 0;
    msg_head = in_buff[cmd_index]; //  << 24) | (str_in[str_index + 1] << 16) | (str_in[str_index + 2] << 8) | str_in[str_index + 3]
    log_printf("head 0x%02x\r\n", msg_head);
    if(msg_head != 0x5A)
    {
        *outlength=0;
        return 0;
    }

    msg_len = (in_buff[cmd_index + 4]) | (in_buff[cmd_index + 5] << 8);
    log_printf("len 0x%04x\r\n", msg_len);
    if(msg_len > inlength)
    {
        *outlength=0;
        return 0;
    }
    
    msg_crc=(in_buff[msg_len - 3] | in_buff[msg_len - 2] << 8);
    crc=crcCompute((unsigned char *)in_buff, (msg_len-3));
    log_printf("crc 0x%04x 0x%04x\r\n", msg_crc,crc);
    if(msg_crc != crc )
    {
        *outlength=0;
        return 0;
    }

    msg_end=in_buff[msg_len-1];
    if(msg_end!=0xA5)
    {
        *outlength=0;
        return 0;
    }
    
    msg_cmd = (in_buff[cmd_index + 6]) | (in_buff[cmd_index + 7] << 8);
    log_printf("cmd 0x%04x\r\n", msg_cmd);
    (*outlength)=(*outlength)-11;
    msg_result=0;
    for ( i = 0; i < NUMBER_CMD; i++)
    {
        if (msg_cmd == cmd_point[i].cmd_code)
        {
            msg_result=cmd_point[i].cmd_handle(in_buff+8, (msg_len-11), (out_buff+8), outlength);
            break;
        }
    }
    dbg_printf("1\r\n");
    
    //illegal cmd
    if(i == NUMBER_CMD)
    {
        msg_result=1;

        *outlength=4;
        out_buff[8]=0xFF;
        out_buff[9]=0xFF;
        out_buff[10]=0xFF;
        out_buff[11]=0xFF;
    }

    out_buff[0]=0x5A;

    out_buff[1]=0x00;
    out_buff[2]=0x00;
    out_buff[3]=0x00;

    *outlength=(*outlength)+11;

    out_buff[4]=((*outlength))&0xFF;
    out_buff[5]=((*outlength)>>8)&0xFF;

    if(msg_result!=0)
    {
        msg_cmd=msg_cmd|0x8000;
    }
    msg_cmd=msg_cmd|0x0001;

    out_buff[6]=msg_cmd&0xFF;
    out_buff[7]=(msg_cmd>>8)&0xFF;
    dbg_printf("2\r\n");

    msg_crc= crcCompute(out_buff, (*outlength)-3);

    out_buff[(*outlength)-3]=msg_crc&0xFF;
    out_buff[(*outlength)-2]=(msg_crc>>8)&0xFF;
    out_buff[(*outlength)-1]=0xA5;
    dbg_printf("3\r\n");

    return BT_PRIVT_ERROR;
}

#if defined __cplusplus
    }
#endif

