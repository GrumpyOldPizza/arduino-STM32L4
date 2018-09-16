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

#if !defined(_STM32L4_ADC_H)
#define _STM32L4_ADC_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
     ADC_INSTANCE_ADC1 = 0,
#if defined(STM32L476xx) || defined(STM32L496xx)
     ADC_INSTANCE_ADC2,
     ADC_INSTANCE_ADC3,
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
     ADC_INSTANCE_COUNT
};

#define ADC_CHANNEL_ADC1_VREFINT                 0
#define ADC_CHANNEL_ADC123_IN1                   1   /* PC0 */
#define ADC_CHANNEL_ADC123_IN2                   2   /* PC1 */
#define ADC_CHANNEL_ADC123_IN3                   3   /* PC2 */
#define ADC_CHANNEL_ADC123_IN4                   4   /* PC3 */
#define ADC_CHANNEL_ADC12_IN5                    5   /* PA0 */
#define ADC_CHANNEL_ADC12_IN6                    6   /* PA1 */
#define ADC_CHANNEL_ADC12_IN7                    7   /* PA2 */
#define ADC_CHANNEL_ADC12_IN8                    8   /* PA3 */
#define ADC_CHANNEL_ADC12_IN9                    9   /* PA4 */
#define ADC_CHANNEL_ADC12_IN10                   10  /* PA5 */
#define ADC_CHANNEL_ADC12_IN11                   11  /* PA6 */
#define ADC_CHANNEL_ADC12_IN12                   12  /* PA7 */
#define ADC_CHANNEL_ADC12_IN13                   13  /* PC4 */
#define ADC_CHANNEL_ADC12_IN14                   14  /* PC5 */
#define ADC_CHANNEL_ADC12_IN15                   15  /* PB0 */
#define ADC_CHANNEL_ADC12_IN16                   16  /* PB1 */
#define ADC_CHANNEL_ADC1_TS                      17  /* TEMPERATURE SENSOR */
#define ADC_CHANNEL_ADC1_VBAT                    18  /* VBAT/3 */
#define ADC_CHANNEL_ADC2_DAC1                    17
#define ADC_CHANNEL_ADC2_DAC2                    18

typedef void (*stm32l4_adc_callback_t)(void *context, uint32_t events);

#define ADC_VREFINT_PERIOD                       4
#define ADC_VBAT_PERIOD                          12
#define ADC_TSENSE_PERIOD                        5

#define ADC_STATE_NONE                         0
#define ADC_STATE_INIT                         1
#define ADC_STATE_BUSY                         2
#define ADC_STATE_READY                        3

typedef struct _stm32l4_adc_t {
    ADC_TypeDef                 *ADCx;
    volatile uint8_t            state;
    uint8_t                     instance;
    uint8_t                     priority;
    stm32l4_adc_callback_t      callback;
    void                        *context;
    uint32_t                    events;
} stm32l4_adc_t;

extern bool     stm32l4_adc_create(stm32l4_adc_t *adc, unsigned int instance, unsigned int priority, unsigned int mode);
extern bool     stm32l4_adc_destroy(stm32l4_adc_t *adc);
extern bool     stm32l4_adc_enable(stm32l4_adc_t *adc, uint32_t option, stm32l4_adc_callback_t callback, void *context, uint32_t events);
extern bool     stm32l4_adc_disable(stm32l4_adc_t *adc);
extern bool     stm32l4_adc_configure(stm32l4_adc_t *adc, uint32_t option);
extern bool     stm32l4_adc_calibrate(stm32l4_adc_t *adc);
extern bool     stm32l4_adc_notify(stm32l4_adc_t *adc, stm32l4_adc_callback_t callback, void *context, uint32_t events);
extern uint32_t stm32l4_adc_convert(stm32l4_adc_t *adc, unsigned int channel, unsigned int period);

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_ADC_H */
