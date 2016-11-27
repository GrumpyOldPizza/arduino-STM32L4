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

#if !defined(_ARMV7M_H)
#define _ARMV7M_H

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

static inline void armv7m_core_yield(void)
{
    /* This odd aequence seems to be required for at least STM32L4. Traces on the logic analyzer
     * showed that after blocking on wfe, then the subsequent wfe would not block. The only WAR
     * is to explicitly clear the EVENT flag via the SEV; WFE sequence.
     */
    __asm__ volatile ("wfe; sev; wfe");
}

extern int armv7m_core_priority(void);
extern void armv7m_core_udelay(uint32_t udelay);

#include "armv7m_atomic.h"
#include "armv7m_bitband.h"
#include "armv7m_pendsv.h"
#include "armv7m_svcall.h"
#include "armv7m_systick.h"
#include "armv7m_timer.h"

#ifdef __cplusplus
}
#endif

#endif /* _ARMV7M_H */
