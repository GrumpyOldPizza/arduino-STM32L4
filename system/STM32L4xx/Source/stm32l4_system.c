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

#include <stdlib.h>

#include "armv7m.h"

#include "stm32l4_system.h"

typedef struct _stm32l4_system_device_t {
    uint32_t reset;
    uint32_t lseclk;
    uint32_t hseclk;
    uint32_t sysclk;
    uint32_t hclk;
    uint32_t pclk1;
    uint32_t pclk2;
    bool     clk48;
} stm32l4_system_device_t;

static stm32l4_system_device_t stm32l4_system_device;

static volatile uint32_t * const stm32l4_system_xlate_RSTR[SYSTEM_PERIPH_COUNT] = {
    &RCC->AHB1RSTR,  /* SYSTEM_PERIPH_FLASH */
    NULL,            /* SYSTEM_PERIPH_SRAM1 */
    NULL,            /* SYSTEM_PERIPH_SRAM2 */
    &RCC->AHB1RSTR,  /* SYSTEM_PERIPH_DMA1 */
    &RCC->AHB1RSTR,  /* SYSTEM_PERIPH_DMA2 */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOA */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOC */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOD */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx)
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOF */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOG */
#endif
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L476xx)
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_USB */
#else
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_USB */
#endif
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_ADC */
    &RCC->AHB3RSTR,  /* SYSTEM_PERIPH_QSPI */
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_USART1 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_UART4 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_UART5 */
#endif
    &RCC->APB1RSTR2, /* SYSTEM_PERIPH_LPUART1 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_I2C2 */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_I2C3 */
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_SPI2 */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_SPI3 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SDMMC */
#endif
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SAI2 */
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_DFSDM */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_CAN */
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM1 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM3 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM4 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM5 */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM6 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM8 */
#endif
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM15 */
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM17 */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_LPTIM1 */
    &RCC->APB1RSTR2, /* SYSTEM_PERIPH_LPTIM2 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_DAC */
};

static uint32_t const stm32l4_system_xlate_RSTMSK[SYSTEM_PERIPH_COUNT] = {
    RCC_AHB1RSTR_FLASHRST,    /* SYSTEM_PERIPH_FLASH */
    0,                        /* SYSTEM_PERIPH_SRAM1 */
    0,                        /* SYSTEM_PERIPH_SRAM2 */
    RCC_AHB1RSTR_DMA1RST,     /* SYSTEM_PERIPH_DMA1 */
    RCC_AHB1RSTR_DMA2RST,     /* SYSTEM_PERIPH_DMA2 */
    RCC_AHB2RSTR_GPIOARST,    /* SYSTEM_PERIPH_GPIOA */
    RCC_AHB2RSTR_GPIOBRST,    /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_AHB2RSTR_GPIOCRST,    /* SYSTEM_PERIPH_GPIOC */
    RCC_AHB2RSTR_GPIODRST,    /* SYSTEM_PERIPH_GPIOD */
    RCC_AHB2RSTR_GPIOERST,    /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx)
    RCC_AHB2RSTR_GPIOFRST,    /* SYSTEM_PERIPH_GPIOF */
    RCC_AHB2RSTR_GPIOGRST,    /* SYSTEM_PERIPH_GPIOG */
#endif
    RCC_AHB2RSTR_GPIOHRST,    /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L476xx)
    RCC_AHB2RSTR_OTGFSRST,    /* SYSTEM_PERIPH_USB */
#else
    RCC_APB1RSTR1_USBFSRST,   /* SYSTEM_PERIPH_USB */
#endif
    RCC_AHB2RSTR_ADCRST,      /* SYSTEM_PERIPH_ADC */
    RCC_AHB3RSTR_QSPIRST,     /* SYSTEM_PERIPH_QSPI */
    RCC_APB2RSTR_USART1RST,   /* SYSTEM_PERIPH_USART1 */
    RCC_APB1RSTR1_USART2RST,  /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1RSTR1_USART3RST,  /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx)
    RCC_APB1RSTR1_UART4RST,   /* SYSTEM_PERIPH_UART4 */
    RCC_APB1RSTR1_UART5RST,   /* SYSTEM_PERIPH_UART5 */
#endif
    RCC_APB1RSTR2_LPUART1RST, /* SYSTEM_PERIPH_LPUART1 */
    RCC_APB1RSTR1_I2C1RST,    /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1RSTR1_I2C2RST,    /* SYSTEM_PERIPH_I2C2 */
#endif
    RCC_APB1RSTR1_I2C3RST,    /* SYSTEM_PERIPH_I2C3 */
    RCC_APB2RSTR_SPI1RST,     /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1RSTR1_SPI2RST,    /* SYSTEM_PERIPH_SPI2 */
#endif
    RCC_APB1RSTR1_SPI3RST,    /* SYSTEM_PERIPH_SPI3 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB2RSTR_SDMMC1RST,   /* SYSTEM_PERIPH_SDMMC */
#endif
    RCC_APB2RSTR_SAI1RST,     /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx)
    RCC_APB2RSTR_SAI2RST,     /* SYSTEM_PERIPH_SAI2 */
    RCC_APB2RSTR_DFSDM1RST,   /* SYSTEM_PERIPH_DFSDM */
#endif
    RCC_APB1RSTR1_CAN1RST,    /* SYSTEM_PERIPH_CAN */
    RCC_APB2RSTR_TIM1RST,     /* SYSTEM_PERIPH_TIM1 */
    RCC_APB1RSTR1_TIM2RST,    /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx)
    RCC_APB1RSTR1_TIM3RST,    /* SYSTEM_PERIPH_TIM3 */
    RCC_APB1RSTR1_TIM4RST,    /* SYSTEM_PERIPH_TIM4 */
    RCC_APB1RSTR1_TIM5RST,    /* SYSTEM_PERIPH_TIM5 */
#endif
    RCC_APB1RSTR1_TIM6RST,    /* SYSTEM_PERIPH_TIM6 */
    RCC_APB1RSTR1_TIM7RST,    /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx)
    RCC_APB2RSTR_TIM8RST,     /* SYSTEM_PERIPH_TIM8 */
#endif
    RCC_APB2RSTR_TIM15RST,    /* SYSTEM_PERIPH_TIM15 */
    RCC_APB2RSTR_TIM16RST,    /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx)
    RCC_APB2RSTR_TIM17RST,    /* SYSTEM_PERIPH_TIM17 */
#endif
    RCC_APB1RSTR1_LPTIM1RST,  /* SYSTEM_PERIPH_LPTIM1 */
    RCC_APB1RSTR2_LPTIM2RST,  /* SYSTEM_PERIPH_LPTIM2 */
    RCC_APB1RSTR1_DAC1RST,    /* SYSTEM_PERIPH_DAC */
};

