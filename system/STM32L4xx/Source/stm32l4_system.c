/*
 * Copyright (c) 2016-2017 Thomas Roell.  All rights reserved.
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
#include "stm32l4_rtc.h"

uint32_t SystemCoreClock = 48000000;

extern uint32_t __rodata2_start__;
extern uint32_t __rodata2_end__;
extern uint32_t __backup_start__;
extern uint32_t __backup_end__;
extern uint32_t __etextbkp;

typedef struct _stm32l4_system_device_t {
    uint16_t                  reset;
    uint16_t                  wakeup;
    uint32_t                  lseclk;
    uint32_t                  hseclk;
    uint32_t                  sysclk;
    uint32_t                  fclk;
    uint32_t                  hclk;
    uint32_t                  pclk1;
    uint32_t                  pclk2;
    uint32_t                  saiclk; 
    uint8_t                   clk48;
    uint8_t                   mco;
    uint8_t                   lsco;
    uint8_t                   lsi;
    uint8_t                   hsi16;
#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    uint8_t                   hsi48;
#endif
    volatile uint32_t         lock[SYSTEM_LOCK_COUNT];
    volatile uint32_t         event[SYSTEM_EVENT_COUNT];
    stm32l4_system_callback_t callback[SYSTEM_NOTIFY_COUNT];
    void                      *context[SYSTEM_NOTIFY_COUNT];
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
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOC */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOD */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOF */
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOG */
#endif
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L496xx)
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_GPIOI */
#endif
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_ADC */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_DAC */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2RSTR,  /* SYSTEM_PERIPH_USB */
#else
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_USB */
#endif
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_USART1 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_UART4 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_UART5 */
#endif
    &RCC->APB1RSTR2, /* SYSTEM_PERIPH_LPUART1 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_I2C2 */
#endif
#if defined(STM32L496xx)
    &RCC->APB1RSTR2, /* SYSTEM_PERIPH_I2C4 */
#endif
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_SPI2 */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_SPI3 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_CAN1 */
#if defined(STM32L496xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_CAN2 */
#endif
    &RCC->AHB3RSTR,  /* SYSTEM_PERIPH_QSPI */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SDMMC1 */
#endif
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_SAI2 */
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_DFSDM1 */
#endif
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM1 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM3 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM4 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM5 */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM6 */
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM8 */
#endif
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM15 */
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2RSTR,  /* SYSTEM_PERIPH_TIM17 */
#endif
    &RCC->APB1RSTR1, /* SYSTEM_PERIPH_LPTIM1 */
    &RCC->APB1RSTR2, /* SYSTEM_PERIPH_LPTIM2 */
};

static uint32_t const stm32l4_system_xlate_RSTMSK[SYSTEM_PERIPH_COUNT] = {
    RCC_AHB1RSTR_FLASHRST,    /* SYSTEM_PERIPH_FLASH */
    0,                        /* SYSTEM_PERIPH_SRAM1 */
    0,                        /* SYSTEM_PERIPH_SRAM2 */
    RCC_AHB1RSTR_DMA1RST,     /* SYSTEM_PERIPH_DMA1 */
    RCC_AHB1RSTR_DMA2RST,     /* SYSTEM_PERIPH_DMA2 */
    RCC_AHB2RSTR_GPIOARST,    /* SYSTEM_PERIPH_GPIOA */
    RCC_AHB2RSTR_GPIOBRST,    /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2RSTR_GPIOCRST,    /* SYSTEM_PERIPH_GPIOC */
    RCC_AHB2RSTR_GPIODRST,    /* SYSTEM_PERIPH_GPIOD */
    RCC_AHB2RSTR_GPIOERST,    /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2RSTR_GPIOFRST,    /* SYSTEM_PERIPH_GPIOF */
    RCC_AHB2RSTR_GPIOGRST,    /* SYSTEM_PERIPH_GPIOG */
#endif
    RCC_AHB2RSTR_GPIOHRST,    /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L496xx)
    RCC_AHB2RSTR_GPIOIRST,    /* SYSTEM_PERIPH_GPIOI */
#endif
    RCC_AHB2RSTR_ADCRST,      /* SYSTEM_PERIPH_ADC */
    RCC_APB1RSTR1_DAC1RST,    /* SYSTEM_PERIPH_DAC */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2RSTR_OTGFSRST,    /* SYSTEM_PERIPH_USB */
#else
    RCC_APB1RSTR1_USBFSRST,   /* SYSTEM_PERIPH_USB */
#endif
    RCC_APB2RSTR_USART1RST,   /* SYSTEM_PERIPH_USART1 */
    RCC_APB1RSTR1_USART2RST,  /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1RSTR1_USART3RST,  /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1RSTR1_UART4RST,   /* SYSTEM_PERIPH_UART4 */
    RCC_APB1RSTR1_UART5RST,   /* SYSTEM_PERIPH_UART5 */
#endif
    RCC_APB1RSTR2_LPUART1RST, /* SYSTEM_PERIPH_LPUART1 */
    RCC_APB1RSTR1_I2C1RST,    /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1RSTR1_I2C2RST,    /* SYSTEM_PERIPH_I2C2 */
#endif
    RCC_APB1RSTR1_I2C3RST,    /* SYSTEM_PERIPH_I2C3 */
#if defined(STM32L496xx)
    RCC_APB1RSTR2_I2C4RST,    /* SYSTEM_PERIPH_I2C4 */
#endif
    RCC_APB2RSTR_SPI1RST,     /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1RSTR1_SPI2RST,    /* SYSTEM_PERIPH_SPI2 */
#endif
    RCC_APB1RSTR1_SPI3RST,    /* SYSTEM_PERIPH_SPI3 */
    RCC_APB1RSTR1_CAN1RST,    /* SYSTEM_PERIPH_CAN1 */
#if defined(STM32L496xx)
    RCC_APB1RSTR1_CAN2RST,    /* SYSTEM_PERIPH_CAN2 */
#endif
    RCC_AHB3RSTR_QSPIRST,     /* SYSTEM_PERIPH_QSPI */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2RSTR_SDMMC1RST,   /* SYSTEM_PERIPH_SDMMC1 */
#endif
    RCC_APB2RSTR_SAI1RST,     /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2RSTR_SAI2RST,     /* SYSTEM_PERIPH_SAI2 */
    RCC_APB2RSTR_DFSDM1RST,   /* SYSTEM_PERIPH_DFSDM1 */
#endif
    RCC_APB2RSTR_TIM1RST,     /* SYSTEM_PERIPH_TIM1 */
    RCC_APB1RSTR1_TIM2RST,    /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1RSTR1_TIM3RST,    /* SYSTEM_PERIPH_TIM3 */
    RCC_APB1RSTR1_TIM4RST,    /* SYSTEM_PERIPH_TIM4 */
    RCC_APB1RSTR1_TIM5RST,    /* SYSTEM_PERIPH_TIM5 */
#endif
    RCC_APB1RSTR1_TIM6RST,    /* SYSTEM_PERIPH_TIM6 */
    RCC_APB1RSTR1_TIM7RST,    /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2RSTR_TIM8RST,     /* SYSTEM_PERIPH_TIM8 */
#endif
    RCC_APB2RSTR_TIM15RST,    /* SYSTEM_PERIPH_TIM15 */
    RCC_APB2RSTR_TIM16RST,    /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2RSTR_TIM17RST,    /* SYSTEM_PERIPH_TIM17 */
#endif
    RCC_APB1RSTR1_LPTIM1RST,  /* SYSTEM_PERIPH_LPTIM1 */
    RCC_APB1RSTR2_LPTIM2RST,  /* SYSTEM_PERIPH_LPTIM2 */
};

static volatile uint32_t * const stm32l4_system_xlate_ENR[SYSTEM_PERIPH_COUNT] = {
    &RCC->AHB1ENR,  /* SYSTEM_PERIPH_FLASH */
    NULL,           /* SYSTEM_PERIPH_SRAM1 */
    NULL,           /* SYSTEM_PERIPH_SRAM2 */
    &RCC->AHB1ENR,  /* SYSTEM_PERIPH_DMA1 */
    &RCC->AHB1ENR,  /* SYSTEM_PERIPH_DMA2 */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOA */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOC */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOD */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOF */
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOG */
#endif
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L496xx)
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_GPIOI */
#endif
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_ADC */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_DAC */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2ENR,  /* SYSTEM_PERIPH_USB */
#else
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_USB */
#endif
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_USART1 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_UART4 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_UART5 */
#endif
    &RCC->APB1ENR2, /* SYSTEM_PERIPH_LPUART1 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_I2C2 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_I2C3 */
#if defined(STM32L496xx)
    &RCC->APB1ENR2, /* SYSTEM_PERIPH_I2C4 */
#endif
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_SPI2 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_SPI3 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_CAN1 */
#if defined(STM32L496xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_CAN2 */
#endif
    &RCC->AHB3ENR,  /* SYSTEM_PERIPH_QSPI */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SDMMC1 */
#endif
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_SAI2 */
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_DFSDM1 */
#endif
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM1 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM3 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM4 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM5 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM6 */
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM8 */
#endif
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM15 */
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2ENR,  /* SYSTEM_PERIPH_TIM17 */
#endif
    &RCC->APB1ENR1, /* SYSTEM_PERIPH_LPTIM1 */
    &RCC->APB1ENR2, /* SYSTEM_PERIPH_LPTIM2 */
};

