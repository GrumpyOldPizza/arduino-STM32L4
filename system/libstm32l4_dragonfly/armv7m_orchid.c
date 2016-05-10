/*
 * Copyright (c) 2015-2106 Thomas Roell.  All rights reserved.
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

#if defined(__ORCHID__)

#include <stdlib.h>

#include "stm32l476xx.h"

#include "k_system.h"
#include "armv7m_orchid.h"

armv7m_context_control_t armv7m_context_control;

void armv7m_context_create(k_task_t *task, uint32_t argument)
{
    armv7m_context_t *context = (armv7m_context_t*)((void*)((uint8_t*)task->stack_top - 2 * sizeof(uint32_t) - sizeof(armv7m_context_t)));
    
    context->exc_return = 0xfffffffd;       /* THREAD mode, PSP */
    context->r4         = 0xdeadbeef;
    context->r5         = 0xdeadbeef;
    context->r6         = 0xdeadbeef;
    context->r7         = 0xdeadbeef;
    context->r8         = 0xdeadbeef;
    context->r9         = 0xdeadbeef;
    context->r10        = 0xdeadbeef;
    context->r11        = 0xdeadbeef;
    context->r0         = argument;
    context->r1         = 0xdeadbeef;
    context->r2         = 0xdeadbeef;
    context->r3         = 0xdeadbeef;
    context->r12        = 0xdeadbeef;
    context->lr         = (uint32_t)&_kernel_task_exit;
    context->pc         = ((uint32_t)task->entry & ~0x00000001);
    context->xpsr       = 0x01000000;

    /* header */
    ((uint32_t*)task->stack_top)[-1] = 0x77777777;
    ((uint32_t*)task->stack_top)[-2] = 0xaaaaaaaa;

    /* footer */
    ((uint32_t*)task->stack_limit)[1] = 0x77777777;
    ((uint32_t*)task->stack_limit)[0] = 0xaaaaaaaa;

    task->stack_current = (void*)context;
}

void armv7m_context_delete(k_task_t *task)
{
    if (armv7m_context_control.task_self == task)
    {
	/* Mark task_self as deleted by setting the LSB
	 */
	armv7m_context_control.task_self = (k_task_t*)((uint32_t)task | 1);

	if (armv7m_context_interrupt())
	{
	    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
	}
    }
}

void armv7m_context_next(k_task_t *task)
{
    armv7m_context_control.task_next = task;

    if (armv7m_context_control.task_self != task)
    {
	if (armv7m_context_interrupt())
	{
	    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
	}
    }
}

void armv7m_context_code(k_task_t *task, uint32_t code)
{
    void *stack_current = task->stack_current;

#if defined (__VFP_FP__) && !defined(__SOFTFP__)
    if (!(((armv7m_context_t*)stack_current)->exc_return & 0x00000010))
    {
	((armv7m_context_fpu_t*)stack_current)->r0 = code;
    }
    else
#endif /* __VFP_FP__ && !__SOFTFP__ */
    {
	((armv7m_context_t*)stack_current)->r0 = code;
    }
}

/* 
 * armv7m_context_switch -- switch to a new context
 *
 * R0 task_self
 * R1 task_next
 * R2 PSP
 * R3 armv7m_context_control
 * LR exception return
 *
 * If task_self has it's LSB set then it is a dead task
 * If task_next is NULL then there is no next task
 */

__attribute__((used,naked)) void armv7m_context_switch(void)
{
    __asm__(
	"tst      r0, #1                           \n"
	"bne.n    .Lswitch                         \n"
	"stmdb    r2!, { r4-r11 }                  \n"
#if defined (__VFP_FP__) && !defined(__SOFTFP__)
        "tst      lr, #0x00000010                  \n"
        "it       eq                               \n"
        "vstmdbeq r2!, { s16-s31 }                 \n"
#endif /* __VFP_FP__ && !__SOFTFP__ */
	"stmdb    r2!, { lr }                      \n"
        "str      r2, [r0, %[offset_STACK_CURRENT]]\n"

	/* Check for stack corruption.
	 */

        "ldr      r4, [r0, %[offset_STACK_LIMIT]]  \n"
        "cbz      r4, .Lswitch                     \n"
        "ldr      r5, [r4, #0]                     \n"
        "cmp      r5, #0xaaaaaaaa                  \n"
        "beq.n    .Lstack                          \n"
        "ldr      r5, [r4, #4]                     \n"
        "cmp      r5, #0x77777777                  \n"
        "beq.n    .Lstack                          \n"
        "ldr      r4, [r0, %[offset_STACK_TOP]]    \n"
        "ldr      r5, [r4, #-8]                    \n"
        "cmp      r5, #0xaaaaaaaa                  \n"
        "beq.n    .Lstack                          \n"
        "ldr      r5, [r4, #-4]                    \n"
        "cmp      r5, #0x77777777                  \n"
        "beq.n    .Lstack                          \n"

	/* Update k_system_info.task_self to reflect
	 * the context switch.
	 */
	".Lswitch:                                 \n"
        "str      r1, [r3, %[offset_TASK_SELF]]    \n"

	

	/* If task_next is NULL, wait ...
	 */
	"cbz.n    r1, .Lwait                       \n"

	/* Restore context from task_next.
	 */

	".Lrestore:                                \n"
        "ldr      r2, [r1, %[offset_STACK_CURRENT]]\n"
	"ldmia    r2!, { lr }                      \n"
#if defined (__VFP_FP__) && !defined(__SOFTFP__)
        "tst      lr, #0x00000010                  \n"
        "it       eq                               \n"
        "vldmiaeq r2!, { s16-s31 }                 \n"
#endif /* __VFP_FP__ && !__SOFTFP__ */
	"ldmdb    r2!, { r4-r11 }                  \n"
        "msr      PSP, r2                          \n"
        "bx       lr                               \n"

	/* Wait via WFE till task_next is non-NULL.
	 */
	".Lwait:                                   \n"
	"wfe                                       \n"
	"ldr     r1, [r3, %[offset_TASK_NEXT]]     \n"
	"cmp     r1, #0                            \n"
	"bne.n   .Lrestore                         \n"
	"b.n     .Lwait                            \n"

	".Lstack:                                  \n"
        "b       _kernel_stack_corruption          \n"
	:
	: [offset_TASK_SELF]      "I" (offsetof(k_system_control_t, task_self)),
	  [offset_TASK_NEXT]      "I" (offsetof(k_system_control_t, task_next)),
	  [offset_STACK_CURRENT]  "I" (offsetof(k_task_t, stack_current)),
	  [offset_STACK_TOP]      "I" (offsetof(k_task_t, stack_top)),
	  [offset_STACK_LIMIT]    "I" (offsetof(k_task_t, stack_limit))
	);
}

#endif /* __ORCHID__ */
