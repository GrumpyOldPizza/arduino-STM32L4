/*
 * Copyright (c) 2008-2017 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVINCED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

        .arch armv7-m
        .syntax unified

        .section .stack
        .align  3
#ifdef __STACK_SIZE
        .equ    Stack_Size, __STACK_SIZE
#else
        .equ    Stack_Size, 0x00000400
#endif
        .globl  __StackTop
        .globl  __StackLimit
__StackLimit:
        .space  Stack_Size
        .size   __StackLimit, . - __StackLimit
__StackTop:
        .size   __StackTop, . - __StackTop
        
        .section .heap
        .align  3
#ifdef __HEAP_SIZE
        .equ    Heap_Size, __HEAP_SIZE
#else
        .equ    Heap_Size, 0x00000000
#endif
        .globl  __HeapBase
        .globl  __HeapLimit
__HeapBase:
        .if     Heap_Size
        .space  Heap_Size
        .endif
        .size   __HeapBase, . - __HeapBase
__HeapLimit:
        .size   __HeapLimit, . - __HeapLimit

        .section .isr_vector
        .align  2
        .globl  __isr_vector
__isr_vector:
        .long   __StackTop            /* Top of Stack */
        .long   Reset_Handler         /* Reset Handler */
        .long   NMI_Handler           /* NMI Handler */
        .long   HardFault_Handler     /* Hard Fault Handler */
        .long   MemManage_Handler     /* MPU Fault Handler */
        .long   BusFault_Handler      /* Bus Fault Handler */
        .long   UsageFault_Handler    /* Usage Fault Handler */
        .long   0                     /* Reserved */
        .long   0                     /* Reserved */
        .long   0                     /* Reserved */
        .long   0                     /* Reserved */
        .long   SVC_Handler           /* SVCall Handler */
        .long   DebugMon_Handler      /* Debug Monitor Handler */
        .long   0                     /* Reserved */
        .long   PendSV_Handler        /* PendSV Handler */
        .long   SysTick_Handler       /* SysTick Handler */

        /* External interrupts */
        .long   WWDG_IRQHandler
        .long   PVD_PVM_IRQHandler
        .long   TAMP_STAMP_IRQHandler
        .long   RTC_WKUP_IRQHandler
        .long   FLASH_IRQHandler
        .long   RCC_IRQHandler
        .long   EXTI0_IRQHandler
        .long   EXTI1_IRQHandler
        .long   EXTI2_IRQHandler
        .long   EXTI3_IRQHandler
        .long   EXTI4_IRQHandler
        .long   DMA1_Channel1_IRQHandler
        .long   DMA1_Channel2_IRQHandler
        .long   DMA1_Channel3_IRQHandler
        .long   DMA1_Channel4_IRQHandler
        .long   DMA1_Channel5_IRQHandler
        .long   DMA1_Channel6_IRQHandler
        .long   DMA1_Channel7_IRQHandler
        .long   ADC1_2_IRQHandler
        .long   CAN1_TX_IRQHandler
        .long   CAN1_RX0_IRQHandler
        .long   CAN1_RX1_IRQHandler
        .long   CAN1_SCE_IRQHandler
        .long   EXTI9_5_IRQHandler
        .long   TIM1_BRK_TIM15_IRQHandler
        .long   TIM1_UP_TIM16_IRQHandler
        .long   TIM1_TRG_COM_TIM17_IRQHandler
        .long   TIM1_CC_IRQHandler
        .long   TIM2_IRQHandler
        .long   TIM3_IRQHandler
        .long   TIM4_IRQHandler
        .long   I2C1_EV_IRQHandler
        .long   I2C1_ER_IRQHandler
        .long   I2C2_EV_IRQHandler
        .long   I2C2_ER_IRQHandler
        .long   SPI1_IRQHandler
        .long   SPI2_IRQHandler
        .long   USART1_IRQHandler
        .long   USART2_IRQHandler
        .long   USART3_IRQHandler
        .long   EXTI15_10_IRQHandler
        .long   RTC_Alarm_IRQHandler
        .long   DFSDM1_FLT3_IRQHandler
        .long   TIM8_BRK_IRQHandler
        .long   TIM8_UP_IRQHandler
        .long   TIM8_TRG_COM_IRQHandler
        .long   TIM8_CC_IRQHandler
        .long   ADC3_IRQHandler
        .long   FMC_IRQHandler
        .long   SDMMC1_IRQHandler
        .long   TIM5_IRQHandler
        .long   SPI3_IRQHandler
        .long   UART4_IRQHandler
        .long   UART5_IRQHandler
        .long   TIM6_DAC_IRQHandler
        .long   TIM7_IRQHandler
        .long   DMA2_Channel1_IRQHandler
        .long   DMA2_Channel2_IRQHandler
        .long   DMA2_Channel3_IRQHandler
        .long   DMA2_Channel4_IRQHandler
        .long   DMA2_Channel5_IRQHandler
        .long   DFSDM1_FLT0_IRQHandler
        .long   DFSDM1_FLT1_IRQHandler
        .long   DFSDM1_FLT2_IRQHandler
        .long   COMP_IRQHandler
        .long   LPTIM1_IRQHandler
        .long   LPTIM2_IRQHandler
        .long   OTG_FS_IRQHandler
        .long   DMA2_Channel6_IRQHandler
        .long   DMA2_Channel7_IRQHandler
        .long   LPUART1_IRQHandler
        .long   QUADSPI_IRQHandler
        .long   I2C3_EV_IRQHandler
        .long   I2C3_ER_IRQHandler
        .long   SAI1_IRQHandler
        .long   SAI2_IRQHandler
        .long   SWPMI1_IRQHandler
        .long   TSC_IRQHandler
        .long   LCD_IRQHandler
        .long   0
        .long   RNG_IRQHandler
        .long   FPU_IRQHandler
        .long   CRS_IRQHandler
        .long   I2C4_EV_IRQHandler
        .long   I2C4_ER_IRQHandler
        .long   DCMI_IRQHandler
        .long   CAN2_TX_IRQHandler
        .long   CAN2_RX0_IRQHandler
        .long   CAN2_RX1_IRQHandler
        .long   CAN2_SCE_IRQHandler
        .long   DMA2D_IRQHandler  

        .size   __isr_vector, . - __isr_vector


        .text
        .thumb
        .thumb_func
        .align  2
        .globl  Reset_Handler
        .type   Reset_Handler, %function
