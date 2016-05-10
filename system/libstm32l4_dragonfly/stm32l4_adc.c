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

#include <stdio.h>

#include "stm32l4xx.h"

#include "armv7m.h"

#include "stm32l4_adc.h"
#include "stm32l4_gpio.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_adc_driver_t {
    stm32l4_adc_t   *instances[ADC_INSTANCE_COUNT];
} stm32l4_adc_driver_t;

static stm32l4_adc_driver_t stm32l4_adc_driver;

static ADC_TypeDef * const stm32l4_adc_xlate_ADC[ADC_INSTANCE_COUNT] = {
    ADC1,
    ADC2,
    ADC3,
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

    armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_ADCSEL, (RCC_CCIPR_ADCSEL_0 | RCC_CCIPR_ADCSEL_1)); /* SYSCLK */
    
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
	armv7m_atomic_and(&ADC123_COMMON->CCR, ~(ADC_CCR_VBATEN | ADC_CCR_TSEN | ADC_CCR_VREFEN));
    }

    if (!stm32l4_adc_driver.instances[ADC_INSTANCE_ADC2] && !stm32l4_adc_driver.instances[ADC_INSTANCE_ADC3])
    {
	stm32l4_system_periph_disable(SYSTEM_PERIPH_ADC);
    }

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
	stm32l4_system_periph_enable(SYSTEM_PERIPH_ADC);

	if (adc->instance == ADC_INSTANCE_ADC1)
	{
	    armv7m_atomic_or(&ADC123_COMMON->CCR, (ADC_CCR_VBATEN | ADC_CCR_TSEN | ADC_CCR_VREFEN));
	}

	/* The shared clock is left to be async SYSCLK ... */

	ADCx->CR &= ~ADC_CR_DEEPPWD;

	ADCx->CR |= ADC_CR_ADVREGEN;

	armv7m_clock_spin(20000);


	/* Finally turn on the ADC */

	ADCx->ISR = ADC_ISR_ADRDY;

	ADCx->CR |= ADC_CR_ADEN;

	while (!(ADCx->ISR & ADC_ISR_ADRDY))
	{
	}
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
    
    ADCx->ISR = ADC_ISR_ADRDY;
    
    ADCx->CR |= ADC_CR_ADEN;
    
    while (!(ADCx->ISR & ADC_ISR_ADRDY))
    {
    }

    return true;
}

uint32_t stm32l4_adc_convert(stm32l4_adc_t *adc, unsigned int channel)
{
    ADC_TypeDef *ADCx = adc->ADCx;

    if (adc->state != ADC_STATE_READY)
    {
	return false;
    }

#if 0
    ADCx->SQR1 = (channel << 6);
    ADCx->SMPR1 = ADC_SMPR1_SMP0_2;  /* 47.5 */

    ADCx->ISR = ADC_ISR_EOC;

    ADCx->CR |= ADC_CR_ADSTART;

    while (!(ADCx->ISR & ADC_ISR_EOC))
    {
    }

    ADCx->ISR = ADC_ISR_EOC;

    ADCx->CR |= ADC_CR_ADSTART;

    while (!(ADCx->ISR & ADC_ISR_EOC))
    {
    }
#endif

    /* Silicon ERRATA 2.4.4. Wrong ADC conversion results when delay between
     * calibration and first conversion or between 2 consecutive conversions is too long. 
     *
     * The WAR is to do 2 back to back conversions that cannot be interrupted.
     */
    ADCx->SQR1 = (channel << 12) | (channel << 6) | 1;
    ADCx->SMPR1 = ADC_SMPR1_SMP0_2 | ADC_SMPR1_SMP1_2;  /* 47.5 */

    ADCx->ISR = ADC_ISR_EOC;

    ADCx->CR |= ADC_CR_ADSTART;

    while (!(ADCx->ISR & ADC_ISR_EOC))
    {
    }

    return ADCx->DR & ADC_DR_RDATA;
}
