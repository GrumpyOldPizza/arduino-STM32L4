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

#include "stm32l4xx.h"

void armv7m_svcall_initialize(void)
{
    NVIC_SetPriority(SVCall_IRQn, ((1 << __NVIC_PRIO_BITS) -1));
}

#if defined(__ORCHID__)

#include "orchid.h"

void __attribute__((naked)) SVC_Handler(void)
{
    __asm__(
        "mrs     r2, PSP                             \n"
        "push    { r2, lr }                          \n"
        "ldmia   r2, { r0, r1, r2, r3, r12 }         \n"
        "blx     r12                                 \n"
        "pop     { r2, lr }                          \n"
        "str     r0, [r2, #0]                        \n"
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

void __attribute__((naked)) SVC_Handler(void)
{
    __asm__(
        "mov     r2, sp                              \n"
        "push    { r2, lr }                          \n"
        "ldmia   r2, { r0, r1, r2, r3, r12 }         \n"
        "blx     r12                                 \n"
        "pop     { r2, lr }                          \n"
        "str     r0, [r2, #0]                        \n"
        "bx      lr                                  \n"
	);
}

#endif /* __ORCHID__ */
