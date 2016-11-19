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

#include "Arduino.h"
#include "stm32l4_wiring_private.h"

#define PWM_INSTANCE_TIM1      0
#define PWM_INSTANCE_TIM16     1

/*
 * Pins descriptions
 */
extern const PinDescription g_APinDescription[NUM_TOTAL_PINS] =
{
    // 0..13 - Digital pins
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA10), GPIO_PIN_PA10,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA9),  GPIO_PIN_PA9,             (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA12), GPIO_PIN_PA12,            (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB0),  GPIO_PIN_PB0_TIM1_CH2N,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM1,  PWM_CHANNEL_2,    ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB7),  GPIO_PIN_PB7,             (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB6),  GPIO_PIN_PB6_TIM16_CH1N,  (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM16, PWM_CHANNEL_1,    ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB1),  GPIO_PIN_PB1_TIM1_CH3N,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM1,  PWM_CHANNEL_3,    ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PC14), GPIO_PIN_PC14,            0,                                             PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { NULL,  GPIO_PIN_MASK(GPIO_PIN_PC15), GPIO_PIN_PC15,            0,                                             PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA8),  GPIO_PIN_PA8_TIM1_CH1,    (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM1,  PWM_CHANNEL_1,    ADC_INPUT_NONE },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA11), GPIO_PIN_PA11_TIM1_CH4,   (PIN_ATTR_PWM | PIN_ATTR_EXTI),                PWM_INSTANCE_TIM1,  PWM_CHANNEL_4,    ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB5),  GPIO_PIN_PB5,             (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB4),  GPIO_PIN_PB4,             (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },
    { GPIOB, GPIO_PIN_MASK(GPIO_PIN_PB3),  GPIO_PIN_PB3,             (PIN_ATTR_EXTI),                               PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_NONE },

    // 14..21 - Analog pins
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA0),  GPIO_PIN_PA0,             (PIN_ATTR_ADC),                                PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_5    },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA1),  GPIO_PIN_PA1,             (PIN_ATTR_ADC),                                PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_6    },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA3),  GPIO_PIN_PA3,             (PIN_ATTR_ADC),                                PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_8    },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA4),  GPIO_PIN_PA4,             (PIN_ATTR_ADC | PIN_ATTR_DAC),                 PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_9    },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA5),  GPIO_PIN_PA5,             (PIN_ATTR_ADC | PIN_ATTR_DAC),                 PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_10   },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA6),  GPIO_PIN_PA6,             (PIN_ATTR_ADC),                                PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_11   },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA7),  GPIO_PIN_PA7,             (PIN_ATTR_ADC),                                PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_12   },
    { GPIOA, GPIO_PIN_MASK(GPIO_PIN_PA2),  GPIO_PIN_PA2,             (PIN_ATTR_ADC),                                PWM_INSTANCE_NONE,  PWM_CHANNEL_NONE, ADC_INPUT_7    },
};

extern const unsigned int g_PWMInstances[PWM_INSTANCE_COUNT] = {
    TIMER_INSTANCE_TIM1,
    TIMER_INSTANCE_TIM16,
};

extern const stm32l4_uart_pins_t g_SerialPins = { GPIO_PIN_PA15_USART2_RX, GPIO_PIN_PA2_USART2_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };
extern const unsigned int g_SerialInstance = UART_INSTANCE_USART2;
extern const unsigned int g_SerialMode = UART_MODE_RX_DMA;

extern const stm32l4_uart_pins_t g_Serial1Pins = { GPIO_PIN_PA10_USART1_RX, GPIO_PIN_PA9_USART1_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };
extern const unsigned int g_Serial1Instance = UART_INSTANCE_USART1;
extern const unsigned int g_Serial1Mode = UART_MODE_RX_DMA | UART_MODE_TX_DMA;

extern const stm32l4_spi_pins_t g_SPIPins = { GPIO_PIN_PB5_SPI3_MOSI, GPIO_PIN_PB4_SPI3_MISO, GPIO_PIN_PB3_SPI3_SCK, GPIO_PIN_NONE };
extern const unsigned int g_SPIInstance = SPI_INSTANCE_SPI3;
extern const unsigned int g_SPIMode = SPI_MODE_RX_DMA | SPI_MODE_TX_DMA | SPI_MODE_RX_DMA_SECONDARY | SPI_MODE_TX_DMA_SECONDARY;

extern const stm32l4_i2c_pins_t g_WirePins = { GPIO_PIN_PB6_I2C1_SCL, GPIO_PIN_PB7_I2C1_SDA };
extern const unsigned int g_WireInstance = I2C_INSTANCE_I2C1;
extern const unsigned int g_WireMode = I2C_MODE_RX_DMA;

