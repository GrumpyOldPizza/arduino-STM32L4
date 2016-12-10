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

#pragma once

#include "HardwareSerial.h"

class USBDeviceClass
{
public:
    USBDeviceClass() {};

    // USB Device API
    void init();
    bool attach();
    bool detach();
    void poll();
    
    bool connected();
    bool configured();
    bool suspended();

private:
    bool initialized;
};

extern USBDeviceClass USBDevice;

#define CDC_RX_BUFFER_SIZE 512
#define CDC_TX_BUFFER_SIZE 512

class CDC : public HardwareSerial
{
public:
    CDC(struct _stm32l4_usbd_cdc_t *usbd_cdc, bool serialEvent);
    void begin(unsigned long baudRate);
    void begin(unsigned long baudrate, uint16_t config);
    void end(void);

    int available(void);
    int availableForWrite(void);
    int peek(void);
    
    int read(void);
    void flush(void);
    size_t write(uint8_t);
    size_t write(const uint8_t *buffer, size_t size);
    using Print::write; // pull in write(str) from Print

    operator bool();

    // These return the settings specified by the USB host for the
    // serial port. These aren't really used, but are offered here
    // in case a sketch wants to act on these settings.
    uint32_t baud();
    uint8_t stopbits();
    uint8_t paritytype();
    uint8_t numbits();
    bool dtr();
    bool rts();

    enum {
	ONE_STOP_BIT = 0,
	ONE_AND_HALF_STOP_BIT = 1,
	TWO_STOP_BITS = 2,
    };
    enum {
	NO_PARITY = 0,
	ODD_PARITY = 1,
	EVEN_PARITY = 2,
	MARK_PARITY = 3,
	SPACE_PARITY = 4,
    };

    // STM32L4 EXTENSTION: non-blocking multi-byte read
    size_t read(uint8_t *buffer, size_t size);

    // STM32L4 EXTENSTION: asynchronous write with callback
    bool write(const uint8_t *buffer, size_t size, void(*callback)(void));
    bool done(void);

    // STM32L4 EXTENSTION: asynchronous receive
    void onReceive(void(*callback)(int));

    // STM32L4 EXTENSTION: enable/disabe blocking writes
    void blockOnOverrun(bool enable);

    // STM32L4 EXTENSTION: isEnabled() check
    bool isEnabled(void);

private:
    struct _stm32l4_usbd_cdc_t *_usbd_cdc;
    bool _blocking;
    uint8_t _rx_data[CDC_RX_BUFFER_SIZE];
    volatile uint16_t _rx_write;
    volatile uint16_t _rx_read;
    volatile uint32_t _rx_count;
    uint8_t _tx_data[CDC_TX_BUFFER_SIZE];
    volatile uint16_t _tx_write;
    volatile uint16_t _tx_read;
    volatile uint32_t _tx_count;
    volatile uint32_t _tx_size;

    const uint8_t *_tx_data2;
    volatile uint32_t _tx_size2;

    void (*_completionCallback)(void);
    void (*_receiveCallback)(int);

    static void _event_callback(void *context, uint32_t events);
    void EventCallback(uint32_t events);
};
