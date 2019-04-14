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

#include <stdio.h>

#include "stm32l4xx.h"

#include "armv7m.h"

#include "stm32l4_adc.h"
#include "stm32l4_gpio.h"
#include "stm32l4_system.h"

#define ADC_SAMPLE_TIME_2_5    0
#define ADC_SAMPLE_TIME_6_5    1
#define ADC_SAMPLE_TIME_12_5   2
#define ADC_SAMPLE_TIME_24_5   3
#define ADC_SAMPLE_TIME_47_5   4
#define ADC_SAMPLE_TIME_92_5   5
#define ADC_SAMPLE_TIME_247_5  6
#define ADC_SAMPLE_TIME_640_5  7


typedef struct _stm32l4_adc_driver_t {
    stm32l4_adc_t     *instances[ADC_INSTANCE_COUNT];
    volatile uint32_t adc;
} stm32l4_adc_driver_t;

static stm32l4_adc_driver_t stm32l4_adc_driver;

static ADC_TypeDef * const stm32l4_adc_xlate_ADC[ADC_INSTANCE_COUNT] = {
    ADC1,
#if defined(STM32L476xx) || defined(STM32L496xx)
    ADC2,
    ADC3,
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
};

bool stm32l4_adc_create(stm32l4_adc_t *adc, unsigned int instance, unsigned int priority, unsigned int mode)
{
    if (instance >= ADC_INSTANCE_COUNT)
    {
	return false;
    }

    adc->ADCx = stm32l4_adc_xlate_ADC[instance];
    adc->state = ADC_STATE_INIT;
    adc->instance = instance;
    adc->priority = priority;
    adc->callback = NULL;
    adc->context = NULL;
    adc->events = 0;
    
    stm32l4_adc_driver.instances[adc->instance] = adc;

    return true;
}

bool stm32l4_adc_destroy(stm32l4_adc_t *adc)
{
    if (adc->state != ADC_STATE_INIT)
    {
	return false;
    }

    stm32l4_adc_driver.instances[adc->instance] = NULL;

    adc->state = ADC_STATE_NONE;

    return true;
}

bool stm32l4_adc_enable(stm32l4_adc_t *adc, uint32_t option, stm32l4_adc_callback_t callback, void *context, uint32_t events)
{
    if (adc->state != ADC_STATE_INIT)
    {
	return false;
    }

#if 0
    /* Not needed as we always use HCLK/1 or HCLK/2 
     */
    armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_ADCSEL, (RCC_CCIPR_ADCSEL_0 | RCC_CCIPR_ADCSEL_1)); /* SYSCLK */
#endif

    adc->state = ADC_STATE_BUSY;

    if (!stm32l4_adc_configure(adc, option))
    {
	adc->state = ADC_STATE_INIT;

	return false;
    }

    stm32l4_adc_notify(adc, callback, context, events);

    adc->state = ADC_STATE_READY;

    return true;
}

bool stm32l4_adc_disable(stm32l4_adc_t *adc)
{
    ADC_TypeDef *ADCx = adc->ADCx;

    if (adc->state != ADC_STATE_READY)
    {
	return false;
    }

    ADCx->CR |= ADC_CR_ADDIS;

    while (ADCx->CR & ADC_CR_ADEN)
    {
    }

    ADCx->CR &= ~ADC_CR_ADVREGEN;
    ADCx->CR |= ADC_CR_DEEPPWD;

    if (adc->instance == ADC_INSTANCE_ADC1)
    {
#if defined(STM32L476xx) || defined(STM32L496xx)
	armv7m_atomic_and(&ADC123_COMMON->CCR, ~(ADC_CCR_VBATEN | ADC_CCR_VREFEN));
#else /* defined(STM32L476xx) || defined(STM32L496xx) */
	armv7m_atomic_and(&ADC1_COMMON->CCR, ~(ADC_CCR_VBATEN | ADC_CCR_VREFEN));
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
    }

    stm32l4_system_periph_cond_disable(SYSTEM_PERIPH_ADC, &stm32l4_adc_driver.adc, (1ul << adc->instance));

    adc->events = 0;
    adc->callback = NULL;
    adc->context = NULL;

    adc->state = ADC_STATE_INIT;

    return true;
}

