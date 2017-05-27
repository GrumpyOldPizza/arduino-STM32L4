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

#if !defined(_STM32L4_SPI_H)
#define _STM32L4_SPI_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#include "stm32l4_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SPI_INSTANCE_SPI1  = 0,
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    SPI_INSTANCE_SPI2,
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
    SPI_INSTANCE_SPI3,
    SPI_INSTANCE_COUNT
};

#define SPI_MODE_RX_DMA                0x00000001
#define SPI_MODE_TX_DMA                0x00000002
#define SPI_MODE_RX_DMA_SECONDARY      0x00000004
#define SPI_MODE_TX_DMA_SECONDARY      0x00000008

#define SPI_OPTION_MODE_MASK           0x00000003
#define SPI_OPTION_MODE_SHIFT          0
#define SPI_OPTION_MODE_0              0x00000000
#define SPI_OPTION_MODE_1              0x00000001
#define SPI_OPTION_MODE_2              0x00000002
#define SPI_OPTION_MODE_3              0x00000003
#define SPI_OPTION_DIV_MASK            0x00000038
#define SPI_OPTION_DIV_SHIFT           3
#define SPI_OPTION_DIV_2               0x00000000
#define SPI_OPTION_DIV_4               0x00000008
#define SPI_OPTION_DIV_8               0x00000010
#define SPI_OPTION_DIV_16              0x00000018
#define SPI_OPTION_DIV_32              0x00000020
#define SPI_OPTION_DIV_64              0x00000028
#define SPI_OPTION_DIV_128             0x00000030
#define SPI_OPTION_DIV_256             0x00000038
#define SPI_OPTION_MSB_FIRST           0x00000000
#define SPI_OPTION_LSB_FIRST           0x00000080

#define SPI_CONTROL_HALFDUPLEX         0x00000001
#define SPI_CONTROL_CRC16              0x00000002

#define SPI_EVENT_RECEIVE_DONE         0x20000000
#define SPI_EVENT_TRANSMIT_DONE        0x40000000
#define SPI_EVENT_TRANSFER_DONE        0x80000000

typedef void (*stm32l4_spi_callback_t)(void *context, uint32_t events);

#define SPI_STATE_NONE                 0
#define SPI_STATE_INIT                 1
#define SPI_STATE_BUSY                 2
#define SPI_STATE_READY                3
#define SPI_STATE_SELECTED             4
#define SPI_STATE_HALFDUPLEX_8_DMA     5
#define SPI_STATE_HALFDUPLEX_8_M       6
#define SPI_STATE_HALFDUPLEX_8_1       7
#define SPI_STATE_RECEIVE_8_DMA        8
#define SPI_STATE_RECEIVE_8_3          9
#define SPI_STATE_RECEIVE_8_2          10
#define SPI_STATE_RECEIVE_8_1          11
#define SPI_STATE_RECEIVE_8_CRC16_H    12
#define SPI_STATE_RECEIVE_8_CRC16_L    13
#define SPI_STATE_RECEIVE_16_DMA       14
#define SPI_STATE_RECEIVE_16_S         15
#define SPI_STATE_RECEIVE_16_M         16
#define SPI_STATE_RECEIVE_16_2         17
#define SPI_STATE_RECEIVE_16_1         18
#define SPI_STATE_RECEIVE_16_CRC16     19
#define SPI_STATE_TRANSMIT_8_DMA       20
#define SPI_STATE_TRANSMIT_8_3         21
#define SPI_STATE_TRANSMIT_8_2         22
#define SPI_STATE_TRANSMIT_8_1         23
#define SPI_STATE_TRANSMIT_8_CRC16_H   24
#define SPI_STATE_TRANSMIT_8_CRC16_L   25
#define SPI_STATE_TRANSMIT_16_DMA      26
#define SPI_STATE_TRANSMIT_16_S        27
#define SPI_STATE_TRANSMIT_16_M        28
#define SPI_STATE_TRANSMIT_16_2        29
#define SPI_STATE_TRANSMIT_16_1        30
#define SPI_STATE_TRANSMIT_16_CRC16    31
#define SPI_STATE_TRANSFER_8_DMA       32
#define SPI_STATE_TRANSFER_8_3         33
#define SPI_STATE_TRANSFER_8_2         34
#define SPI_STATE_TRANSFER_8_1         35
#define SPI_STATE_TRANSFER_8_CRC16_H   36
#define SPI_STATE_TRANSFER_8_CRC16_L   37
#define SPI_STATE_TRANSFER_16_DMA      38
#define SPI_STATE_TRANSFER_16_S        39
#define SPI_STATE_TRANSFER_16_M        40
#define SPI_STATE_TRANSFER_16_2        41
#define SPI_STATE_TRANSFER_16_1        42
#define SPI_STATE_TRANSFER_16_CRC16    43


