#ifndef __TRANSMISSION_CALLBACK_H__
#define __TRANSMISSION_CALLBACK_H__

#include "../common/preh.h"
#include "gatt_client.h"



typedef uint16_t (*pfun_module_att_read_callback)(uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
typedef int (*pfun_module_att_write_callback)(uint16_t transaction_mode, uint16_t offset, const uint8_t *buffer, uint16_t buffer_size);

typedef struct {
    uint16_t cmd;
    pfun_module_att_read_callback fun;
} module_att_read_callback_t;

typedef struct {
    uint16_t cmd;
    pfun_module_att_write_callback fun;
} module_att_write_callback_t;


int module_att_read_callback_register(uint16_t handler, pfun_module_att_read_callback fun);
int module_att_write_callback_register(uint16_t handler, pfun_module_att_write_callback fun);



uint16_t module_handle_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset,
                                  uint8_t * buffer, uint16_t buffer_size);
                                  
int module_handle_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, const uint8_t *buffer, uint16_t buffer_size);

#endif