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

#include "stm32l4_gpio.h"
#include "stm32l4_timer.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_timer_driver_t {
    stm32l4_timer_t   *instances[TIMER_INSTANCE_COUNT];
} stm32l4_timer_driver_t;

static stm32l4_timer_driver_t stm32l4_timer_driver;

static TIM_TypeDef * const stm32l4_timer_xlate_TIM[TIMER_INSTANCE_COUNT] = {
    TIM1,
    TIM2,
#if defined(STM32L476xx) || defined(STM32L496xx)
    TIM3,
    TIM4,
    TIM5,
#endif
    TIM6,
    TIM7,
#if defined(STM32L476xx) || defined(STM32L496xx)
    TIM8,
#endif
    TIM15,
    TIM16,
#if defined(STM32L476xx) || defined(STM32L496xx)
    TIM17,
#endif
};

static const IRQn_Type stm32l4_timer_xlate_IRQn[TIMER_INSTANCE_COUNT] = {
    TIM1_CC_IRQn,
    TIM2_IRQn,
#if defined(STM32L476xx) || defined(STM32L496xx)
    TIM3_IRQn,
    TIM4_IRQn,
    TIM5_IRQn,
#endif
    TIM6_DAC_IRQn,
    TIM7_IRQn,
#if defined(STM32L476xx) || defined(STM32L496xx)
    TIM8_CC_IRQn,
#endif
    TIM1_BRK_TIM15_IRQn,
    TIM1_UP_TIM16_IRQn,
#if defined(STM32L476xx) || defined(STM32L496xx)
    TIM1_TRG_COM_TIM17_IRQn,
#endif
};

static void stm32l4_timer_interrupt(stm32l4_timer_t *timer)
{
    TIM_TypeDef *TIM = timer->TIM;
    uint32_t tim_sr, events;

    events = 0;

    tim_sr = TIM->SR;
    // TIM->SR = (TIM_SR_UIF | TIM_SR_CC1IF | TIM_SR_CC2IF | TIM_SR_CC3IF | TIM_SR_CC4IF | TIM_SR_CC1OF | TIM_SR_CC2OF | TIM_SR_CC3OF | TIM_SR_CC4OF);
    TIM->SR = 0;

    if (timer)
    {
	if (tim_sr & TIM_SR_UIF)
	{
	    events |= TIMER_EVENT_PERIOD;
	}

	if (tim_sr & (TIM_SR_CC1IF | TIM_SR_CC2IF | TIM_SR_CC3IF | TIM_SR_CC4IF))
	{
	    if (tim_sr & TIM_SR_CC1IF)
	    {
		events |= TIMER_EVENT_CHANNEL_1;

		timer->capture[0] = TIM->CCR1;
	    }

	    if (tim_sr & TIM_SR_CC2IF)
	    {
		events |= TIMER_EVENT_CHANNEL_2;

		timer->capture[1] = TIM->CCR2;
	    }

	    if (tim_sr & TIM_SR_CC3IF)
	    {
		events |= TIMER_EVENT_CHANNEL_3;

		timer->capture[2] = TIM->CCR3;
	    }

	    if (tim_sr & TIM_SR_CC4IF)
	    {
		events |= TIMER_EVENT_CHANNEL_4;

		timer->capture[3] = TIM->CCR4;
	    }
	}
    
	events &= timer->events;

	if (events)
	{
	    (*timer->callback)(timer->context, events);
	}
    }
}

bool stm32l4_timer_create(stm32l4_timer_t *timer, unsigned int instance, unsigned int priority, unsigned int mode)
{
    if (instance >= TIMER_INSTANCE_COUNT)
    {
	return false;
    }

    timer->TIM = stm32l4_timer_xlate_TIM[instance];
    timer->state = TIMER_STATE_INIT;
    timer->instance = instance;
    timer->priority = priority;
    timer->callback = NULL;
    timer->context = NULL;
    timer->events = 0;
    timer->channels = 0;
    timer->capture[0] = 0;
    timer->capture[1] = 0;
    timer->capture[2] = 0;
    timer->capture[3] = 0;
    
    stm32l4_timer_driver.instances[timer->instance] = timer;

    return true;
}

bool stm32l4_timer_destroy(stm32l4_timer_t *timer)
{
    if (timer->state != TIMER_STATE_INIT)
    {
	return false;
    }

    stm32l4_timer_driver.instances[timer->instance] = NULL;

    timer->state = TIMER_STATE_NONE;

    return true;
}

