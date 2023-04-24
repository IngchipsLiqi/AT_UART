#include "utils.h"

#include <stdio.h>
#include <string.h>
#include "btstack_util.h"



uint32_t get_sig_short_uuid(const uint8_t *uuid128)
{
    return uuid_has_bluetooth_prefix(uuid128) ? big_endian_read_32(uuid128, 0) : 0;
}


void hint_ce_len(uint16_t interval)
{
    uint16_t ce_len = interval << 1;
    if (ce_len > 20)
        ll_hint_on_ce_len(0, ce_len - 15, ce_len - 15);
}

void reverse(uint8_t* dst, const uint8_t* src, uint32_t size)
{
    for (uint32_t i = 0; i < size; ++i)
        dst[i] = src[size - i - 1];
}

void hex2str(uint8_t* buff, uint32_t size, uint8_t* out_str)
{
    uint8_t msb,lsb;
    uint32_t mi,li,i;
    
    for (i = 0; i < size; ++i)
    {
        mi = 2 * i;
        li = mi + 1;
        
        msb = (buff[i] & 0xF0) >> 4;
        lsb = (buff[i] & 0x0F);
        
        if (msb >= 0x0A) 
            out_str[mi] = 'A' + (msb - 0x0A);
        else 
            out_str[mi] = '0' + (msb);
        
        if (lsb >= 0x0A) 
            out_str[li] = 'A' + (lsb - 0x0A);
        else 
            out_str[li] = '0' + (lsb);
    }
    out_str[i * 2] = 0;
}

void str2hex(char* str, uint32_t str_len, uint8_t* out_buf, uint32_t out_buf_len)
{
    if (str_len % 2 != 0)
        return;
    
    uint32_t limit = str_len / 2;
    if (limit > out_buf_len)
        limit = out_buf_len;
    
    uint8_t msb,lsb;
    uint32_t i = 0;
    uint32_t mi,li;
    while(i < limit)
    {
        msb = lsb = 0;
        mi = i * 2;
        li = mi + 1;
        
        if      ('0' <= str[mi] && str[mi] <= '9') msb = str[mi] - '0';
        else if ('A' <= str[mi] && str[mi] <= 'Z') msb = str[mi] - 'A' + 10;
        else if ('a' <= str[mi] && str[mi] <= 'z') msb = str[mi] - 'a' + 10;
        
        if      ('0' <= str[li] && str[li] <= '9') lsb = str[li] - '0';
        else if ('A' <= str[li] && str[li] <= 'Z') lsb = str[li] - 'A' + 10;
        else if ('a' <= str[li] && str[li] <= 'z') lsb = str[li] - 'a' + 10;
        
        out_buf[i] = (msb << 4) + lsb;
        
        i ++;
    }
}
