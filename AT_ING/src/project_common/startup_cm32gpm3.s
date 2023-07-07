;/**************************************************************************//**
; * @file     startup_ARMCM3.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM3 Device Series
; * @version  V1.08
; * @date     03. February 2012
; *
; * @note
; * Copyright (C) 2012 ARM Limited. All rights reserved.
; *
; * @par
; * ARM Limited (ARM) is supplying this software for use with Cortex-M
; * processor based microcontrollers.  This file can be freely distributed
; * within development tools that are supporting such ARM based processors.
; *
; * @par
; * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
; * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; *
; ******************************************************************************/
;/*
;//-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
;*/


; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00000400

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Heap_Size       EQU     0x00000100

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit


                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     DebugMon_Handler          ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

                ; External Interrupts
                DCD     n00_BB0_IRQHandler_ERR
                DCD     n01_BB1_IRQHandler_LLE
                DCD     n02_RTC_M0_IRQHandler
                DCD     n03_RTC_M1_IRQHandler
                DCD     n04_TMR0_IRQHandler
                DCD     n05_TMR1_IRQHandler
                DCD     n06_TMR2_IRQHandler
                DCD     n07_EWK_IRQHandler
                DCD     n08_EXINT_IRQHandler
                DCD     n09_GPIO_IRQHandler
                DCD     n10_BB0_IRQHandler
                DCD     n11_BB1_IRQHandler
                DCD     n12_DMA_IRQHandler
                DCD     n13_TMR1_IRQHandler
                DCD     n14_SPI0_IRQHandler
                DCD     n15_SPI1_IRQHandler
                DCD     n16_URT0_IRQHandler
                DCD     n17_URT1_IRQHandler
                DCD     n18_I2C_IRQHandler
                DCD     n19_DMA_IRQHandler
                DCD     n20_BB0_IRQHandler
                DCD     n21_BB1_IRQHandler
                DCD     n22_TMR2_IRQHandler
                DCD     n23_SPI1_IRQHandler
                DCD     n24_EXINT_IRQHandler
                DCD     n25_GPIO_IRQHandler
                DCD     n26_I2C_IRQHandler
                DCD     n27_URT0_IRQHandler
                DCD     n28_URT1_IRQHandler





__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors


                AREA    |.text|, CODE, READONLY



; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  __main

    IF :DEF:POWER_SAVING
                IMPORT sysStateBackup

                LDR  R0, =0x40040164    ; reserved RTC reg
                LDR  R1, [R0]
                TST  R1, #1
                BEQ  START_LOAD         ; not deep sleep, continue load

                ; clear deep sleep flag
                MOV  R2, #1
                BIC  R1, R2
                STR  R1, [R0]

                LDR  R0, =sysStateBackup
                LDR  R0, [R0]
                BX   R0

    ENDIF


START_LOAD      LDR     R0, =__main
                BX      R0
                ENDP



; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler          [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP



Default_Handler PROC
                EXPORT n00_BB0_IRQHandler_ERR   [WEAK]
                EXPORT n01_BB1_IRQHandler_LLE   [WEAK]
                EXPORT n02_RTC_M0_IRQHandler    [WEAK]
                EXPORT n03_RTC_M1_IRQHandler    [WEAK]
                EXPORT n04_TMR0_IRQHandler      [WEAK]
                EXPORT n05_TMR1_IRQHandler      [WEAK]
                EXPORT n06_TMR2_IRQHandler      [WEAK]
                EXPORT n07_EWK_IRQHandler       [WEAK]
                EXPORT n08_EXINT_IRQHandler     [WEAK]
                EXPORT n09_GPIO_IRQHandler      [WEAK]
                EXPORT n10_BB0_IRQHandler       [WEAK]
                EXPORT n11_BB1_IRQHandler       [WEAK]
                EXPORT n12_DMA_IRQHandler       [WEAK]
                EXPORT n13_TMR1_IRQHandler      [WEAK]
                EXPORT n14_SPI0_IRQHandler      [WEAK]
                EXPORT n15_SPI1_IRQHandler      [WEAK]
                EXPORT n16_URT0_IRQHandler      [WEAK]
                EXPORT n17_URT1_IRQHandler      [WEAK]
                EXPORT n18_I2C_IRQHandler       [WEAK]
                EXPORT n19_DMA_IRQHandler       [WEAK]
                EXPORT n20_BB0_IRQHandler       [WEAK]
                EXPORT n21_BB1_IRQHandler       [WEAK]
                EXPORT n22_TMR2_IRQHandler      [WEAK]
                EXPORT n23_SPI1_IRQHandler      [WEAK]
                EXPORT n24_EXINT_IRQHandler     [WEAK]
                EXPORT n25_GPIO_IRQHandler      [WEAK]
                EXPORT n26_I2C_IRQHandler       [WEAK]
                EXPORT n27_URT0_IRQHandler      [WEAK]
                EXPORT n28_URT1_IRQHandler      [WEAK]

n00_BB0_IRQHandler_ERR
n01_BB1_IRQHandler_LLE
n02_RTC_M0_IRQHandler
n03_RTC_M1_IRQHandler
n04_TMR0_IRQHandler
n05_TMR1_IRQHandler
n06_TMR2_IRQHandler
n07_EWK_IRQHandler
n08_EXINT_IRQHandler
n09_GPIO_IRQHandler
n10_BB0_IRQHandler
n11_BB1_IRQHandler
n12_DMA_IRQHandler
n13_TMR1_IRQHandler
n14_SPI0_IRQHandler
n15_SPI1_IRQHandler
n16_URT0_IRQHandler
n17_URT1_IRQHandler
n18_I2C_IRQHandler
n19_DMA_IRQHandler
n20_BB0_IRQHandler
n21_BB1_IRQHandler
n22_TMR2_IRQHandler
n23_SPI1_IRQHandler
n24_EXINT_IRQHandler
n25_GPIO_IRQHandler
n26_I2C_IRQHandler
n27_URT0_IRQHandler
n28_URT1_IRQHandler


                B       .
                ENDP


                ALIGN


; User Initial Stack & Heap

                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap

__user_initial_stackheap
                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR
                ENDP

                ALIGN

                ENDIF


                END
