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

#if !defined(_STM32L4_I2C_H)
#define _STM32L4_I2C_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

#include "stm32l4_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    I2C_INSTANCE_I2C1  = 0,
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    I2C_INSTANCE_I2C2,
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
    I2C_INSTANCE_I2C3,
#if defined(STM32L496xx)
    I2C_INSTANCE_I2C4,
#endif /* defined(STM32L496xx) */
    I2C_INSTANCE_COUNT
};

#define I2C_MODE_TX_DMA                0x00000001
#define I2C_MODE_RX_DMA                0x00000002
#define I2C_MODE_TX_DMA_SECONDARY      0x00000004
#define I2C_MODE_RX_DMA_SECONDARY      0x00000008

#define I2C_CLOCK_10                   10000
#define I2C_CLOCK_100                  100000
#define I2C_CLOCK_400                  400000
#define I2C_CLOCK_1000                 1000000

#define I2C_OPTION_ALTERNATE           0x00000001
#define I2C_OPTION_RESET               0x00000002
#define I2C_OPTION_WAKEUP              0x00000004
#define I2C_OPTION_GENERAL_CALL        0x00000008
#define I2C_OPTION_ADDRESS_MASK        0xff000000
#define I2C_OPTION_ADDRESS_SHIFT       24

#define I2C_EVENT_ADDRESS_NACK         0x00000001
#define I2C_EVENT_DATA_NACK            0x00000002
#define I2C_EVENT_ARBITRATION_LOST     0x00000004
#define I2C_EVENT_BUS_ERROR            0x00000008
#define I2C_EVENT_OVERRUN              0x00000010
#define I2C_EVENT_ADDRESS_MATCH        0x01000000
#define I2C_EVENT_RECEIVE_REQUEST      0x02000000
#define I2C_EVENT_RECEIVE_ERROR        0x04000000
#define I2C_EVENT_RECEIVE_DONE         0x08000000
#define I2C_EVENT_TRANSMIT_REQUEST     0x10000000
#define I2C_EVENT_TRANSMIT_ERROR       0x20000000
#define I2C_EVENT_TRANSMIT_DONE        0x40000000
#define I2C_EVENT_TRANSFER_DONE        0x80000000

#define I2C_STATUS_ADDRESS_NACK        0x00000001
#define I2C_STATUS_DATA_NACK           0x00000002
#define I2C_STATUS_ARBITRATION_LOST    0x00000004
#define I2C_STATUS_BUS_ERROR           0x00000008
#define I2C_STATUS_OVERRUN             0x00000010
#define I2C_STATUS_ABORT               0x00000020

#define I2C_CONTROL_RESTART            0x00000001

typedef void (*stm32l4_i2c_callback_t)(void *context, uint32_t events);

#define I2C_STATE_NONE                 0
#define I2C_STATE_INIT                 1
#define I2C_STATE_BUSY                 2
#define I2C_STATE_READY                3
#define I2C_STATE_SLAVE_RECEIVE        4
#define I2C_STATE_SLAVE_TRANSMIT       5
#define I2C_STATE_MASTER_RESTART       6
#define I2C_STATE_MASTER_RECEIVE       7
#define I2C_STATE_MASTER_TRANSMIT      8

typedef struct _stm32l4_i2c_pins_t {
    uint16_t                     scl;
    uint16_t                     sda;
} stm32l4_i2c_pins_t;

typedef struct _stm32l4_i2c_t {
    I2C_TypeDef                  *I2C;
    volatile uint8_t             state;
    uint8_t                      instance;
    stm32l4_i2c_pins_t           pins;
    uint8_t                      interrupt;
    uint8_t                      priority;
    uint8_t                      mode;
    uint32_t                     clock;
    uint32_t                     option;
    stm32l4_i2c_callback_t       callback;
    void                         *context;
    uint32_t                     events;
    uint16_t                     xf_address;
    uint16_t                     xf_control;
    uint16_t                     xf_status;
    uint16_t                     xf_count;
    uint16_t                     tx_count;
    uint16_t                     rx_count;
    const uint8_t                *tx_data;
    uint8_t                      *rx_data;
    stm32l4_dma_t                tx_dma;
    stm32l4_dma_t                rx_dma;
} stm32l4_i2c_t;


extern bool stm32l4_i2c_create(stm32l4_i2c_t *i2c, unsigned int instance, const stm32l4_i2c_pins_t *pins, unsigned int priority, unsigned int mode);
extern bool stm32l4_i2c_destroy(stm32l4_i2c_t *i2c);
extern bool stm32l4_i2c_reset(stm32l4_i2c_t *i2c);
extern bool stm32l4_i2c_enable(stm32l4_i2c_t *i2c, uint32_t clock, uint32_t option, stm32l4_i2c_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_i2c_disable(stm32l4_i2c_t *i2c);
extern bool stm32l4_i2c_configure(stm32l4_i2c_t *i2c, uint32_t clock, uint32_t option);
extern bool stm32l4_i2c_notify(stm32l4_i2c_t *i2c, stm32l4_i2c_callback_t callback, void *context, uint32_t events);
extern bool stm32l4_i2c_reset(stm32l4_i2c_t *i2c);
extern bool stm32l4_i2c_receive(stm32l4_i2c_t *i2c, uint16_t address, uint8_t *rx_data, uint16_t rx_count, uint32_t control);
extern bool stm32l4_i2c_transmit(stm32l4_i2c_t *i2c, uint16_t address, const uint8_t *tx_data, uint16_t tx_count, uint32_t control);
extern bool stm32l4_i2c_transfer(stm32l4_i2c_t *i2c, uint16_t address, const uint8_t *tx_data, uint16_t tx_count, uint8_t *rx_data, uint16_t rx_count, uint32_t control);
extern bool stm32l4_i2c_service(stm32l4_i2c_t *i2c, uint8_t *xf_data, uint16_t xf_count);
extern bool stm32l4_i2c_done(stm32l4_i2c_t *i2c);
extern unsigned int stm32l4_i2c_address(stm32l4_i2c_t *i2c);
extern unsigned int stm32l4_i2c_count(stm32l4_i2c_t *i2c);
extern unsigned int stm32l4_i2c_status(stm32l4_i2c_t *i2c);
extern void stm32l4_i2c_poll(stm32l4_i2c_t *i2c);

extern void I2C1_EV_IRQHandler(void);
extern void I2C1_ER_IRQHandler(void);
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
extern void I2C2_EV_IRQHandler(void);
extern void I2C2_ER_IRQHandler(void);
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
extern void I2C3_EV_IRQHandler(void);
extern void I2C3_ER_IRQHandler(void);
#if defined(STM32L496xx)
extern void I2C4_EV_IRQHandler(void);
extern void I2C4_ER_IRQHandler(void);
#endif /* defined(STM32L496xx) */

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_I2C_H */
