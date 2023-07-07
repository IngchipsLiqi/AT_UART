
/*
** COPYRIGHT (c) 2023 by INGCHIPS
*/

#include "stdint.h"
#include "string.h"

#include "app.h"
#include "ingsoc.h"
#include "platform_api.h"

#include "project_common.h"
#include "sdk_private_flash_data.h"
#include "bt_cmd_data_uart_io_adp.h"

#if defined __cplusplus
    extern "C" {
#endif

static void sdk_config_uart(UART_TypeDef* port, uint32_t freq, uint32_t baud, uint32_t int_en)
{
    UART_sStateStruct config;

    config.word_length       = UART_WLEN_8_BITS;
    config.parity            = UART_PARITY_NOT_CHECK;
    config.fifo_enable       = 1;
    config.two_stop_bits     = 0;
    config.receive_en        = 1;
    config.transmit_en       = 1;
    config.UART_en           = 1;
    config.cts_en            = 0;
    config.rts_en            = 0;
    config.rxfifo_waterlevel = 28;
    config.txfifo_waterlevel = 1;
    config.ClockFrequency    = freq;
    config.BaudRate          = baud;

    apUART_Initialize(port, &config, int_en);

    return;
}

static uint32_t bt_cmd_data_uart1_isr(void *user_data)
{
    uint32_t status;

    while(1)
    {
        status = apUART_Get_all_raw_int_stat(COM_PORT);
        if (status == 0)
            break;

        COM_PORT->IntClear = status;

        // rx int
        if (status & (1 << bsUART_RECEIVE_INTENAB))
        {
            bt_cmd_data_uart_recv_data(UART_READ_TRIG_SOURCE_ISR);
        }
    }
    return 0;
}

static void app_setup_uart1_isr(void)
{
    platform_set_irq_callback(PLATFORM_CB_IRQ_UART1, bt_cmd_data_uart1_isr, NULL);
    return;
}

#define CHANNEL_ID  0
#define DMA_UART    APB_UART1
#define SIZE 8

uint8_t dma_dst[24]  = {0};

DMA_Descriptor test __attribute__((aligned (8)));


static uint32_t DMA_cb_isr(void *user_data)
{
	dbg_printf("11111111_00000010  \r\n");
    uint32_t state = DMA_GetChannelIntState(CHANNEL_ID);
    DMA_ClearChannelIntState(CHANNEL_ID, state);
	#if 0
	for(uint8_t i = 0; i < 24; i++)
	{
		printf("dma_dst[%d] = %c\n", i,dma_dst[i]);
	}
	#else
	dbg_printf("%c\n", dma_dst[0]);
	#endif
	DMA_EnableChannel(CHANNEL_ID, &test);
    return 0;
}

static void app_setup_uart1_dma(void)
{
    SYSCTRL_ClearClkGateMulti(1 << SYSCTRL_ClkGate_APB_DMA);
    DMA_Reset(1);
    DMA_Reset(0);

    SYSCTRL_SelectUsedDmaItems(1 << SYSCTRL_DMA_UART0_RX);

    platform_set_irq_callback(PLATFORM_CB_IRQ_DMA, DMA_cb_isr, 0);
    test.Next = (DMA_Descriptor *)0;
    DMA_PreparePeripheral2Mem(  &test,
                                dma_dst,
                                SYSCTRL_DMA_UART0_RX,
                                1, DMA_ADDRESS_INC, 0);

    dbg_printf("78  \n");
    //test.Ctrl = 0;
    //DMA_Reset(0);

    dbg_printf("89  \n");
    DMA_EnableChannel(CHANNEL_ID, &test);
    dbg_printf("90  \n");
    UART_DmaEnable(APB_UART0, 1, 1, 0);
    return;
}

#define USER_UART1_IO_TX GIO_GPIO_9
#define USER_UART1_IO_RX GIO_GPIO_8
static void sdk_config_uart1_com(void)
{
    SYSCTRL_ClearClkGateMulti((1 << SYSCTRL_ClkGate_APB_UART1) | (1 << SYSCTRL_ClkGate_APB_PinCtrl));

    PINCTRL_SetPadMux(USER_UART1_IO_TX, IO_SOURCE_UART1_TXD);
    PINCTRL_SetPadMux(USER_UART1_IO_RX, IO_SOURCE_GENERAL);
    PINCTRL_SelUartRxdIn(UART_PORT_1, USER_UART1_IO_RX);
    PINCTRL_Pull(USER_UART1_IO_RX, PINCTRL_PULL_UP);

    sdk_config_uart(COM_PORT, OSC_CLK_FREQ, 115200, 1 << bsUART_RECEIVE_INTENAB);

    //app_setup_uart1_dma();
    app_setup_uart1_isr();

    return;
}

// api

void app_setup_peripherals(void)
{
    uint8_t start_char[] = {'A', 'T'};
    SYSCTRL_ClearClkGateMulti(1 << SYSCTRL_ClkGate_APB_WDT);
    sdk_config_uart1_com();

    GIO_EnableRetentionGroupB(0);
    GIO_EnableRetentionGroupA(0);

    bt_cmd_data_uart_out(start_char, sizeof(start_char));
    return;
}

void app_low_power_exit_callback(void)
{
    timer_start_main_cmd_data();
    return;
}

void app_setup_peripherals_before_sleep(void)
{
    PINCTRL_SetPadMux(GIO_GPIO_35, IO_SOURCE_GPIO);
    PINCTRL_Pull(GIO_GPIO_35, PINCTRL_PULL_UP);
    GIO_SetDirection(GIO_GPIO_35, GIO_DIR_INPUT);
    GIO_ConfigIntSource(GIO_GPIO_35, GIO_INT_EN_LOGIC_LOW_OR_FALLING_EDGE, GIO_INT_EDGE);
    GIO_EnableDeepSleepWakeupSource(GIO_GPIO_35, 1, 0, PINCTRL_PULL_UP);

    PINCTRL_SetPadMux(USER_UART1_IO_RX, IO_SOURCE_GPIO);
    PINCTRL_Pull(USER_UART1_IO_RX, PINCTRL_PULL_UP);
    GIO_SetDirection(USER_UART1_IO_RX, GIO_DIR_INPUT);
    GIO_ConfigIntSource(USER_UART1_IO_RX, GIO_INT_EN_LOGIC_LOW_OR_FALLING_EDGE, GIO_INT_EDGE);
    GIO_EnableDeepSleepWakeupSource(USER_UART1_IO_RX, 1, 0, PINCTRL_PULL_UP);

    GIO_SetDirection(USER_UART1_IO_TX, GIO_DIR_OUTPUT);
    GIO_WriteValue(USER_UART1_IO_TX, 1);
    PINCTRL_Pull(USER_UART1_IO_TX, PINCTRL_PULL_DISABLE);
    PINCTRL_SetPadMux(USER_UART1_IO_TX, IO_SOURCE_GPIO);
    return;
}


static void setup_pll_clk_and_sleep_para(void)
{
#define PLL_EN     1
#define PLL_LOOP   70
#define HCLK_DIV   SYSCTRL_CLK_PLL_DIV_5
#define FLASH_DIV  SYSCTRL_CLK_PLL_DIV_2
#define TIME_REDUCTION 4500
#define LL_DELAY       700
#define LATENCY_PRE    10
    platform_config(PLATFORM_CFG_DEEP_SLEEP_TIME_REDUCTION, TIME_REDUCTION);
    platform_config(PLATFORM_CFG_LL_DELAY_COMPENSATION, LL_DELAY);
//   platform_config(PLATFORM_CFG_DEEP_SLEEP_TIME_REDUCTION, 5200);
//   platform_config(PLATFORM_CFG_LL_DELAY_COMPENSATION, 280);
    ll_config(LL_CFG_SLAVE_LATENCY_PRE_WAKE_UP, LATENCY_PRE);

    SYSCTRL_EnableConfigClocksAfterWakeup(PLL_EN, PLL_LOOP, HCLK_DIV, FLASH_DIV, 0);

    SYSCTRL_EnableSlowRC(0, SYSCTRL_SLOW_RC_24M);

    return;
}

static void setup_rf_clk_and_sleep_para(void)
{
    SYSCTRL_SelectSlowClk(SYSCTRL_SLOW_CLK_24M_RF);
    SYSCTRL_SelectFlashClk(SYSCTRL_CLK_SLOW);
    SYSCTRL_SelectHClk(SYSCTRL_CLK_SLOW);
    SYSCTRL_EnablePLL(0);
    SYSCTRL_EnableSlowRC(0, SYSCTRL_SLOW_RC_24M);

    platform_config(PLATFORM_CFG_DEEP_SLEEP_TIME_REDUCTION, 6250);
    platform_config(PLATFORM_CFG_LL_DELAY_COMPENSATION, 1500);

    SYSCTRL_EnableConfigClocksAfterWakeup(0, PLL_BOOT_DEF_LOOP, 0, 0, 0);
}

static void print_dev_info(void)
{
    if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_MASTER) {
        log_printf("[info]: i'm master, peer addr:%02x_%02x_%02x_%02x_%02x_%02x",
                   g_power_off_save_data_in_ram.peer_mac_address[0],
                   g_power_off_save_data_in_ram.peer_mac_address[1],
                   g_power_off_save_data_in_ram.peer_mac_address[2],
                   g_power_off_save_data_in_ram.peer_mac_address[3],
                   g_power_off_save_data_in_ram.peer_mac_address[4],
                   g_power_off_save_data_in_ram.peer_mac_address[5]);
    } else if (g_power_off_save_data_in_ram.dev_type == BLE_DEV_TYPE_SLAVE) {
        log_printf("[info]: i'm slave, my addr:%02x_%02x_%02x_%02x_%02x_%02x",
                   g_power_off_save_data_in_ram.module_mac_address[0],
                   g_power_off_save_data_in_ram.module_mac_address[1],
                   g_power_off_save_data_in_ram.module_mac_address[2],
                   g_power_off_save_data_in_ram.module_mac_address[3],
                   g_power_off_save_data_in_ram.module_mac_address[4],
                   g_power_off_save_data_in_ram.module_mac_address[5]);
    } else {
        log_printf("[info]: data error:%d\r\n", g_power_off_save_data_in_ram.dev_type);
    }
    return;
}

