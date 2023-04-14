#ifndef __TRANSMISSION_SERVICE_H__
#define __TRANSMISSION_SERVICE_H__

#include "../common/preh.h"

#include "att_db.h"
#include "att_db_util.h"

#define GATT_CHARACTERISTIC_MAX_DATA_LEN 20

#define TRANSMISSION_SERVICE_UUID_16 0xFF00
#define TRANSMISSION_CHARACTER_INPUT_UUID_16 0xFF01
#define TRANSMISSION_CHARACTER_OUPUT_UUID_16 0xFF02
#define TRANSMISSION_CHARACTER_FLOW_CTRL_UUID_16 0xFF03
#ifndef SIG_UUID_DESCRIP_GATT_CLIENT_CHARACTERISTIC_CONFIGURATION
#define SIG_UUID_DESCRIP_GATT_CLIENT_CHARACTERISTIC_CONFIGURATION 0x2902
#endif


void init_service(void);


int att_write_output_desc_callback(uint16_t offset, const uint8_t *buffer, uint16_t buffer_size);
int att_write_input_callback(uint16_t offset, const uint8_t *buffer, uint16_t buffer_size);


#endif