uint32_t stm32l4_timer_clock(stm32l4_timer_t *timer)
{
    uint32_t hclk, pclk;

    hclk = stm32l4_system_hclk();

    if ((timer->instance == TIMER_INSTANCE_TIM2) ||
#if defined(STM32L476xx) || defined(STM32L496xx)
	(timer->instance == TIMER_INSTANCE_TIM3) ||
	(timer->instance == TIMER_INSTANCE_TIM4) ||
	(timer->instance == TIMER_INSTANCE_TIM5) ||
#endif
	(timer->instance == TIMER_INSTANCE_TIM6) ||
	(timer->instance == TIMER_INSTANCE_TIM7))
    {
	pclk = stm32l4_system_pclk1();
    }
    else
    {
	pclk = stm32l4_system_pclk2();
    }

    return ((hclk == pclk) ? hclk : (2 * pclk));
}

bool stm32l4_timer_enable(stm32l4_timer_t *timer, uint32_t prescaler, uint32_t period, uint32_t option, stm32l4_timer_callback_t callback, void *context, uint32_t events)
{
    if (timer->state != TIMER_STATE_INIT)
    {
	return false;
    }

    timer->state = TIMER_STATE_BUSY;

    if (!stm32l4_timer_configure(timer, prescaler, period, option))
    {
	timer->state = TIMER_STATE_INIT;

	return false;
    }

    stm32l4_timer_notify(timer, callback, context, events);

    timer->state = TIMER_STATE_READY;

    if (timer->instance == TIMER_INSTANCE_TIM1)
    {
	NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, timer->priority);
	NVIC_SetPriority(TIM1_UP_TIM16_IRQn, timer->priority);
#if defined(STM32L476xx) || defined(STM32L496xx)
	NVIC_SetPriority(TIM1_TRG_COM_TIM17_IRQn, timer->priority);
#else
	NVIC_SetPriority(TIM1_TRG_COM_IRQn, timer->priority);
#endif
	NVIC_SetPriority(TIM1_CC_IRQn, timer->priority);

	NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);
	NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
#if defined(STM32L476xx) || defined(STM32L496xx)
	NVIC_EnableIRQ(TIM1_TRG_COM_TIM17_IRQn);
#else
	NVIC_EnableIRQ(TIM1_TRG_COM_IRQn);
#endif
	NVIC_EnableIRQ(TIM1_CC_IRQn);
    }
#if defined(STM32L476xx) || defined(STM32L496xx)
    else if (timer->instance == TIMER_INSTANCE_TIM8)
    {
	NVIC_SetPriority(TIM8_BRK_IRQn, timer->priority);
	NVIC_SetPriority(TIM8_UP_IRQn, timer->priority);
	NVIC_SetPriority(TIM8_TRG_COM_IRQn, timer->priority);
	NVIC_SetPriority(TIM8_CC_IRQn, timer->priority);

	NVIC_EnableIRQ(TIM8_BRK_IRQn);
	NVIC_EnableIRQ(TIM8_UP_IRQn);
	NVIC_EnableIRQ(TIM8_TRG_COM_IRQn);
	NVIC_EnableIRQ(TIM8_CC_IRQn);
    }
#endif
    else
    {
	NVIC_SetPriority(stm32l4_timer_xlate_IRQn[timer->instance], timer->priority);

	NVIC_EnableIRQ(stm32l4_timer_xlate_IRQn[timer->instance]);
    }

    return true;
}

bool stm32l4_timer_disable(stm32l4_timer_t *timer)
{
    if (timer->state != TIMER_STATE_READY)
    {
	return false;
    }

    timer->events = 0;
    timer->callback = NULL;
    timer->context = NULL;

    switch (timer->instance) {
    case TIMER_INSTANCE_TIM1:
	armv7m_atomic_and(&RCC->APB2ENR, ~RCC_APB2ENR_TIM1EN);
	break;
    case TIMER_INSTANCE_TIM2:
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_TIM2EN);
	break;
#if defined(STM32L476xx) || defined(STM32L496xx)
    case TIMER_INSTANCE_TIM3:
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_TIM3EN);
	break;
    case TIMER_INSTANCE_TIM4:
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_TIM4EN);
	break;
    case TIMER_INSTANCE_TIM5:
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_TIM5EN);
	break;
#endif
    case TIMER_INSTANCE_TIM6:
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_TIM6EN);
	break;
    case TIMER_INSTANCE_TIM7:
	armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_TIM7EN);
	break;