Reset_Handler:
        /*
         *  The __copy_table has the following format per entry:
         *
         *    offset 0: LMA of start of a DATA section to copy from
         *    offset 4: VMA of start of a DATA section to copy to
         *    offset 8: Size of this DATA sectiony. Must be multiply of 4
         *
         *  All addresses must be aligned to 4 bytes boundary.
         */

        ldr     r1, =__copy_table_start__
        ldr     r2, =__copy_table_end__
        b.n     3f
1:      ldmia   r1!, { r3, r4, r5 }
        cbz.n   r5, 3f
2:      subs    r5, #4
        ldr     r0, [r3, r5]
        str     r0, [r4, r5]
        bne.n   2b
3:      cmp     r1, r2
        bne.n   1b

        /*
         *  The __zero_table has the following format per entry:
         *
         *    offset 0: VMA of start of a BSS section to zero
         *    offset 4: Size of this BSS section. Must be multiply of 4
         *
         *  All addresses must be aligned to 4 bytes boundary.
         */

        movs    r0, #0
        ldr     r1, =__zero_table_start__
        ldr     r2, =__zero_table_end__
        b.n     6f
4:      ldmia   r1!, { r3, r4 }
        cbz.n   r4, 6f
5:      subs    r4, #4
        str     r0, [r3, r4]
        bne.n   5b
6:      cmp     r1, r2
        bne.n   4b
        
        /* ##### default starts is "_start" */
#ifndef __START
#define __START main
#endif
        b       __START

        .size   Reset_Handler, . - Reset_Handler

        .text
        .thumb
        .thumb_func
        .align  2
        .weak   Default_Handler
        .type   Default_Handler, %function
