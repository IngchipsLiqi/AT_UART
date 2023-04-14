#ifndef __PRIVATE_USER_PACKET_HANDLER_H__
#define __PRIVATE_USER_PACKET_HANDLER_H__

#include "common/preh.h"

/**
 * Set callbacks related to protocol stack events.
 * including receive broadcast events, connection events, disconnection events, and so on
 * 
 */
void init_private_user_packet_handler();

void notify_rx_port_has_data();
void notify_slave_port_has_data();
void notify_master_port_has_data();

void send_slave_flow_control_start();
void send_slave_flow_control_over();

void send_master_flow_control_start();
void send_master_flow_control_over();

void start_probe();
void stop_probe();

#endif