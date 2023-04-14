#ifndef __AT_PARSER_H__
#define __AT_PARSER_H__

#include "../common/preh.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "../util/buffer.h"

#define CMD_LIST_CAPACITY 30
#define CMD_MAX_SIZE 30
#define DEVICE_LIST_CAPACITY 10

uint32_t at_contains_device(uint8_t* le_mac_addr);
void at_processor_add_scan_device(uint8_t* le_mac_addr);

void init_at_processor();
void at_processor_start(uint8_t* buf, uint32_t size);

#endif