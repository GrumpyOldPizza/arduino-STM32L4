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

#if !defined(_STM32L4_DAC_H)
#define _STM32L4_DAC_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#ifdef __cplusplus
 extern "C" {
#endif

enum {
    DAC_INSTANCE_DAC    = 0,
    DAC_INSTANCE_COUNT
};

#define DAC_CHANNEL_1                            0
#define DAC_CHANNEL_2                            1

#define DAC_CONTROL_DISABLE                      0x00000000
#define DAC_CONTROL_INTERNAL                     0x00000001
#define DAC_CONTROL_EXTERNAL                     0x00000002
#define DAC_CONTROL_CALIBRATE                    0x00000004

typedef void (*stm32l4_dac_callback_t)(void *context, uint32_t events);

#define DAC_STATE_NONE                         0
#define DAC_STATE_INIT                         1
#define DAC_STATE_BUSY                         2
#define DAC_STATE_READY                        3

typedef struct _stm32l4_dac_t {
    DAC_TypeDef                 *DACx;
    volatile uint8_t            state;
    uint8_t                     instance;
    uint8_t                     priority;
    stm32l4_dac_callback_t      callback;
    void                        *context;
    uint32_t                    events;
} stm32l4_dac_t;

extern bool     stm32l4_dac_create(stm32l4_dac_t *dac, unsigned int instance, unsigned int priority, unsigned int mode);
extern bool     stm32l4_dac_destroy(stm32l4_dac_t *dac);
extern uint32_t stm32l4_dac_clock(stm32l4_dac_t *dac);
extern bool     stm32l4_dac_enable(stm32l4_dac_t *dac, uint32_t option, stm32l4_dac_callback_t callback, void *context, uint32_t events);
extern bool     stm32l4_dac_disable(stm32l4_dac_t *dac);
extern bool     stm32l4_dac_configure(stm32l4_dac_t *dac, uint32_t option);
extern bool     stm32l4_dac_notify(stm32l4_dac_t *dac, stm32l4_dac_callback_t callback, void *context, uint32_t events);
extern bool     stm32l4_dac_channel(stm32l4_dac_t *dac, unsigned int channel, uint32_t output, uint32_t control);
extern bool     stm32l4_dac_convert(stm32l4_dac_t *dac, unsigned int channel, uint32_t output);

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_DAC_H */