static volatile uint32_t * const stm32l4_system_xlate_ENR[SYSTEM_PERIPH_COUNT] = {
    &RCC->AHB1ENR,  /* SYSTEM_PERIPH_FLASH */
    NULL,           /* SYSTEM_PERIPH_SRAM1 */
    NULL,           /* SYSTEM_PERIPH_SRAM2 */
    &RCC->AHB1ENR,  /* SYSTEM_PERIPH_DMA1 */
    &RCC->AHB1ENR,  /* SYSTEM_PERIPH_DMA2 */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOA */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOC */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOD */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx)
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOF */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOG */
#endif
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L476xx)
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_USB */
#else
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_USB */
#endif
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_ADC */
    &RCC->AHB3ENR,  /* SYSTEM_PERIPH_QSPI */
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_USART1 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_UART4 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_UART5 */
#endif
    &RCC->APB1ENR2, /* SYSTEM_PERIPH_LPUART1 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_I2C2 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_I2C3 */
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_SPI2 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_SPI3 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SDMMC */
#endif
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SAI2 */
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_DFSDM */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_CAN */
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM1 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM3 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM4 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM5 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM6 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM8 */
#endif
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM15 */
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM17 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_LPTIM1 */
    &RCC->APB1ENR2, /* SYSTEM_PERIPH_LPTIM2 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_DAC */
};

static uint32_t const stm32l4_system_xlate_ENMSK[SYSTEM_PERIPH_COUNT] = {
    RCC_AHB1ENR_FLASHEN,    /* SYSTEM_PERIPH_FLASH */
    0,                      /* SYSTEM_PERIPH_SRAM1 */
    0,                      /* SYSTEM_PERIPH_SRAM2 */
    RCC_AHB1ENR_DMA1EN,     /* SYSTEM_PERIPH_DMA1 */
    RCC_AHB1ENR_DMA2EN,     /* SYSTEM_PERIPH_DMA2 */
    RCC_AHB2ENR_GPIOAEN,    /* SYSTEM_PERIPH_GPIOA */
    RCC_AHB2ENR_GPIOBEN,    /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_AHB2ENR_GPIOCEN,    /* SYSTEM_PERIPH_GPIOC */
    RCC_AHB2ENR_GPIODEN,    /* SYSTEM_PERIPH_GPIOD */
    RCC_AHB2ENR_GPIOEEN,    /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx)
    RCC_AHB2ENR_GPIOFEN,    /* SYSTEM_PERIPH_GPIOF */
    RCC_AHB2ENR_GPIOGEN,    /* SYSTEM_PERIPH_GPIOG */
#endif
    RCC_AHB2ENR_GPIOHEN,    /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L476xx)
    RCC_AHB2ENR_OTGFSEN,    /* SYSTEM_PERIPH_USB */
#else
    RCC_APB1ENR1_USBFSEN,   /* SYSTEM_PERIPH_USB */
#endif
    RCC_AHB2ENR_ADCEN,      /* SYSTEM_PERIPH_ADC */
    RCC_AHB3ENR_QSPIEN,     /* SYSTEM_PERIPH_QSPI */
    RCC_APB2ENR_USART1EN,   /* SYSTEM_PERIPH_USART1 */
    RCC_APB1ENR1_USART2EN,  /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1ENR1_USART3EN,  /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx)
    RCC_APB1ENR1_UART4EN,   /* SYSTEM_PERIPH_UART4 */
    RCC_APB1ENR1_UART5EN,   /* SYSTEM_PERIPH_UART5 */
#endif
    RCC_APB1ENR2_LPUART1EN, /* SYSTEM_PERIPH_LPUART1 */
    RCC_APB1ENR1_I2C1EN,    /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1ENR1_I2C2EN,    /* SYSTEM_PERIPH_I2C2 */
#endif
    RCC_APB1ENR1_I2C3EN,    /* SYSTEM_PERIPH_I2C3 */
    RCC_APB2ENR_SPI1EN,     /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1ENR1_SPI2EN,    /* SYSTEM_PERIPH_SPI2 */
#endif
    RCC_APB1ENR1_SPI3EN,    /* SYSTEM_PERIPH_SPI3 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB2ENR_SDMMC1EN,   /* SYSTEM_PERIPH_SDMMC */
#endif
    RCC_APB2ENR_SAI1EN,     /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx)
    RCC_APB2ENR_SAI2EN,     /* SYSTEM_PERIPH_SAI2 */
    RCC_APB2ENR_DFSDM1EN,   /* SYSTEM_PERIPH_DFSDM */
#endif
    RCC_APB1ENR1_CAN1EN,    /* SYSTEM_PERIPH_CAN */
    RCC_APB2ENR_TIM1EN,     /* SYSTEM_PERIPH_TIM1 */
    RCC_APB1ENR1_TIM2EN,    /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx)
    RCC_APB1ENR1_TIM3EN,    /* SYSTEM_PERIPH_TIM3 */
    RCC_APB1ENR1_TIM4EN,    /* SYSTEM_PERIPH_TIM4 */
    RCC_APB1ENR1_TIM5EN,    /* SYSTEM_PERIPH_TIM5 */
#endif
    RCC_APB1ENR1_TIM6EN,    /* SYSTEM_PERIPH_TIM6 */
    RCC_APB1ENR1_TIM7EN,    /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx)
    RCC_APB2ENR_TIM8EN,     /* SYSTEM_PERIPH_TIM8 */
#endif
    RCC_APB2ENR_TIM15EN,    /* SYSTEM_PERIPH_TIM15 */
    RCC_APB2ENR_TIM16EN,    /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx)
    RCC_APB2ENR_TIM17EN,    /* SYSTEM_PERIPH_TIM17 */
#endif
    RCC_APB1ENR1_LPTIM1EN,  /* SYSTEM_PERIPH_LPTIM1 */
    RCC_APB1ENR2_LPTIM2EN,  /* SYSTEM_PERIPH_LPTIM2 */
    RCC_APB1ENR1_DAC1EN,    /* SYSTEM_PERIPH_DAC */
};

