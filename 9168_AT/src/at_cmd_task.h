#ifndef _AT_CMD_TASK_H_
#define _AT_CMD_TASK_H_


#include <stdint.h>
#include <stdbool.h>

#define IDLE_ROLE    (0)
#define SLAVE_ROLE   (1<<0)
#define MASTER_ROLE  (1<<1)
#define BLE_CONNECTION_MAX (8)

struct at_ctrl
{
    bool adv_ongoing;   //record adv status
    bool scan_ongoing;  //record scan status
    bool initialization_ongoing;    //record init status
    bool upgrade_start;
    bool async_evt_on_going;  //async event is on going, must wait util it end
    bool transparent_start;  //transparent flag
    bool one_slot_send_start;  //
    uint8_t curr_adv_int;  //current adv interval
    uint8_t transparent_conidx;  //
    uint32_t one_slot_send_len;  //
    uint16_t scan_duration; //uint: 10ms
};
struct recv_cmd_t
{
    uint16_t recv_length;
    uint8_t recv_data[];
};

struct at_buff_env
{
    uint16_t flash_page_idx;
    uint16_t flash_write_cnt;


 
};

extern struct at_ctrl gAT_ctrl_env;
void at_recv_cmd_handler(struct recv_cmd_t *param);

#endif
