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

#if !defined(_ARMV7M_ORCHID_H)
#define _ARMV7M_ORCHID_H

#include "armv7m.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct _armv7m_context_t {
    uint32_t    exc_return;
    uint32_t    r4;
    uint32_t    r5;
    uint32_t    r6;
    uint32_t    r7;
    uint32_t    r8;
    uint32_t    r9;
    uint32_t    r10;
    uint32_t    r11;
    uint32_t    r0;
    uint32_t    r1;
    uint32_t    r2;
    uint32_t    r3;
    uint32_t    r12;
    uint32_t    lr;
    uint32_t    pc;
    uint32_t    xpsr;
} armv7m_context_t;

#if defined (__VFP_FP__) && !defined(__SOFTFP__)

typedef struct _armv7m_context_fpu_t {
    uint32_t    exc_return;
    uint32_t    s16;
    uint32_t    s17;
    uint32_t    s18;
    uint32_t    s19;
    uint32_t    s20;
    uint32_t    s21;
    uint32_t    s22;
    uint32_t    s23;
    uint32_t    s24;
    uint32_t    s25;
    uint32_t    s26;
    uint32_t    s27;
    uint32_t    s28;
    uint32_t    s29;
    uint32_t    s30;
    uint32_t    s31;
    uint32_t    r4;
    uint32_t    r5;
    uint32_t    r6;
    uint32_t    r7;
    uint32_t    r8;
    uint32_t    r9;
    uint32_t    r10;
    uint32_t    r11;
    uint32_t    r0;
    uint32_t    r1;
    uint32_t    r2;
    uint32_t    r3;
    uint32_t    r12;
    uint32_t    lr;
    uint32_t    pc;
    uint32_t    xpsr;
    uint32_t    s0;
    uint32_t    s1;
    uint32_t    s2;
    uint32_t    s3;
    uint32_t    s4;
    uint32_t    s5;
    uint32_t    s6;
    uint32_t    s7;
    uint32_t    s8;
    uint32_t    s9;
    uint32_t    s10;
    uint32_t    s11;
    uint32_t    s12;
    uint32_t    s13;
    uint32_t    s14;
    uint32_t    s15;
    uint32_t    fpscr;
    uint32_t    reserved;
} armv7m_context_fpu_t;

#endif /* __VFP_FP__ && !__SOFTFP__ */

typedef struct _armv7m_context_control_t {
    k_task_t           *task_self;
    k_task_t           *task_next;
} armv7m_context_control_t;

extern armv7m_context_control_t armv7m_context_control;

extern void armv7m_context_create(k_task_t *task, uint32_t argument);
extern void armv7m_context_delete(k_task_t *task);
extern void armv7m_context_next(k_task_t *task);
extern void armv7m_context_code(k_task_t *task, uint32_t code);
extern void armv7m_context_switch(void);

static inline k_task_t * armv7m_context_self(void)
{
    return (k_task_t*)((uint32_t)armv7m_context_control.task_self & ~1);
}

static inline bool armv7m_context_interrupt(void)
{
    uint32_t ipsr;

    __asm__ volatile ("mrs %0, IPSR" : "=r" (ipsr));

    return (ipsr > 14);
}

typedef armv7m_timer_t k_timer_t;

#ifdef __cplusplus
}
#endif

#endif /* _ARMV7M_ORCHID_H */