#define SPI_CR1_BR_DIV2   (0)
#define SPI_CR1_BR_DIV4   (SPI_CR1_BR_0)
#define SPI_CR1_BR_DIV8   (SPI_CR1_BR_1)
#define SPI_CR1_BR_DIV16  (SPI_CR1_BR_0 | SPI_CR1_BR_1)
#define SPI_CR1_BR_DIV32  (SPI_CR1_BR_2)
#define SPI_CR1_BR_DIV64  (SPI_CR1_BR_0 | SPI_CR1_BR_2)
#define SPI_CR1_BR_DIV128 (SPI_CR1_BR_1 | SPI_CR1_BR_2)
#define SPI_CR1_BR_DIV256 (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2)

#define SPI_CR2_DS_8BIT   (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2)
#define SPI_CR2_DS_16BIT  (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2 | SPI_CR2_DS_3)

typedef struct _stm32l4_spi_pins_t {
    uint16_t                    mosi;
    uint16_t                    miso;
    uint16_t                    sck;
    uint16_t                    ss;
} stm32l4_spi_pins_t;

typedef struct _stm32l4_spi_t {
    SPI_TypeDef                 *SPI;
    volatile uint8_t            state;
    uint8_t                     instance;
    uint8_t                     interrupt;
    uint8_t                     priority;
    stm32l4_spi_pins_t          pins;
    uint8_t                     mode;
    uint8_t                     select;
    uint32_t                    option;
    uint32_t                    cr1;
    uint32_t                    cr2;
    stm32l4_spi_callback_t      callback;
    void                        *context;
    uint32_t                    events;
    const uint8_t               *tx_data;
    uint8_t                     *rx_data;
    const uint8_t               *tx_data_e;
    uint8_t                     *rx_data_e;
    uint32_t                    xf_count;
    uint32_t                    xf_size;
    uint16_t                    tx_default;
    uint16_t                    rx_null;
    uint16_t                    crc16;
    uint8_t                     rx_crc16[2];
    stm32l4_dma_t               tx_dma;
    stm32l4_dma_t               rx_dma;
} stm32l4_spi_t;

extern bool stm32l4_spi_create(stm32l4_spi_t *spi, unsigned int instance, const stm32l4_spi_pins_t *pins, unsigned int priority, unsigned int mode);
extern bool stm32l4_spi_destroy(stm32l4_spi_t *spi);
extern uint32_t stm32l4_spi_clock(stm32l4_spi_t *spi);
extern bool stm32l4_spi_enable(stm32l4_spi_t *spi, stm32l4_spi_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_spi_disable(stm32l4_spi_t *spi);
extern bool stm32l4_spi_notify(stm32l4_spi_t *spi, stm32l4_spi_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_spi_select(stm32l4_spi_t *spi, uint32_t option);
extern bool stm32l4_spi_unselect(stm32l4_spi_t *spi);
extern bool stm32l4_spi_configure(stm32l4_spi_t *spi, uint32_t option);
extern void stm32l4_spi_exchange(stm32l4_spi_t *spi, const uint8_t *tx_data, uint8_t *rx_data, unsigned int count);
extern uint8_t stm32l4_spi_exchange8(stm32l4_spi_t *spi, uint8_t data);
extern uint16_t stm32l4_spi_exchange16(stm32l4_spi_t *spi, uint16_t data);
extern bool stm32l4_spi_receive(stm32l4_spi_t *spi, uint8_t *rx_data, unsigned int rx_count, uint32_t control);
extern bool stm32l4_spi_transmit(stm32l4_spi_t *spi, const uint8_t *tx_data, unsigned int tx_count, uint32_t control);
extern bool stm32l4_spi_transfer(stm32l4_spi_t *spi, const uint8_t *tx_data, uint8_t *rx_data, unsigned int count, uint32_t control);
extern bool stm32l4_spi_done(stm32l4_spi_t *spi);
extern uint16_t stm32l4_spi_crc16(stm32l4_spi_t *spi);
extern void stm32l4_spi_poll(stm32l4_spi_t *spi);

extern void SPI1_IRQHandler(void);
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
extern void SPI2_IRQHandler(void);
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
extern void SPI3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_SPI_H */