static volatile uint32_t * const stm32l4_system_xlate_SMENR[SYSTEM_PERIPH_COUNT] = {
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_FLASH */
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_SRAM1 */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_SRAM2 */
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_DMA1 */
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_DMA2 */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOA */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOC */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOD */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx)
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOF */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOG */
#endif
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L476xx)
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_USB */
#else
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_USB */
#endif
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_ADC */
    &RCC->AHB3SMENR,  /* SYSTEM_PERIPH_QSPI */
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_USART1 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_UART4 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_UART5 */
#endif
    &RCC->APB1SMENR2, /* SYSTEM_PERIPH_LPUART1 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_I2C2 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_I2C3 */
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_SPI2 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_SPI3 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SDMMC */
#endif
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SAI2 */
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_DFSDM */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_CAN */
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM1 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM3 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM4 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM5 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM6 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM8 */
#endif
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM15 */
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM17 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_LPTIM1 */
    &RCC->APB1SMENR2, /* SYSTEM_PERIPH_LPTIM2 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_DAC */
};

static uint32_t const stm32l4_system_xlate_SMENMSK[SYSTEM_PERIPH_COUNT] = {
    RCC_AHB1SMENR_FLASHSMEN,    /* SYSTEM_PERIPH_FLASH */
    RCC_AHB1SMENR_SRAM1SMEN,    /* SYSTEM_PERIPH_SRAM1 */
    RCC_AHB2SMENR_SRAM2SMEN,    /* SYSTEM_PERIPH_SRAM2 */
    RCC_AHB1SMENR_DMA1SMEN,     /* SYSTEM_PERIPH_DMA1 */
    RCC_AHB1SMENR_DMA2SMEN,     /* SYSTEM_PERIPH_DMA2 */
    RCC_AHB2SMENR_GPIOASMEN,    /* SYSTEM_PERIPH_GPIOA */
    RCC_AHB2SMENR_GPIOBSMEN,    /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_AHB2SMENR_GPIOCSMEN,    /* SYSTEM_PERIPH_GPIOC */
    RCC_AHB2SMENR_GPIODSMEN,    /* SYSTEM_PERIPH_GPIOD */
    RCC_AHB2SMENR_GPIOESMEN,    /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx)
    RCC_AHB2SMENR_GPIOFSMEN,    /* SYSTEM_PERIPH_GPIOF */
    RCC_AHB2SMENR_GPIOGSMEN,    /* SYSTEM_PERIPH_GPIOG */
#endif
    RCC_AHB2SMENR_GPIOHSMEN,    /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L476xx)
    RCC_AHB2SMENR_OTGFSSMEN,    /* SYSTEM_PERIPH_USB */
#else
    RCC_APB1SMENR1_USBFSSMEN,   /* SYSTEM_PERIPH_USB */
#endif
    RCC_AHB2SMENR_ADCSMEN,      /* SYSTEM_PERIPH_ADC */
    RCC_AHB3SMENR_QSPISMEN,     /* SYSTEM_PERIPH_QSPI */
    RCC_APB2SMENR_USART1SMEN,   /* SYSTEM_PERIPH_USART1 */
    RCC_APB1SMENR1_USART2SMEN,  /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1SMENR1_USART3SMEN,  /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx)
    RCC_APB1SMENR1_UART4SMEN,   /* SYSTEM_PERIPH_UART4 */
    RCC_APB1SMENR1_UART5SMEN,   /* SYSTEM_PERIPH_UART5 */
#endif
    RCC_APB1SMENR2_LPUART1SMEN, /* SYSTEM_PERIPH_LPUART1 */
    RCC_APB1SMENR1_I2C1SMEN,    /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1SMENR1_I2C2SMEN,    /* SYSTEM_PERIPH_I2C2 */
#endif
    RCC_APB1SMENR1_I2C3SMEN,    /* SYSTEM_PERIPH_I2C3 */
    RCC_APB2SMENR_SPI1SMEN,     /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB1SMENR1_SPI2SMEN,    /* SYSTEM_PERIPH_SPI2 */
#endif
    RCC_APB1SMENR1_SPI3SMEN,    /* SYSTEM_PERIPH_SPI3 */
#if defined(STM32L433xx) || defined(STM32L476xx)
    RCC_APB2SMENR_SDMMC1SMEN,   /* SYSTEM_PERIPH_SDMMC */
#endif
    RCC_APB2SMENR_SAI1SMEN,     /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx)
    RCC_APB2SMENR_SAI2SMEN,     /* SYSTEM_PERIPH_SAI2 */
    RCC_APB2SMENR_DFSDM1SMEN,   /* SYSTEM_PERIPH_DFSDM */
#endif
    RCC_APB1SMENR1_CAN1SMEN,    /* SYSTEM_PERIPH_CAN */
    RCC_APB2SMENR_TIM1SMEN,     /* SYSTEM_PERIPH_TIM1 */
    RCC_APB1SMENR1_TIM2SMEN,    /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx)
    RCC_APB1SMENR1_TIM3SMEN,    /* SYSTEM_PERIPH_TIM3 */
    RCC_APB1SMENR1_TIM4SMEN,    /* SYSTEM_PERIPH_TIM4 */
    RCC_APB1SMENR1_TIM5SMEN,    /* SYSTEM_PERIPH_TIM5 */
#endif
    RCC_APB1SMENR1_TIM6SMEN,    /* SYSTEM_PERIPH_TIM6 */
    RCC_APB1SMENR1_TIM7SMEN,    /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx)
    RCC_APB2SMENR_TIM8SMEN,     /* SYSTEM_PERIPH_TIM8 */
#endif
    RCC_APB2SMENR_TIM15SMEN,    /* SYSTEM_PERIPH_TIM15 */
    RCC_APB2SMENR_TIM16SMEN,    /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx)
    RCC_APB2SMENR_TIM17SMEN,    /* SYSTEM_PERIPH_TIM17 */
#endif
    RCC_APB1SMENR1_LPTIM1SMEN,  /* SYSTEM_PERIPH_LPTIM1 */
    RCC_APB1SMENR2_LPTIM2SMEN,  /* SYSTEM_PERIPH_LPTIM2 */
    RCC_APB1SMENR1_DAC1SMEN,    /* SYSTEM_PERIPH_DAC */
};

void stm32l4_system_periph_reset(unsigned int periph)
{
    armv7m_atomic_or(stm32l4_system_xlate_RSTR[periph], stm32l4_system_xlate_RSTMSK[periph]);
    armv7m_atomic_and(stm32l4_system_xlate_RSTR[periph], ~stm32l4_system_xlate_RSTMSK[periph]);
}

void stm32l4_system_periph_enable(unsigned int periph)
{
    armv7m_atomic_or(stm32l4_system_xlate_ENR[periph], stm32l4_system_xlate_ENMSK[periph]);
}

void stm32l4_system_periph_disable(unsigned int periph)
{
    armv7m_atomic_and(stm32l4_system_xlate_ENR[periph], ~stm32l4_system_xlate_ENMSK[periph]);
}

