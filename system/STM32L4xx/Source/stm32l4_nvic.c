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
#include "stm32l4_nvic.h"

#include "armv7m.h"

__attribute__((section(".isr_vector_sram"))) uint32_t __isr_vector_sram[16+96];

uint32_t NVIC_CatchIRQ(IRQn_Type IRQn, uint32_t vector)
{
    volatile uint32_t *vectors, *o_vectors;
    uint32_t primask;
    unsigned int i;

    vectors = (volatile uint32_t*)SCB->VTOR;

    if (vectors != &__isr_vector_sram[0])
    {
         primask = __get_PRIMASK();

	__disable_irq();

	vectors = (volatile uint32_t*)SCB->VTOR;

	if (vectors != &__isr_vector_sram[0])
	{
	    o_vectors = vectors;
	    vectors = &__isr_vector_sram[0];
	    
	    for (i = 0; i < sizeof(__isr_vector_sram) / sizeof(__isr_vector_sram[0]); i++)
	    {
		vectors[i] = o_vectors[i];
	    }
	    SCB->VTOR = (uint32_t)vectors;
	}

	__set_PRIMASK(primask);
    }

    vector = armv7m_atomic_exchange(&vectors[IRQn + 16], vector);

    return vector;
}
