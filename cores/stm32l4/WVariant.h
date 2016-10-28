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

#ifdef __cplusplus
extern "C" {
#endif

#define PIN_ATTR_DAC           (1UL<<0)
#define PIN_ATTR_ADC           (1UL<<1)
#define PIN_ATTR_PWM           (1UL<<2)
#define PIN_ATTR_EXTI          (1UL<<3)

#define PWM_INSTANCE_NONE      255

#define PWM_CHANNEL_1          0
#define PWM_CHANNEL_2          1
#define PWM_CHANNEL_3          2
#define PWM_CHANNEL_4          3
#define PWM_CHANNEL_NONE       255

#define ADC_INPUT_1            1
#define ADC_INPUT_2            2
#define ADC_INPUT_3            3
#define ADC_INPUT_4            4
#define ADC_INPUT_5            5
#define ADC_INPUT_6            6
#define ADC_INPUT_7            7
#define ADC_INPUT_8            8
#define ADC_INPUT_9            9
#define ADC_INPUT_10           10
#define ADC_INPUT_11           11
#define ADC_INPUT_12           12
#define ADC_INPUT_13           13
#define ADC_INPUT_14           14
#define ADC_INPUT_15           15
#define ADC_INPUT_16           16
#define ADC_INPUT_NONE         255


/* Types used for the table below */
typedef struct _PinDescription
{
  void                    *GPIO;
  uint16_t                bit;
  uint16_t                pin;
  uint8_t                 attr;
  uint8_t                 pwm_instance;
  uint8_t                 pwm_channel;
  uint8_t                 adc_input;
} PinDescription ;

/* Pins table to be instantiated into variant.cpp */
extern const PinDescription g_APinDescription[] ;

extern const unsigned int g_PWMInstances[] ;

#define digitalPinToPort(P)        ( g_APinDescription[P].GPIO )
#define digitalPinToBitMask(P)     ( g_APinDescription[P].bit )
#define portInputRegister(port)    ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x10) ) // IDR
#define portOutputRegister(port)   ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x14) ) // ODR
#define portSetRegister(port)      ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x18) ) // BSRR
#define portClearRegister(port)    ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x28) ) // BRR
#define digitalPinHasPWM(P)        ( g_APinDescription[P].attr & PIN_ATTR_PWM )

#ifdef __cplusplus
} // extern "C"
#endif
