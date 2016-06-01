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

#if !defined(_STM32L4_QSPI_H)
#define _STM32L4_QSPI_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#include "stm32l4_dma.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define QSPI_INSTANCE_QUADSPI            0
#define QSPI_INSTANCE_COUNT              1

#define QSPI_MODE_DMA                    0x00000001
#define QSPI_MODE_DMA_SECONDARY          0x00000002

#define QSPI_OPTION_MODE_MASK            0x00000001
#define QSPI_OPTION_MODE_SHIFT           0
#define QSPI_OPTION_MODE_0               0x00000000
#define QSPI_OPTION_MODE_3               0x00000001

#define QSPI_EVENT_WAIT_DONE             0x20000000
#define QSPI_EVENT_RECEIVE_DONE          0x40000000
#define QSPI_EVENT_TRANSMIT_DONE         0x80000000

#define QSPI_COMMAND_OPCODE_MASK         0x000000ff
#define QSPI_COMMAND_OPCODE_SHIFT        0
#define QSPI_COMMAND_INSTRUCTION_MASK    0x00000300
#define QSPI_COMMAND_INSTRUCTION_NONE    0x00000000
#define QSPI_COMMAND_INSTRUCTION_SINGLE  0x00000100
#define QSPI_COMMAND_INSTRUCTION_DUAL    0x00000200
#define QSPI_COMMAND_INSTRUCTION_QUAD    0x00000300
#define QSPI_COMMAND_ADDRESS_MASK        0x00000c00
#define QSPI_COMMAND_ADDRESS_NONE        0x00000000
#define QSPI_COMMAND_ADDRESS_SINGLE      0x00000400
#define QSPI_COMMAND_ADDRESS_DUAL        0x00000800
#define QSPI_COMMAND_ADDRESS_QUAD        0x00000c00
#define QSPI_COMMAND_ADDRESS_24BIT       0x00002000
#define QSPI_COMMAND_ADDRESS_32BIT       0x00003000
#define QSPI_COMMAND_MODE_MASK           0x0000c000
#define QSPI_COMMAND_MODE_NONE           0x00000000
#define QSPI_COMMAND_MODE_SINGLE         0x00004000
#define QSPI_COMMAND_MODE_DUAL           0x00008000
#define QSPI_COMMAND_MODE_QUAD           0x0000c000
#define QSPI_COMMAND_WAIT_STATES_MASK    0x007c0000
#define QSPI_COMMAND_WAIT_STATES_SHIFT   18
#define QSPI_COMMAND_DATA_MASK           0x03000000
#define QSPI_COMMAND_DATA_NONE           0x00000000
#define QSPI_COMMAND_DATA_SINGLE         0x01000000
#define QSPI_COMMAND_DATA_DUAL           0x02000000
#define QSPI_COMMAND_DATA_QUAD           0x03000000

#define QSPI_CONTROL_ASYNC               0x00000001

typedef void (*stm32l4_qspi_callback_t)(void *context, uint32_t events);

#define QSPI_STATE_NONE                  0
#define QSPI_STATE_BUSY                  1
#define QSPI_STATE_READY                 2
#define QSPI_STATE_SELECTED              3
#define QSPI_STATE_RECEIVE               4
#define QSPI_STATE_TRANSMIT              5
#define QSPI_STATE_WAIT                  6

typedef struct _stm32l4_qspi_pins_t {
    uint16_t                    clk;
    uint16_t                    ncs;
    uint16_t                    io0;
    uint16_t                    io1;
    uint16_t                    io2;
    uint16_t                    io3;
} stm32l4_qspi_pins_t;

typedef struct _stm32l4_qspi_t {
    volatile uint8_t            state;
    uint8_t                     instance;
    uint8_t                     interrupt;
    uint8_t                     priority;
    stm32l4_qspi_pins_t         pins;
    uint8_t                     mode;
    uint16_t                    select;
    uint32_t                    clock;
    uint32_t                    option;
    stm32l4_qspi_callback_t     callback;
    void                        *context;
    uint32_t                    events;
    uint8_t                     *rx_data;
    uint32_t                    rx_count;
    stm32l4_dma_t               dma;
} stm32l4_qspi_t;

extern bool stm32l4_qspi_create(stm32l4_qspi_t *qspi, unsigned int instance, const stm32l4_qspi_pins_t *pins, unsigned int priority, unsigned int mode);
extern bool stm32l4_qspi_destroy(stm32l4_qspi_t *qspi);
extern bool stm32l4_qspi_enable(stm32l4_qspi_t *qspi, uint32_t clock, uint32_t option, stm32l4_qspi_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_qspi_disable(stm32l4_qspi_t *qspi);
extern bool stm32l4_qspi_configure(stm32l4_qspi_t *qspi, uint32_t clock, uint32_t option);
extern bool stm32l4_qspi_notify(stm32l4_qspi_t *qspi, stm32l4_qspi_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_qspi_select(stm32l4_qspi_t *qspi);
extern bool stm32l4_qspi_unselect(stm32l4_qspi_t *qspi);
extern bool stm32l4_qspi_mode(stm32l4_qspi_t *qspi, uint32_t mode);
extern bool stm32l4_qspi_command(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address);
extern bool stm32l4_qspi_receive(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address, uint8_t *rx_data, unsigned int rx_count, uint32_t control);
extern bool stm32l4_qspi_transmit(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address, const uint8_t *tx_data, unsigned int tx_count, uint32_t control);
extern bool stm32l4_qspi_wait(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address, uint8_t *rx_data, unsigned int rx_count, uint32_t mask, uint32_t match, uint32_t control);
extern bool stm32l4_qspi_done(stm32l4_qspi_t *qspi);

extern void QUADSPI_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_QSPI_H */
