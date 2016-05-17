/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx.h"

#include "stm32l4_adc.h"
#include "stm32l4_dac.h"
#include "stm32l4_exti.h"
#include "stm32l4_timer.h"
#include "stm32l4_gpio.h"
#include "stm32l4_uart.h"
#include "stm32l4_i2c.h"
#include "stm32l4_servo.h"
#include "stm32l4_spi.h"
#include "stm32l4_usbd_cdc.h"
#include "stm32l4_system.h"

#include "wiring_constants.h"

#define STM32L4_SVCALL_IRQ_PRIORITY  15
#define STM32L4_PENDSV_IRQ_PRIORITY  15

#define STM32L4_ADC_IRQ_PRIORITY     15
#define STM32L4_DAC_IRQ_PRIORITY     15
#define STM32L4_PWM_IRQ_PRIORITY     15

#define STM32L4_USB_IRQ_PRIORITY     14
#define STM32L4_I2C_IRQ_PRIORITY     13
#define STM32L4_UART_IRQ_PRIORITY    12
#define STM32L4_SPI_IRQ_PRIORITY     11

#define STM32L4_EXTI_IRQ_PRIORITY    4
#define STM32L4_SYSTICK_IRQ_PRIORITY 3
#define STM32L4_TONE_IRQ_PRIORITY    2
#define STM32L4_SERVO_IRQ_PRIORITY   1

/************************************************************************
 * DMA map:
 * 
 * DMA_CHANNEL_DMA1_CH1_ADC1
 * DMA_CHANNEL_DMA1_CH2_USART3_TX
 * DMA_CHANNEL_DMA1_CH3_USART3_RX
 * DMA_CHANNEL_DMA1_CH4_SPI2_RX / DMA_CHANNEL_DMA1_CH4_DFSDM0
 * DMA_CHANNEL_DMA1_CH5_SPI2_TX / DMA_CHANNEL_DMA1_CH5_DFSDM1 
 * DMA_CHANNEL_DMA1_CH6_SAI2_A
 * DMA_CHANNEL_DMA1_CH7_I2C1_RX
 * DMA_CHANNEL_DMA2_CH1_SPI3_RX
 * DMA_CHANNEL_DMA2_CH2_SPI3_TX
 * DMA_CHANNEL_DMA2_CH3_SPI1_RX
 * DMA_CHANNEL_DMA2_CH4_SPI1_TX
 * DMA_CHANNEL_DMA2_CH5_UART4_RX
 * DMA_CHANNEL_DMA2_CH6_SAI1_A
 * DMA_CHANNEL_DMA2_CH7_QUADSPI
 *
 ************************************************************************/

/************************************************************************
 * TIM map:
 * 
 * TIM1    PWM
 * TIM2    CAPTURE
 * TIM3    PWM
 * TIM4    PWM
 * TIM5    PWM
 * TIM6    (ADC)
 * TIM7    TONE
 * TIM8
 * TIM15   SERVO
 * TIM16   IR
 * TIM17   IR
 * 
 ************************************************************************/

extern stm32l4_adc_t stm32l4_adc;
extern stm32l4_exti_t stm32l4_exti;

#ifdef __cplusplus
} // extern "C"

#include "HardwareSerial.h"

#endif