static uint32_t const stm32l4_system_xlate_ENMSK[SYSTEM_PERIPH_COUNT] = {
    RCC_AHB1ENR_FLASHEN,    /* SYSTEM_PERIPH_FLASH */
    0,                      /* SYSTEM_PERIPH_SRAM1 */
    0,                      /* SYSTEM_PERIPH_SRAM2 */
    RCC_AHB1ENR_DMA1EN,     /* SYSTEM_PERIPH_DMA1 */
    RCC_AHB1ENR_DMA2EN,     /* SYSTEM_PERIPH_DMA2 */
    RCC_AHB2ENR_GPIOAEN,    /* SYSTEM_PERIPH_GPIOA */
    RCC_AHB2ENR_GPIOBEN,    /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2ENR_GPIOCEN,    /* SYSTEM_PERIPH_GPIOC */
    RCC_AHB2ENR_GPIODEN,    /* SYSTEM_PERIPH_GPIOD */
    RCC_AHB2ENR_GPIOEEN,    /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2ENR_GPIOFEN,    /* SYSTEM_PERIPH_GPIOF */
    RCC_AHB2ENR_GPIOGEN,    /* SYSTEM_PERIPH_GPIOG */
#endif
    RCC_AHB2ENR_GPIOHEN,    /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L496xx)
    RCC_AHB2ENR_GPIOIEN,    /* SYSTEM_PERIPH_GPIOI */
#endif
    RCC_AHB2ENR_ADCEN,      /* SYSTEM_PERIPH_ADC */
    RCC_APB1ENR1_DAC1EN,    /* SYSTEM_PERIPH_DAC */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2ENR_OTGFSEN,    /* SYSTEM_PERIPH_USB */
#else
    RCC_APB1ENR1_USBFSEN,   /* SYSTEM_PERIPH_USB */
#endif
    RCC_APB2ENR_USART1EN,   /* SYSTEM_PERIPH_USART1 */
    RCC_APB1ENR1_USART2EN,  /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1ENR1_USART3EN,  /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1ENR1_UART4EN,   /* SYSTEM_PERIPH_UART4 */
    RCC_APB1ENR1_UART5EN,   /* SYSTEM_PERIPH_UART5 */
#endif
    RCC_APB1ENR2_LPUART1EN, /* SYSTEM_PERIPH_LPUART1 */
    RCC_APB1ENR1_I2C1EN,    /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1ENR1_I2C2EN,    /* SYSTEM_PERIPH_I2C2 */
#endif
    RCC_APB1ENR1_I2C3EN,    /* SYSTEM_PERIPH_I2C3 */
#if defined(STM32L496xx)
    RCC_APB1ENR2_I2C4EN,    /* SYSTEM_PERIPH_I2C4 */
#endif
    RCC_APB2ENR_SPI1EN,     /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1ENR1_SPI2EN,    /* SYSTEM_PERIPH_SPI2 */
#endif
    RCC_APB1ENR1_SPI3EN,    /* SYSTEM_PERIPH_SPI3 */
    RCC_APB1ENR1_CAN1EN,    /* SYSTEM_PERIPH_CAN1 */
#if defined(STM32L496xx)
    RCC_APB1ENR1_CAN2EN,    /* SYSTEM_PERIPH_CAN2 */
#endif
    RCC_AHB3ENR_QSPIEN,     /* SYSTEM_PERIPH_QSPI */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2ENR_SDMMC1EN,   /* SYSTEM_PERIPH_SDMMC1 */
#endif
    RCC_APB2ENR_SAI1EN,     /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2ENR_SAI2EN,     /* SYSTEM_PERIPH_SAI2 */
    RCC_APB2ENR_DFSDM1EN,   /* SYSTEM_PERIPH_DFSDM1 */
#endif
    RCC_APB2ENR_TIM1EN,     /* SYSTEM_PERIPH_TIM1 */
    RCC_APB1ENR1_TIM2EN,    /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1ENR1_TIM3EN,    /* SYSTEM_PERIPH_TIM3 */
    RCC_APB1ENR1_TIM4EN,    /* SYSTEM_PERIPH_TIM4 */
    RCC_APB1ENR1_TIM5EN,    /* SYSTEM_PERIPH_TIM5 */
#endif
    RCC_APB1ENR1_TIM6EN,    /* SYSTEM_PERIPH_TIM6 */
    RCC_APB1ENR1_TIM7EN,    /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2ENR_TIM8EN,     /* SYSTEM_PERIPH_TIM8 */
#endif
    RCC_APB2ENR_TIM15EN,    /* SYSTEM_PERIPH_TIM15 */
    RCC_APB2ENR_TIM16EN,    /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2ENR_TIM17EN,    /* SYSTEM_PERIPH_TIM17 */
#endif
    RCC_APB1ENR1_LPTIM1EN,  /* SYSTEM_PERIPH_LPTIM1 */
    RCC_APB1ENR2_LPTIM2EN,  /* SYSTEM_PERIPH_LPTIM2 */
};

static volatile uint32_t * const stm32l4_system_xlate_SMENR[SYSTEM_PERIPH_COUNT] = {
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_FLASH */
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_SRAM1 */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_SRAM2 */
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_DMA1 */
    &RCC->AHB1SMENR,  /* SYSTEM_PERIPH_DMA2 */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOA */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOC */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOD */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOF */
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOG */
#endif
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L496xx)
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_GPIOI */
#endif
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_ADC */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_DAC */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->AHB2SMENR,  /* SYSTEM_PERIPH_USB */
#else
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_USB */
#endif
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_USART1 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_UART4 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_UART5 */
#endif
    &RCC->APB1SMENR2, /* SYSTEM_PERIPH_LPUART1 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_I2C2 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_I2C3 */
#if defined(STM32L496xx)
    &RCC->APB1SMENR2, /* SYSTEM_PERIPH_I2C4 */
#endif
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_SPI2 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_SPI3 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_CAN1 */
#if defined(STM32L496xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_CAN2 */
#endif
    &RCC->AHB3SMENR,  /* SYSTEM_PERIPH_QSPI */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SDMMC1 */
#endif
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_SAI2 */
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_DFSDM1 */
#endif
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM1 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM3 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM4 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM5 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM6 */
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM8 */
#endif
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM15 */
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    &RCC->APB2SMENR,  /* SYSTEM_PERIPH_TIM17 */
#endif
    &RCC->APB1SMENR1, /* SYSTEM_PERIPH_LPTIM1 */
    &RCC->APB1SMENR2, /* SYSTEM_PERIPH_LPTIM2 */
};

