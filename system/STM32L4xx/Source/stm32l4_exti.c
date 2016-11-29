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

#include "stm32l4_exti.h"
#include "stm32l4_gpio.h"
#include "stm32l4_system.h"

#include "armv7m.h"

typedef struct _stm32l4_exti_driver_t {
    stm32l4_exti_t         *instances[1];
} stm32l4_exti_driver_t;

static stm32l4_exti_driver_t stm32l4_exti_driver;

bool stm32l4_exti_create(stm32l4_exti_t *exti, unsigned int priority)
{
    exti->state = EXTI_STATE_INIT;
    exti->priority = priority;

    exti->enables = 0;
    exti->mask = ~0ul;

    stm32l4_exti_driver.instances[0] = exti;
    
    return true;
}

bool stm32l4_exti_destroy(stm32l4_exti_t *exti)
{
    if (exti->state != EXTI_STATE_INIT)
    {
	return false;
    }

    exti->state = EXTI_STATE_NONE;

    stm32l4_exti_driver.instances[0] = NULL;
    
    return true;
}

bool stm32l4_exti_enable(stm32l4_exti_t *exti)
{
    if (exti->state != EXTI_STATE_INIT)
    {
	return false;
    }

    exti->state = EXTI_STATE_BUSY;

    NVIC_SetPriority(EXTI0_IRQn, exti->priority);
    NVIC_SetPriority(EXTI1_IRQn, exti->priority);
    NVIC_SetPriority(EXTI2_IRQn, exti->priority);
    NVIC_SetPriority(EXTI3_IRQn, exti->priority);
    NVIC_SetPriority(EXTI4_IRQn, exti->priority);
    NVIC_SetPriority(EXTI9_5_IRQn, exti->priority);
    NVIC_SetPriority(EXTI15_10_IRQn, exti->priority);

    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    exti->enables = 0;
    exti->mask = ~0ul;

    exti->state = EXTI_STATE_READY;

    return true;
}

bool stm32l4_exti_disable(stm32l4_exti_t *exti)
{
    if (exti->state != EXTI_STATE_READY)
    {
	return false;
    }

    armv7m_atomic_and(&EXTI->IMR1, ~0x0000ffff);
    armv7m_atomic_and(&EXTI->EMR1, ~0x0000ffff);

    NVIC_DisableIRQ(EXTI15_10_IRQn);
    NVIC_DisableIRQ(EXTI9_5_IRQn);
    NVIC_DisableIRQ(EXTI4_IRQn);
    NVIC_DisableIRQ(EXTI3_IRQn);
    NVIC_DisableIRQ(EXTI2_IRQn);
    NVIC_DisableIRQ(EXTI1_IRQn);
    NVIC_DisableIRQ(EXTI0_IRQn);

    exti->state = EXTI_STATE_INIT;

    return true;
}

bool stm32l4_exti_suspend(stm32l4_exti_t *exti, uint32_t mask)
{
    if (exti->state != EXTI_STATE_READY)
    {
	return false;
    }

    armv7m_atomic_and(&exti->mask, ~mask);

    armv7m_atomic_modify(&EXTI->IMR1, 0x0000ffff, (exti->enables & exti->mask));
    armv7m_atomic_modify(&EXTI->EMR1, 0x0000ffff, (exti->enables & exti->mask));

    return true;
}

bool stm32l4_exti_resume(stm32l4_exti_t *exti, uint32_t mask)
{
    if (exti->state != EXTI_STATE_READY)
    {
	return false;
    }

    armv7m_atomic_or(&exti->mask, mask);

    armv7m_atomic_modify(&EXTI->EMR1, 0x0000ffff, (exti->enables & exti->mask));
    armv7m_atomic_modify(&EXTI->IMR1, 0x0000ffff, (exti->enables & exti->mask));

    return true;
}

