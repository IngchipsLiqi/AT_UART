
/*
** COPYRIGHT (c) 2023 by INGCHIPS
*/

#include <stdio.h>
#include <string.h>

#include "ingsoc.h"
#include "platform_api.h"
#include "profile.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "gap.h"
#include "btstack_event.h"

#include "low_power.h"
#include "app.h"

#if defined __cplusplus
    extern "C" {
#endif

static TimerHandle_t enter_low_power_timer = 0;
static uint8_t exit_saving = 0;
uint16_t g_connection_interval = 0;
uint16_t g_sys_work_status_bits = 0;

static void low_power_start_timer(void);
static void low_power_stop_timer(void);
static void low_power_enter_saving(void)
{
    log_printf("[lp]: working 0x%x\r\n", g_sys_work_status_bits);
    if (g_sys_work_status_bits != 0) {
        low_power_start_timer();
        return;
    }

//    exit_saving = 0;
//    platform_config(PLATFORM_CFG_POWER_SAVING, PLATFORM_CFG_ENABLE);
//    low_power_stop_timer();
    return;
}

static void enter_low_power_timer_callback(TimerHandle_t xTimer)
{
    TMR_WatchDogRestart();
    low_power_enter_saving();
    return;
}

static TaskHandle_t low_power_task_handler;
static SemaphoreHandle_t low_power_task_BinarySemaphore;
static void low_power_task(void *pvParameters)
{
    while(1) {
        BaseType_t r = xSemaphoreTake(low_power_task_BinarySemaphore, portMAX_DELAY);
        if (pdTRUE != r) {
            continue;
        }
        log_printf("[lp]: exit\r\n");
        platform_config(PLATFORM_CFG_POWER_SAVING, PLATFORM_CFG_DISABLE);
        low_power_start_timer();
        app_low_power_exit_callback();
    }
}

static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
static uint8_t low_power_timer_is_run = 0;
static void low_power_start_timer(void)
{
    if (low_power_timer_is_run) {
        return;
    }
    xTimerStartFromISR(enter_low_power_timer, &xHigherPriorityTaskWoken);
    low_power_timer_is_run = 1;
    return;
}

static void low_power_stop_timer(void)
{
    if (!low_power_timer_is_run) {
        return;
    }
    xTimerStopFromISR(enter_low_power_timer, &xHigherPriorityTaskWoken);
    low_power_timer_is_run = 0;
    return;
}

void low_power_exit_saving(low_power_exit_reason_t reason)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static low_power_exit_reason_t last_reason = LOW_POWER_EXIT_REASON_MAX;

    if (last_reason != reason) {
        g_sys_work_status_bits = (g_sys_work_status_bits | (0x1 << reason));
        last_reason = reason;
    }

    if (exit_saving) {
        return;
    }
    
    exit_saving = 1;

    app_setup_peripherals();
    xSemaphoreGiveFromISR(low_power_task_BinarySemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return;
}

void low_power_create_timer(void)
{
    enter_low_power_timer = xTimerCreate("enter low power",
                        pdMS_TO_TICKS(1000 * 20),
                        pdTRUE,
                        NULL,
                        enter_low_power_timer_callback);

    low_power_task_BinarySemaphore = xSemaphoreCreateBinary();
    xTaskCreate((TaskFunction_t)low_power_task,
               "key",
               configMINIMAL_STACK_SIZE,
               NULL,
               (configMAX_PRIORITIES - configMAX_PRIORITIES + 5),
               (TaskHandle_t*)&low_power_task_handler);
    return;
}

#if defined __cplusplus
    }
#endif

