/*
 * Copyright (c) 2017 Thomas Roell.  All rights reserved.
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
#include "stm32l4_iap.h"

static void default_irq_stm32l4xx(void);
void reset_stm32l4xx(void);

const __attribute__((section(".isr_vector"))) uint32_t vectors_stm32l4xx[16] = {
#if defined(STM32L432xx) || defined(STM32L433xx)
    0x2000c000,                        /* Top of Stack */
#endif /* defined(STM32L432xx) || defined(STM32L433xx) */
#if defined(STM32L476xx)
    0x20018000,                        /* Top of Stack */
#endif /* defined(STM32L496xx) */
#if defined(STM32L496xx)
    0x20040000,                        /* Top of Stack */
#endif /* defined(STM32L496xx) */
    (uint32_t)&reset_stm32l4xx,        /* Reset Handler */
    (uint32_t)&default_irq_stm32l4xx,  /* NMI Handler */
    (uint32_t)&default_irq_stm32l4xx,  /* Hard Fault Handler */
    (uint32_t)&default_irq_stm32l4xx,  /* MPU Fault Handler */
    (uint32_t)&default_irq_stm32l4xx,  /* Bus Fault Handler */
    (uint32_t)&default_irq_stm32l4xx,  /* Usage Fault Handler */
    0,                                 /* Reserved */
    0,                                 /* Reserved */
    0,                                 /* Reserved */
    0,                                 /* Reserved */
    (uint32_t)&default_irq_stm32l4xx,  /* SVCall Handler */
    (uint32_t)&default_irq_stm32l4xx,  /* Debug Monitor Handler */
    0,                                 /* Reserved */
    (uint32_t)&default_irq_stm32l4xx,  /* PendSV Handler */
    (uint32_t)&default_irq_stm32l4xx,  /* SysTick Handler */
};

static void default_irq_stm32l4xx(void)
{
    while (1) { }
}

__attribute__((naked)) void reset_stm32l4xx(void)
{
    uint32_t flash_acr;

    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
    RCC->APB1ENR1 |= RCC_APB1ENR1_RTCAPBEN;
#endif

    /* Switch to Main Flash @ 0x00000000. Make sure the I/D CACHE is
     * disabled to avoid stale data in the cache.
     */

    flash_acr = FLASH->ACR;

    FLASH->ACR = flash_acr & ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN);
	
    SYSCFG->MEMRMP = 0;

    PWR->CR1 |= PWR_CR1_DBP;
    
    while (!(PWR->CR1 & PWR_CR1_DBP))
    {
    }
    
    if (!(RCC->BDCR & RCC_BDCR_RTCEN))
    {
	RCC->BDCR |= RCC_BDCR_BDRST;
	
	RCC->BDCR &= ~RCC_BDCR_BDRST;
    }
    
    /* If RTC_CR_BCK is set it means the reset was triggered to
     * branch throu to the STM32 BOOTLOADER.
     */
    if (RTC->CR & RTC_CR_BCK)
    {
	RTC->WPR = 0xca;
	RTC->WPR = 0x53;
	RTC->CR &= ~RTC_CR_BCK;
	RTC->WPR = 0x00;

	SYSCFG->MEMRMP = SYSCFG_MEMRMP_MEM_MODE_0;

	FLASH->ACR = (flash_acr & ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN)) | (FLASH_ACR_ICRST | FLASH_ACR_DCRST);
	FLASH->ACR = flash_acr;

#if defined(STM32L432xx)
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN); 
#else
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN); 
#endif
	GPIOA->LCKR = 0x0001e7ff;
	GPIOA->LCKR = 0x0000e7ff;
	GPIOA->LCKR = 0x0001e7ff;
	GPIOA->LCKR;
	GPIOB->LCKR = 0x0001ffff;
	GPIOB->LCKR = 0x0000ffff;
	GPIOB->LCKR = 0x0001ffff;
	GPIOB->LCKR;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	GPIOC->LCKR = 0x00011fff;
	GPIOC->LCKR = 0x00001fff;
	GPIOC->LCKR = 0x00011fff;
	GPIOC->LCKR;
#endif
#if defined(STM32L432xx)
	RCC->AHB2ENR &= ~(RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN);
#else
	RCC->AHB2ENR &= ~(RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN); 
#endif

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
	RCC->APB1ENR1 &= ~RCC_APB1ENR1_RTCAPBEN;
#endif

	RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;

	RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;

	SCB->VTOR = 0x00000000;
	
	/* This needs to be assembly code as GCC catches NULL 
	 * dereferences ...
	 */
	__asm__ volatile ("   ldr     r2, =0x00000000                \n"
			  "   ldr     r0, [r2, #0]                   \n"
			  "   ldr     r1, [r2, #4]                   \n"
			  "   msr     MSP, r0                        \n"
			  "   dsb                                    \n"
			  "   isb                                    \n"
			  "   bx      r1                             \n");
    }
    else
    {
	/* Enable 16MHz HSI and 48MHz MSI, switch to MSI as sysclk.
	 */

	FLASH->ACR = (flash_acr & ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN)) | (FLASH_ACR_ICRST | FLASH_ACR_DCRST);
	FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS;

	RCC->CIER = 0x00000000;

	RCC->CR |= RCC_CR_HSION;
	
	while (!(RCC->CR & RCC_CR_HSIRDY))
	{
	}

	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;

	RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE) | RCC_CR_MSIRANGE_11 | RCC_CR_MSIRGSEL | RCC_CR_MSION;
	
	while (!(RCC->CR & RCC_CR_MSIRDY))
	{
	}

	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_MSI;

	FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_2WS;
    
	// if (!(PWR->SR1 & PWR_SR1_SBF) && !(RCC->CSR & (RCC_CSR_LPWRRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF | RCC_CSR_BORRSTF | RCC_CSR_OBLRSTF | RCC_CSR_FWRSTF)))
	if (!(PWR->SR1 & PWR_SR1_SBF) && !(RCC->CSR & (RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF)))
	{
	    stm32l4_iap();
	}

	SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */

	SCB->VTOR = 0x08000800;

	/* Branch off to the application code. Needs to be in assembly for GCC.
	 */
	__asm__ volatile ("   ldr     r2, =0x08000800                \n"
			  "   ldr     r0, [r2, #0]                   \n"
			  "   ldr     r1, [r2, #4]                   \n"
			  "   msr     MSP, r0                        \n"
			  "   dsb                                    \n"
			  "   isb                                    \n"
			  "   bx      r1                             \n");
    }
}