void stm32l4_system_periph_wake(unsigned int periph)
{
    armv7m_atomic_or(stm32l4_system_xlate_SMENR[periph], stm32l4_system_xlate_SMENMSK[periph]);
}

void stm32l4_system_periph_sleep(unsigned int periph)
{
    armv7m_atomic_and(stm32l4_system_xlate_SMENR[periph], ~stm32l4_system_xlate_SMENMSK[periph]);
}

void stm32l4_system_periph_cond_enable(unsigned int periph, volatile uint32_t *p_mask, uint32_t mask)
{
    armv7m_atomic_or(p_mask, mask);
    armv7m_atomic_or(stm32l4_system_xlate_ENR[periph], stm32l4_system_xlate_ENMSK[periph]);
}

void stm32l4_system_periph_cond_disable(unsigned int periph, volatile uint32_t *p_mask, uint32_t mask)
{
    uint32_t o_mask, n_mask;

    o_mask = *p_mask;
	  
    do
    {
	n_mask = o_mask & ~mask;
	      
	if (n_mask == 0)
	{
	    armv7m_atomic_and(stm32l4_system_xlate_ENR[periph], ~stm32l4_system_xlate_ENMSK[periph]);
	}
	else
	{
	    armv7m_atomic_or(stm32l4_system_xlate_ENR[periph], stm32l4_system_xlate_ENMSK[periph]);
	}
    }
    while (!armv7m_atomic_compare_exchange(p_mask, &o_mask, n_mask));
}

void stm32l4_system_periph_cond_wake(unsigned int periph, volatile uint32_t *p_mask, uint32_t mask)
{
    armv7m_atomic_or(p_mask, mask);
    armv7m_atomic_or(stm32l4_system_xlate_SMENR[periph], stm32l4_system_xlate_SMENMSK[periph]);
}

void stm32l4_system_periph_cond_sleep(unsigned int periph, volatile uint32_t *p_mask, uint32_t mask)
{
    uint32_t o_mask, n_mask;

    o_mask = *p_mask;
	  
    do
    {
	n_mask = o_mask & ~mask;
	      
	if (n_mask == 0)
	{
	    armv7m_atomic_and(stm32l4_system_xlate_SMENR[periph], ~stm32l4_system_xlate_SMENMSK[periph]);
	}
	else
	{
	    armv7m_atomic_or(stm32l4_system_xlate_SMENR[periph], stm32l4_system_xlate_SMENMSK[periph]);
	}
    }
    while (!armv7m_atomic_compare_exchange(p_mask, &o_mask, n_mask));
}

static void stm32l4_system_msi4_sysclk(void)
{
    uint32_t apb1enr1;

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS;

    /* Select the proper voltage range */

    apb1enr1 = RCC->APB1ENR1;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
    }

    if (PWR->CR1 & PWR_CR1_LPR)
    {
	PWR->CR1 &= ~PWR_CR1_LPR;
	
	while (PWR->SR2 & PWR_SR2_REGLPF)
	{
	}
    }

    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;

    while (PWR->SR2 & PWR_SR2_VOSF)
    {
    }

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
    }

    /* Select the HSI as system clock source */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    
    /* Wait till the main HSI is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    {
    }

    SystemCoreClock = 16000000;


    /* Select MSI @ 4MHz as system clock source.
     */

    if (!(RCC->CR & RCC_CR_MSION))
    {
	RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL | RCC_CR_MSION;

	while (!(RCC->CR & RCC_CR_MSIRDY))
	{
	}
    }
    else
    {
	RCC->CR = (RCC->CR & ~(RCC_CR_MSIRANGE | RCC_CR_MSIPLLEN)) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL;

	armv7m_clock_spin(500);
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_MSI;
    
    /* Wait till the main MSI is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI)
    {
    }

    SystemCoreClock = 4000000;

#if defined(STM32L476xx)
    /* Disable PLL/PLLSAI1/PLLSAI2 */
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLSAI1ON | RCC_CR_PLLSAI2ON);
    
    while (RCC->CR & (RCC_CR_PLLRDY | RCC_CR_PLLSAI1RDY | RCC_CR_PLLSAI2RDY))
    {
    }
#else
    /* Disable PLL/PLLSAI1 */
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLSAI1ON);
    
    while (RCC->CR & (RCC_CR_PLLRDY | RCC_CR_PLLSAI1RDY))
    {
    }
#endif    

    /* Disable HSE */
    if (stm32l4_system_device.hseclk)
    {
	RCC->CR &= ~RCC_CR_HSEON;
	
	while (RCC->CR & RCC_CR_HSERDY)
	{
	}
    }

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) | (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_0WS;
}


void stm32l4_system_bootloader(void)
{
    __disable_irq();

    /* Set the BCK bit to flag a reboot into the booloader */
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;
    RTC->CR |= RTC_CR_BCK;

    NVIC_SystemReset();
    
    while (1) { }
}

