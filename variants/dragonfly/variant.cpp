/*
 * Copyright (c) 2016 Thomas Roell.  All rights reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#include "variant.h"
#include "wiring_private.h"

/*
 * Pins descriptions
 */
const PinDescription g_APinDescription[NUM_TOTAL_PINS] =
{
    // 0..13 - Digital pins
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC5),  GPIO_PIN_PC5,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC4),  GPIO_PIN_PC4,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB12), GPIO_PIN_PB12,           (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB13), GPIO_PIN_PB13_TIM1_CH1N, (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM1, PWM_CHANNEL_1,    ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB14), GPIO_PIN_PB14_TIM1_CH2N, (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM1, PWM_CHANNEL_2,    ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB15), GPIO_PIN_PB15_TIM1_CH3N, (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM1, PWM_CHANNEL_3,    ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC9),  GPIO_PIN_PC9_TIM3_CH4,   (PIN_ATTR_PWM),                                PWM_INSTANCE_TIM3, PWM_CHANNEL_4,    ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC8),  GPIO_PIN_PC8_TIM3_CH3,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM3, PWM_CHANNEL_3,    ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA3),  GPIO_PIN_PA3_TIM5_CH4,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM5, PWM_CHANNEL_4,    ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA2),  GPIO_PIN_PA2_TIM5_CH3,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM5, PWM_CHANNEL_3,    ADC_INPUT_NONE },
#if defined(STM32L4_CONFIG_DOSFS_SDCARD)
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PD2),  GPIO_PIN_PD2,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PC12), GPIO_PIN_PC12,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PC11), GPIO_PIN_PC11,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PC10), GPIO_PIN_PC10,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
#else /* STM32L4_CONFIG_DOSFS_SDCARD */
    { GPIOD, GPIO_PIN_MASK(GPIO_PIN_PD2),  GPIO_PIN_PD2,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC12), GPIO_PIN_PC12,           (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC11), GPIO_PIN_PC11,           (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC10), GPIO_PIN_PC10,           (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
#endif /* STM32L4_CONFIG_DOSFS_SDCARD */

    // 14..19 - Analog pins
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA4),  GPIO_PIN_PA4,            (PIN_ATTR_ADC | PIN_ATTR_DAC | PIN_ATTR_EXTI), PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_9    },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA5),  GPIO_PIN_PA5,            (PIN_ATTR_ADC | PIN_ATTR_DAC | PIN_ATTR_EXTI), PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_10   },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC3),  GPIO_PIN_PC3,            (PIN_ATTR_ADC | PIN_ATTR_EXTI),                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_4    },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC2),  GPIO_PIN_PC2,            (PIN_ATTR_ADC | PIN_ATTR_EXTI),                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_3    },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC1),  GPIO_PIN_PC1,            (PIN_ATTR_ADC | PIN_ATTR_EXTI),                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_2    },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC0),  GPIO_PIN_PC0,            (PIN_ATTR_ADC | PIN_ATTR_EXTI),                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_1    },

    // 20..21 - I2C pins (SDA,SCL)
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB9),  GPIO_PIN_PB9_TIM4_CH4,   (PIN_ATTR_PWM),                                PWM_INSTANCE_TIM4, PWM_CHANNEL_4,    ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB8),  GPIO_PIN_PB8_TIM4_CH3,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM4, PWM_CHANNEL_3,    ADC_INPUT_NONE },

    // 22..24 - SPI/ICSP pins (MISO,SCK,MOSI)
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB4),  GPIO_PIN_PB4,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB3),  GPIO_PIN_PB3,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB5),  GPIO_PIN_PB5,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },

    // 25..26 - RX/TX LEDS (output only)
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB2),  GPIO_PIN_PB2,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA10), GPIO_PIN_PA10,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },

    // 27..29 - USB (VBUS,DM,DP)
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PA9),  GPIO_PIN_PA9,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PA11), GPIO_PIN_PA11,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PA12), GPIO_PIN_PA12,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },

    // 30..31 - UART pins (TX,RX
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA0),  GPIO_PIN_PA0_TIM5_CH1,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM5, PWM_CHANNEL_1,    ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA1),  GPIO_PIN_PA1_TIM5_CH2,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM5, PWM_CHANNEL_2,    ADC_INPUT_NONE },

    // 32..37 QSPI pins (NCS,CK,IO0,IO1,IO2,IO3)
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PB11), GPIO_PIN_PB11,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PB10), GPIO_PIN_PB10,           0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PB1),  GPIO_PIN_PB1,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PB0),  GPIO_PIN_PB0,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PA7),  GPIO_PIN_PA7,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PA6),  GPIO_PIN_PA6,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },

    // 38..40 - Digital pins (ATN,39,40)
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA8),  GPIO_PIN_PA8,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC7),  GPIO_PIN_PC7_TIM3_CH2,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM3, PWM_CHANNEL_2,    ADC_INPUT_NONE },
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC6),  GPIO_PIN_PC6_TIM3_CH1,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM3, PWM_CHANNEL_1,    ADC_INPUT_NONE },

    // 41..43 - PAD pins (INT,SDA,SCL)
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA15), GPIO_PIN_PA15,           (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB7),  GPIO_PIN_PB7,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB6),  GPIO_PIN_PB6,            0,                                             PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },

    // 44 - Button 
    { GPIOC, GPIO_PIN_MASK(GPIO_PIN_PC13), GPIO_PIN_PC13,           (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, ADC_INPUT_NONE },
};
