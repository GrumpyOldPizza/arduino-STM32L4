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

#if !defined(_STM32L4_SAI_H)
#define _STM32L4_SAI_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#include "stm32l4_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SAI_INSTANCE_SAI1A = 0,
    SAI_INSTANCE_SAI1B = 1,
#if defined(STM32L476xx) || defined(STM32L496xx)
    SAI_INSTANCE_SAI2A = 2,
    SAI_INSTANCE_SAI2B = 3,
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
    SAI_INSTANCE_COUNT
};

#define SAI_MODE_DMA                      0x00000001
#define SAI_MODE_DMA_SECONDARY            0x00000002

#define SAI_OPTION_FORMAT_MASK            0x00000007
#define SAI_OPTION_FORMAT_SHIFT           0
#define SAI_OPTION_FORMAT_I2S             0x00000000
#define SAI_OPTION_FORMAT_LEFT_JUSTIFIED  0x00000001
#define SAI_OPTION_FORMAT_RIGHT_JUSTIFIED 0x00000002
#define SAI_OPTION_FORMAT_PCM_SHORT       0x00000003
#define SAI_OPTION_FORMAT_PCM_LONG        0x00000004
#define SAI_OPTION_FORMAT_DSP             0x00000005
#define SAI_OPTION_MONO                   0x00000008
#define SAI_OPTION_MCK                    0x00000010

#define SAI_EVENT_RECEIVE_DONE            0x10000000
#define SAI_EVENT_RECEIVE_REQUEST         0x20000000
#define SAI_EVENT_TRANSMIT_DONE           0x40000000
#define SAI_EVENT_TRANSMIT_REQUEST        0x80000000

typedef void (*stm32l4_sai_callback_t)(void *context, uint32_t events);

  // typedef void (*stm32l4_sai_xf_callback_t)(struct _stm32l4_sai_t *sai);

#define SAI_STATE_NONE                 0
#define SAI_STATE_INIT                 1
#define SAI_STATE_BUSY                 2
#define SAI_STATE_READY                3
#define SAI_STATE_RECEIVE_8            4
#define SAI_STATE_RECEIVE_16           5
#define SAI_STATE_RECEIVE_32           6
#define SAI_STATE_RECEIVE_DMA          7
#define SAI_STATE_RECEIVE_REQUEST      8
#define SAI_STATE_RECEIVE_DONE         9
#define SAI_STATE_TRANSMIT_8           10
#define SAI_STATE_TRANSMIT_16          11
#define SAI_STATE_TRANSMIT_32          12
#define SAI_STATE_TRANSMIT_DMA         13
#define SAI_STATE_TRANSMIT_REQUEST     14
#define SAI_STATE_TRANSMIT_DONE        15

typedef struct _stm32l4_sai_pins_t {
    uint16_t                     sck;
    uint16_t                     fs;
    uint16_t                     sd;
    uint16_t                     mck;
} stm32l4_sai_pins_t;

typedef struct _stm32l4_sai_t {
    SAI_Block_TypeDef            *SAIx;
    volatile uint8_t             state;
    uint8_t                      instance;
    stm32l4_sai_pins_t           pins;
    uint8_t                      interrupt;
    uint8_t                      priority;
    uint8_t                      mode;
    uint8_t                      width;
    uint32_t                     option;
    stm32l4_sai_callback_t       callback;
    void                         *context;
    uint32_t                     events;
    void                         (*xf_callback)(struct _stm32l4_sai_t *sai);
    void                         *xf_data;
    void                         *xf_data_e;
    stm32l4_dma_t                dma;
} stm32l4_sai_t;


extern bool stm32l4_sai_create(stm32l4_sai_t *sai, unsigned int instance, const stm32l4_sai_pins_t *pins, unsigned int priority, unsigned int mode);
extern bool stm32l4_sai_destroy(stm32l4_sai_t *sai);
extern bool stm32l4_sai_enable(stm32l4_sai_t *sai, uint32_t width, uint32_t clock, uint32_t option, stm32l4_sai_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_sai_disable(stm32l4_sai_t *sai);
extern bool stm32l4_sai_configure(stm32l4_sai_t *sai, uint32_t width, uint32_t clock, uint32_t option);
extern bool stm32l4_sai_notify(stm32l4_sai_t *sai, stm32l4_sai_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_sai_receive(stm32l4_sai_t *sai, uint8_t *rx_data, uint16_t rx_count);
extern bool stm32l4_sai_transmit(stm32l4_sai_t *sai, const uint8_t *tx_data, uint16_t tx_count);
extern bool stm32l4_sai_done(stm32l4_sai_t *sai);

extern void SAI1_IRQHandler(void);
#if defined(STM32L476xx) || defined(STM32L496xx)
extern void SAI2_IRQHandler(void);
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */

#ifdef __cplusplus
}
#endif

#if 0

L476, MCK, voltage range 1, (44100 / 192000)

8000 7998 [86 7 12]
12000 11997 [86 7 8]
16000 15997 [86 7 6]
24000 23995 [86 7 4]
32000 31994 [86 7 3]
48000 47991 [86 7 2]
96000 95982 [86 7 1]
11025 11029 [48 17 2]
22050 22058 [48 17 1]
44100 44117 [48 17 0]

L433/L432/L496, no MCK, voltage range 2, (44100 / 96000 / 32000)

8000 8072 [31 15 2]
12000 12109 [31 5 4]
16000 16145 [31 15 1]
24000 24218 [31 5 2]
32000 32291 [31 15 0]
48000 48437 [31 5 1]
96000 96875 [31 5 0]
11025 11008 [31 11 2]
22050 22017 [31 11 1]
44100 44034 [31 11 0]

L433/L432/L496, MCK, voltage range 1, (44100 / 96000 / 32000)

8000 7998 [43 21 2]
12000 11997 [43 7 4]
16000 15997 [43 21 1]
24000 23995 [43 7 2]
32000 31994 [43 21 0]
48000 47991 [43 7 1]
96000 95982 [43 7 0]
11025 11023 [48 17 2]
22050 22046 [48 17 1]
44100 44092 [48 17 0]

#endif

#endif /* _STM32L4_SAI_H */