bool stm32l4_system_configure(uint32_t lseclk, uint32_t hseclk, uint32_t hclk, uint32_t pclk1, uint32_t pclk2)
{
    uint32_t sysclk, fclk, fvco, fpll, fpllout, mout, nout, rout, qout, n, r;
    uint32_t count, msirange, hpre, ppre1, ppre2, latency;
    uint32_t apb1enr1;

    /* Unlock SYSCFG (and leave it unlocked for EXTI use).
     */
    
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* Detect LSE/HSE on the first pass.
     */

    if (!stm32l4_system_device.reset)
    {
        stm32l4_system_device.reset = (RCC->CSR & 0xff000000) | RCC_CSR_RMVF;

	RCC->CSR |= RCC_CSR_RMVF;
	RCC->CSR &= ~RCC_CSR_RMVF;

	stm32l4_system_device.lseclk = lseclk;
	stm32l4_system_device.hseclk = hseclk;

	/* This is executed only on the very forst goaround, while interrupts are disabled.
	 * So RCC does not need atomics.
	 */

	RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

#if defined(STM32L432xx) || defined(STM32L433xx)
	/* Unlock RTCAPBEN (and leave it unlocked for RTC/BKUP use).
	 */
    
	RCC->APB1ENR1 |= RCC_APB1ENR1_RTCAPBEN;
#endif

	PWR->CR1 |= PWR_CR1_DBP;
	    
	while (!(PWR->CR1 & PWR_CR1_DBP))
	{
	}

	if (!(RCC->BDCR & RCC_BDCR_RTCEN))
	{
	    RCC->BDCR |= RCC_BDCR_BDRST;

	    RCC->BDCR &= ~RCC_BDCR_BDRST;
	}
	    
	if (RTC->CR & RTC_CR_BCK)
	{
	    RTC->WPR = 0xca;
	    RTC->WPR = 0x53;
	    RTC->CR &= ~RTC_CR_BCK;

	    /* Switch to System Memory @ 0x00000000.
	     */

	    SYSCFG->MEMRMP = (SYSCFG->MEMRMP & ~SYSCFG_MEMRMP_MEM_MODE) | SYSCFG_MEMRMP_MEM_MODE_0;
	    RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;

	    RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;

	    SCB->VTOR = 0;

	    /* This needs to be assembly code as GCC catches NULL 
	     * dereferences ...
	     */
	    __asm__ volatile (
		"   mov     r2, #0x00000000                \n"
		"   ldr     r0, [r2, #0]                   \n"
		"   ldr     r1, [r2, #4]                   \n"
		"   msr     MSP, r0                        \n"
		"   isb                                    \n"
		"   bx      r1                             \n");
	}

	if (lseclk)
	{
	    if (!(RCC->BDCR & RCC_BDCR_LSEON))
	    {
		RCC->BDCR |= RCC_BDCR_LSEON;
		
		/* The loop below take about 8 cycles per iteration. The startup time for
		 * LSE is 5000ms. At 16MHz this corresponds to about 10000000 iterations.
		 */
		count = 0;
		
		while (!(RCC->BDCR & RCC_BDCR_LSERDY))
		{
		    if (++count >= 10000000)
		    {
			stm32l4_system_device.lseclk = 0;
			break;
		    }
		}
	    }
	}

	if (stm32l4_system_device.lseclk == 0)
	{
	    RCC->BDCR &= ~RCC_BDCR_LSEON;
			
	    RCC->CSR |= RCC_CSR_LSION;
	    
	    while (!(RCC->CSR & RCC_CSR_LSIRDY))
	    {
	    }
	}
	else
	{
	    RCC->CSR &= ~RCC_CSR_LSION;
	}

	/* Enable RTC properly and write protect it. */
	if (!(RCC->BDCR & RCC_BDCR_RTCEN))
	{
	    /* Use LSI/LSE as source for RTC */

	    if (stm32l4_system_device.lseclk == 0)
	    {
		RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | (RCC_BDCR_RTCSEL_1 | RCC_BDCR_RTCEN);
	    }
	    else
	    {
		RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | (RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCEN);
	    }

	    RTC->WPR = 0xca;
	    RTC->WPR = 0x53;

	    RTC->ISR |= RTC_ISR_INIT;
	    
	    while (!(RTC->ISR & RTC_ISR_INITF))
	    {
	    }

	    RTC->CR = RTC_CR_BYPSHAD;
	    RTC->PRER = (stm32l4_system_device.lseclk == 0) ? 0x007d00ff : 0x007f00ff;

	    RTC->ISR &= ~RTC_ISR_INIT;
	}

	RTC->WPR = 0x00;
	RTC->WPR = 0x00;

#if defined(STM32L476xx)
	/* Enable VBAT charging (for Dragonfly).
	 */
	PWR->CR4 |= PWR_CR4_VBE;
#endif

	RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;

	if (hseclk)
	{
	    RCC->CR |= RCC_CR_HSEON;
	    
	    /* The loop below take about 8 cycles per iteration. The startup time for
	     * HSE is 100ms. At 16MHz this corresponds to about 200000 iterations.
	     */
	    count = 0;
	    
	    while (!(RCC->CR & RCC_CR_HSERDY))
	    {
		if (++count >= 200000)
		{
		    stm32l4_system_device.hseclk = 0;

		    RCC->CR &= ~RCC_CR_HSEON;
		    break;
		}
	    }
	}
    }

    if (hclk <= 24000000)
    {
	/* Range 2, use MSI. To allow USB operation, limit MSI down to 4MHz, and use a SYSCLK divider to get to 
	 * the target frequency.
	 */
	
	if      (hclk >= 24000000) { hclk = 24000000; msirange = RCC_CR_MSIRANGE_9; mout = 3; }
	else if (hclk >= 16000000) { hclk = 16000000; msirange = RCC_CR_MSIRANGE_8; mout = 2; }
	else if (hclk >=  8000000) { hclk =  8000000; msirange = RCC_CR_MSIRANGE_7; mout = 1; }
	else if (hclk >=  4000000) { hclk =  4000000; msirange = RCC_CR_MSIRANGE_6; mout = 1; }
	else if (hclk >=  2000000) { hclk =  2000000; msirange = RCC_CR_MSIRANGE_5; mout = 1; }
	else                       { hclk =  1000000; msirange = RCC_CR_MSIRANGE_4; mout = 1; }

	sysclk = hclk;

	
	/* PLLM is setup so that fclk = 8000000 into N/Q for USB. fvco is 96MHz to work around errata via
	 * PLLN set to 12, and then divided down to 48MHz via PLLQ set to 2.
	 */
	
	fclk = 8000000;
	nout = 12;
	rout = 2;
	qout = 2;
    }
    else
    {
	/* Range 1, use HSE/PLL or MSI/PLL */
	
	if (hclk < 16000000)
	{
	    hclk = 16000000;
	}
	
	if (hclk > 80000000)
	{
	    hclk = 80000000;
	}
	
	sysclk = hclk;
	
	msirange = RCC_CR_MSIRANGE_11;
	
	if (stm32l4_system_device.hseclk)
	{
	    mout = stm32l4_system_device.hseclk / 8000000; 
	}
	else
	{
	    /* MSI with 48MHz */
	    
	    mout = 6;
	}
	
	fclk    = 8000000;
	fpllout = 0;
	nout    = 0;
	rout    = 0;
	qout    = 2;
	
	for (r = 2; r <= 8; r += 2)
	{
	    n = (sysclk * r) / fclk;
	    
	    fvco = fclk * n;
	    
	    if ((n >= 8) && (n <= 86) && (fvco <= 344000000) && (fvco >= 96000000))
	    {
		fpll = fvco / r;
		
		/* Prefer lower N,R pairs for lower PLL current. */
		if (fpllout < fpll)
		{
		    fpllout = fpll;
		    
		    nout = n;
		    rout = r;
		}
	    }
	}
	
	sysclk = fpllout;
    }

    if      (hclk >= sysclk)         { hclk = sysclk;         hpre = RCC_CFGR_HPRE_DIV1;   }
    else if (hclk >= (sysclk / 2))   { hclk = (sysclk / 2);   hpre = RCC_CFGR_HPRE_DIV2;   }
    else if (hclk >= (sysclk / 4))   { hclk = (sysclk / 4);   hpre = RCC_CFGR_HPRE_DIV4;   }
    else if (hclk >= (sysclk / 8))   { hclk = (sysclk / 8);   hpre = RCC_CFGR_HPRE_DIV8;   }
    else if (hclk >= (sysclk / 16))  { hclk = (sysclk / 16);  hpre = RCC_CFGR_HPRE_DIV16;  }
    else if (hclk >= (sysclk / 64))  { hclk = (sysclk / 64);  hpre = RCC_CFGR_HPRE_DIV64;  }
    else if (hclk >= (sysclk / 128)) { hclk = (sysclk / 128); hpre = RCC_CFGR_HPRE_DIV128; }
    else if (hclk >= (sysclk / 256)) { hclk = (sysclk / 256); hpre = RCC_CFGR_HPRE_DIV256; }
    else                             { hclk = (sysclk / 512); hpre = RCC_CFGR_HPRE_DIV512; }
    
    if      (pclk1 >= hclk)       { pclk1 = hclk;        ppre1 = RCC_CFGR_PPRE1_DIV1;  }
    else if (pclk1 >= (hclk / 2)) { pclk1 = (hclk / 2);  ppre1 = RCC_CFGR_PPRE1_DIV2;  }
    else if (pclk1 >= (hclk / 4)) { pclk1 = (hclk / 4);  ppre1 = RCC_CFGR_PPRE1_DIV4;  }
    else if (pclk1 >= (hclk / 8)) { pclk1 = (hclk / 8);  ppre1 = RCC_CFGR_PPRE1_DIV8;  }
    else                          { pclk1 = (hclk / 16); ppre1 = RCC_CFGR_PPRE1_DIV16; }
    
    if      (pclk2 >= hclk)       { pclk2 = hclk;        ppre2 = RCC_CFGR_PPRE2_DIV1;  }
    else if (pclk2 >= (hclk / 2)) { pclk2 = (hclk / 2);  ppre2 = RCC_CFGR_PPRE2_DIV2;  }
    else if (pclk2 >= (hclk / 4)) { pclk2 = (hclk / 4);  ppre2 = RCC_CFGR_PPRE2_DIV4;  }
    else if (pclk2 >= (hclk / 8)) { pclk2 = (hclk / 8);  ppre2 = RCC_CFGR_PPRE2_DIV8;  }
    else                          { pclk2 = (hclk / 16); ppre2 = RCC_CFGR_PPRE2_DIV16; }
    
    /* #### Add code to prepare the clock switch, and if it fails cancel it.
     */

    /* First switch to HSI as system clock.
     */

    apb1enr1 = RCC->APB1ENR1;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
    }
    
    /* Select Range 1 to switch clocks */
    
    if (PWR->CR1 & PWR_CR1_LPR)
    {
	PWR->CR1 &= ~PWR_CR1_LPR;
	
	while (PWR->SR2 & PWR_SR2_REGLPF)
	{
	}
    }
    
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    
    while (PWR->SR2 & PWR_SR2_VOSF)
    {
    }
    
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS;
    
    /* Make sure HSI is on, switch to HSI as clock source */
    RCC->CR |= RCC_CR_HSION;
    
    while (!(RCC->CR & RCC_CR_HSIRDY))
    {
    }
    
    /* Select the HSI as system clock source */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    
    /* Wait till the main HSI is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    {
    }
    
    SystemCoreClock = 16000000;
    
#if defined(STM32L476xx)
    /* Disable PLL/PLLSAI1/PLLSAI2 */
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLSAI1ON | RCC_CR_PLLSAI2ON);
    
    while (RCC->CR & (RCC_CR_PLLRDY | RCC_CR_PLLSAI1RDY | RCC_CR_PLLSAI2RDY))
    {
    }