bool stm32l4_exti_notify(stm32l4_exti_t *exti, uint16_t pin, uint32_t control, stm32l4_exti_callback_t callback, void *context)
{
    unsigned int mask, index, group;

    if (exti->state != EXTI_STATE_READY)
    {
	return false;
    }

    index = (pin & GPIO_PIN_INDEX_MASK) >> GPIO_PIN_INDEX_SHIFT;
    group = (pin & GPIO_PIN_GROUP_MASK) >> GPIO_PIN_GROUP_SHIFT;

    mask = 1ul << index;

    armv7m_atomic_and(&EXTI->IMR1, ~mask);
    armv7m_atomic_and(&EXTI->EMR1, ~mask);
    armv7m_atomic_and(&exti->enables, ~mask);
    
    exti->channels[index].callback = callback;
    exti->channels[index].context = context;

    if (control & (EXTI_CONTROL_FALLING_EDGE | EXTI_CONTROL_RISING_EDGE))
    {
	if (control & EXTI_CONTROL_RISING_EDGE)
	{
	    armv7m_atomic_or(&EXTI->RTSR1, mask);
	}
	else
	{
	    armv7m_atomic_and(&EXTI->RTSR1, ~mask);
	}
	
	if (control & EXTI_CONTROL_FALLING_EDGE)
	{
	    armv7m_atomic_or(&EXTI->FTSR1, mask);
	}
	else
	{
	    armv7m_atomic_and(&EXTI->FTSR1, ~mask);
	}

	armv7m_atomic_modify(&SYSCFG->EXTICR[index >> 2], (0x0000000f << ((index & 3) << 2)), (group << ((index & 3) << 2)));

	armv7m_atomic_or(&exti->enables, mask);

	if ((exti->enables & exti->mask) & mask)
	{
	    armv7m_atomic_or(&EXTI->EMR1, mask);
	    armv7m_atomic_or(&EXTI->IMR1, mask);
	}
    }

    return true;
}

void EXTI0_IRQHandler(void)
{
    stm32l4_exti_t *exti = stm32l4_exti_driver.instances[0];

    EXTI->PR1 = (1ul << 0);

    (*exti->channels[0].callback)(exti->channels[0].context);
}

void EXTI1_IRQHandler(void)
{
    stm32l4_exti_t *exti = stm32l4_exti_driver.instances[0];

    EXTI->PR1 = (1ul << 1);

    (*exti->channels[1].callback)(exti->channels[1].context);
}

void EXTI2_IRQHandler(void)
{
    stm32l4_exti_t *exti = stm32l4_exti_driver.instances[0];

    EXTI->PR1 = (1ul << 2);

    (*exti->channels[2].callback)(exti->channels[2].context);
}

void EXTI3_IRQHandler(void)
{
    stm32l4_exti_t *exti = stm32l4_exti_driver.instances[0];

    EXTI->PR1 = (1ul << 3);

    (*exti->channels[3].callback)(exti->channels[3].context);
}

void EXTI4_IRQHandler(void)
{
    stm32l4_exti_t *exti = stm32l4_exti_driver.instances[0];

    EXTI->PR1 = (1ul << 4);

    (*exti->channels[4].callback)(exti->channels[4].context);
}

void EXTI9_5_IRQHandler(void)
{
    stm32l4_exti_t *exti = stm32l4_exti_driver.instances[0];
    uint32_t mask = (exti->mask & EXTI->PR1) & 0x03e0;
    unsigned int index;

    while (mask) 
    {
	index = __builtin_ctz(mask);

	EXTI->PR1 = (1ul << index);

	mask &= ~(1ul << index); 

	(*exti->channels[index].callback)(exti->channels[index].context);
    }
}

void EXTI15_10_IRQHandler(void)
{
    stm32l4_exti_t *exti = stm32l4_exti_driver.instances[0];
    uint32_t mask = (exti->mask & EXTI->PR1) & 0xfc00;
    unsigned int index;

    while (mask) 
    {
	index = __builtin_ctz(mask);

	EXTI->PR1 = (1ul << index);

	mask &= ~(1ul << index); 

	(*exti->channels[index].callback)(exti->channels[index].context);
    }
}