bool stm32l4_adc_configure(stm32l4_adc_t *adc, uint32_t option)
{
    ADC_TypeDef *ADCx = adc->ADCx;

    if ((adc->state != ADC_STATE_BUSY) && (adc->state != ADC_STATE_READY))
    {
	return false;
    }
    
    if (adc->state == ADC_STATE_BUSY)
    {
	stm32l4_system_periph_cond_enable(SYSTEM_PERIPH_ADC, &stm32l4_adc_driver.adc, (1ul << adc->instance));

	if ((stm32l4_system_hclk() <= 48000000) && (stm32l4_system_hclk() == stm32l4_system_sysclk()))
	{
#if defined(STM32L476xx) || defined(STM32L496xx)
	    armv7m_atomic_modify(&ADC123_COMMON->CCR, ADC_CCR_CKMODE, ADC_CCR_CKMODE_0); /* HCLK / 1 */
#else /* defined(STM32L476xx) || defined(STM32L496xx) */
	    armv7m_atomic_modify(&ADC1_COMMON->CCR, ADC_CCR_CKMODE, ADC_CCR_CKMODE_0); /* HCLK / 1 */
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
	}
	else
	{
#if defined(STM32L476xx) || defined(STM32L496xx)
	    armv7m_atomic_modify(&ADC123_COMMON->CCR, ADC_CCR_CKMODE, ADC_CCR_CKMODE_1); /* HCLK / 2 */
#else /* defined(STM32L476xx) || defined(STM32L496xx) */
	    armv7m_atomic_modify(&ADC1_COMMON->CCR, ADC_CCR_CKMODE, ADC_CCR_CKMODE_1); /* HCLK / 2 */
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
	}

	if (adc->instance == ADC_INSTANCE_ADC1)
	{
#if defined(STM32L476xx) || defined(STM32L496xx)
	    armv7m_atomic_or(&ADC123_COMMON->CCR, (ADC_CCR_VBATEN | ADC_CCR_VREFEN));
#else /* defined(STM32L476xx) || defined(STM32L496xx) */
	    armv7m_atomic_or(&ADC1_COMMON->CCR, (ADC_CCR_VBATEN | ADC_CCR_VREFEN));
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
	}

	ADCx->CR &= ~ADC_CR_DEEPPWD;

	ADCx->CR |= ADC_CR_ADVREGEN;

	armv7m_core_udelay(20);

	/* Finally turn on the ADC */

	ADCx->ISR = ADC_ISR_ADRDY;

	do
	{
	    ADCx->CR |= ADC_CR_ADEN;
	}
	while (!(ADCx->ISR & ADC_ISR_ADRDY));

	ADCx->CFGR = ADC_CFGR_OVRMOD | ADC_CFGR_JQDIS;
    }

    return true;
}

bool stm32l4_adc_notify(stm32l4_adc_t *adc, stm32l4_adc_callback_t callback, void *context, uint32_t events)
{
    if ((adc->state != ADC_STATE_BUSY) && (adc->state != ADC_STATE_READY))
    {
	return false;
    }

    adc->events = 0;

    adc->callback = callback;
    adc->context = context;
    adc->events = events;

    return true;
}

bool stm32l4_adc_calibrate(stm32l4_adc_t *adc)
{
    ADC_TypeDef *ADCx = adc->ADCx;

    if (adc->state != ADC_STATE_READY)
    {
	return false;
    }

    ADCx->CR |= ADC_CR_ADDIS;

    while (ADCx->CR & ADC_CR_ADEN)
    {
    }

    /* Single-Ended Input Calibration */
    ADCx->CR &= ~ADC_CR_ADCALDIF;
    ADCx->CR |= ADC_CR_ADCAL;
    
    while (ADCx->CR & ADC_CR_ADCAL)
    {
    }

    /* Differential Input Calibration */
    ADCx->CR |= (ADC_CR_ADCALDIF | ADC_CR_ADCAL);

    while (ADCx->CR & ADC_CR_ADCAL)
    {
    }

    armv7m_core_udelay(100);

    ADCx->ISR = ADC_ISR_ADRDY;

    do
    {
	ADCx->CR |= ADC_CR_ADEN;
    }
    while (!(ADCx->ISR & ADC_ISR_ADRDY));

    return true;
}