#else
    /* Disable PLL/PLLSAI1 */
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLSAI1ON);
    
    while (RCC->CR & (RCC_CR_PLLRDY | RCC_CR_PLLSAI1RDY))
    {
    }
#endif
    
    /* Set HCLK/PCLK1/PCLK2 prescalers */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) | (hpre | ppre1 | ppre2);
    
    
    if (hclk <= 24000000)
    {
	/* Range 2, use MSI */
	
	/* Disable HSE */
	if (stm32l4_system_device.hseclk)
	{
	    RCC->CR &= ~RCC_CR_HSEON;
	    
	    while (RCC->CR & RCC_CR_HSERDY)
	    {
	    }
	}
	
	if (!(RCC->CR & RCC_CR_MSION))
	{
	    RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | msirange | RCC_CR_MSIRGSEL | RCC_CR_MSION;
	    
	    while (!(RCC->CR & RCC_CR_MSIRDY))
	    {
	    }
	}
	else
	{
	    RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | msirange | RCC_CR_MSIRGSEL;
	    
	    armv7m_clock_spin(500);
	}

	if (stm32l4_system_device.lseclk)
	{
	    /* Enable the MSI PLL */
	    RCC->CR |= RCC_CR_MSIPLLEN;
	}
	
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_MSI;
	
	/* Wait till the main MSI is used as system clock source */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI)
	{
	}
	
	/* Configure the main PLL */
	RCC->PLLCFGR = (((mout -1) << 4) |
			(nout << 8) |
			(((rout >> 1) -1) << 25) |
			(((qout >> 1) -1) << 21) |
			RCC_PLLCFGR_PLLSRC_MSI |
			RCC_PLLCFGR_PLLQEN);
	
	if (stm32l4_system_device.clk48)
	{
	    /* Enable the main PLL */
	    RCC->CR |= RCC_CR_PLLON;
	    
	    /* Wait till the main PLL is ready */
	    while((RCC->CR & RCC_CR_PLLRDY) == 0)
	    {
	    }
	}
	else
	{
	    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_1;
	    
	    while (PWR->SR2 & PWR_SR2_VOSF)
	    {
	    }

	    /* Entry LPRUN is sysclk is low enough */
	    if (sysclk <= 2000000)
	    {
		PWR->CR1 |= PWR_CR1_LPR;
		
		while (!(PWR->SR2 & PWR_SR2_REGLPF))
		{
		}
	    }
	}

	/* Switch CLK48SEL to PLL48M1CLK */
	RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_CLK48SEL) | (RCC_CCIPR_CLK48SEL_1);
    }
    else
    {
	/* Range 1, use HSE/PLL or MSI/PLL */
	
	/* Be careful not to change HSE/MSI settings which might be shared
	 * with PLLSAI1/PLLSAI2.
	 */
	if (stm32l4_system_device.hseclk)
	{
	    if (!(RCC->CR & RCC_CR_HSEON))
	    {
		RCC->CR |= RCC_CR_HSEON;
		
		while (!(RCC->CR & RCC_CR_HSERDY))
		{
		}
	    }
	}
	
	if (!stm32l4_system_device.hseclk || stm32l4_system_device.clk48)
	{
	    if (!(RCC->CR & RCC_CR_MSION))
	    {
		RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_11 | RCC_CR_MSIRGSEL | RCC_CR_MSION;
		
		while (!(RCC->CR & RCC_CR_MSIRDY))
		{
		}
	    }
	    else
	    {
		RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_11 | RCC_CR_MSIRGSEL;
		
		armv7m_clock_spin(500);
	    }
	    
	    if (stm32l4_system_device.lseclk)
	    {
		/* Enable the MSI PLL */
		RCC->CR |= RCC_CR_MSIPLLEN;
	    }
	}
	else
	{
	    /* ERRATA 2.1.15. WAR: Switch MSI to <= 16MHz before turing off */
	    RCC->CR = (RCC->CR & ~(RCC_CR_MSIRANGE | RCC_CR_MSIPLLEN)) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL;
	    
	    armv7m_clock_spin(500);
	    
	    RCC->CR &= ~RCC_CR_MSION;
	    
	    while (RCC->CR & RCC_CR_MSIRDY)
	    {
	    }
	}
	
	/* Configure the main PLL */
	RCC->PLLCFGR = (((mout -1) << 4) |
			(nout << 8) |
			(((rout >> 1) -1) << 25) |
			(((qout >> 1) -1) << 21) |
			(stm32l4_system_device.hseclk ? RCC_PLLCFGR_PLLSRC_HSE : RCC_PLLCFGR_PLLSRC_MSI) |
			RCC_PLLCFGR_PLLREN);
	
	/* Enable the main PLL */
	RCC->CR |= RCC_CR_PLLON;
	
	/* Wait till the main PLL is ready */
	while((RCC->CR & RCC_CR_PLLRDY) == 0)
	{
	}
	
	/* Select the main PLL as system clock source */
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
	
	/* Wait till the main PLL is used as system clock source */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
	{
	}

	/* Switch CLK48SEL to MSI */
	RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_CLK48SEL) | (RCC_CCIPR_CLK48SEL_0 | RCC_CCIPR_CLK48SEL_1);
    }

    SystemCoreClock = hclk;

    if (!stm32l4_system_device.clk48 && (hclk <= 24000000))
    {
	if      (hclk <=  6000000) { latency = FLASH_ACR_LATENCY_0WS; }
	else if (hclk <= 12000000) { latency = FLASH_ACR_LATENCY_1WS; }
	else if (hclk <= 18000000) { latency = FLASH_ACR_LATENCY_2WS; }
	else                       { latency = FLASH_ACR_LATENCY_3WS; }
    }
    else
    {
	if      (hclk <= 16000000) { latency = FLASH_ACR_LATENCY_0WS; }
	else if (hclk <= 32000000) { latency = FLASH_ACR_LATENCY_1WS; }
	else if (hclk <= 48000000) { latency = FLASH_ACR_LATENCY_2WS; }
	else if (hclk <= 64000000) { latency = FLASH_ACR_LATENCY_3WS; }
	else                       { latency = FLASH_ACR_LATENCY_4WS; }
    }

    if (hclk <= 24000000)
    {
	FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | latency;
    }
    else
    {
	FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | latency;
    }

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
    }

    stm32l4_system_device.sysclk = sysclk;
    stm32l4_system_device.hclk = hclk;
    stm32l4_system_device.pclk1 = pclk1;
    stm32l4_system_device.pclk2 = pclk2;

    /* #### Add code to complete the switch */

    return true;
}

