#include "common/preh.h"
#include <stdio.h>
#include <string.h>
#include "ingsoc.h"
#include "platform_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "string.h"
#include "at_recv_cmd.h"
#include "profile.h"
#include "common/flash_data.h"
#include "router.h"
#include "at/at_parser.h"
#include "util/rtos_util.h"
#include "semphr.h"

#define PIN_WAKEUP GIO_GPIO_0

SemaphoreHandle_t sem_delay = NULL;
uint8_t  buf[3]={1,2,3};


void semaphore_delay(uint32_t millisecond)
{
    xSemaphoreTake(sem_delay, pdMS_TO_TICKS(millisecond));
}

static uint32_t cb_hard_fault(hard_fault_info_t *info, void *_)
{
    platform_printf("HARDFAULT:\nPC : 0x%08X\nLR : 0x%08X\nPSR: 0x%08X\n"
                    "R0 : 0x%08X\nR1 : 0x%08X\nR2 : 0x%08X\nP3 : 0x%08X\n"
                    "R12: 0x%08X\n",
                    info->pc, info->lr, info->psr,
                    info->r0, info->r1, info->r2, info->r3, info->r12);
    for (;;);
}

static uint32_t cb_assertion(assertion_info_t *info, void *_)
{
    platform_printf("[ASSERTION] @ %s:%d\n",
                    info->file_name,
                    info->line_no);
    for (;;);
}

static uint32_t cb_heap_out_of_mem(uint32_t tag, void *_)
{
    platform_printf("[OOM] @ %d\n", tag);
    for (;;);
}

uint32_t cb_lle_init(void *dummy, void *user_data)
{
    (void)(dummy);
    (void)(user_data);
    return 0;
}

uint32_t cb_putc(char *c, void *dummy)
{
    while (apUART_Check_TXFIFO_FULL(PRINT_PORT) == 1);
    UART_SendData(PRINT_PORT, (uint8_t)*c);
    return 0;
}

int fputc(int ch, FILE *f)
{
    cb_putc((char *)&ch, NULL);
    return ch;
}



static uint32_t wdt_isr(void *user_data)
{
    TMR_WatchDogClearInt();
    
    printf("Trigger WatchDog ISR\r\n");
    
    return 0;
}

void config_uart(uint32_t freq, uint32_t baud)
{
    UART_sStateStruct config;

    config.word_length       = UART_WLEN_8_BITS;
    config.parity            = UART_PARITY_NOT_CHECK;
    config.fifo_enable       = 1;
    config.two_stop_bits     = 0;
    config.receive_en        = 0;
    config.transmit_en       = 1;
    config.UART_en           = 1;
    config.cts_en            = 0;
    config.rts_en            = 0;
    config.rxfifo_waterlevel = 1;
    config.txfifo_waterlevel = 1;
    config.ClockFrequency    = freq;
    config.BaudRate          = baud;

    apUART_Initialize(APB_UART0, &config, 0);
}

void setup_peripherals(void)
{

    SYSCTRL_ClearClkGateMulti(  (1 << SYSCTRL_ITEM_APB_PinCtrl)
                              | (1 << SYSCTRL_ITEM_APB_UART0)
                              | (1 << SYSCTRL_ITEM_APB_UART1)
                              | (1 << SYSCTRL_ClkGate_APB_GPIO0)
                              | (1 << SYSCTRL_ITEM_APB_WDT));
    
    config_uart(OSC_CLK_FREQ, 115200);
    at_init();
    
    GIO_EnableRetentionGroupA(0);
    PINCTRL_SetPadMux(PIN_WAKEUP, IO_SOURCE_GPIO);
    GIO_SetDirection((GIO_Index_t)PIN_WAKEUP, GIO_DIR_INPUT);
    PINCTRL_Pull(PIN_WAKEUP, PINCTRL_PULL_DOWN);

    TMR_WatchDogEnable3(WDT_INTTIME_INTERVAL_16S, 200, 1);
    platform_set_irq_callback(PLATFORM_CB_IRQ_WDT, (f_platform_irq_cb)wdt_isr, NULL);
}

