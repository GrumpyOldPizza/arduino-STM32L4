/*
 * Copyright (c) 2014,2015 Thomas Roell.  All rights reserved.
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

#include "armv7m.h"

#include "stm32l476xx.h"

int armv7m_core_priority(void)
{
    uint32_t ipsr, faultmask, primask, basepri;
    int priority;
    
    ipsr = __get_IPSR();

    if (ipsr == 0)
    {
	priority = 256 >> (8 - __NVIC_PRIO_BITS);
    }
    else
    {
	if (ipsr >= 4)
	{
	    priority = NVIC_GetPriority((int)ipsr - 16);
	}
	else
	{
	    priority = (int)ipsr - 4; /* -1 for HardFault, and -2 for NMI */
	}
    }

    faultmask = __get_FAULTMASK();

    if (faultmask)
    {
	if (priority > -1)
	{
	    priority = -1;
	}
    }
    else
    {
	primask = __get_PRIMASK();

	if (primask)
	{
	    if (priority > 0)
	    {
		priority = 0;
	    }
	}
	else
	{
	    basepri = __get_BASEPRI() >> (8 - __NVIC_PRIO_BITS);

	    if (basepri)
	    {
		if (priority > basepri)
		{
		    priority = basepri;
		}
	    }
	}
    }

    return priority;
}

