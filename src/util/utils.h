#ifndef __UTILS_H__
#define __UTILS_H__

#include "../common/preh.h"

uint32_t get_sig_short_uuid(const uint8_t *uuid128);

void hint_ce_len(uint16_t interval);

void str2hex(char* str, uint32_t str_len, uint8_t* out_buf, uint32_t out_buf_len);

void reverse(uint8_t* dst, uint8_t* src, uint32_t size);

#endif