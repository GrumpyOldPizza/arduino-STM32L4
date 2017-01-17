/*
 * Copyright (c) 2015 Thomas Roell.  All rights reserved.
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

#include "stm32l476xx.h"

typedef struct _armv7m_timer_control_t {
    struct _armv7m_timer_t *next;
    struct _armv7m_timer_t *previous;
    uint32_t               millis;
} armv7m_timer_control_t;

static armv7m_timer_control_t armv7m_timer_control;

static void armv7m_timer_insert(void *context, uint32_t data)
{
    armv7m_timer_t *timer, *element;
    uint32_t remaining;

    timer = (armv7m_timer_t*)context;

    if (timer->next)
    {
	timer->next->previous = timer->previous;
	timer->previous->next = timer->next;
    
	timer->next = NULL;
	timer->previous = NULL;
    }

    remaining = data;
    element = armv7m_timer_control.next;

    while (element != (armv7m_timer_t*)&armv7m_timer_control)
    {
	if (remaining < element->remaining)
	{
	    element->remaining -= remaining;
	    break;
	}
	
	remaining -= element->remaining;
	element = element->next;
    }

    timer->previous = element->previous;
    timer->next = element;

    timer->previous->next = timer;
    timer->next->previous = timer;
    
    timer->remaining = remaining;
}

static void armv7m_timer_remove(void *context, uint32_t data)
{
    armv7m_timer_t *timer;

    timer = (armv7m_timer_t*)context;

    if (timer->next)
    {
	timer->next->previous = timer->previous;
	timer->previous->next = timer->next;
    
	timer->next = NULL;
	timer->previous = NULL;
    }

    armv7m_atomic_or((volatile uint32_t *)&timer->callback, 1);
}

void armv7m_timer_create(armv7m_timer_t *timer, armv7m_timer_callback_t callback)
{
    timer->next = NULL;
    timer->previous = NULL;
    timer->callback = callback;
    timer->remaining = 0;
}

bool armv7m_timer_start(armv7m_timer_t *timer, uint32_t timeout)
{
    IRQn_Type irq;
    bool success = true;

    irq = ((__get_IPSR() & 0x1ff) - 16);

    if (irq >= SysTick_IRQn)
    {
	success = (armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)armv7m_timer_insert, (void*)timer, timeout) != NULL);
    }
    else if (irq >= SVCall_IRQn)
    {
	armv7m_timer_insert((void*)timer, timeout);
    }
    else
    {
	armv7m_svcall_2((uint32_t)&armv7m_timer_insert, (uint32_t)timer, timeout);
    }

    return success;
}

bool armv7m_timer_stop(armv7m_timer_t *timer)
{
    IRQn_Type irq;
    bool success = true;

    irq = ((__get_IPSR() & 0x1ff) - 16);

    if (irq >= SysTick_IRQn)
    {
	success = (armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)armv7m_timer_remove, (void*)timer, 0) != NULL);

	if (success)
	{
	    armv7m_atomic_and((volatile uint32_t *)&timer->callback, ~1);
	}
    }
    else if (irq >= SVCall_IRQn)
    {
	armv7m_timer_remove((void*)timer, 0);
    }
    else
    {
	armv7m_svcall_2((uint32_t)&armv7m_timer_remove, (uint32_t)timer, 0);
    }

    return success;
}

static void armv7m_timer_callback(void *context, uint32_t data)
{
    armv7m_timer_t *timer;
    armv7m_timer_callback_t callback;
    uint32_t millis;

    millis = data;

    while (armv7m_timer_control.millis != millis)
    {
	timer = armv7m_timer_control.next;

	if (timer != (armv7m_timer_t*)&armv7m_timer_control)
	{
	    timer->remaining--;

	    while (timer != (armv7m_timer_t*)&armv7m_timer_control)
	    {
		if (timer->remaining)
		{
		    break;
		}
		
		callback = timer->callback;
		
		armv7m_timer_remove(timer, 0);
		
		if ((uint32_t)callback & 1)
		{
		    (*callback)(timer);
		}
		
		timer = armv7m_timer_control.next;
	    }
	}

	armv7m_timer_control.millis++;
    }
}

void armv7m_timer_initialize(void)
{
    armv7m_timer_control.next = (armv7m_timer_t*)&armv7m_timer_control;
    armv7m_timer_control.previous = (armv7m_timer_t*)&armv7m_timer_control;
    armv7m_timer_control.millis = armv7m_systick_millis();

    armv7m_systick_notify(armv7m_timer_callback, NULL);
}
