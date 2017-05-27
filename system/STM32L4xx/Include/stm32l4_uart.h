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

#if !defined(_STM32L4_UART_H)
#define _STM32L4_UART_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#include "stm32l4_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    UART_INSTANCE_USART1 = 0,
    UART_INSTANCE_USART2,
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    UART_INSTANCE_USART3,
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    UART_INSTANCE_UART4,
    UART_INSTANCE_UART5,
#endif
    UART_INSTANCE_LPUART1,
    UART_INSTANCE_COUNT
};

#define UART_MODE_TX_DMA             0x00000001
#define UART_MODE_RX_DMA             0x00000002
#define UART_MODE_TX_DMA_SECONDARY   0x00000004
#define UART_MODE_RX_DMA_SECONDARY   0x00000008

#define UART_OPTION_STOP_MASK        0x0000000f
#define UART_OPTION_STOP_SHIFT       0
#define UART_OPTION_STOP_1           0x00000000
#define UART_OPTION_STOP_1_5         0x00000001
#define UART_OPTION_STOP_2           0x00000002
#define UART_OPTION_PARITY_MASK      0x000000f0
#define UART_OPTION_PARITY_SHIFT     4
#define UART_OPTION_PARITY_NONE      0x00000000
#define UART_OPTION_PARITY_ODD       0x00000010
#define UART_OPTION_PARITY_EVEN      0x00000020
#define UART_OPTION_DATA_SIZE_MASK   0x00000f00
#define UART_OPTION_DATA_SIZE_SHIFT  8
#define UART_OPTION_DATA_SIZE_7      0x00000000
#define UART_OPTION_DATA_SIZE_8      0x00000100
#define UART_OPTION_DATA_SIZE_9      0x00000200
#define UART_OPTION_CTS              0x00001000
#define UART_OPTION_RTS              0x00002000
#define UART_OPTION_DE_MODE          0x00004000
#define UART_OPTION_DE_ACTIVE_LOW    0x00000000
#define UART_OPTION_DE_ACTIVE_HIGH   0x00008000
#define UART_OPTION_LIN_MODE         0x00010000
#define UART_OPTION_LIN_BREAK_SHORT  0x00000000
#define UART_OPTION_LIN_BREAK_LONG   0x00020000
#define UART_OPTION_HALF_DUPLEX_MODE 0x00040000
#define UART_OPTION_RX_INVERT        0x00080000
#define UART_OPTION_TX_INVERT        0x00100000
#define UART_OPTION_DATA_INVERT      0x00200000

#define UART_EVENT_IDLE              0x00000001
#define UART_EVENT_BREAK             0x00000002
#define UART_EVENT_NOISE             0x00000004
#define UART_EVENT_PARITY            0x00000008
#define UART_EVENT_FRAMING           0x00000010
#define UART_EVENT_OVERRUN           0x00000020
#define UART_EVENT_TIMEOUT           0x20000000
#define UART_EVENT_RECEIVE           0x40000000
#define UART_EVENT_TRANSMIT          0x80000000

typedef void (*stm32l4_uart_callback_t)(void *context, uint32_t events);

#define UART_STATE_NONE              0
#define UART_STATE_INIT              1
#define UART_STATE_BUSY              2
#define UART_STATE_READY             3
#define UART_STATE_TRANSMIT          4
#define UART_STATE_BREAK             5

typedef struct _stm32l4_uart_pins_t {
    uint16_t                     rx;
    uint16_t                     tx;
    uint16_t                     cts;
    uint16_t                     rts_de;
} stm32l4_uart_pins_t;

typedef struct _stm32l4_uart_t {
    USART_TypeDef              *USART;
    volatile uint8_t           state;
    uint8_t                    instance;
    uint8_t                    interrupt;
    uint8_t                    priority;
    uint8_t                    mode;
    stm32l4_uart_pins_t        pins;
    stm32l4_uart_callback_t    callback;
    void                       *context;
    volatile uint32_t          events;
    const uint8_t              *tx_data;
    uint16_t                   tx_count;
    volatile uint16_t          tx_size;
    uint8_t                    rx_fifo[16];
    uint8_t                    *rx_data;
    uint16_t                   rx_size;
    uint16_t                   rx_read;
    uint16_t                   rx_write;
    uint16_t                   rx_index;
    uint16_t                   rx_event;
    volatile uint32_t          rx_count;
    stm32l4_dma_t              tx_dma;
    stm32l4_dma_t              rx_dma;
} stm32l4_uart_t;

extern bool stm32l4_uart_create(stm32l4_uart_t *uart, unsigned int instance, const stm32l4_uart_pins_t *pins, unsigned int priority, unsigned int mode);
extern bool stm32l4_uart_destroy(stm32l4_uart_t *uart);
extern bool stm32l4_uart_enable(stm32l4_uart_t *uart, uint8_t *rx_data, uint16_t rx_size, uint32_t bitrate, uint32_t option, stm32l4_uart_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_uart_disable(stm32l4_uart_t *uart);
extern bool stm32l4_uart_configure(stm32l4_uart_t *uart, uint32_t bitrate, uint32_t option);
extern bool stm32l4_uart_notify(stm32l4_uart_t *uart, stm32l4_uart_callback_t callback, void *context, uint32_t events);
extern unsigned int stm32l4_uart_receive(stm32l4_uart_t *uart, uint8_t *rx_data, uint16_t rx_count);
extern unsigned int stm32l4_uart_count(stm32l4_uart_t *uart);
extern int stm32l4_uart_peek(stm32l4_uart_t *uart);
extern bool stm32l4_uart_transmit(stm32l4_uart_t *uart, const uint8_t *tx_data, uint16_t tx_count);
extern bool stm32l4_uart_send_break(stm32l4_uart_t *uart);
extern bool stm32l4_uart_done(stm32l4_uart_t *uart);
extern void stm32l4_uart_poll(stm32l4_uart_t *uart);

extern void USART1_IRQHandler(void);
extern void USART2_IRQHandler(void);
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
extern void USART3_IRQHandler(void);
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
extern void UART4_IRQHandler(void);
extern void UART5_IRQHandler(void);
#endif
extern void LPUART1_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_UART_H */