#if defined(STM32L476xx) || defined(STM32L496xx)
    case TIMER_INSTANCE_TIM8:
	armv7m_atomic_and(&RCC->APB2ENR, ~RCC_APB2ENR_TIM8EN);
	break;
#endif
    case TIMER_INSTANCE_TIM15:
	armv7m_atomic_and(&RCC->APB2ENR, ~RCC_APB2ENR_TIM15EN);
	break;
    case TIMER_INSTANCE_TIM16:
	armv7m_atomic_and(&RCC->APB2ENR, ~RCC_APB2ENR_TIM16EN);
	break;
#if defined(STM32L476xx) || defined(STM32L496xx)
    case TIMER_INSTANCE_TIM17:
	armv7m_atomic_and(&RCC->APB2ENR, ~RCC_APB2ENR_TIM17EN);
	break;
#endif
    }

    timer->state = TIMER_STATE_INIT;

    return true;
}

bool stm32l4_timer_configure(stm32l4_timer_t *timer, uint32_t prescaler, uint32_t period, uint32_t option)
{
    TIM_TypeDef *TIM = timer->TIM;
    uint32_t tim_cr1, tim_smcr, tim_bdtr;

    if ((timer->state != TIMER_STATE_BUSY) && (timer->state != TIMER_STATE_READY))
    {
	return false;
    }
    
    if (timer->state == TIMER_STATE_BUSY)
    {
	switch (timer->instance) {
	case TIMER_INSTANCE_TIM1:
	    armv7m_atomic_or(&RCC->APB2ENR, RCC_APB2ENR_TIM1EN);
	    break;
	case TIMER_INSTANCE_TIM2:
	    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_TIM2EN);
	    break;
#if defined(STM32L476xx) || defined(STM32L496xx)
	case TIMER_INSTANCE_TIM3:
	    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_TIM3EN);
	    break;
	case TIMER_INSTANCE_TIM4:
	    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_TIM4EN);
	    break;
	case TIMER_INSTANCE_TIM5:
	    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_TIM5EN);
	    break;
#endif
	case TIMER_INSTANCE_TIM6:
	    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_TIM6EN);
	    break;
	case TIMER_INSTANCE_TIM7:
	    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_TIM7EN);
	    break;
#if defined(STM32L476xx) || defined(STM32L496xx)
	case TIMER_INSTANCE_TIM8:
	    armv7m_atomic_or(&RCC->APB2ENR, RCC_APB2ENR_TIM8EN);
	    break;
#endif
	case TIMER_INSTANCE_TIM15:
	    armv7m_atomic_or(&RCC->APB2ENR, RCC_APB2ENR_TIM15EN);
	    break;
	case TIMER_INSTANCE_TIM16:
	    armv7m_atomic_or(&RCC->APB2ENR, RCC_APB2ENR_TIM16EN);
	    break;
#if defined(STM32L476xx) || defined(STM32L496xx)
	case TIMER_INSTANCE_TIM17:
	    armv7m_atomic_or(&RCC->APB2ENR, RCC_APB2ENR_TIM17EN);
	    break;
#endif
	}
    }

    tim_cr1 = 0;
    tim_smcr = 0;
    tim_bdtr = TIM_BDTR_MOE;

    if (option & TIMER_OPTION_ENCODER_MODE_MASK)
    {
	tim_smcr |= (((option & TIMER_OPTION_ENCODER_MODE_MASK) >> TIMER_OPTION_ENCODER_MODE_SHIFT) << 0);
    }
    else
    {
	if (option & (TIMER_OPTION_CLOCK_EXTERNAL_CHANNEL_1 | TIMER_OPTION_CLOCK_EXTERNAL_CHANNEL_2))
	{
	    tim_smcr |= (TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_2); /* EXTERNAL 1 */

	    if (option & TIMER_OPTION_CLOCK_EXTERNAL_CHANNEL_1)
	    {
		tim_smcr |= (TIM_SMCR_TS_0 | TIM_SMCR_TS_2); /* TI1FP1 */
	
	    }
	    else
	    {
		tim_smcr |= (TIM_SMCR_TS_1 | TIM_SMCR_TS_2); /* TI2FP2 */
	    }
	}

	if (option & TIMER_OPTION_COUNT_CENTER_MASK)
	{
	    tim_cr1 |= (((option & TIMER_OPTION_COUNT_CENTER_MASK) >> TIMER_OPTION_COUNT_CENTER_SHIFT) << 5);
	}
	else
	{
	    if (option & TIMER_OPTION_COUNT_DOWN)
	    {
		tim_cr1 |= TIM_CR1_DIR;
	    }
	}

	if (option & TIMER_OPTION_COUNT_PRELOAD)
	{
	    tim_cr1 |= TIM_CR1_ARPE;
	}
    }

    TIM->CR1  = tim_cr1;
    TIM->SMCR = tim_smcr;
    TIM->BDTR = tim_bdtr;
    TIM->ARR  = period;
    TIM->PSC  = prescaler;

    if (timer->instance == TIMER_INSTANCE_TIM1)
    {
	TIM->RCR = 0;
    }