uint32_t stm32l4_adc_convert(stm32l4_adc_t *adc, unsigned int channel, unsigned int period)
{
    ADC_TypeDef *ADCx = adc->ADCx;
    uint32_t convert, threshold, adcclk, adc_smp;

    if (adc->state != ADC_STATE_READY)
    {
	return false;
    }

    /* Silicon ERRATA 2.4.4. Wrong ADC conversion results when delay between
     * calibration and first conversion or between 2 consecutive conversions is too long. 
     */

    if ((adc->instance == ADC_INSTANCE_ADC1) && (channel == ADC_CHANNEL_ADC1_TS))
    {
	ADCx->CR |= ADC_CR_ADDIS;

	while (ADCx->CR & ADC_CR_ADEN)
	{
	}
	    
#if defined(STM32L476xx) || defined(STM32L496xx)
	armv7m_atomic_or(&ADC123_COMMON->CCR, ADC_CCR_TSEN);
#else /* defined(STM32L476xx) || defined(STM32L496xx) */
	armv7m_atomic_or(&ADC1_COMMON->CCR, ADC_CCR_TSEN);
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */

	ADCx->ISR = ADC_ISR_ADRDY;

	do
	{
	    ADCx->CR |= ADC_CR_ADEN;
	}
	while (!(ADCx->ISR & ADC_ISR_ADRDY));

	armv7m_core_udelay(120);
    }

    if ((stm32l4_system_hclk() <= 48000000) && (stm32l4_system_hclk() == stm32l4_system_sysclk()))
    {
	adcclk = stm32l4_system_hclk();
    }
    else
    {
	adcclk = stm32l4_system_hclk() / 2;
    }

    /* period is in uS. 1e6 / adcclk is one tick in terms of uS.
     *
     * (period * adcclk) / 1e6 is the threshold for the sampling time.
     *
     * The upper limit for period is 50uS, and adcclk limited to 48MHz,
     * which means no overflow handling is needed.
     */

    if (period > 50)
    {
        period = 50;
    }
    
    threshold = ((uint32_t)period * adcclk);

    if      (threshold < (uint32_t)(  2.5 * 1e6)) { adc_smp = ADC_SAMPLE_TIME_2_5;   } 
    else if (threshold < (uint32_t)(  6.5 * 1e6)) { adc_smp = ADC_SAMPLE_TIME_6_5;   } 
    else if (threshold < (uint32_t)( 12.5 * 1e6)) { adc_smp = ADC_SAMPLE_TIME_12_5;  } 
    else if (threshold < (uint32_t)( 24.5 * 1e6)) { adc_smp = ADC_SAMPLE_TIME_24_5;  } 
    else if (threshold < (uint32_t)( 47.5 * 1e6)) { adc_smp = ADC_SAMPLE_TIME_47_5;  } 
    else if (threshold < (uint32_t)( 92.5 * 1e6)) { adc_smp = ADC_SAMPLE_TIME_92_5;  } 
    else if (threshold < (uint32_t)(247.5 * 1e6)) { adc_smp = ADC_SAMPLE_TIME_247_5; } 
    else                                          { adc_smp = ADC_SAMPLE_TIME_640_5; } 

    ADCx->SQR1 = (channel << 6);
    ADCx->SMPR1 = (channel < 10) ? (adc_smp << (channel * 3)) : 0;
    ADCx->SMPR2 = (channel >= 10) ? (adc_smp << ((channel * 3) - 30)) : 0;

    ADCx->CR |= ADC_CR_ADSTART;
    
    while (!(ADCx->ISR & ADC_ISR_EOC))
    {
    }
    
    convert = ADCx->DR & ADC_DR_RDATA;
	
    ADCx->ISR = ADC_ISR_EOC;
    
    ADCx->CR |= ADC_CR_ADSTART;
    
    while (!(ADCx->ISR & ADC_ISR_EOC))
    {
    }
    
    convert = ADCx->DR & ADC_DR_RDATA;

    ADCx->ISR = ADC_ISR_EOC;

    if ((adc->instance == ADC_INSTANCE_ADC1) && (channel == ADC_CHANNEL_ADC1_TS))
    {
	ADCx->CR |= ADC_CR_ADDIS;

	while (ADCx->CR & ADC_CR_ADEN)
	{
	}
	
#if defined(STM32L476xx) || defined(STM32L496xx)
	armv7m_atomic_and(&ADC123_COMMON->CCR, ~ADC_CCR_TSEN);
#else /* defined(STM32L476xx) || defined(STM32L496xx) */
	armv7m_atomic_and(&ADC1_COMMON->CCR, ~ADC_CCR_TSEN);
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */

	ADCx->ISR = ADC_ISR_ADRDY;

	do
	{
	    ADCx->CR |= ADC_CR_ADEN;
	}
	while (!(ADCx->ISR & ADC_ISR_ADRDY));
    }

    return convert;
}
