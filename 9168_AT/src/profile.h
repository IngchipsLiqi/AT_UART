#ifndef __PROFILE_H__
#define __PROFILE_H__

#include "common/preh.h"
#include "gatt_client.h"


enum at_cb_func_idx
{
    AT_GAP_CB_ADV_END,
    AT_GAP_CB_SCAN_END,
    AT_GAP_CB_ADV_RPT,
    AT_GAP_CB_CONN_END,
    AT_GAP_CB_DISCONNECT,
    AT_GAP_CB_MAX,
};
typedef void (*at_cb_func_t)(void *arg);

/*********************************************************************
 * @fn      at_set_gap_cb_func
 *
 * @brief   Fucntion to set at event call back function
 *
 * @param   func_idx - at event idx 
 *       	func 	 - at event call back function 
 *
 * @return  None.
 */
void at_set_gap_cb_func(enum at_cb_func_idx func_idx,at_cb_func_t func);


/*********************************************************************
 * @fn      config_adv_and_set_interval
 *
 * @brief   Fucntion to config adv param
 *
 * @param   intervel    - advertising interval
 *
 * @return  None.
 */
void config_adv_and_set_interval(uint32_t intervel);

/*********************************************************************
 * @fn      start_adv
 *
 * @brief   Fucntion to start adv
 *
 * @param   None.
 *
 * @return  None.
 */
void start_adv(void);

/*********************************************************************
 * @fn      stop_adv
 *
 * @brief   Fucntion to stop adv
 *
 * @param   None.
 *
 * @return  None.
 */
void stop_adv(void);

/*********************************************************************
 * @fn      config_scan
 *
 * @brief   Fucntion to config scan param
 *
 * @param   None.
 *
 * @return  None.
 */
void config_scan(void);

/*********************************************************************
 * @fn      start_continuous_scan
 *
 * @brief   Fucntion to start continuous scan
 *
 * @param   duration    - scan continuous time, unit: 10ms
 *
 * @return  None.
 */
void start_continuous_scan(uint16_t duration);


/*********************************************************************
 * @fn      start_scan
 *
 * @brief   Fucntion to start scan
 *
 * @param   None.
 *
 * @return  None.
 */
void start_scan(void);

/*********************************************************************
 * @fn      stop_scan
 *
 * @brief   Fucntion to stop scan
 *
 * @param   None.
 *
 * @return  None.
 */
void stop_scan(void);

/*********************************************************************
 * @fn      at_slave_encrypted
 *
 * @brief   at event call back function, handle after link is lost
 *
 * @param   arg - point to buff of gap_evt_disconnect_t struct type
 *       
 *
 * @return  None.
 */
void at_cb_disconnected(void *arg);

extern gatt_client_service_t slave_service;
extern gatt_client_characteristic_t slave_input_char;
extern gatt_client_characteristic_t slave_output_char;
extern gatt_client_characteristic_descriptor_t slave_output_desc;
extern gatt_client_notification_t slave_output_notify;

uint32_t setup_profile(void *data, void *user_data);

void process_rx_port_data();

#endif