#if defined(STM32L476xx) || defined(STM32L496xx)
    if (timer->instance == TIMER_INSTANCE_TIM8)
    {
	TIM->RCR = 0;
    }
#endif

    // TIM->EGR = TIM_EGR_UG;

    return true;
}


bool stm32l4_timer_notify(stm32l4_timer_t *timer, stm32l4_timer_callback_t callback, void *context, uint32_t events)
{
    TIM_TypeDef *TIM = timer->TIM;
    uint32_t dier; 

    if ((timer->state != TIMER_STATE_BUSY) && (timer->state != TIMER_STATE_READY))
    {
	return false;
    }

    timer->callback = callback;
    timer->context = context;
    timer->events = events;

    dier = 0;

    if (events & TIMER_EVENT_PERIOD)
    {
	dier |= TIM_DIER_UIE;
    }

    if ((events & timer->channels) & TIMER_EVENT_CHANNEL_1)
    {
	dier |= TIM_DIER_CC1IE;
    }

    if ((events & timer->channels) & TIMER_EVENT_CHANNEL_2)
    {
	dier |= TIM_DIER_CC2IE;
    }

    if ((events & timer->channels) & TIMER_EVENT_CHANNEL_3)
    {
	dier |= TIM_DIER_CC3IE;
    }

    if ((events & timer->channels) & TIMER_EVENT_CHANNEL_4)
    {
	dier |= TIM_DIER_CC4IE;
    }
    
    armv7m_atomic_modify(&TIM->DIER, (TIM_DIER_UIE | TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC3IE | TIM_DIER_CC4IE), dier);

    return true;
}

bool stm32l4_timer_start(stm32l4_timer_t *timer, bool oneshot)
{
    TIM_TypeDef *TIM = timer->TIM;

    if ((timer->state != TIMER_STATE_READY) && (timer->state != TIMER_STATE_ACTIVE))
    {
	return false;
    }

    TIM->SR = 0;

    if (oneshot)
    {
	armv7m_atomic_or(&TIM->CR1, (TIM_CR1_OPM | TIM_CR1_CEN));
    }
    else
    {
	armv7m_atomic_and(&TIM->CR1, ~TIM_CR1_OPM);
	armv7m_atomic_or(&TIM->CR1, TIM_CR1_CEN);
    }

    timer->state = TIMER_STATE_ACTIVE;

    return true;
}

bool stm32l4_timer_stop(stm32l4_timer_t *timer)
{
    TIM_TypeDef *TIM = timer->TIM;

    if (timer->state != TIMER_STATE_ACTIVE)
    {
	return false;
    }

    armv7m_atomic_and(&TIM->CR1, ~TIM_CR1_CEN);

    timer->state = TIMER_STATE_READY;

    return true;
}

uint32_t stm32l4_timer_count(stm32l4_timer_t *timer)
{
    TIM_TypeDef *TIM = timer->TIM;

    if ((timer->state != TIMER_STATE_READY) && (timer->state != TIMER_STATE_ACTIVE))
    {
	return 0;
    }

    return TIM->CNT;
}

bool stm32l4_timer_period(stm32l4_timer_t *timer, uint32_t period, bool offset)
{
    TIM_TypeDef *TIM = timer->TIM;
    uint32_t primask;

    if ((timer->state != TIMER_STATE_READY) && (timer->state != TIMER_STATE_ACTIVE))
    {
	return false;
    }

    if (offset)
    {
	primask = __get_PRIMASK();

	__disable_irq();

	TIM->ARR = period - TIM->CNT;

	__set_PRIMASK(primask);
    }
    else
    {
	TIM->ARR = period;
    }

    return true;
}

