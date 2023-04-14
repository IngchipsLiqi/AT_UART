#include "transmission_callback.h"

#define GATT_HANDLE_MAX_NUM 20


static module_att_read_callback_t module_att_read_callback[GATT_HANDLE_MAX_NUM] = { 0 };
static int module_att_read_callback_num = 0;

static module_att_write_callback_t module_att_write_callback[GATT_HANDLE_MAX_NUM] = { 0 };
static int module_att_write_callback_num = 0;


int module_att_read_callback_register(uint16_t cmd, pfun_module_att_read_callback fun)
{
    if (module_att_read_callback_num >= GATT_HANDLE_MAX_NUM) {
        LOG_MSG("register fail!\r\n");
        return BT_PRIVT_ERROR;
    }
    
    for (uint32_t i = 0; i < module_att_read_callback_num; ++i) {
        if (module_att_read_callback[i].cmd == cmd) {
            module_att_read_callback[i].fun = fun;
            return BT_PRIVT_OK;
        }
    }
    
    module_att_read_callback[module_att_read_callback_num].cmd = cmd;
    module_att_read_callback[module_att_read_callback_num].fun = fun;
    module_att_read_callback_num++;
    
    return BT_PRIVT_OK;
}

int module_att_write_callback_register(uint16_t cmd, pfun_module_att_write_callback fun)
{
    if (module_att_write_callback_num >= GATT_HANDLE_MAX_NUM) {
        LOG_MSG("register fail!\r\n");
        return BT_PRIVT_ERROR;
    }
    
    for (uint32_t i = 0; i < module_att_write_callback_num; ++i) {
        if (module_att_write_callback[i].cmd == cmd) {
            module_att_write_callback[i].fun = fun;
            return BT_PRIVT_OK;
        }
    }
    
    module_att_write_callback[module_att_write_callback_num].cmd = cmd;
    module_att_write_callback[module_att_write_callback_num].fun = fun;
    module_att_write_callback_num++;
    
    return BT_PRIVT_OK;
}




uint16_t module_handle_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset,
                                  uint8_t * buffer, uint16_t buffer_size)
{
    for (uint32_t i = 0; i < module_att_read_callback_num; ++i) {
        if (module_att_read_callback[i].cmd == att_handle) {
            module_att_read_callback[i].fun(offset, buffer, buffer_size);
        }
    }
    return BT_PRIVT_OK;
}
                                  
int module_handle_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                              uint16_t offset, const uint8_t *buffer, uint16_t buffer_size)
{
    for (uint32_t i = 0; i < module_att_write_callback_num; ++i) {
        if (module_att_write_callback[i].cmd == att_handle) {
            module_att_write_callback[i].fun(offset, buffer, buffer_size);
        }
    }
    return BT_PRIVT_OK;
}

