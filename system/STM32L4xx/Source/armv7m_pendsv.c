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

#include <stdlib.h>

#include "armv7m.h"

#include "stm32l4_nvic.h"

typedef struct _armv7m_pendsv_entry_t {
    armv7m_pendsv_routine_t          routine;
    void                             *context;
    uint32_t                         data;
} armv7m_pendsv_entry_t;

typedef struct _armv7m_pendsv_control_t {
    volatile armv7m_pendsv_entry_t   *pendsv_read;
    volatile armv7m_pendsv_entry_t   *pendsv_write;
    armv7m_pendsv_entry_t            pendsv_data[ARMV7M_PENDSV_ENTRY_COUNT];
} armv7m_pendsv_control_t;

static armv7m_pendsv_control_t armv7m_pendsv_control;

volatile armv7m_pendsv_routine_t * armv7m_pendsv_enqueue(armv7m_pendsv_routine_t routine, void *context, uint32_t data)
{
    volatile armv7m_pendsv_entry_t *pendsv_write, *pendsv_write_next;

    do
    {
        pendsv_write = armv7m_pendsv_control.pendsv_write;
	pendsv_write_next = pendsv_write + 1;

	if (pendsv_write_next == &armv7m_pendsv_control.pendsv_data[ARMV7M_PENDSV_ENTRY_COUNT])
	{
	    pendsv_write_next = &armv7m_pendsv_control.pendsv_data[0];
	}

	if (pendsv_write_next == armv7m_pendsv_control.pendsv_read)
	{
	    return NULL;
	}
    }
    while (!armv7m_atomic_compare_exchange((volatile uint32_t*)&armv7m_pendsv_control.pendsv_write, (uint32_t*)&pendsv_write, (uint32_t)pendsv_write_next));

    pendsv_write->routine = routine;
    pendsv_write->context = context;
    pendsv_write->data = data;

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

    return &pendsv_write->routine;
}

static __attribute__((used)) void armv7m_pendsv_dequeue(void)
{
    volatile armv7m_pendsv_entry_t *pendsv_read;
    armv7m_pendsv_routine_t routine;
    void *context;
    uint32_t data;

    pendsv_read = armv7m_pendsv_control.pendsv_read;

    while (pendsv_read != armv7m_pendsv_control.pendsv_write)
    {

	routine = pendsv_read->routine;
	context = pendsv_read->context;
	data = pendsv_read->data;

	pendsv_read = pendsv_read + 1;

	if (pendsv_read == &armv7m_pendsv_control.pendsv_data[ARMV7M_PENDSV_ENTRY_COUNT])
	{
	    pendsv_read = &armv7m_pendsv_control.pendsv_data[0];
	}

	armv7m_pendsv_control.pendsv_read = pendsv_read;

	(*routine)(context, data);
    }
}


void armv7m_pendsv_initialize(void)
{
    armv7m_pendsv_control.pendsv_read = &armv7m_pendsv_control.pendsv_data[0];
    armv7m_pendsv_control.pendsv_write = &armv7m_pendsv_control.pendsv_data[0];

    NVIC_SetPriority(PendSV_IRQn, ((1 << __NVIC_PRIO_BITS) -1));
}

#if defined(__ORCHID__)

#include "orchid.h"

void __attribute__((naked)) PendSV_Handler(void)
{
    __asm__(
        "mrs     r2, PSP                             \n"
        "push    { r2, lr }                          \n"
        "bl      armv7m_pendsv_dequeue               \n"
        "pop     { r2, lr }                          \n"
	"movw    r3, #:lower16:armv7m_context_control\n"
	"movt    r3, #:upper16:armv7m_context_control\n"
	"ldr     r0, [r3, %[offset_TASK_SELF]]       \n"
	"ldr     r1, [r3, %[offset_TASK_NEXT]]       \n"
	"cmp     r0, r1                              \n"
	"bne     armv7m_context_switch               \n"
        "bx      lr                                  \n"
	:
	: [offset_TASK_SELF]      "I" (offsetof(armv7m_context_control_t, task_self)),
	  [offset_TASK_NEXT]      "I" (offsetof(armv7m_context_control_t, task_next))
	);
}

#else /* __ORCHID__ */

void PendSV_Handler(void)
{
    armv7m_pendsv_dequeue();
}

#endif  /* __ORCHID__ */