bool stm32l4_timer_channel(stm32l4_timer_t *timer, unsigned int channel, uint32_t compare, uint32_t control)
{
    TIM_TypeDef *TIM = timer->TIM;
    uint32_t tim_ccmr, tim_ccer;

    if ((timer->state != TIMER_STATE_READY) && (timer->state != TIMER_STATE_ACTIVE))
    {
	return false;
    }

    armv7m_atomic_and(&TIM->CCER, ~(TIM_CCER_CC1E << (channel * 4)));

    tim_ccmr = 0;
    tim_ccer = 0;

    if (control & (TIMER_CONTROL_CAPTURE_MASK | TIMER_CONTROL_COMPARE_MASK))
    {
	if (control & TIMER_CONTROL_CAPTURE_MASK)
	{
	    tim_ccer |= TIM_CCER_CC1E;

	    tim_ccmr |= ((control & TIMER_CONTROL_CAPTURE_ALTERNATE ? TIM_CCMR1_CC1S_1 : TIM_CCMR1_CC1S_0) |
			 (((control & TIMER_CONTROL_CAPTURE_PRESCALE_MASK) >> TIMER_CONTROL_CAPTURE_PRESCALE_SHIFT) << 2) |
			 (((control & TIMER_CONTROL_CAPTURE_FILTER_MASK) >> TIMER_CONTROL_CAPTURE_FILTER_SHIFT) << 4));
	    
	    if (control & TIMER_CONTROL_CAPTURE_POLARITY)
	    {
		if (control & TIMER_CONTROL_CAPTURE_RISING_EDGE)
		{
		    tim_ccer |= ((control & TIMER_CONTROL_CAPTURE_FALLING_EDGE) ? (TIM_CCER_CC1P | TIM_CCER_CC1NP) : TIM_CCER_CC1P);
		}
	    }
	    else
	    {
		if (control & TIMER_CONTROL_CAPTURE_FALLING_EDGE)
		{
		    tim_ccer |= ((control & TIMER_CONTROL_CAPTURE_RISING_EDGE) ? (TIM_CCER_CC1P | TIM_CCER_CC1NP) : TIM_CCER_CC1P);
		}
	    }
 	}
	else
	{
	    switch (control & TIMER_CONTROL_COMPARE_MASK) {
	    case TIMER_CONTROL_COMPARE_TIMING:          tim_ccmr |= (                  (0 << 4)); break;
	    case TIMER_CONTROL_COMPARE_ACTIVE:          tim_ccmr |= (                  (1 << 4)); break;
	    case TIMER_CONTROL_COMPARE_INACTIVE:        tim_ccmr |= (                  (2 << 4)); break;
	    case TIMER_CONTROL_COMPARE_TOGGLE:          tim_ccmr |= (                  (3 << 4)); break;
	    case TIMER_CONTROL_COMPARE_FORCED_ACTIVE:   tim_ccmr |= (                  (5 << 4)); break;
	    case TIMER_CONTROL_COMPARE_FORCED_INACTIVE: tim_ccmr |= (                  (4 << 4)); break;
	    case TIMER_CONTROL_PWM:                     tim_ccmr |= (TIM_CCMR1_OC1PE | (6 << 4)); break;
	    case TIMER_CONTROL_PWM_INVERTED:            tim_ccmr |= (TIM_CCMR1_OC1PE | (7 << 4)); break;
	    }

	    tim_ccer |= (TIM_CCER_CC1E |
			 TIM_CCER_CC1NE |
			 ((control & TIMER_CONTROL_PWM_COMPLEMENTARY) ? 0 : TIM_CCER_CC1NP));
	}

	if (channel <= TIMER_CHANNEL_4)
	{
	    armv7m_atomic_or(&timer->channels, (TIMER_EVENT_CHANNEL_1 << channel));
	}
    }
    else
    {
	/* If no CAPTURE/COMPARE is used the input can still be used clock input.
	 */

	tim_ccer |= TIM_CCER_CC1E;

	tim_ccmr |= ((control & TIMER_CONTROL_CAPTURE_ALTERNATE ? TIM_CCMR1_CC1S_1 : TIM_CCMR1_CC1S_0) |
		     (((control & TIMER_CONTROL_CAPTURE_PRESCALE_MASK) >> TIMER_CONTROL_CAPTURE_PRESCALE_SHIFT) << 2) |
		     (((control & TIMER_CONTROL_CAPTURE_FILTER_MASK) >> TIMER_CONTROL_CAPTURE_FILTER_SHIFT) << 4));
	
	if (control & TIMER_CONTROL_CAPTURE_POLARITY)
	{
	    tim_ccer |= TIM_CCER_CC1P;
	}
	    
	if (channel > TIMER_CHANNEL_4)
	{
	    armv7m_atomic_and(&timer->channels, ~(TIMER_EVENT_CHANNEL_1 << channel));
	}
    }

    if (channel <= TIMER_CHANNEL_4)
    {
	if ((timer->channels & timer->events) & (TIMER_EVENT_CHANNEL_1 << channel))
	{
	    armv7m_atomic_or(&TIM->DIER, (TIM_DIER_CC1IE << channel));
	}
	else
	{
	    armv7m_atomic_and(&TIM->DIER, ~(TIM_DIER_CC1IE << channel));
	}
    }

    switch (channel) {
    case TIMER_CHANNEL_1: armv7m_atomic_modify(&TIM->CCMR1, 0x000100ff, (tim_ccmr << 0)); TIM->CCR1 = compare; break;
    case TIMER_CHANNEL_2: armv7m_atomic_modify(&TIM->CCMR1, 0x0100ff00, (tim_ccmr << 8)); TIM->CCR2 = compare; break;
    case TIMER_CHANNEL_3: armv7m_atomic_modify(&TIM->CCMR2, 0x000100ff, (tim_ccmr << 0)); TIM->CCR3 = compare; break;
    case TIMER_CHANNEL_4: armv7m_atomic_modify(&TIM->CCMR2, 0x0100ff00, (tim_ccmr << 8)); TIM->CCR4 = compare; break;
    case TIMER_CHANNEL_5: armv7m_atomic_modify(&TIM->CCMR2, 0x000100ff, (tim_ccmr << 0)); TIM->CCR5 = compare; break;
    case TIMER_CHANNEL_6: armv7m_atomic_modify(&TIM->CCMR2, 0x0100ff00, (tim_ccmr << 8)); TIM->CCR6 = compare; break;
    }
	
    armv7m_atomic_modify(&TIM->CCER, (0x0000000f << (channel * 4)), (tim_ccer << (channel * 4)));
	
    return true;
}

