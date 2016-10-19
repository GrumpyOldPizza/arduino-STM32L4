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

#include <stdio.h>

#include "armv7m.h"

#include "stm32l4xx.h"

typedef struct _armv7m_clock_control_t {
    uint32_t               clock;
    uint32_t               delay;     /* ticks per ms */
    uint32_t               scale;
} armv7m_clock_control_t;

static armv7m_clock_control_t armv7m_clock_control;

uint32_t armv7m_clock_read(void)
{
    return DWT->CYCCNT;
}

uint32_t armv7m_clock_ns2ticks(uint32_t ns)
{
    return (uint32_t)(((uint64_t)ns * (uint64_t)SystemCoreClock) / (uint64_t)1000000000);
}

uint32_t armv7m_clock_ticks2ns(uint32_t ticks)
{
    return (uint32_t)(((uint64_t)ticks * (uint64_t)1000000000) / (uint64_t)SystemCoreClock);
}

void armv7m_clock_spin(uint32_t ns)
{
    uint32_t ticks, delay, ms;

    if (armv7m_clock_control.clock != SystemCoreClock)
    {
	armv7m_clock_control.clock = SystemCoreClock;
	armv7m_clock_control.delay = (uint32_t)(SystemCoreClock / 1000);
	armv7m_clock_control.scale = (uint32_t)(((uint64_t)1000000000 * (uint64_t)4096) / (uint64_t)SystemCoreClock);

	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    if (ns > 1000000)
    {
	ms = ns / 1000000;
	ns -= ms * 1000000;

	delay = ms * armv7m_clock_control.delay;
	ticks = DWT->CYCCNT;
    
	while ((uint32_t)(DWT->CYCCNT - ticks) < delay);
	{
	}
    }

    delay = (ns * 4096) / armv7m_clock_control.scale;

    ticks = DWT->CYCCNT;
    
    while ((uint32_t)(DWT->CYCCNT - ticks) < delay);
    {
    }
}

#if 0
void armv7m_clock_initialize(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
#endif
