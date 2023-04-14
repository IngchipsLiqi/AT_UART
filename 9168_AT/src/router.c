#include "router.h"

#include "gatt_client.h"
#include "btstack_defines.h"
#include "att_dispatch.h"
#include "att_db.h"

#include "private_user_packet_handler.h"
#include "btstack_sync.h"
#include "FreeRTOS.h"
#include "task.h"


//==============================================================================================================
//* GATT
//==============================================================================================================
extern hci_con_handle_t master_connect_handle;
extern hci_con_handle_t slave_connect_handle;
extern gatt_client_service_t slave_service;
extern gatt_client_characteristic_t slave_input_char;
extern gatt_client_characteristic_t slave_output_char;
extern uint8_t notify_enable;
extern uint16_t g_ble_input_handle;
extern uint16_t g_ble_output_handle;
extern uint16_t g_ble_output_desc_handle;
//==============================================================================================================

extern private_flash_data_t g_power_off_save_data_in_ram;

extern uint32_t rx_sum;
extern uint32_t tx_sum;
extern uint32_t receive_slave_sum;
extern uint32_t receive_master_sum;
extern uint32_t send_to_slave_sum;
extern uint32_t send_to_master_sum;

volatile uint8_t send_to_slave_data_to_be_continued = 0;
volatile uint8_t send_to_master_data_to_be_continued = 0;

uint8_t temp_buf[1000] = { 0 };
circular_queue_t* buffer = NULL;

void receive_rx_data() 
{
    uint8_t c;
    while (apUART_Check_RXFIFO_EMPTY(APB_UART1) != 1) {
        if (circular_queue_get_elem_num(buffer) >= BUFFER_THRESHOLD_TOP)
            break;
        
        c = APB_UART1->DataRead;
        rx_sum++;
        circular_queue_enqueue(buffer, (uint8_t*)&c);
    }
}

void send_data_to_ble_slave()
{
    LOG_MSG("into send_to_slave\r\n");
    
    uint16_t send_packet_len = 0;
    uint32_t r, valid_data = 0;
    
    gatt_client_get_mtu(slave_connect_handle, &send_packet_len);
    send_packet_len -= 3;
    
    uint32_t is_complete = 1;
    while ((valid_data = circular_queue_get_elem_num(buffer)) > 0) {
        
        if (send_packet_len > valid_data)
            send_packet_len = valid_data;
        
        circular_queue_read_batch(buffer, temp_buf, 0, send_packet_len);
        
        r = gatt_client_write_value_of_characteristic_without_response(slave_connect_handle, 
                                                                       slave_input_char.value_handle, 
                                                                       send_packet_len, temp_buf);
        
        if (r == 0) {
            circular_queue_discard_batch(buffer, send_packet_len);
            send_to_slave_sum += send_packet_len;
        } else if (r == BTSTACK_ACL_BUFFERS_FULL) {
            LOG_MSG("request_can_send_now_event\r\n");
            att_dispatch_client_request_can_send_now_event(slave_connect_handle); // Request can send now
            is_complete = 0;
            break;
        } else {
            LOG_MSG("Unknown r:%d\r\n", r);
            break;
        }
    }
    if (is_complete) {
        send_data_to_ble_slave_over();
    }
}



void send_data_to_ble_master() 
{
    if (0 == notify_enable)
    {
        LOG_MSG("discard\r\n");
        circular_queue_discard_batch(buffer, circular_queue_get_elem_num(buffer));
        send_data_to_ble_master_over();
        return;
    }
    
    uint16_t len = att_server_get_mtu(master_connect_handle) - 3;
    
    uint32_t is_complete = 1;
    uint32_t valid_data = 0;
    while ((valid_data = circular_queue_get_elem_num(buffer)) > 0) {
        
        if (len > valid_data)
            len = valid_data;
        
        if (att_server_can_send_packet_now(master_connect_handle))
        {
            circular_queue_dequeue_batch(buffer, temp_buf, len);
            
            att_server_notify(master_connect_handle, g_ble_output_handle, temp_buf, len);
            
            send_to_master_sum += len;
        }
        else
        {
            LOG_MSG("request can send now\r\n");
            att_server_request_can_send_now_event(master_connect_handle); // Request can send now
            is_complete = 0;
            break;
        }
    }
    if (is_complete) {
        send_data_to_ble_master_over();
    }
}




void send_data_to_ble_slave_start()
{
    if (send_to_slave_data_to_be_continued == 1)
        return;
    
    LOG_MSG("send_data_to_slave_start\r\n");
    send_to_slave_data_to_be_continued = 1;
    
    
    btstack_push_user_msg(USER_MSG_SEND_DATA_TO_BLE_SLAVE, 0, 0);
}
void send_data_to_ble_slave_over()
{
    LOG_MSG("send_data_to_slave_over\r\n");
    send_to_slave_data_to_be_continued = 0;
}

void send_data_to_ble_master_start()
{
    if (send_to_master_data_to_be_continued == 1)
        return;
    
    LOG_MSG("send_data_to_master_start\r\n");
    send_to_master_data_to_be_continued = 1;
    
    btstack_push_user_msg(USER_MSG_SEND_DATA_TO_BLE_MASTER, 0, 0);
}
void send_data_to_ble_master_over()
{
    LOG_MSG("send_data_to_master_over\r\n");
    send_to_master_data_to_be_continued = 0;
}

void init_rx_buffer()
{
    buffer = circular_queue_create(RX_INPUT_BUFFER_CAPACITY, 1);
    if (buffer == NULL)
        LOG_MSG("Failed to create rx data buffer.\r\n");
}