void app_start(void)
{
    app_setup_peripherals();
    bt_cmd_data_process_init();

    platform_config(PLATFORM_CFG_POWER_SAVING, PLATFORM_CFG_ENABLE);

    setup_pll_clk_and_sleep_para();
    //SYSCTRL_SelectMemoryBlocks(SYSCTRL_RESERVED_MEM_BLOCKS);

    print_dev_info();

    return;
}

void print_addr(uint8_t *addr)
{
    int i;
    log_printf("[app]: addr");
    for (i = 0; i < 6; i++) {
        log_printf(" %02x", addr[i]);
    }
    log_printf("\r\n");
    return;
}

void dump_ram_data_in_char(uint8_t *p_data, uint16_t data_len)
{
    int i;
    dbg_printf("[app]: dump:");
#if (DEBUG == 0)
#else
    for (i = 0; i < data_len; i++) {
        cb_putc((char *)&p_data[i], 0);
    }
#endif
    dbg_printf("\r\n");
    return;
}

//#define bsDMA_INT_C_MASK         1
//#define bsDMA_DST_REQ_SEL        4
//#define bsDMA_SRC_REQ_SEL        8
//#define bsDMA_DST_ADDR_CTRL      12
//#define bsDMA_SRC_ADDR_CTRL      14
//#define bsDMA_DST_MODE           16
//#define bsDMA_SRC_MODE           17
//#define bsDMA_DST_WIDTH          18
//#define bsDMA_SRC_WIDTH          21
//#define bsDMA_SRC_BURST_SIZE     24

