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

#ifndef TwoWire_h
#define TwoWire_h

#include "Stream.h"
#include "variant.h"

#define TWI_RX_BUFFER_SIZE 32
#define TWI_TX_BUFFER_SIZE 32

 // WIRE_HAS_END means Wire has end()
#define WIRE_HAS_END 1

class TwoWire : public Stream
{
  public:
    TwoWire(struct _stm32l4_i2c_t *i2c, unsigned int instance, const struct _stm32l4_i2c_pins_t *pins, unsigned int priority, unsigned int mode);
    void begin(uint8_t address, uint32_t clock, bool alternate);
    inline void begin() { return begin(0, TWI_CLOCK, false); }
    inline void begin(uint8_t address) { return begin(address, TWI_CLOCK, false); }
    void end();
    void setClock(uint32_t);

    void beginTransmission(uint8_t);
    uint8_t endTransmission(bool stopBit);
    uint8_t endTransmission(void);

    uint8_t requestFrom(uint8_t address, size_t quantity, bool stopBit);
    uint8_t requestFrom(uint8_t address, size_t quantity);

    size_t write(uint8_t data);
    size_t write(const uint8_t *buffer, size_t quantity);

    int available(void);
    int read(void);
    int peek(void);
    void flush(void);
    void onReceive(void(*callback)(int));
    void onRequest(void(*callback)(void));

    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write;

    // STM32L4 EXTENSTION: synchronous composite transmit/receive 
    uint8_t transfer(uint8_t address, const uint8_t *txBuffer, size_t txSize, uint8_t *rxBuffer, size_t rxSize, bool stopBit = true);

    // STM32L4 EXTENSTION: asynchronous composite transmit/receive with callback
    bool transfer(uint8_t address, const uint8_t *txBuffer, size_t txSize, uint8_t *rxBuffer, size_t rxSize, bool stopBit, void(*callback)(uint8_t));
    bool done(void);
    uint8_t status(void);

    // STM32L4 EXTENSTION: isEnabled() check
    bool isEnabled(void);

    // STM32L4 EXTENSTION: reset I2C bus
    void reset(void);

  private:
    struct _stm32l4_i2c_t *_i2c;
    uint32_t _option;

    uint8_t _rx_data[TWI_RX_BUFFER_SIZE];
    uint8_t _rx_read;
    uint8_t _rx_write;

    uint8_t _tx_data[TWI_TX_BUFFER_SIZE];
    uint8_t _tx_write;
    uint8_t _tx_address;
    bool _tx_active;

    void (*_completionCallback)(uint8_t);
    void (*_requestCallback)(void);
    void (*_receiveCallback)(int);

    static void _eventCallback(void *context, uint32_t events);
    void EventCallback(uint32_t events);

    static const uint32_t TWI_CLOCK = 100000;
};

#if WIRE_INTERFACES_COUNT > 0
  extern TwoWire Wire;
#endif
#if WIRE_INTERFACES_COUNT > 1
  extern TwoWire Wire1;
#endif
#if WIRE_INTERFACES_COUNT > 2
  extern TwoWire Wire2;
#endif

#endif
