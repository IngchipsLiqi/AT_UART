#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "common/preh.h"

#include "common/flash_data.h"
#include "util/circular_queue.h"

void init_rx_buffer();

void receive_rx_data();
void send_data_to_tx();


void receive_ble_master_data(const uint8_t* buffer, uint32_t size);

void send_data_to_ble_master();
void send_data_to_ble_master_start();
void send_data_to_ble_master_over();



void receive_ble_slave_data(const uint8_t* buffer, uint16_t size);

void send_data_to_ble_slave();
void send_data_to_ble_slave_start();
void send_data_to_ble_slave_over();

#endif