static uint32_t const stm32l4_system_xlate_SMENMSK[SYSTEM_PERIPH_COUNT] = {
    RCC_AHB1SMENR_FLASHSMEN,    /* SYSTEM_PERIPH_FLASH */
    RCC_AHB1SMENR_SRAM1SMEN,    /* SYSTEM_PERIPH_SRAM1 */
    RCC_AHB2SMENR_SRAM2SMEN,    /* SYSTEM_PERIPH_SRAM2 */
    RCC_AHB1SMENR_DMA1SMEN,     /* SYSTEM_PERIPH_DMA1 */
    RCC_AHB1SMENR_DMA2SMEN,     /* SYSTEM_PERIPH_DMA2 */
    RCC_AHB2SMENR_GPIOASMEN,    /* SYSTEM_PERIPH_GPIOA */
    RCC_AHB2SMENR_GPIOBSMEN,    /* SYSTEM_PERIPH_GPIOB */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2SMENR_GPIOCSMEN,    /* SYSTEM_PERIPH_GPIOC */
    RCC_AHB2SMENR_GPIODSMEN,    /* SYSTEM_PERIPH_GPIOD */
    RCC_AHB2SMENR_GPIOESMEN,    /* SYSTEM_PERIPH_GPIOE */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2SMENR_GPIOFSMEN,    /* SYSTEM_PERIPH_GPIOF */
    RCC_AHB2SMENR_GPIOGSMEN,    /* SYSTEM_PERIPH_GPIOG */
#endif
    RCC_AHB2SMENR_GPIOHSMEN,    /* SYSTEM_PERIPH_GPIOH */
#if defined(STM32L496xx)
    RCC_AHB2SMENR_GPIOISMEN,    /* SYSTEM_PERIPH_GPIOI */
#endif
    RCC_AHB2SMENR_ADCSMEN,      /* SYSTEM_PERIPH_ADC */
    RCC_APB1SMENR1_DAC1SMEN,    /* SYSTEM_PERIPH_DAC */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_AHB2SMENR_OTGFSSMEN,    /* SYSTEM_PERIPH_USB */
#else
    RCC_APB1SMENR1_USBFSSMEN,   /* SYSTEM_PERIPH_USB */
#endif
    RCC_APB2SMENR_USART1SMEN,   /* SYSTEM_PERIPH_USART1 */
    RCC_APB1SMENR1_USART2SMEN,  /* SYSTEM_PERIPH_USART2 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1SMENR1_USART3SMEN,  /* SYSTEM_PERIPH_USART3 */
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1SMENR1_UART4SMEN,   /* SYSTEM_PERIPH_UART4 */
    RCC_APB1SMENR1_UART5SMEN,   /* SYSTEM_PERIPH_UART5 */
#endif
    RCC_APB1SMENR2_LPUART1SMEN, /* SYSTEM_PERIPH_LPUART1 */
    RCC_APB1SMENR1_I2C1SMEN,    /* SYSTEM_PERIPH_I2C1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1SMENR1_I2C2SMEN,    /* SYSTEM_PERIPH_I2C2 */
#endif
    RCC_APB1SMENR1_I2C3SMEN,    /* SYSTEM_PERIPH_I2C3 */
#if defined(STM32L496xx)
    RCC_APB1SMENR2_I2C4SMEN,    /* SYSTEM_PERIPH_I2C4 */
#endif
    RCC_APB2SMENR_SPI1SMEN,     /* SYSTEM_PERIPH_SPI1 */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1SMENR1_SPI2SMEN,    /* SYSTEM_PERIPH_SPI2 */
#endif
    RCC_APB1SMENR1_SPI3SMEN,    /* SYSTEM_PERIPH_SPI3 */
    RCC_APB1SMENR1_CAN1SMEN,    /* SYSTEM_PERIPH_CAN1 */
#if defined(STM32L496xx)
    RCC_APB1SMENR1_CAN2SMEN,    /* SYSTEM_PERIPH_CAN2 */
#endif
    RCC_AHB3SMENR_QSPISMEN,     /* SYSTEM_PERIPH_QSPI */
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2SMENR_SDMMC1SMEN,   /* SYSTEM_PERIPH_SDMMC1 */
#endif
    RCC_APB2SMENR_SAI1SMEN,     /* SYSTEM_PERIPH_SAI1 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2SMENR_SAI2SMEN,     /* SYSTEM_PERIPH_SAI2 */
    RCC_APB2SMENR_DFSDM1SMEN,   /* SYSTEM_PERIPH_DFSDM1 */
#endif
    RCC_APB2SMENR_TIM1SMEN,     /* SYSTEM_PERIPH_TIM1 */
    RCC_APB1SMENR1_TIM2SMEN,    /* SYSTEM_PERIPH_TIM2 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB1SMENR1_TIM3SMEN,    /* SYSTEM_PERIPH_TIM3 */
    RCC_APB1SMENR1_TIM4SMEN,    /* SYSTEM_PERIPH_TIM4 */
    RCC_APB1SMENR1_TIM5SMEN,    /* SYSTEM_PERIPH_TIM5 */
#endif
    RCC_APB1SMENR1_TIM6SMEN,    /* SYSTEM_PERIPH_TIM6 */
    RCC_APB1SMENR1_TIM7SMEN,    /* SYSTEM_PERIPH_TIM7 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2SMENR_TIM8SMEN,     /* SYSTEM_PERIPH_TIM8 */
#endif
    RCC_APB2SMENR_TIM15SMEN,    /* SYSTEM_PERIPH_TIM15 */
    RCC_APB2SMENR_TIM16SMEN,    /* SYSTEM_PERIPH_TIM16 */
#if defined(STM32L476xx) || defined(STM32L496xx)
    RCC_APB2SMENR_TIM17SMEN,    /* SYSTEM_PERIPH_TIM17 */
#endif
    RCC_APB1SMENR1_LPTIM1SMEN,  /* SYSTEM_PERIPH_LPTIM1 */
    RCC_APB1SMENR2_LPTIM2SMEN,  /* SYSTEM_PERIPH_LPTIM2 */
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

void stm32l4_system_initialize(uint32_t hclk, uint32_t pclk1, uint32_t pclk2, uint32_t lseclk, uint32_t hseclk, uint32_t option)
{
    uint32_t primask, count;
    uint32_t *data_s, *data_e;
    const uint32_t *data_t;

    primask = __get_PRIMASK();

    __disable_irq();

    if (PWR->SR1 & PWR_SR1_SBF)
    {
	stm32l4_system_device.reset = SYSTEM_RESET_STANDBY;
	stm32l4_system_device.wakeup = (PWR->SR1 & PWR_SR1_WUF);
	
	if (PWR->SR1 & PWR_SR1_WUFI)
	{
	    if (RTC->ISR & (RTC_ISR_ALRAF | RTC_ISR_ALRBF))
	    {
		stm32l4_system_device.wakeup |= SYSTEM_WAKEUP_ALARM;
	    }
	    
	    if (RTC->ISR & RTC_ISR_TSF)
	    {
		stm32l4_system_device.wakeup |= SYSTEM_WAKEUP_SYNC;
	    }
	    
	    if (RTC->ISR & RTC_ISR_WUTF)
	    {
		stm32l4_system_device.wakeup |= SYSTEM_WAKEUP_TIMEOUT;
	    }
	    
	    RTC->ISR &= ~(RTC_ISR_ALRAF | RTC_ISR_ALRBF | RTC_ISR_TSF | RTC_ISR_WUTF);
	}
    }
    else
    {
	stm32l4_system_device.reset = SYSTEM_RESET_POWERON;
	stm32l4_system_device.wakeup = 0;
	
	if (RCC->CSR & (RCC_CSR_LPWRRSTF | RCC_CSR_OBLRSTF))
	{
	    stm32l4_system_device.reset = SYSTEM_RESET_OTHER;
	}
	
	else if (RCC->CSR & RCC_CSR_FWRSTF)
	{
	    stm32l4_system_device.reset = SYSTEM_RESET_FIREWALL;
	}
	
	else if (RCC->CSR & RCC_CSR_BORRSTF)
	{
	    stm32l4_system_device.reset = SYSTEM_RESET_BROWNOUT;
	}
	
	else if (RCC->CSR & (RCC_CSR_IWDGRSTF | RCC_CSR_WWDGRSTF))
	{
	    stm32l4_system_device.reset = SYSTEM_RESET_WATCHDOG;
	}
	
	else if (RCC->CSR & RCC_CSR_SFTRSTF)
	{
	    stm32l4_system_device.reset = SYSTEM_RESET_SOFTWARE;
	}
	
	else if (RCC->CSR & RCC_CSR_PINRSTF)
	{
	    stm32l4_system_device.reset = SYSTEM_RESET_EXTERNAL;
	}
    }
    
    RCC->CSR |= RCC_CSR_RMVF;
    RCC->CSR &= ~RCC_CSR_RMVF;
    
    PWR->SCR = (PWR_SCR_CSBF | PWR_SCR_CWUF5 | PWR_SCR_CWUF4 | PWR_SCR_CWUF3 | PWR_SCR_CWUF2 | PWR_SCR_CWUF1);
    
    stm32l4_system_device.lseclk = lseclk;
    stm32l4_system_device.hseclk = hseclk;
    
    if (lseclk)
    {
	if (option & SYSTEM_OPTION_LSE_BYPASS)
	{
	    RCC->BDCR |= RCC_BDCR_LSEBYP;
	}
	else
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

			RCC->BDCR &= ~RCC_BDCR_LSEON;
			break;
		    }
		}
	    }
	}
    }

    if (stm32l4_system_device.lseclk == 0)
    {
	RCC->CSR |= RCC_CSR_LSION;
	
	while (!(RCC->CSR & RCC_CSR_LSIRDY))
	{
	}

	stm32l4_system_device.lsi = 1;
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
	
	if (stm32l4_system_device.lseclk == 0)
	{
	    RTC->PRER = 0x000000ff;
	    RTC->PRER |= 0x007c0000;
	}
	else
	{
	    RTC->PRER = 0x000000ff;
	    RTC->PRER |= 0x007f0000;
	}
	
	RTC->ISR &= ~RTC_ISR_INIT;
    }
    
    /* Clear out pending RTC flags from a STANDBY return */
    RTC->ISR &= ~(RTC_ISR_WUTF | RTC_ISR_ALRBF | RTC_ISR_ALRAF);
    RTC->WPR = 0x00;
    
    if (option & SYSTEM_OPTION_VBAT_CHARGING)
    {
	/* Enable VBAT charging.
	 */
	PWR->CR4 |= PWR_CR4_VBE;
    }

    if (hseclk)
    {
	if (option & SYSTEM_OPTION_HSE_BYPASS)
	{
	    /* Bypasse the HSE oscillator. On L432 the input is PA0 and on
	     * L476/L433 it's PH0 (which means PH1 is a GPIO).
	     */
	    RCC->CR |= RCC_CR_HSEBYP;
	}
	else
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
    
    /* If not coming back from STANDBY, initialize the .databkp section */
    if (stm32l4_system_device.reset != SYSTEM_RESET_STANDBY)
    {
	data_s = &__backup_start__;
	data_e = &__backup_end__;
	data_t = &__etextbkp;
	
	while (data_s != data_e) { *data_s++ = *data_t++; };
    }
    
    /* If there is a non-empty .databkp section enable SRAM2 retention for STANDBY */
    if (&__backup_start__ != &__backup_end__)
    {
	PWR->CR3 |= PWR_CR3_RRS;
    }
    else
    {
	PWR->CR3 &= ~PWR_CR3_RRS;
    }
    
    /* Write protect .rodata2 in SRAM2 */
    if (&__rodata2_start__ != &__rodata2_end__)
    {
	SYSCFG->SWPR = 0xffffffff >> (32 - ((((uint8_t*)&__rodata2_end__ - (uint8_t*)&__rodata2_start__) + 1023) / 1024));
    }

    RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;
    
    /* For some reason DWT is enabled per default. Disable it with the proper reset values */
    if (!(CoreDebug->DHCSR & 0x00000001))
    {
	DWT->CTRL      = 0x00000000;
	DWT->FUNCTION0 = 0x00000000;
	DWT->FUNCTION1 = 0x00000000;
	DWT->FUNCTION2 = 0x00000000;
	DWT->FUNCTION3 = 0x00000000;
	
	CoreDebug->DEMCR &= ~0x01000000;
    }

    /* Avoid IWDG/WWDG triggering reset while debugging.
     */
    DBGMCU->APB1FZR1 |= (DBGMCU_APB1FZR1_DBG_IWDG_STOP | DBGMCU_APB1FZR1_DBG_WWDG_STOP);

    RCC->AHB1SMENR &= ~(RCC_AHB1SMENR_FLASHSMEN |
			RCC_AHB1SMENR_SRAM1SMEN);
    
    RCC->AHB2SMENR &= ~(RCC_AHB2SMENR_GPIOASMEN |
			RCC_AHB2SMENR_GPIOBSMEN |
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
			RCC_AHB2SMENR_GPIOCSMEN |
			RCC_AHB2SMENR_GPIODSMEN |
			RCC_AHB2SMENR_GPIOESMEN |
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
			RCC_AHB2SMENR_GPIOFSMEN |
			RCC_AHB2SMENR_GPIOGSMEN |
#endif
			RCC_AHB2SMENR_GPIOHSMEN |
#if defined(STM32L496xx)
			RCC_AHB2SMENR_GPIOISMEN |
#endif
			RCC_AHB2SMENR_SRAM2SMEN);
    
    RCC->APB1SMENR1 &= ~RCC_APB1SMENR1_PWRSMEN;

    RCC->APB2SMENR  &= ~RCC_APB2SMENR_SYSCFGSMEN;

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    RCC->APB1SMENR1 &= ~RCC_APB1SMENR1_RTCAPBSMEN;
#endif

    stm32l4_system_sysclk_configure(hclk, pclk1, pclk2);

    __set_PRIMASK(primask);
}

