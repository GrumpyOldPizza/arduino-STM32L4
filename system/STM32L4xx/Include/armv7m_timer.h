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

#if !defined(_ARMV7M_TIMER_H)
#define _ARMV7M_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct _armv7m_timer_t armv7m_timer_t;

typedef void (*armv7m_timer_callback_t)(armv7m_timer_t *timer);

struct _armv7m_timer_t {
    armv7m_timer_t                   *next;
    armv7m_timer_t                   *previous;
    volatile armv7m_timer_callback_t callback;
    uint32_t                         remaining;
};

#define ARMV7M_TIMER_INIT(_callback,_timeout) { NULL, NULL, (_callback), (_timeout) }

extern void armv7m_timer_create(armv7m_timer_t *timer, armv7m_timer_callback_t callback);
extern bool armv7m_timer_start(armv7m_timer_t *timer, uint32_t timeout);
extern bool armv7m_timer_stop(armv7m_timer_t *timer);

extern void armv7m_timer_initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* _ARMV7M_TIMER_H */