//#define DMA_RX_CHANNEL_ID  1
//#define DMA_TX_CHANNEL_ID  0

//DMA_Descriptor test __attribute__((aligned (8)));
//char dst[256];
//char src[] = "Finished to receive a frame!\n";

//void setup_peripheral_dma()
//{
//    printf("setup_peripheral_dma\n");
//    SYSCTRL_ClearClkGate(SYSCTRL_ITEM_APB_DMA);

//    //??DMA??
//    APB_DMA->Channels[1].Descriptor.Ctrl = ((uint32_t)0x0 << bsDMA_INT_C_MASK)
//                                            | ((uint32_t)0x0 <<bsDMA_DST_REQ_SEL)
//                                            | ((uint32_t)0x0 << bsDMA_SRC_REQ_SEL)
//                                            | ((uint32_t)0x0 << bsDMA_DST_ADDR_CTRL)
//                                            | ((uint32_t)0x2 << bsDMA_SRC_ADDR_CTRL)   //DMA_ADDRESS_FIXED
//                                            | ((uint32_t)0x0 << bsDMA_DST_MODE)
//                                            | ((uint32_t)0x1 << bsDMA_SRC_MODE)     //
//                                            | ((uint32_t)0x0 << bsDMA_DST_WIDTH)
//                                            | ((uint32_t)0x0 << bsDMA_SRC_WIDTH)
//                                            | ((uint32_t)0x2 << bsDMA_SRC_BURST_SIZE); //4 transefers