Default_Handler:
        b       .
        .size   Default_Handler, . - Default_Handler

        /*
         * Macro to define default handlers. Default handler
         * will be weak symbol and just dead loops. They can be
         * overwritten by other handlers
         */
        .macro  def_irq_handler handler_name
        .weak   \handler_name
        .set    \handler_name, Default_Handler
        .endm

        def_irq_handler NMI_Handler
        def_irq_handler HardFault_Handler
        def_irq_handler MemManage_Handler
        def_irq_handler BusFault_Handler
        def_irq_handler UsageFault_Handler
        def_irq_handler SVC_Handler
        def_irq_handler DebugMon_Handler
        def_irq_handler PendSV_Handler
        def_irq_handler SysTick_Handler
        def_irq_handler WWDG_IRQHandler
        def_irq_handler PVD_PVM_IRQHandler
        def_irq_handler TAMP_STAMP_IRQHandler
        def_irq_handler RTC_WKUP_IRQHandler
        def_irq_handler FLASH_IRQHandler
        def_irq_handler RCC_IRQHandler
        def_irq_handler EXTI0_IRQHandler
        def_irq_handler EXTI1_IRQHandler
        def_irq_handler EXTI2_IRQHandler
        def_irq_handler EXTI3_IRQHandler
        def_irq_handler EXTI4_IRQHandler
        def_irq_handler DMA1_Channel1_IRQHandler
        def_irq_handler DMA1_Channel2_IRQHandler
        def_irq_handler DMA1_Channel3_IRQHandler
        def_irq_handler DMA1_Channel4_IRQHandler
        def_irq_handler DMA1_Channel5_IRQHandler
        def_irq_handler DMA1_Channel6_IRQHandler
        def_irq_handler DMA1_Channel7_IRQHandler
        def_irq_handler ADC1_2_IRQHandler
        def_irq_handler CAN1_TX_IRQHandler
        def_irq_handler CAN1_RX0_IRQHandler
        def_irq_handler CAN1_RX1_IRQHandler
        def_irq_handler CAN1_SCE_IRQHandler
        def_irq_handler EXTI9_5_IRQHandler
        def_irq_handler TIM1_BRK_TIM15_IRQHandler
        def_irq_handler TIM1_UP_TIM16_IRQHandler
        def_irq_handler TIM1_TRG_COM_TIM17_IRQHandler
        def_irq_handler TIM1_CC_IRQHandler
        def_irq_handler TIM2_IRQHandler
        def_irq_handler TIM3_IRQHandler
        def_irq_handler TIM4_IRQHandler
        def_irq_handler I2C1_EV_IRQHandler
        def_irq_handler I2C1_ER_IRQHandler
        def_irq_handler I2C2_EV_IRQHandler
        def_irq_handler I2C2_ER_IRQHandler
        def_irq_handler SPI1_IRQHandler
        def_irq_handler SPI2_IRQHandler
        def_irq_handler USART1_IRQHandler
        def_irq_handler USART2_IRQHandler
        def_irq_handler USART3_IRQHandler
        def_irq_handler EXTI15_10_IRQHandler
        def_irq_handler RTC_Alarm_IRQHandler
        def_irq_handler DFSDM1_FLT3_IRQHandler
        def_irq_handler TIM8_BRK_IRQHandler
        def_irq_handler TIM8_UP_IRQHandler
        def_irq_handler TIM8_TRG_COM_IRQHandler
        def_irq_handler TIM8_CC_IRQHandler
        def_irq_handler ADC3_IRQHandler
        def_irq_handler FMC_IRQHandler
        def_irq_handler SDMMC1_IRQHandler
        def_irq_handler TIM5_IRQHandler
        def_irq_handler SPI3_IRQHandler
        def_irq_handler UART4_IRQHandler
        def_irq_handler UART5_IRQHandler
        def_irq_handler TIM6_DAC_IRQHandler
        def_irq_handler TIM7_IRQHandler
        def_irq_handler DMA2_Channel1_IRQHandler
        def_irq_handler DMA2_Channel2_IRQHandler
        def_irq_handler DMA2_Channel3_IRQHandler
        def_irq_handler DMA2_Channel4_IRQHandler
        def_irq_handler DMA2_Channel5_IRQHandler
        def_irq_handler DFSDM1_FLT0_IRQHandler
        def_irq_handler DFSDM1_FLT1_IRQHandler
        def_irq_handler DFSDM1_FLT2_IRQHandler
        def_irq_handler COMP_IRQHandler
        def_irq_handler LPTIM1_IRQHandler
        def_irq_handler LPTIM2_IRQHandler
        def_irq_handler OTG_FS_IRQHandler
        def_irq_handler DMA2_Channel6_IRQHandler
        def_irq_handler DMA2_Channel7_IRQHandler
        def_irq_handler LPUART1_IRQHandler
        def_irq_handler QUADSPI_IRQHandler
        def_irq_handler I2C3_EV_IRQHandler
        def_irq_handler I2C3_ER_IRQHandler
        def_irq_handler SAI1_IRQHandler
        def_irq_handler SAI2_IRQHandler
        def_irq_handler SWPMI1_IRQHandler
        def_irq_handler TSC_IRQHandler
        def_irq_handler LCD_IRQHandler
        def_irq_handler RNG_IRQHandler
        def_irq_handler FPU_IRQHandler
        def_irq_handler CRS_IRQHandler
        def_irq_handler I2C4_EV_IRQHandler
        def_irq_handler I2C4_ER_IRQHandler
        def_irq_handler DCMI_IRQHandler
        def_irq_handler CAN2_TX_IRQHandler
        def_irq_handler CAN2_RX0_IRQHandler
        def_irq_handler CAN2_RX1_IRQHandler
        def_irq_handler CAN2_SCE_IRQHandler
        def_irq_handler DMA2D_IRQHandler  