bool stm32l4_system_sysclk_configure(uint32_t hclk, uint32_t pclk1, uint32_t pclk2)
{
    uint32_t sysclk, fclk, oclk, fvco, fpll, mout, nout, rout, n, r;
    uint32_t msirange, hpre, ppre1, ppre2, latency;
    uint32_t primask, apb1enr1, mask, slot;
    
    if (hclk <= 24000000)
    {
	/* Range 2, use MSI. Always use MSI >= 4MHz, so that SAICLK/CLK48 can be generated.
	 */
	
	if      (hclk >= 24000000) { sysclk = 23986176; fclk = sysclk / 1; hclk = 24000000; hpre = RCC_CFGR_HPRE_DIV1; msirange = RCC_CR_MSIRANGE_9; mout = 6; } /*  732 * 32768 */
	else if (hclk >= 16000000) { sysclk = 15990784; fclk = sysclk / 1; hclk = 16000000; hpre = RCC_CFGR_HPRE_DIV1; msirange = RCC_CR_MSIRANGE_8; mout = 4; } /*  488 * 32768 */
	else if (hclk >=  8000000) { sysclk =  7995392; fclk = sysclk / 1; hclk =  8000000; hpre = RCC_CFGR_HPRE_DIV1; msirange = RCC_CR_MSIRANGE_7; mout = 2; } /*  244 * 32768 */
	else if (hclk >=  4000000) { sysclk =  3997696; fclk = sysclk / 1; hclk =  4000000; hpre = RCC_CFGR_HPRE_DIV1; msirange = RCC_CR_MSIRANGE_6; mout = 1; } /*  122 * 32768 */
	else if (hclk >=  2000000) { sysclk =  3997696; fclk = sysclk / 2; hclk =  2000000; hpre = RCC_CFGR_HPRE_DIV2; msirange = RCC_CR_MSIRANGE_6; mout = 1; } /*   61 * 32768 */
	else                       { sysclk =  3997696; fclk = sysclk / 4; hclk =  1000000; hpre = RCC_CFGR_HPRE_DIV4; msirange = RCC_CR_MSIRANGE_6; mout = 1; } /* 30.5 * 32768 */
	
	oclk = sysclk / mout;
	nout = 24;
	rout = 2;
    }
    else
    {
	/* Range 1, use PLL with HSE/MSI */
	
	if (hclk < 16000000)
	{
	    hclk = 16000000;
	}
	
	if (hclk > 80000000)
	{
	    hclk = 80000000;
	}
	
	hpre = RCC_CFGR_HPRE_DIV1; /* SYCLK == HCLK */

	msirange = RCC_CR_MSIRANGE_6;
	
	if (stm32l4_system_device.hseclk)
	{
	    oclk = 4000000;
	    mout = stm32l4_system_device.hseclk / 4000000; 
	}
	else
	{
	    oclk = 3997696;
	    mout = 1;
	}
	
	sysclk = 0;
	nout   = 0;
	rout   = 0;
	
	for (r = 2; r <= 8; r += 2)
	{
	    n = (hclk * r) / oclk;
	    
	    fvco = oclk * n;
	    
	    if ((n >= 8) && (n <= 86) && (fvco <= 344000000) && (fvco >= 96000000))
	    {
		fpll = fvco / r;
		
		/* Prefer lower N,R pairs for lower PLL current. */
		if (sysclk < fpll)
		{
		    sysclk = fpll;
		    
		    nout = n;
		    rout = r;
		}
	    }
	}

	fclk = sysclk;

	/* Round hclk to the next MHzHz.
	 */
	hclk = ((sysclk + 500000) / 1000000) * 1000000;
    }

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

    if      (hclk <= 16000000) { latency = FLASH_ACR_LATENCY_0WS; }
    else if (hclk <= 32000000) { latency = FLASH_ACR_LATENCY_1WS; }
    else if (hclk <= 48000000) { latency = FLASH_ACR_LATENCY_2WS; }
    else if (hclk <= 64000000) { latency = FLASH_ACR_LATENCY_3WS; }
    else                       { latency = FLASH_ACR_LATENCY_4WS; }
    
    if (hclk <= 24000000)
    {
#if defined(STM32L476xx)
	if (!stm32l4_system_device.saiclk)
#endif
	{
	    if (!(stm32l4_system_device.clk48 & (SYSTEM_CLK48_REFERENCE_USB | SYSTEM_CLK48_REFERENCE_RNG)))
	    {
		if      (hclk <=  6000000) { latency = FLASH_ACR_LATENCY_0WS; }
		else if (hclk <= 12000000) { latency = FLASH_ACR_LATENCY_1WS; }
		else if (hclk <= 18000000) { latency = FLASH_ACR_LATENCY_2WS; }
		else                       { latency = FLASH_ACR_LATENCY_3WS; }
	    }
	}
    }

    primask = __get_PRIMASK();
    
    __disable_irq();
    
    if (stm32l4_system_device.lock[SYSTEM_LOCK_CLOCKS])
    {
	__set_PRIMASK(primask);
	
	return false;
    }
    
    mask = stm32l4_system_device.event[SYSTEM_INDEX_PREPARE_CLOCKS];
    
    while (mask) 
    {
	slot = __builtin_clz(mask);
	mask &= ~(0x80000000 >> slot);
	
	(*stm32l4_system_device.callback[slot])(stm32l4_system_device.context[slot], SYSTEM_EVENT_PREPARE_CLOCKS);
    }
    
    /* First switch to HSI as system clock.
     */
    
    apb1enr1 = RCC->APB1ENR1;
    
    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    }
    
    /* Leave LPRUN */
    if (PWR->CR1 & PWR_CR1_LPR)
    {
	PWR->CR1 &= ~PWR_CR1_LPR;
	
	while (PWR->SR2 & PWR_SR2_REGLPF)
	{
	}
    }
    
    /* Switch to Range 1 */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    
    while (PWR->SR2 & PWR_SR2_VOSF)
    {
    }
    
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS;

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    if (stm32l4_system_device.hsi48)
    {
	if (stm32l4_system_device.lseclk)
	{
	    /* Disable CRS on HSI48 */
	    CRS->CR &= ~(CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
	    
	    RCC->APB1ENR1 &= ~RCC_APB1ENR1_CRSEN;
	}
	
	/* Disable HSI48 */
	RCC->CRRCR &= ~RCC_CRRCR_HSI48ON;
	
	while(RCC->CRRCR & RCC_CRRCR_HSI48RDY)
	{
	}
    }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */

#if defined(STM32L432xx) || defined(STM32L433xx)
    /* Disable PLLSAI1 */
    RCC->CR &= ~RCC_CR_PLLSAI1ON;
    
    while (RCC->CR & RCC_CR_PLLSAI1RDY)
    {
    }

#else /* defined(STM32L432xx) || defined(STM32L433xx) */
    /* Disable PLLSAI1/PLLSAI2 */
    RCC->CR &= ~(RCC_CR_PLLSAI1ON | RCC_CR_PLLSAI2ON);
    
    while (RCC->CR & (RCC_CR_PLLSAI1RDY | RCC_CR_PLLSAI2RDY))
    {
    }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) */
    
    /* Make sure HSI is on, switch to HSI as clock source */
    RCC->CR |= RCC_CR_HSION;
    
    while (!(RCC->CR & RCC_CR_HSIRDY))
    {
    }

    /* Disable SYSTICK as late as possible.
     */
    armv7m_systick_disable();
    
    /* Select the HSI as system clock source */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    
    /* Wait till the main HSI is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    {
    }
    
    SystemCoreClock = 16000000;
    
    /* Disable PLL */
    RCC->CR &= ~RCC_CR_PLLON;
    
    while (RCC->CR & RCC_CR_PLLRDY)
    {
    }
    
    /* Set HCLK/PCLK1/PCLK2 prescalers */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) | (hpre | ppre1 | ppre2);
    
    /* Configure the main PLL */
    RCC->PLLCFGR = (((mout -1) << 4) | (nout << 8) | (((rout >> 1) -1) << 25));
    
    if (hclk <= 24000000)
    {
	/* Range 2, use MSI */
	
	RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | msirange | RCC_CR_MSIRGSEL | RCC_CR_MSION;
	
	armv7m_core_udelay(1);
	
	while (!(RCC->CR & RCC_CR_MSIRDY))
	{
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
	
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_MSI;
    }
    else
    {
	/* Range 1, use HSE/PLL or MSI/PLL */
	
	if (stm32l4_system_device.hseclk)
	{
	    if (!(RCC->CR & RCC_CR_HSEBYP))
	    {
		RCC->CR |= RCC_CR_HSEON;
		
		while (!(RCC->CR & RCC_CR_HSERDY))
		{
		}
	    }
	    
	    RCC->PLLCFGR |= (RCC_PLLCFGR_PLLREN | RCC_PLLCFGR_PLLSRC_HSE);
	}
	else
	{
	    RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL | RCC_CR_MSION;
	    
	    armv7m_core_udelay(1);
	    
	    while (!(RCC->CR & RCC_CR_MSIRDY))
	    {
	    }
	    
	    if (stm32l4_system_device.lseclk)
	    {
		/* Enable the MSI PLL */
		RCC->CR |= RCC_CR_MSIPLLEN;
	    }
	    
	    RCC->PLLCFGR |= (RCC_PLLCFGR_PLLREN | RCC_PLLCFGR_PLLSRC_MSI);
	}
	
	/* Enable the main PLL */
	RCC->CR |= RCC_CR_PLLON;
	
	/* Wait till the main PLL is ready */
	while ((RCC->CR & RCC_CR_PLLRDY) == 0)
	{
	}
	
	/* Select the main PLL as system clock source */
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
	
	/* Wait till the main PLL is used as system clock source */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
	{
	}
    }
    
    SystemCoreClock = hclk;
    
    stm32l4_system_device.sysclk = sysclk;
    stm32l4_system_device.fclk = fclk;
    stm32l4_system_device.hclk = hclk;
    stm32l4_system_device.pclk1 = pclk1;
    stm32l4_system_device.pclk2 = pclk2;
    
    /* Enable SYSTICK as early as possible.
     */
    
    armv7m_systick_enable();

    if (hclk <= 24000000)
    {
	/* Disable HSE */
	if (RCC->CR & RCC_CR_HSEON)
	{
	    RCC->CR &= ~RCC_CR_HSEON;
	    
	    while (RCC->CR & RCC_CR_HSERDY)
	    {
	    }
	}
	
#if defined(STM32L476xx)
	if (!stm32l4_system_device.saiclk)
#endif
	{
	    if (!(stm32l4_system_device.clk48 & (SYSTEM_CLK48_REFERENCE_USB | SYSTEM_CLK48_REFERENCE_RNG)))
	    {
		PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_1;
		
		while (PWR->SR2 & PWR_SR2_VOSF)
		{
		}
		
		/* Enter LPRUN if hclk is low enough */
		if (hclk <= 2000000)
		{
		    PWR->CR1 |= PWR_CR1_LPR;
		    
		    while (!(PWR->SR2 & PWR_SR2_REGLPF))
		    {
		    }
		}
	    }
	}
	
	/* Select MSI as stop wakeup clock */
	RCC->CFGR &= ~RCC_CFGR_STOPWUCK;
    }
    else
    {
	if (stm32l4_system_device.hseclk)
	{
	    /* ERRATA 2.1.15. WAR: Switch MSI to <= 16MHz before turing off */
	    RCC->CR = (RCC->CR & ~(RCC_CR_MSIRANGE | RCC_CR_MSIPLLEN)) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL;
	    
	    armv7m_core_udelay(1);
	    
	    RCC->CR &= ~RCC_CR_MSION;
	    
	    while (RCC->CR & RCC_CR_MSIRDY)
	    {
	    }
	}
	
	/* Select HSI16 as stop wakeup clock */
	RCC->CFGR |= RCC_CFGR_STOPWUCK;
    }
    
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | latency;
    
#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    if (stm32l4_system_device.saiclk)
    {
	if (hclk <= 24000000)
	{
	    if      (stm32l4_system_device.saiclk == SYSTEM_SAICLK_8192000)  { RCC->PLLSAI1CFGR = ((15 << 27) | (31 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else if (stm32l4_system_device.saiclk == SYSTEM_SAICLK_11289600) { RCC->PLLSAI1CFGR = ((11 << 27) | (31 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else                                                             { RCC->PLLSAI1CFGR = (( 5 << 27) | (31 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	}
	else
	{
	    if      (stm32l4_system_device.saiclk == SYSTEM_SAICLK_8192000)  { RCC->PLLSAI1CFGR = ((21 << 27) | (43 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else if (stm32l4_system_device.saiclk == SYSTEM_SAICLK_11289600) { RCC->PLLSAI1CFGR = ((17 << 27) | (48 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else                                                             { RCC->PLLSAI1CFGR = (( 7 << 27) | (43 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	}
	
	RCC->CR |= RCC_CR_PLLSAI1ON;
	
	/* Wait till the PLLSAI1 is ready */
	while (!(RCC->CR & RCC_CR_PLLSAI1RDY))
	{
	}
    }
    
    if (stm32l4_system_device.hsi48)
    {
	/* Enable HSI48 */
	RCC->CRRCR |= RCC_CRRCR_HSI48ON;
	
	while(!(RCC->CRRCR & RCC_CRRCR_HSI48RDY))
	{
	}
	
	if (stm32l4_system_device.lseclk)
	{
	    /* Enable CRS on HSI48 */
	    RCC->APB1ENR1 |= RCC_APB1ENR1_CRSEN;
	    
	    /* 32768Hz * 1465 = 48005120Hz, Sync Source is LSE */
	    CRS->CFGR = ((1465 -1) << CRS_CFGR_RELOAD_Pos) | (1 << CRS_CFGR_FELIM_Pos) | CRS_CFGR_SYNCSRC_0;
	    CRS->CR |= (CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
	}
    }
    
#if defined(STM32L496xx)
    /* Switch CLK48SEL to HSI48, SAI1SEL to SAI1PLL, SAI2SEL to SAI1PLL */
    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_CLK48SEL | RCC_CCIPR_SAI1SEL | RCC_CCIPR_SAI2SEL));
#else /* defined(STM32L496xx) */
    /* Switch CLK48SEL to HSI48, SAI1SEL to SAI1PLL */
    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_CLK48SEL | RCC_CCIPR_SAI1SEL));
#endif
    
#else /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
    
    /* PLLM is setup so that fclk = 4000000 into N/Q for USB. fvco is 192MHz to work around errata via
     * PLLSAI1N set to 48, and then divided down to 48MHz via PLLSAI1Q set to 4. 192MHz / 17 is also
     * used to produce 11.2941MHz to serve as 44100Hz * 256.
     *
     * PLLSAI2 is set up for 49..1520MHz to serve as 192000Mhz * 256.
     */
    
    RCC->PLLSAI1CFGR = ((48 << 8) |(((4 >> 1) -1) << 21) | RCC_PLLSAI1CFGR_PLLSAI1P);
    RCC->PLLSAI2CFGR = ((86 << 8));
    
    if (stm32l4_system_device.saiclk)
    {
	if (stm32l4_system_device.saiclk == SYSTEM_SAICLK_11289600)
	{
	    RCC->PLLSAI1CFGR |= RCC_PLLSAI1CFGR_PLLSAI1PEN;
	    
	    RCC->CR |= RCC_CR_PLLSAI1ON;
	    
	    /* Wait till the PLLSAI1 is ready */
	    while (!(RCC->CR & RCC_CR_PLLSAI1RDY))
	    {
	    }
	    
	    /* Switch SAI1/SAI2 to use PLLSAI1 */
	    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_SAI1SEL | RCC_CCIPR_SAI2SEL));
	}
	else
	{
	    RCC->PLLSAI2CFGR |= RCC_PLLSAI2CFGR_PLLSAI2PEN;
	    
	    RCC->CR |= RCC_CR_PLLSAI2ON;
	    
	    /* Wait till the PLLSAI2 is ready */
	    while (!(RCC->CR & RCC_CR_PLLSAI2RDY))
	    {
	    }
	    
	    /* Switch SAI1/SAI2 to use PLLSAI2 */
	    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_SAI1SEL | RCC_CCIPR_SAI2SEL)) | (RCC_CCIPR_SAI1SEL_0 | RCC_CCIPR_SAI2SEL_0);
	}
    }
    
    if (stm32l4_system_device.clk48)
    {
	RCC->PLLSAI1CFGR |= RCC_PLLSAI1CFGR_PLLSAI1QEN;
	
	if (stm32l4_system_device.saiclk != SYSTEM_SAICLK_11289600)
	{
	    RCC->CR |= RCC_CR_PLLSAI1ON;
	    
	    /* Wait till the PLLSAI1 is ready */
	    while(!(RCC->CR & RCC_CR_PLLSAI1RDY))
	    {
	    }
	}
    }
    
    /* Switch CLK48SEL to PLL48M2CLK */
    RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_CLK48SEL) | (RCC_CCIPR_CLK48SEL_0);
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
    
    if (!stm32l4_system_device.hsi16)
    {
	RCC->CR &= ~RCC_CR_HSION;
	
	while (RCC->CR & RCC_CR_HSIRDY)
	{
	}
    }

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;
    }

    mask = stm32l4_system_device.event[SYSTEM_INDEX_CHANGE_CLOCKS];

    while (mask) 
    {
	slot = __builtin_clz(mask);
	mask &= ~(0x80000000 >> slot);

	(*stm32l4_system_device.callback[slot])(stm32l4_system_device.context[slot], SYSTEM_EVENT_CHANGE_CLOCKS);
    }

    __set_PRIMASK(primask);

    return true;
}

static void stm32l4_system_select_range_1(void)
{
    uint32_t apb1enr1, latency;

    if (stm32l4_system_device.hclk <= 24000000)
    {
	apb1enr1 = RCC->APB1ENR1;
	    
	if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
	{
	    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
	}

	/* Leave LPRUN */
	if (PWR->CR1 & PWR_CR1_LPR)
	{
	    PWR->CR1 &= ~PWR_CR1_LPR;
	    
	    while (PWR->SR2 & PWR_SR2_REGLPF)
	    {
	    }
	}

	/* Switch to Range 1 */
	PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    
	while (PWR->SR2 & PWR_SR2_VOSF)
	{
	}

	if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
	{
	    RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;
	}

	/* Adjust FLASH latency */
	if   (stm32l4_system_device.hclk <= 16000000) { latency = FLASH_ACR_LATENCY_0WS; }
	else                                          { latency = FLASH_ACR_LATENCY_1WS; }

	FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | latency;
    }
}

static void stm32l4_system_select_range_2(void)
{
    uint32_t apb1enr1, latency;

    if (stm32l4_system_device.hclk <= 24000000)
    {
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
	    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
	}
	
	PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_1;
	
	while (PWR->SR2 & PWR_SR2_VOSF)
	{
	}

	/* Enter LPRUN if hclk is low enough */
	if (stm32l4_system_device.hclk <= 2000000)
	{
	    PWR->CR1 |= PWR_CR1_LPR;
	    
	    while (!(PWR->SR2 & PWR_SR2_REGLPF))
	    {
	    }
	}
	
	if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
	{
	    RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;
	}
    }
}

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)

static void stm32l4_system_hsi48_enable(void)
{
    uint32_t primask, hsi48;

    primask = __get_PRIMASK();

    __disable_irq();

    hsi48 = stm32l4_system_device.hsi48;
    
    if (!hsi48)
    {
	/* Enable HSI48 */
	RCC->CRRCR |= RCC_CRRCR_HSI48ON;

	while(!(RCC->CRRCR & RCC_CRRCR_HSI48RDY))
	{
	}

	if (stm32l4_system_device.lseclk)
	{
	    /* Enable CRS on HSI48 */
	    RCC->APB1ENR1 |= RCC_APB1ENR1_CRSEN;

	    /* 32768Hz * 1465 = 48005120Hz, Sync Source is LSE */
	    CRS->CFGR = ((1465 -1) << CRS_CFGR_RELOAD_Pos) | (1 << CRS_CFGR_FELIM_Pos) | CRS_CFGR_SYNCSRC_0;
	    CRS->CR |= (CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
	}
    }

    stm32l4_system_device.hsi48 = hsi48 + 1;

    __set_PRIMASK(primask);
}

static void stm32l4_system_hsi48_disable(void)
{
    uint32_t primask, hsi48;

    primask = __get_PRIMASK();

    __disable_irq();

    hsi48 = stm32l4_system_device.hsi48 - 1;
    
    if (!hsi48)
    {
	if (stm32l4_system_device.lseclk)
	{
	    /* Disable CRS on HSI48 */
	    CRS->CR &= ~(CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
	    
	    RCC->APB1ENR1 &= ~RCC_APB1ENR1_CRSEN;
	}

	/* Disable HSI48 */
	RCC->CRRCR &= ~RCC_CRRCR_HSI48ON;

	while(RCC->CRRCR & RCC_CRRCR_HSI48RDY)
	{
	}
    }

    stm32l4_system_device.hsi48 = hsi48;

    __set_PRIMASK(primask);
}

#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */

void stm32l4_system_saiclk_configure(unsigned int clock)
{
#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    RCC->PLLSAI1CFGR &= ~RCC_PLLSAI1CFGR_PLLSAI1PEN;

    RCC->CR &= ~RCC_CR_PLLSAI1ON;
    
    /* Wait till the PLLSAI1 is turned off */
    while (RCC->CR & RCC_CR_PLLSAI1RDY)
    {
    }

    if (clock)
    {
	if (stm32l4_system_device.hclk <= 24000000)
	{
	    if      (clock == SYSTEM_SAICLK_8192000)  { RCC->PLLSAI1CFGR = ((15 << 27) | (31 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else if (clock == SYSTEM_SAICLK_11289600) { RCC->PLLSAI1CFGR = ((11 << 27) | (31 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else                                      { RCC->PLLSAI1CFGR = (( 5 << 27) | (31 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); clock = SYSTEM_SAICLK_24576000; }
	}
	else
	{
	    if      (clock == SYSTEM_SAICLK_8192000)  { RCC->PLLSAI1CFGR = ((21 << 27) | (43 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else if (clock == SYSTEM_SAICLK_11289600) { RCC->PLLSAI1CFGR = ((17 << 27) | (48 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); }
	    else                                      { RCC->PLLSAI1CFGR = (( 7 << 27) | (43 << 8) | RCC_PLLSAI1CFGR_PLLSAI1PEN); clock = SYSTEM_SAICLK_24576000; }
	}

	RCC->CR |= RCC_CR_PLLSAI1ON;
		
	/* Wait till the PLLSAI1 is ready */
	while (!(RCC->CR & RCC_CR_PLLSAI1RDY))
	{
	}
    }

    stm32l4_system_device.saiclk = clock;
#else /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
    if (clock)
    {
	if (!stm32l4_system_device.saiclk)
	{
	    stm32l4_system_select_range_1();
	}

	if (clock == SYSTEM_SAICLK_11289600)
	{
	    RCC->PLLSAI1CFGR |= RCC_PLLSAI1CFGR_PLLSAI1PEN;
	    
	    if (!stm32l4_system_device.clk48)
	    {
		RCC->CR |= RCC_CR_PLLSAI1ON;
		
		/* Wait till the PLLSAI1 is ready */
		while (!(RCC->CR & RCC_CR_PLLSAI1RDY))
		{
		}
	    }
	    
	    /* Switch SAI1/SAI2 to use PLLSAI1 */
	    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_SAI1SEL | RCC_CCIPR_SAI2SEL));
	}
	else
	{
	    clock = SYSTEM_SAICLK_49152000;
	    
	    RCC->PLLSAI2CFGR |= RCC_PLLSAI2CFGR_PLLSAI2PEN;
		
	    RCC->CR |= RCC_CR_PLLSAI2ON;
	    
	    /* Wait till the PLLSAI2 is ready */
	    while (!(RCC->CR & RCC_CR_PLLSAI2RDY))
	    {
	    }
	    
	    /* Switch SAI1/SAI2 to use PLLSAI2 */
	    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_SAI1SEL | RCC_CCIPR_SAI2SEL)) | (RCC_CCIPR_SAI1SEL_0 | RCC_CCIPR_SAI2SEL_0);
	}
    }
	    
    stm32l4_system_device.saiclk = clock;

    if (stm32l4_system_device.saiclk != SYSTEM_SAICLK_11289600)
    {
	RCC->PLLSAI1CFGR &= ~RCC_PLLSAI1CFGR_PLLSAI1PEN;

	if (!stm32l4_system_device.clk48)
	{
	    RCC->CR &= ~RCC_CR_PLLSAI1ON;
	    
	    /* Wait till the PLLSAI1 is turned off */
	    while (RCC->CR & RCC_CR_PLLSAI1RDY)
	    {
	    }
	}
    }

    if (stm32l4_system_device.saiclk != SYSTEM_SAICLK_49152000)
    {
	RCC->PLLSAI2CFGR &= ~RCC_PLLSAI2CFGR_PLLSAI2PEN;
		    
	RCC->CR &= ~RCC_CR_PLLSAI2ON;
		    
	/* Wait till the PLLSAI2 is turned off */
	while (RCC->CR & RCC_CR_PLLSAI2RDY)
	{
	}
    }

    if (!stm32l4_system_device.saiclk)
    {
	if (!(stm32l4_system_device.clk48 & (SYSTEM_CLK48_REFERENCE_USB | SYSTEM_CLK48_REFERENCE_RNG)))
	{
	    stm32l4_system_select_range_2();
	}
    }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
}

void stm32l4_system_clk48_acquire(unsigned int reference)
{
    uint32_t primask;

    primask = __get_PRIMASK();

    __disable_irq();

    if ((stm32l4_system_device.clk48 | reference) & (SYSTEM_CLK48_REFERENCE_USB | SYSTEM_CLK48_REFERENCE_RNG))
    {
	stm32l4_system_select_range_1();
    }

    if (!stm32l4_system_device.clk48)
    {
#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
	stm32l4_system_hsi48_enable();
#else /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
	RCC->PLLSAI1CFGR |= RCC_PLLSAI1CFGR_PLLSAI1QEN;

	if (stm32l4_system_device.saiclk != SYSTEM_SAICLK_11289600)
	{
	    RCC->CR |= RCC_CR_PLLSAI1ON;
	
	    /* Wait till the PLLSAI1 is ready */
	    while (!(RCC->CR & RCC_CR_PLLSAI1RDY))
	    {
	    }
	}
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
    }

    stm32l4_system_device.clk48 |= reference;

    __set_PRIMASK(primask);
}

void stm32l4_system_clk48_release(unsigned int reference)
{
    uint32_t primask;
    
    primask = __get_PRIMASK();
    
    __disable_irq();

    if (stm32l4_system_device.clk48)
    {
	if (!(stm32l4_system_device.clk48 & ~reference))
	{
#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
	    stm32l4_system_hsi48_disable();
#else /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
	    RCC->PLLSAI1CFGR &= ~RCC_PLLSAI1CFGR_PLLSAI1QEN;
	    
	    if (stm32l4_system_device.saiclk != SYSTEM_SAICLK_11289600)
	    {
		RCC->CR &= ~RCC_CR_PLLSAI1ON;
		
		while (RCC->CR & RCC_CR_PLLSAI1RDY)
		{
		}
	    }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
	}
    }

    stm32l4_system_device.clk48 &= ~reference;
    
#if defined(STM32L476xx)
    if (!stm32l4_system_device.saiclk)
#endif
    {
	if (!(stm32l4_system_device.clk48 & (SYSTEM_CLK48_REFERENCE_USB | SYSTEM_CLK48_REFERENCE_RNG)))
	{
	    stm32l4_system_select_range_2();
	}
    }
    
    __set_PRIMASK(primask);
}

void stm32l4_system_lsi_enable(void)
{
    uint32_t primask, lsi;

    primask = __get_PRIMASK();

    __disable_irq();

    lsi = stm32l4_system_device.lsi;
    
    if (!lsi)
    {
	RCC->CSR |= RCC_CSR_LSION;
	
	while (!(RCC->CSR & RCC_CSR_LSIRDY))
	{
	}
    }

    stm32l4_system_device.lsi = lsi + 1;

    __set_PRIMASK(primask);
}

void stm32l4_system_lsi_disable(void)
{
    uint32_t primask, lsi;

    primask = __get_PRIMASK();

    __disable_irq();

    lsi = stm32l4_system_device.lsi - 1;
    
    if (!lsi)
    {
	RCC->CSR &= ~RCC_CSR_LSION;
	
	while (RCC->CSR & RCC_CSR_LSIRDY)
	{
	}
    }

    stm32l4_system_device.lsi = lsi;

    __set_PRIMASK(primask);
}

void stm32l4_system_hsi16_enable(void)
{
    uint32_t primask, hsi16;

    primask = __get_PRIMASK();

    __disable_irq();

    hsi16 = stm32l4_system_device.hsi16;
    
    if (!hsi16)
    {
	RCC->CR |= RCC_CR_HSION;
    
	while (!(RCC->CR & RCC_CR_HSIRDY))
	{
	}
    }

    stm32l4_system_device.hsi16 = hsi16 + 1;

    __set_PRIMASK(primask);
}

void stm32l4_system_hsi16_disable(void)
{
    uint32_t primask, hsi16;

    primask = __get_PRIMASK();

    __disable_irq();

    hsi16 = stm32l4_system_device.hsi16 - 1;
    
    if (!hsi16)
    {
	RCC->CR &= ~RCC_CR_HSION;
    
	while (RCC->CR & RCC_CR_HSIRDY)
	{
	}
    }

    stm32l4_system_device.hsi16 = hsi16;

    __set_PRIMASK(primask);
}

void stm32l4_system_mco_configure(unsigned int mode, unsigned int divider)
{
    uint32_t primask, mcopre;

    primask = __get_PRIMASK();

    __disable_irq();

    RCC->CFGR &= ~(RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE);

    if (stm32l4_system_device.mco != mode)
    {
	switch (stm32l4_system_device.mco) {
	case SYSTEM_MCO_MODE_NONE:
	case SYSTEM_MCO_MODE_SYSCLK:
	case SYSTEM_MCO_MODE_MSI:
	case SYSTEM_MCO_MODE_HSE:
	case SYSTEM_MCO_MODE_PLL:
	case SYSTEM_MCO_MODE_LSE:
	    break;

	case SYSTEM_MCO_MODE_LSI:
	    stm32l4_system_lsi_disable();
	    break;

	case SYSTEM_MCO_MODE_HSI16:
	    stm32l4_system_hsi16_disable();
	    break;

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
	case SYSTEM_MCO_MODE_HSI48:
	    stm32l4_system_hsi48_disable();
	    break;
#endif
	}

	stm32l4_system_device.mco = mode;

	switch (stm32l4_system_device.mco) {
	case SYSTEM_MCO_MODE_NONE:
	case SYSTEM_MCO_MODE_SYSCLK:
	case SYSTEM_MCO_MODE_MSI:
	case SYSTEM_MCO_MODE_HSE:
	case SYSTEM_MCO_MODE_PLL:
	case SYSTEM_MCO_MODE_LSE:
	    break;

	case SYSTEM_MCO_MODE_LSI:
	    stm32l4_system_lsi_enable();
	    break;

	case SYSTEM_MCO_MODE_HSI16:
	    stm32l4_system_hsi16_enable();
	    break;

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
	case SYSTEM_MCO_MODE_HSI48:
	    stm32l4_system_hsi48_enable();
	    break;
#endif
	}
    }

    if      (divider <=  1) { mcopre = RCC_CFGR_MCOPRE_DIV1;  }
    else if (divider <=  2) { mcopre = RCC_CFGR_MCOPRE_DIV2;  }
    else if (divider <=  4) { mcopre = RCC_CFGR_MCOPRE_DIV4;  }
    else if (divider <=  8) { mcopre = RCC_CFGR_MCOPRE_DIV8;  }
    else                    { mcopre = RCC_CFGR_MCOPRE_DIV16; }

    RCC->CFGR |= ((mode << RCC_CFGR_MCOSEL_Pos) | mcopre);

    __set_PRIMASK(primask);
}

void stm32l4_system_lsco_configure(unsigned int mode)
{
    uint32_t primask;

    primask = __get_PRIMASK();

    __disable_irq();

    if (stm32l4_system_device.lsco != mode)
    {
	RCC->BDCR &= ~RCC_BDCR_LSCOEN;

	if (stm32l4_system_device.lsco == SYSTEM_LSCO_MODE_LSI)
	{
	    RCC->BDCR &= ~RCC_BDCR_LSCOSEL;

	    stm32l4_system_lsi_disable();
	}

	stm32l4_system_device.lsco = mode;

	if (stm32l4_system_device.lsco == SYSTEM_LSCO_MODE_LSI)
	{
	    stm32l4_system_lsi_enable();

	    RCC->BDCR |= RCC_BDCR_LSCOSEL;
	}

	if (stm32l4_system_device.lsco != SYSTEM_LSCO_MODE_NONE)
	{
	    RCC->BDCR |= RCC_BDCR_LSCOEN;
	}
    }

    __set_PRIMASK(primask);
}

uint32_t stm32l4_system_reset_cause(void)
{
    return stm32l4_system_device.reset;
}

uint32_t stm32l4_system_wakeup_reason(void)
{
    return stm32l4_system_device.wakeup;
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

uint32_t stm32l4_system_fclk(void)
{
    return stm32l4_system_device.fclk;
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

uint32_t stm32l4_system_saiclk(void)
{
    return stm32l4_system_device.saiclk;
}

int stm32l4_system_notify(int slot, stm32l4_system_callback_t callback, void *context, uint32_t events)
{
    unsigned int i, mask;

    if (slot < 0)
    {
	for (slot = 0; slot < SYSTEM_NOTIFY_COUNT; slot++)
	{
	    if (!stm32l4_system_device.callback[slot])
	    {
		break;
	    }
	}

	if (slot == SYSTEM_NOTIFY_COUNT)
	{
	    return -1;
	}
    }

    mask = 0x80000000 >> slot;

    for (i = 0; i < SYSTEM_EVENT_COUNT; i++)
    {
	armv7m_atomic_and(&stm32l4_system_device.event[i], ~mask);
    }

    stm32l4_system_device.callback[slot] = callback;
    stm32l4_system_device.context[slot] = context;

    for (i = 0; i < SYSTEM_EVENT_COUNT; i++, events >>= 1)
    {
	if (events & 1)
	{
	    armv7m_atomic_or(&stm32l4_system_device.event[i], mask);
	}
    }

    return slot;
}

void stm32l4_system_lock(uint32_t lock)
{
    armv7m_atomic_add(&stm32l4_system_device.lock[lock], 1);
}

void stm32l4_system_unlock(uint32_t lock)
{
    armv7m_atomic_sub(&stm32l4_system_device.lock[lock], 1);
}

static void stm32l4_system_suspend(void)
{
#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    if (stm32l4_system_device.hsi48)
    {
	if (stm32l4_system_device.lseclk)
	{
	    /* Disable CRS on HSI48 */
	    CRS->CR &= ~(CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
	    
	    RCC->APB1ENR1 &= ~RCC_APB1ENR1_CRSEN;
	}
    }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) */

    if (stm32l4_system_device.hclk <= 24000000)
    {
	if (stm32l4_system_device.hsi16)
	{
	    RCC->CR |= RCC_CR_HSIASFS;
	}
	else
	{
	    RCC->CR &= ~RCC_CR_HSIASFS;
	}
    }
    else
    {
	/* Make sure HSI is on, switch to HSI as clock source */
	if (!(RCC->CR & RCC_CR_HSION))
	{
	    RCC->CR |= RCC_CR_HSION;
	    
	    while (!(RCC->CR & RCC_CR_HSIRDY))
	    {
	    }
	}
	
	/* Select the HSI as system clock source */
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
	
	/* Wait till the main HSI is used as system clock source */
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
	{
	}

	RCC->CR &= ~RCC_CR_HSIASFS;

	SystemCoreClock = 16000000;
    }

    /* Disable FLASH in sleep/deepsleep */
    FLASH->ACR |= FLASH_ACR_SLEEP_PD;
}

static void stm32l4_system_resume(void)
{
    /* Enable FLASH in sleep/deepsleep */
    FLASH->ACR &= ~FLASH_ACR_SLEEP_PD;

    if (stm32l4_system_device.hclk <= 24000000)
    {
	if (stm32l4_system_device.lseclk)
	{
	    /* Enable the MSI PLL */
	    RCC->CR |= RCC_CR_MSIPLLEN;
	}
    }
    else
    {
	/* We get here with HSI16 as wakeup clock. So enable HSE/MSI as required,
	 * turn on the PLL and switch the clock source.
	 */
	if (stm32l4_system_device.hseclk)
	{
	    if (!(RCC->CR & RCC_CR_HSEBYP))
	    {
		RCC->CR |= RCC_CR_HSEON;
	    
		while (!(RCC->CR & RCC_CR_HSERDY))
		{
		}
	    }
	}
	else
	{
	    RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL | RCC_CR_MSION;
	    
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

	if (!stm32l4_system_device.hsi16)
	{
	    RCC->CR &= ~RCC_CR_HSION;
	    
	    while (RCC->CR & RCC_CR_HSIRDY)
	    {
	    }
	}

	SystemCoreClock = stm32l4_system_device.hclk;
    }

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    if (stm32l4_system_device.saiclk)
    {
	RCC->CR |= RCC_CR_PLLSAI1ON;
		
	/* Wait till the PLLSAI1 is ready */
	while (!(RCC->CR & RCC_CR_PLLSAI1RDY))
	{
	}
    }

    if (stm32l4_system_device.hsi48)
    {
	/* Enable HSI48 */
	RCC->CRRCR |= RCC_CRRCR_HSI48ON;

	while(!(RCC->CRRCR & RCC_CRRCR_HSI48RDY))
	{
	}

	if (stm32l4_system_device.lseclk)
	{
	    /* Enable/Reset CRS on HSI48 */
	    RCC->APB1ENR1 |= RCC_APB1ENR1_CRSEN;

	    /* 32768Hz * 1465 = 48005120Hz, Sync Source is LSE */
	    CRS->CFGR = ((1465 -1) << CRS_CFGR_RELOAD_Pos) | (1 << CRS_CFGR_FELIM_Pos) | CRS_CFGR_SYNCSRC_0;
	    CRS->CR |= (CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
	}
    }
#else /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
    if ((stm32l4_system_device.saiclk == SYSTEM_SAICLK_11289600) || stm32l4_system_device.clk48)
    {
	RCC->CR |= RCC_CR_PLLSAI1ON;
		
	/* Wait till the PLLSAI1 is ready */
	while (!(RCC->CR & RCC_CR_PLLSAI1RDY))
	{
	}
    }

    if (stm32l4_system_device.saiclk == SYSTEM_SAICLK_49152000)
    {
	RCC->CR |= RCC_CR_PLLSAI2ON;
	
	/* Wait till the PLLSAI2 is ready */
	while (!(RCC->CR & RCC_CR_PLLSAI2RDY))
	{
	}
    }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */

    if (RTC->ISR & (RTC_ISR_ALRAF | RTC_ISR_ALRBF | RTC_ISR_TSF | RTC_ISR_WUTF))
    {
	if ((RTC->ISR & (RTC_ISR_ALRAF | RTC_ISR_ALRBF)) && !(EXTI->IMR1 & EXTI_IMR1_IM18))
	{
	    RTC->ISR &= ~(RTC_ISR_ALRAF | RTC_ISR_ALRBF);
	    
	    EXTI->PR1 = EXTI_PR1_PIF18;
	}
	
	if ((RTC->ISR & RTC_ISR_TSF) && !(EXTI->IMR1 & EXTI_IMR1_IM19))
	{
	    RTC->ISR &= ~RTC_ISR_TSF;
	    
	    EXTI->PR1 = EXTI_PR1_PIF19;
	}
	
	if ((RTC->ISR & RTC_ISR_WUTF) && !(EXTI->IMR1 & EXTI_IMR1_IM20))
	{
	    RTC->ISR &= ~RTC_ISR_WUTF;
	    
	    EXTI->PR1 = EXTI_PR1_PIF20;
	}
    }
}

bool stm32l4_system_stop(uint32_t timeout)
{
    uint32_t primask, apb1enr1, slot, mask;

    primask = __get_PRIMASK();

    __disable_irq();

    if (stm32l4_system_device.lock[SYSTEM_LOCK_SLEEP])
    {
	__set_PRIMASK(primask);

	return false;
    }

    mask = stm32l4_system_device.event[SYSTEM_INDEX_SUSPEND];

    while (mask) 
    {
	slot = __builtin_clz(mask);
	mask &= ~(0x80000000 >> slot);

	(*stm32l4_system_device.callback[slot])(stm32l4_system_device.context[slot], SYSTEM_EVENT_SUSPEND);
    }

    armv7m_systick_disable();

    if (timeout)
    {
	stm32l4_rtc_wakeup(timeout);
    }

    apb1enr1 = RCC->APB1ENR1;

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    }

    stm32l4_system_suspend();
    
    PWR->CR1 = ((PWR->CR1 & ~PWR_CR1_LPMS) |
		(stm32l4_system_device.lock[SYSTEM_LOCK_STOP_0]
		 ? PWR_CR1_LPMS_STOP0
		 : (stm32l4_system_device.lock[SYSTEM_LOCK_STOP_1]
		    ? PWR_CR1_LPMS_STOP1
		    : PWR_CR1_LPMS_STOP2)));

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __DMB();

    __SEV();
    __WFE();
    __WFE();

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    stm32l4_system_resume();

    if (!(apb1enr1 & RCC_APB1ENR1_PWREN))
    {
	RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;
    }

    if (timeout)
    {
	stm32l4_rtc_wakeup(0);
    }

    armv7m_systick_enable();

    mask = stm32l4_system_device.event[SYSTEM_INDEX_RESUME];

    while (mask) 
    {
	slot = __builtin_clz(mask);
	mask &= ~(0x80000000 >> slot);

	(*stm32l4_system_device.callback[slot])(stm32l4_system_device.context[slot], SYSTEM_EVENT_RESUME);
    }

    __set_PRIMASK(primask);

    return true;
}

static void stm32l4_system_deepsleep(uint32_t lpms, uint32_t config, uint32_t timeout)
{
    if (timeout)
    {
	stm32l4_rtc_wakeup(timeout);
    }

    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

    /* ERRATA 2.1.15. WAR: Switch MSI to 4MHz before entering STANDBY/SHUTDOWN mode */

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS;

    /* Select the proper voltage range */

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

    /* Make sure HSI is on, switch to HSI as clock source */
    if (!(RCC->CR & RCC_CR_HSION))
    {
	RCC->CR |= RCC_CR_HSION;
	
	while (!(RCC->CR & RCC_CR_HSIRDY))
	{
	}
    }

    /* Select the HSI as system clock source */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    
    /* Wait till the main HSI is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    {
    }

    SystemCoreClock = 16000000;

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    if (stm32l4_system_device.hsi48)
    {
	if (stm32l4_system_device.lseclk)
	{
	    /* Disable CRS on HSI48 */
	    CRS->CR &= ~(CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
	    
	    RCC->APB1ENR1 &= ~RCC_APB1ENR1_CRSEN;
	}
    }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */

    /* Select MSI @ 4MHz as system clock source (disable MSIPLL). */
    RCC->CR = (RCC->CR & ~(RCC_CR_MSIPLLEN | RCC_CR_MSIRANGE)) | RCC_CR_MSIRANGE_6 | RCC_CR_MSIRGSEL | RCC_CR_MSION;

    armv7m_core_udelay(1);
    
    while (!(RCC->CR & RCC_CR_MSIRDY))
    {
    }

    /* Select the MSI as system clock source */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_MSI;
    
    /* Wait till the main MSI is used as system clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI)
    {
    }

    SystemCoreClock = 4000000;

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) | (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

    FLASH->ACR = FLASH_ACR_LATENCY_0WS;

    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | lpms;
    PWR->CR3 = (PWR->CR3 & ~PWR_CR3_EWUP) | ((config >> 0) & PWR_CR3_EWUP) | PWR_CR3_EIWF;
    PWR->CR4 = (PWR->CR4 & ~(PWR_CR4_WP1 | PWR_CR4_WP2 | PWR_CR4_WP3 | PWR_CR4_WP4 | PWR_CR4_WP5)) | ((config >> 8) & (PWR_CR4_WP1 | PWR_CR4_WP2 | PWR_CR4_WP3 | PWR_CR4_WP4 | PWR_CR4_WP5));

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __DMB();

    __SEV();
    __WFE();
    __WFE();

    while (1)
    {
    }
}

void stm32l4_system_standby(uint32_t config, uint32_t timeout)
{
    uint32_t primask, slot, mask;

    primask = __get_PRIMASK();

    __disable_irq();

    if (stm32l4_system_device.lock[SYSTEM_LOCK_SLEEP] ||
	stm32l4_system_device.lock[SYSTEM_LOCK_STOP_0] ||
	stm32l4_system_device.lock[SYSTEM_LOCK_STOP_1] ||
	stm32l4_system_device.lock[SYSTEM_LOCK_STOP_2])
    {
	__set_PRIMASK(primask);

	return;
    }

    mask = stm32l4_system_device.event[SYSTEM_INDEX_STANDBY];

    while (mask) 
    {
	slot = __builtin_clz(mask);
	mask &= ~(0x80000000 >> slot);

	(*stm32l4_system_device.callback[slot])(stm32l4_system_device.context[slot], SYSTEM_EVENT_STANDBY);
    }

    stm32l4_system_deepsleep(PWR_CR1_LPMS_STANDBY, config, timeout);

    while (1)
    {
    }
}

void stm32l4_system_shutdown(uint32_t config, uint32_t timeout)
{
    uint32_t primask, slot, mask;

    primask = __get_PRIMASK();

    __disable_irq();

    if (stm32l4_system_device.lock[SYSTEM_LOCK_SLEEP] ||
	stm32l4_system_device.lock[SYSTEM_LOCK_STOP_0] ||
	stm32l4_system_device.lock[SYSTEM_LOCK_STOP_1] ||
	stm32l4_system_device.lock[SYSTEM_LOCK_STOP_2] ||
	stm32l4_system_device.lock[SYSTEM_LOCK_STANDBY])
    {
	__set_PRIMASK(primask);

	return;
    }

    mask = stm32l4_system_device.event[SYSTEM_INDEX_SHUTDOWN];

    while (mask) 
    {
	slot = __builtin_clz(mask);
	mask &= ~(0x80000000 >> slot);

	(*stm32l4_system_device.callback[slot])(stm32l4_system_device.context[slot], SYSTEM_EVENT_SHUTDOWN);
    }

    stm32l4_system_deepsleep(PWR_CR1_LPMS_SHUTDOWN, config, timeout);

    while (1)
    {
    }
}

void stm32l4_system_reset(void)
{
    unsigned int slot, mask;

    __disable_irq();

    mask = stm32l4_system_device.event[SYSTEM_INDEX_RESET];

    while (mask) 
    {
	slot = __builtin_clz(mask);
	mask &= ~(0x80000000 >> slot);

	(*stm32l4_system_device.callback[slot])(stm32l4_system_device.context[slot], SYSTEM_EVENT_RESET);
    }

    NVIC_SystemReset();
    
    while (1)
    {
    }
}

void stm32l4_system_dfu(void)
{
    __disable_irq();

    /* Set the BCK bit to flag a reboot into the booloader */
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;
    RTC->CR |= RTC_CR_BCK;

    stm32l4_system_reset();
}
