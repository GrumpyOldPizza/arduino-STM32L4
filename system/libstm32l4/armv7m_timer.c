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

static void armv7m_timer_insert(armv7m_timer_t *timer)
{
    armv7m_timer_t *element;
    uint32_t remaining;

    remaining = timer->remaining;

    element = armv7m_timer_control.next;

    while (element != (armv7m_timer_t*)&armv7m_timer_control.next)
    {
	if (remaining < element->remaining)
	{
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

static void armv7m_timer_remove(armv7m_timer_t *timer)
{
    timer->next->previous = timer->previous;
    timer->previous->next = timer->next;
      
    timer->next = NULL;
    timer->previous = NULL;
}

void armv7m_timer_create(armv7m_timer_t *timer, armv7m_timer_callback_t callback, uint32_t timeout)
{
    timer->next = NULL;
    timer->previous = NULL;
    timer->callback = callback;
    timer->remaining = timeout;
}

bool armv7m_timer_start(armv7m_timer_t *timer)
{
    IRQn_Type irq;
    bool success = true;

    irq = ((__get_IPSR() & 0x1ff) - 16);

    if (irq >= SysTick_IRQn)
    {
	success = (armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)armv7m_timer_insert, (uint32_t)timer) != NULL);
    }
    else if (irq >= SVCall_IRQn)
    {
	armv7m_timer_insert(timer);
    }
    else
    {
	armv7m_svcall_1((uint32_t)&armv7m_timer_insert, (uint32_t)timer);
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
	success = (armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)armv7m_timer_stop, (uint32_t)timer) != NULL);
    }
    else if (irq >= SVCall_IRQn)
    {
	armv7m_timer_stop(timer);
    }
    else
    {
	armv7m_svcall_1((uint32_t)&armv7m_timer_stop, (uint32_t)timer);
    }

    return success;
}

void armv7m_timer_cancel(armv7m_timer_t *timer)
{
    timer->callback = NULL;
}

static void armv7m_timer_routine(uint32_t heartbeat)
{
    armv7m_timer_t *timer;

    while (heartbeat--)
    {
	timer = armv7m_timer_control.next;

	if (timer != (armv7m_timer_t*)&armv7m_timer_control.next)
	{
	    timer->remaining--;

	    do
	    {
		if (timer->remaining != 0)
		{
		    break;
		}

		armv7m_timer_remove(timer);

		if (timer->callback)
		{
		    (*timer->callback)(timer);
		}

		timer = armv7m_timer_control.next;
	    }
	    while (timer != (armv7m_timer_t*)&armv7m_timer_control.next);
	}

	armv7m_timer_control.millis++;
    }
}

void armv7m_timer_initialize(void)
{
    armv7m_timer_control.next = (armv7m_timer_t*)&armv7m_timer_control.next;
    armv7m_timer_control.previous = (armv7m_timer_t*)&armv7m_timer_control.previous;
    armv7m_timer_control.millis = armv7m_systick_millis();

    armv7m_systick_routine(armv7m_timer_routine);
}
