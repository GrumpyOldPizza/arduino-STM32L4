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

#pragma once

#include "HardwareSerial.h"

#define UART_RX_BUFFER_SIZE 64
#define UART_TX_BUFFER_SIZE 64

class Uart : public HardwareSerial
{
public:
    Uart(struct _stm32l4_uart_t *uart, unsigned int instance, const struct _stm32l4_uart_pins_t *pins, unsigned int priority, unsigned int mode, bool serialEvent);
    void begin(unsigned long baudRate);
    void begin(unsigned long baudrate, uint32_t config);
    void begin(unsigned long baudRate, uint8_t *buffer, size_t size);
    void begin(unsigned long baudrate, uint32_t config, uint8_t *buffer, size_t size);
    void end();
    int available();
    int availableForWrite(void);
    int peek();
    int read();
    void flush();
    size_t write(const uint8_t data);
    size_t write(const uint8_t *buffer, size_t size);
    using Print::write; // pull in write(str) and write(buf, size) from Print

    operator bool() { return true; }

    // STM32L4 EXTENSTION: non-blocking multi-byte read
    size_t read(uint8_t *buffer, size_t size);

    // STM32L4 EXTENSTION: asynchronous write with callback
    bool write(const uint8_t *buffer, size_t size, void(*callback)(void));
    bool done(void);

    // STM32L4 EXTENSTION: asynchronous receive
    void onReceive(void(*callback)(void));

    // STM32L4 EXTENSTION: enable/disabe blocking writes
    void blockOnOverrun(bool enable);

private:
    struct _stm32l4_uart_t *_uart;
    bool _blocking;
    uint8_t _rx_data[UART_RX_BUFFER_SIZE];
    uint8_t _tx_data[UART_TX_BUFFER_SIZE];
    volatile uint16_t _tx_write;
    volatile uint16_t _tx_read;
    volatile uint32_t _tx_count;
    volatile uint32_t _tx_size;

    const uint8_t *_tx_data2;
    volatile uint32_t _tx_size2;

    void (*_completionCallback)(void);
    void (*_receiveCallback)(void);

    static void _event_callback(void *context, uint32_t events);
    void EventCallback(uint32_t events);
};
