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

#ifndef _I2S_H_INCLUDED
#define _I2S_H_INCLUDED

#include <Arduino.h>

#define I2S_BUFFER_SIZE 512

typedef enum {
    I2S_PHILIPS_MODE,
    I2S_RIGHT_JUSTIFIED_MODE,
    I2S_LEFT_JUSTIFIED_MODE
} i2s_mode_t;

class I2SClass : public Stream
{
public:
    I2SClass(struct _stm32l4_sai_t *sai, unsigned int instance, const struct _stm32l4_sai_pins_t *pins, unsigned int priority, unsigned int mode);

    // the SCK and FS pins are driven as outputs using the sample rate
    int begin(int mode, long sampleRate, int bitsPerSample, bool masterClock = false);
    // the SCK and FS pins are inputs, other side controls sample rate
    int begin(int mode, int bitsPerSample);
    void end();

    // from Stream
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();

    // from Print
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buffer, size_t size);
    
    virtual size_t availableForWrite();

    int read(void* buffer, size_t size);
    
    size_t write(int);
    size_t write(int32_t);
    size_t write(const void *buffer, size_t size);
    
    void onReceive(void(*)(void));
    void onTransmit(void(*)(void));
    
private:
    struct _stm32l4_sai_t *_sai;
    uint8_t _state;
    uint8_t _width;
    volatile uint8_t _xf_active;
    volatile uint8_t _xf_pending;
    volatile uint32_t _xf_offset;
    uint32_t _xf_data[2][I2S_BUFFER_SIZE / sizeof(uint32_t)];

    void (*_receiveCallback)(void);
    void (*_transmitCallback)(void);

    static void _eventCallback(void *context, uint32_t events);
    void EventCallback(uint32_t events);
};

#if I2S_INTERFACES_COUNT > 0
extern I2SClass I2S;
#endif

#if I2S_INTERFACES_COUNT > 1
extern I2SClass I2S1;
#endif

#endif
