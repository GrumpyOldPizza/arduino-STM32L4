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

#if !defined(_STM32L4_EXTI_H)
#define _STM32L4_EXTI_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define EXTI_CHANNEL_COUNT              16

#define EXTI_CONTROL_DISABLE            0x00000000
#define EXTI_CONTROL_FALLING_EDGE       0x00000001
#define EXTI_CONTROL_RISING_EDGE        0x00000002
#define EXTI_CONTROL_BOTH_EDGES         0x00000003

typedef void (*stm32l4_exti_callback_t)(void *context);

#define EXTI_STATE_NONE                 0
#define EXTI_STATE_INIT                 1
#define EXTI_STATE_BUSY                 2
#define EXTI_STATE_READY                3

typedef struct _stm32l4_exti_t {
    volatile uint8_t        state;
    uint8_t                 priority;
    uint32_t                enables;
    uint32_t                mask;
    struct {
      stm32l4_exti_callback_t callback;
      void*                   context;
    }                       channels[EXTI_CHANNEL_COUNT];
} stm32l4_exti_t;

extern bool stm32l4_exti_create(stm32l4_exti_t *exti, unsigned int priority);
extern bool stm32l4_exti_destroy(stm32l4_exti_t *exti);
extern bool stm32l4_exti_enable(stm32l4_exti_t *exti);
extern bool stm32l4_exti_disable(stm32l4_exti_t *exti);
extern bool stm32l4_exti_suspend(stm32l4_exti_t *exti, uint32_t mask);
extern bool stm32l4_exti_resume(stm32l4_exti_t *exti, uint32_t mask);
extern bool stm32l4_exti_notify(stm32l4_exti_t *exti, uint16_t pin, uint32_t control, stm32l4_exti_callback_t callback, void *context);

extern void EXTI0_IRQHandler(void);
extern void EXTI1_IRQHandler(void);
extern void EXTI2_IRQHandler(void);
extern void EXTI3_IRQHandler(void);
extern void EXTI4_IRQHandler(void);
extern void EXTI9_5_IRQHandler(void);
extern void EXTI15_10_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_EXTI_H */