bool stm32l4_timer_compare(stm32l4_timer_t *timer, unsigned int channel, uint32_t compare)
{
    TIM_TypeDef *TIM = timer->TIM;

    if ((timer->state != TIMER_STATE_READY) && (timer->state != TIMER_STATE_ACTIVE))
    {
	return false;
    }

    switch (channel) {
    case TIMER_CHANNEL_1: TIM->CCR1 = compare; break;
    case TIMER_CHANNEL_2: TIM->CCR2 = compare; break;
    case TIMER_CHANNEL_3: TIM->CCR3 = compare; break;
    case TIMER_CHANNEL_4: TIM->CCR4 = compare; break;
    case TIMER_CHANNEL_5: TIM->CCR5 = compare; break;
    case TIMER_CHANNEL_6: TIM->CCR6 = compare; break;
    }
    
    return true;
}

uint32_t stm32l4_timer_capture(stm32l4_timer_t *timer, unsigned int channel)
{
    if (channel <= TIMER_CHANNEL_4)
    {
	return timer->capture[channel];
    }
    else
    {
	return 0;
    }
}

void TIM1_BRK_TIM15_IRQHandler(void)
{
    if (TIM1->SR & (TIM_SR_BIF | TIM_SR_B2IF | TIM_SR_SBIF))
    {
	stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM1]);
    }

    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM15]);
}

void TIM1_UP_TIM16_IRQHandler(void)
{
    if (TIM1->SR & TIM_SR_UIF)
    {
	stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM1]);
    }

    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM16]);
}

#if defined(STM32L476xx) || defined(STM32L496xx)
void TIM1_TRG_COM_TIM17_IRQHandler(void)
{
    if (TIM1->SR & (TIM_SR_TIF | TIM_SR_COMIF))
    {
	stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM1]);
    }

    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM17]);
}

#else

void TIM1_TRG_COM_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM1]);
}

#endif

void TIM1_CC_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM1]);
}

void TIM2_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM2]);
}

#if defined(STM32L476xx) || defined(STM32L496xx)

void TIM3_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM3]);
}

void TIM4_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM4]);
}

void TIM5_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM5]);
}

#endif

void TIM6_DAC_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM6]);
}

void TIM7_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM7]);
}

#if defined(STM32L476xx) || defined(STM32L496xx)

void TIM8_BRK_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM8]);
}

void TIM8_UP_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM8]);
}

void TIM8_TRG_COM_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM8]);
}

void TIM8_CC_IRQHandler(void)
{
    stm32l4_timer_interrupt(stm32l4_timer_driver.instances[TIMER_INSTANCE_TIM8]);
}

#endif