bool stm32l4_system_clk48_enable(void)
{
    uint32_t apb1enr1, latency;

    if (!stm32l4_system_device.lseclk || (stm32l4_system_device.hclk < 16000000))
    {
	return false;
    }

    if (!stm32l4_system_device.clk48)
    {
	if (stm32l4_system_device.hclk <= 24000000)
	{
	    /* Switch to Range 1 */

	    apb1enr1 = RCC->APB1ENR1;
	    
	    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
	    {
		armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
	    }

	    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    
	    while (PWR->SR2 & PWR_SR2_VOSF)
	    {
	    }

	    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
	    {
		armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
	    }

	    /* Adjust FLASH latency */
	    if   (stm32l4_system_device.hclk <= 16000000) { latency = FLASH_ACR_LATENCY_0WS; }
	    else                                          { latency = FLASH_ACR_LATENCY_1WS; }

	    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | latency;

	    /* Enable main PLL to produce PLL48M1CLK */
	    RCC->CR |= RCC_CR_PLLON;
	
	    /* Wait till the main PLL is ready */
	    while((RCC->CR & RCC_CR_PLLRDY) == 0)
	    {
	    }
	}
	else
	{
	    if (stm32l4_system_device.hseclk)
	    {
		RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_11 | RCC_CR_MSIRGSEL | RCC_CR_MSION;
	    
		while (!(RCC->CR & RCC_CR_MSIRDY))
		{
		}

		if (stm32l4_system_device.lseclk)
		{
		    /* Enable the MSI PLL */
		    RCC->CR |= RCC_CR_MSIPLLEN;
		}
	    }
	}

	stm32l4_system_device.clk48 = true;
    }

    return true;
}

bool stm32l4_system_clk48_disable(void)
{
    uint32_t apb1enr1, latency;

    if (!stm32l4_system_device.lseclk || (stm32l4_system_device.hclk < 16000000))
    {
	return false;
    }

    if (stm32l4_system_device.clk48)
    {
	if (stm32l4_system_device.hclk <= 24000000)
	{
	    /* Disable PLL */
	    RCC->CR &= ~RCC_CR_PLLON;
    
	    while (RCC->CR & RCC_CR_PLLRDY)
	    {
	    }
	    
	    /* Adjust FLASH latency */
	    if      (stm32l4_system_device.hclk <=  6000000) { latency = FLASH_ACR_LATENCY_0WS; }
	    else if (stm32l4_system_device.hclk <= 12000000) { latency = FLASH_ACR_LATENCY_1WS; }
	    else if (stm32l4_system_device.hclk <= 18000000) { latency = FLASH_ACR_LATENCY_2WS; }
	    else                                             { latency = FLASH_ACR_LATENCY_3WS; }

	    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | latency;

	    /* Switch to Range 2 */
	    apb1enr1 = RCC->APB1ENR1;
	    
	    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
	    {
		armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
	    }

	    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_1;
	    
	    while (PWR->SR2 & PWR_SR2_VOSF)
	    {
	    }

	    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
	    {
		armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
	    }
	}
	else
	{
	    if (stm32l4_system_device.hseclk)
	    {
		/* ERRATA 2.1.15. WAR: Switch MSI to <= 16MHz before turing off */
		RCC->CR = (RCC->CR & ~(RCC_CR_MSIRANGE | RCC_CR_MSIPLLEN)) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL;
		
		armv7m_clock_spin(500);
		
		RCC->CR &= ~RCC_CR_MSION;
		
		while (RCC->CR & RCC_CR_MSIRDY)
		{
		}
	    }
	}

	stm32l4_system_device.clk48 = false;
    }

    return true;
}