uint32_t on_deep_sleep_wakeup(const platform_wakeup_call_info_t *info, void *user_data)
{
    if (PLATFORM_WAKEUP_REASON_NORMAL == info->reason)
    {
        setup_peripherals();
    }
    else
        GIO_EnableRetentionGroupA(0);
    return 1;
}

uint32_t query_deep_sleep_allowed(void *dummy, void *user_data)
{
    (void)(dummy);
    (void)(user_data);
    if (IS_DEBUGGER_ATTACHED())
        return 0;
    GIO_EnableRetentionGroupA(1);
    return PLATFORM_ALLOW_DEEP_SLEEP;
}

static void watchdog_task(void *pdata)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(8000));
        TMR_WatchDogRestart();
    }
}


static void set_reg_bits(volatile uint32_t *reg, uint32_t v, uint8_t bit_width, uint8_t bit_offset)
{
    uint32_t mask = ((1 << bit_width) - 1) << bit_offset;
    *reg = (*reg & ~mask) | (v << bit_offset);
}

static void set_reg_bit(volatile uint32_t *reg, uint8_t v, uint8_t bit_offset)
{
    uint32_t mask = 1 << bit_offset;
    *reg = (*reg & ~mask) | (v << bit_offset);
}

int app_main()
{
    platform_32k_rc_auto_tune();
    sem_delay = xSemaphoreCreateBinary();

    // Load flash data. Data is not lost after power off
    sdk_load_private_flash_data();
    
    // Peripheral initialization
    setup_peripherals();
    
    xTaskCreate(watchdog_task,
           "w",
           configMINIMAL_STACK_SIZE,
           NULL,
           (configMAX_PRIORITIES - 1),
           NULL);
    
    platform_set_evt_callback(PLATFORM_CB_EVT_HARD_FAULT, (f_platform_evt_cb)cb_hard_fault, NULL);
    platform_set_evt_callback(PLATFORM_CB_EVT_ASSERTION, (f_platform_evt_cb)cb_assertion, NULL);
    platform_set_evt_callback(PLATFORM_CB_EVT_HEAP_OOM, (f_platform_evt_cb)cb_heap_out_of_mem, NULL);
    platform_set_evt_callback(PLATFORM_CB_EVT_LLE_INIT, cb_lle_init, NULL);
    platform_set_evt_callback(PLATFORM_CB_EVT_PUTC, (f_platform_evt_cb)cb_putc, NULL);
    platform_set_evt_callback(PLATFORM_CB_EVT_ON_DEEP_SLEEP_WAKEUP, (f_platform_evt_cb)on_deep_sleep_wakeup, NULL);
    platform_set_evt_callback(PLATFORM_CB_EVT_QUERY_DEEP_SLEEP_ALLOWED, (f_platform_evt_cb)query_deep_sleep_allowed, NULL);
    
    // Set the callback for the protocol stack to be prepared
    platform_set_evt_callback(PLATFORM_CB_EVT_PROFILE_INIT, setup_profile, NULL);
    
    
    platform_config(PLATFORM_CFG_RTOS_ENH_TICK, PLATFORM_CFG_ENABLE);
    LOG_MSG("MAIN_OK\r\n");
    
    
    // Config wakeup source
    GIO_EnableDeepSleepWakeupSource(PIN_WAKEUP, 1, 1, PINCTRL_PULL_DOWN);
    //GIO_EnableDeepSleepWakeupSource(g_power_off_save_data_in_ram.default_info.wakeup_source, 1, 1, PINCTRL_PULL_DOWN);
    if (g_power_off_save_data_in_ram.default_info.auto_sleep) {
        platform_config(PLATFORM_CFG_POWER_SAVING, PLATFORM_CFG_ENABLE);
    }
    return 0;
}