//    APB_DMA->Channels[1].Descriptor.SrcAddr = (uint32_t)&APB_UART0->DataRead;
//    APB_DMA->Channels[1].Descriptor.DstAddr = (uint32_t)dst;
//    APB_DMA->Channels[1].Descriptor.TranSize = 48;

//    DMA_EnableChannel(1, &APB_DMA->Channels[1].Descriptor);
//}

////??UART??DMA?????
//void UART_trigger_DmaSend(void)
//{
//    DMA_PrepareMem2Peripheral(  &test,
//                                SYSCTRL_DMA_UART0_TX,
//                                src, strlen(src),
//                                DMA_ADDRESS_INC, 0);
//    DMA_EnableChannel(DMA_TX_CHANNEL_ID, &test);
//}

//void setup_peripheral_uart()
//{
//    APB_UART0->FifoSelect = (0 << bsUART_TRANS_INT_LEVEL ) |
//                    (0X7 << bsUART_RECV_INT_LEVEL  ) ;
//    APB_UART0->IntMask = (0 << bsUART_RECEIVE_INTENAB) | (0 << bsUART_TRANSMIT_INTENAB) |
//                        (1 << bsUART_TIMEOUT_INTENAB);
//    APB_UART0->Control = 1 << bsUART_RECEIVE_ENABLE |
//                1 << bsUART_TRANSMIT_ENABLE |
//                1 << bsUART_ENABLE          |
//                0 << bsUART_CTS_ENA         |
//                0 << bsUART_RTS_ENA;
//}

//uint32_t uart_isr(void *user_data)
//{
//    uint32_t status;
//    printf("@%x #%x #%x\n",APB_UART0->IntMask,APB_UART0->IntRaw,APB_UART0->Interrupt);

//    while(1)
//    {
//        status = apUART_Get_all_raw_int_stat(APB_UART0);

//        if (status == 0)
//            break;

//        APB_UART0->IntClear = status;

//        // rx timeout_int
//        if (status & (1 << bsUART_TIMEOUT_INTENAB))
//        {
//            while (apUART_Check_RXFIFO_EMPTY(APB_UART0) != 1)
//            {
//                char c = APB_UART0->DataRead;
//                int index = APB_DMA->Channels[1].Descriptor.DstAddr - (uint32_t)dst;
//                dst[index] = c;
//                if (index == 255)
//                {
//                    APB_DMA->Channels[1].Descriptor.DstAddr = (uint32_t)dst;
//                    UART_trigger_DmaSend();
//                }
//                else
//                    APB_DMA->Channels[1].Descriptor.DstAddr++;

//                APB_DMA->Channels[1].Descriptor.TranSize = 48;
//            }
//            printf("\nlen=%d, dst = %s\n",APB_DMA->Channels[1].Descriptor.TranSize,dst);
//        }
//    }
//    return 0;
//}

//void uart_peripherals_read_data()
//{
//    //??uart0??
//    platform_set_irq_callback(PLATFORM_CB_IRQ_UART0, uart_isr, NULL);
//    setup_peripheral_uart();
//    setup_peripheral_dma();
//}

#if defined __cplusplus
    }
#endif

