#ifndef __PROFILE_H__
#define __PROFILE_H__

#include "common/preh.h"
#include "gatt_client.h"

extern gatt_client_service_t slave_service;
extern gatt_client_characteristic_t slave_input_char;
extern gatt_client_characteristic_t slave_output_char;
extern gatt_client_characteristic_descriptor_t slave_output_desc;
extern gatt_client_notification_t slave_output_notify;

uint32_t setup_profile(void *data, void *user_data);

void process_rx_port_data();

#endif