uint32_t stm32l4_system_lseclk(void)
{
    return stm32l4_system_device.lseclk;
}

uint32_t stm32l4_system_hseclk(void)
{
    return stm32l4_system_device.hseclk;
}

uint32_t stm32l4_system_sysclk(void)
{
    return stm32l4_system_device.sysclk;
}

uint32_t stm32l4_system_hclk(void)
{
    return stm32l4_system_device.hclk;
}

uint32_t stm32l4_system_pclk1(void)
{
    return stm32l4_system_device.pclk1;
}

uint32_t stm32l4_system_pclk2(void)
{
    return stm32l4_system_device.pclk2;
}

bool stm32l4_system_suspend(void)
{
    uint32_t apb1enr1;

    /* #### Add code here that calls STOP_ENTER, and if that fails calls out STOP_CANCEL */
       
    /* Disable FLASH in sleep/deepsleep */
    FLASH->ACR |= FLASH_ACR_SLEEP_PD;

    if (stm32l4_system_device.hclk <= 24000000)
    {
	/* Select MSI as wakeup clock */

	RCC->CR |= RCC_CR_HSIASFS;
	RCC->CFGR &= ~RCC_CFGR_STOPWUCK;
    }
    else
    {
	/* There are a few ERRATA which make it reasonable to simply 
	 * switch to HSI and turn off PLL/PLLSAI1/PLLSAI2/MSE/MSI manually.
	 */

	/* Select the HSI as system clock source */
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
	
	/* Wait till the main HSI is used as system clock source */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
	{
	}
	
	/* Select MSI @ 4MHz as system clock source.
	 */
	
	if (RCC->CR & RCC_CR_MSION)
	{
	    /* ERRATA 2.1.15. WAR: Switch MSI to <= 16MHz before turing off */
	    RCC->CR = (RCC->CR & ~(RCC_CR_MSIRANGE | RCC_CR_MSIPLLEN)) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL;

	    armv7m_clock_spin(500);
	    
	    RCC->CR &= ~RCC_CR_MSION;
	    
	    while (RCC->CR & RCC_CR_MSIRDY)
	    {
	    }
	}

#if defined(STM32L476xx)
	/* Disable PLL/PLLSAI1/PLLSAI2 */
	RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLSAI1ON | RCC_CR_PLLSAI2ON);
	
	while (RCC->CR & (RCC_CR_PLLRDY | RCC_CR_PLLSAI1RDY | RCC_CR_PLLSAI2RDY))
	{
	}
#else
	/* Disable PLL/PLLSAI1 */
	RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLSAI1ON);
	
	while (RCC->CR & (RCC_CR_PLLRDY | RCC_CR_PLLSAI1RDY))
	{
	}
#endif	
	/* Disable HSE */
	if (stm32l4_system_device.hseclk)
	{
	    RCC->CR &= ~RCC_CR_HSEON;
	    
	    while (RCC->CR & RCC_CR_HSERDY)
	    {
	    }
	}

	/* Select HSI as wakeup clock */
	RCC->CFGR |= RCC_CFGR_STOPWUCK;
    }

    /* Set up STOP1 */

    apb1enr1 = RCC->APB1ENR1;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
    }
    
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | PWR_CR1_LPMS_STOP1;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
    }

    return true;
}

bool stm32l4_system_restore(void)
{
    if (stm32l4_system_device.hclk <= 24000000)
    {
    }
    else
    {
	if (stm32l4_system_device.hseclk)
	{
	    RCC->CR |= RCC_CR_HSEON;
	    
	    while (!(RCC->CR & RCC_CR_HSERDY))
	    {
	    }
	}

	if (!stm32l4_system_device.hseclk || stm32l4_system_device.clk48)
	{
	    RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_11 | RCC_CR_MSIRGSEL | RCC_CR_MSION;
		    
	    while (!(RCC->CR & RCC_CR_MSIRDY))
	    {
	    }
		    
	    if (stm32l4_system_device.lseclk)
	    {
		/* Enable the MSI PLL */
		RCC->CR |= RCC_CR_MSIPLLEN;
	    }
	}

	/* Enable the main PLL */
	RCC->CR |= RCC_CR_PLLON;
	
	/* Wait till the main PLL is ready */
	while((RCC->CR & RCC_CR_PLLRDY) == 0)
	{
	}
	
	/* Select the main PLL as system clock source */
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
	
	/* Wait till the main PLL is used as system clock source */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
	{
	}
    }

    /* Enable FLASH in sleep/deepsleep */
    FLASH->ACR &= ~FLASH_ACR_SLEEP_PD;

    /* #### Add code here that calls STOP_LEAVE */

    return true;
}

bool stm32l4_system_stop(void)
{
    if (!stm32l4_system_suspend())
    {
	return false;
    }

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __SEV();
    __WFE();
    __WFE();

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    return stm32l4_system_restore();
}

bool stm32l4_system_standby(void)
{
    uint32_t apb1enr1;

    /* #### Add code here that calls STANDBY_ENTER, and if that fails calls out STANDBY_CANCEL */

    /* ERRATA 2.1.15. WAR: Switch MSI to 4MHz before entering low power mode */

    stm32l4_system_msi4_sysclk();

    apb1enr1 = RCC->APB1ENR1;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
    }
    
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | PWR_CR1_LPMS_STANDBY;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
    }

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __WFI();

    return true;
}

bool stm32l4_system_shutdown(void)
{
    uint32_t apb1enr1;

    /* #### Add code here that calls SHUTDOWN_ENTER, and if that fails calls out SHUTDOWN_CANCEL */

    /* ERRATA 2.1.15. WAR: Switch MSI to 4MHz before entering low power mode */

    stm32l4_system_msi4_sysclk();

    apb1enr1 = RCC->APB1ENR1;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
    }
    
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | PWR_CR1_LPMS_SHUTDOWN;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
    }

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __WFI();

    return true;
}
