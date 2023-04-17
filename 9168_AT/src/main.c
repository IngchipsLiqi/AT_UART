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
                              | (1 << SYSCTRL_ITEM_APB_WDT));
    
    config_uart(OSC_CLK_FREQ, 115200);
    at_init();

    TMR_WatchDogEnable3(WDT_INTTIME_INTERVAL_16S, 200, 1);
    platform_set_irq_callback(PLATFORM_CB_IRQ_WDT, (f_platform_irq_cb)wdt_isr, NULL);

}

static void watchdog_task(void *pdata)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(8000));
        TMR_WatchDogRestart();
    }
}


int app_main()
{
    platform_32k_rc_auto_tune();
    sem_delay = xSemaphoreCreateBinary();
    
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
    
    // Set the callback for the protocol stack to be prepared
    platform_set_evt_callback(PLATFORM_CB_EVT_PROFILE_INIT, setup_profile, NULL);

    // Load flash data. Data is not lost after power off
    sdk_load_private_flash_data();
    
    // init rx buffer
    //init_rx_buffer();
    
    //init at processor
    //init_at_processor();
    
    platform_config(PLATFORM_CFG_RTOS_ENH_TICK, PLATFORM_CFG_ENABLE);
    platform_printf("MAIN_OK\r\n");
    
    return 0;
}

