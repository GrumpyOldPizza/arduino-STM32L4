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

#include "Arduino.h"
#include "stm32l4_wiring_private.h"
#include "I2S.h"

#define I2S_STATE_IDLE     0
#define I2S_STATE_READY    1
#define I2S_STATE_RECEIVE  2
#define I2S_STATE_TRANSMIT 3

I2SClass::I2SClass(struct _stm32l4_sai_t *sai, unsigned int instance, const struct _stm32l4_sai_pins_t *pins, unsigned int priority, unsigned int mode)
{
    _sai = sai;

    stm32l4_sai_create(sai, instance, pins, priority, mode);

    _state = I2S_STATE_IDLE;

    _xf_active = 0;
    _xf_pending = 0;
    _xf_offset = 0;

    _receiveCallback = NULL;
    _transmitCallback = NULL;
}

int I2SClass::begin(int mode, long sampleRate, int bitsPerSample, bool masterClock)
{
    uint32_t option;

    if (_state != I2S_STATE_IDLE) {
	return 0;
    }

    switch (mode) {
    case I2S_PHILIPS_MODE:
	option = SAI_OPTION_FORMAT_I2S;
	break;
    case I2S_RIGHT_JUSTIFIED_MODE:
	option = SAI_OPTION_FORMAT_RIGHT_JUSTIFIED;
	break;
    case I2S_LEFT_JUSTIFIED_MODE:
	option = SAI_OPTION_FORMAT_LEFT_JUSTIFIED;
	break;
    default:
	return 0;
    }

    switch (sampleRate) {
    case 8000:
    case 12000:
    case 16000:
    case 24000:
    case 32000:
    case 48000:
    case 96000:
    case 11025:
    case 22050:
    case 44100:
	break;
    default:
	return 0;
    }

    switch (bitsPerSample) {
    case 8:
    case 16:
    case 32:
	_width = bitsPerSample;
	break;
    default:
	return 0;
    }

    if (masterClock) {
	option |= SAI_OPTION_MCK;
    }

    stm32l4_sai_enable(_sai, bitsPerSample, sampleRate, option, I2SClass::_eventCallback, (void*)this, (SAI_EVENT_RECEIVE_REQUEST | SAI_EVENT_TRANSMIT_REQUEST));

    _state = I2S_STATE_READY;

    return 1;
}

int I2SClass::begin(int mode, int bitsPerSample)
{
    uint32_t option;

    if (_state != I2S_STATE_IDLE) {
	return 0;
    }

    switch (mode) {
    case I2S_PHILIPS_MODE:
	option = SAI_OPTION_FORMAT_I2S;
	break;
    case I2S_RIGHT_JUSTIFIED_MODE:
	option = SAI_OPTION_FORMAT_RIGHT_JUSTIFIED;
	break;
    case I2S_LEFT_JUSTIFIED_MODE:
	option = SAI_OPTION_FORMAT_LEFT_JUSTIFIED;
	break;
    default:
	return 0;
    }

    switch (bitsPerSample) {
    case 8:
    case 16:
    case 32:
	_width = bitsPerSample;
	break;
    default:
	return 0;
    }

    stm32l4_sai_enable(_sai, bitsPerSample, 0, option, I2SClass::_eventCallback, (void*)this, (SAI_EVENT_RECEIVE_REQUEST | SAI_EVENT_TRANSMIT_REQUEST));

    _state = I2S_STATE_READY;

    return 1;
}

void I2SClass::end()
{
    while (_xf_active) {
	armv7m_core_yield();
    }

    stm32l4_sai_disable(_sai);

    _state = I2S_STATE_IDLE;

    _xf_active = 0;
    _xf_pending = 0;
    _xf_offset = 0;
}

int I2SClass::available()
{
    uint32_t xf_offset, xf_index, xf_count;

    if (!((_state == I2S_STATE_READY) || (_state == I2S_STATE_RECEIVE))) {
	return 0;
    }

    _state = I2S_STATE_RECEIVE;

    if (!_xf_active)
    {
	xf_offset = _xf_offset;
	xf_index  = xf_offset >> 31;
	xf_count  = xf_offset & 0x7fffffff;

	if (xf_count == 0)
	{
	    if (stm32l4_sai_done(_sai))
	    {
		if (_xf_pending)
		{
		    _xf_pending = false;
		    xf_count = I2S_BUFFER_SIZE;
		}
		
		_xf_active = true;
		_xf_offset = ((xf_index ^ 1) << 31) | xf_count;
		
		stm32l4_sai_receive(_sai, (uint8_t*)&_xf_data[xf_index][0], I2S_BUFFER_SIZE);
	    }
	}
    }

    return (_xf_offset & 0x7fffffff);
}

int I2SClass::peek()
{
    uint32_t xf_offset, xf_index, xf_count;

    if (!((_state == I2S_STATE_READY) || (_state == I2S_STATE_RECEIVE))) {
	return 0;
    }

    _state = I2S_STATE_RECEIVE;

    if (!_xf_active)
    {
	xf_offset = _xf_offset;
	xf_index  = xf_offset >> 31;
	xf_count  = xf_offset & 0x7fffffff;

	if (xf_count == 0)
	{
	    if (stm32l4_sai_done(_sai))
	    {
		if (_xf_pending)
		{
		    _xf_pending = false;
		    xf_count = I2S_BUFFER_SIZE;
		}
		
		_xf_active = true;
		_xf_offset = ((xf_index ^ 1) << 31) | xf_count;
		
		stm32l4_sai_receive(_sai, (uint8_t*)&_xf_data[xf_index][0], I2S_BUFFER_SIZE);
	    }
	}
    }

    xf_offset = _xf_offset;
    xf_index  = xf_offset >> 31;
    xf_count  = xf_offset & 0x7fffffff;

    if (_width == 32)
    {
	if (xf_count >= 4) {
	    return *((int32_t*)&(((uint8_t*)&_xf_data[xf_index][0])[xf_count - 4]));
	}
    }

    else if (_width == 16)
    {
	if (xf_count >= 2) {
	    return *((int16_t*)&(((uint8_t*)&_xf_data[xf_index][0])[xf_count - 2]));
	}
    }
    else
    {
	if (xf_count >= 1) {
	    return *((uint8_t*)&(((uint8_t*)&_xf_data[xf_index][0])[xf_count - 1]));
	}
    }

    return 0;
}

int I2SClass::read()
{
    if (_width == 32)      { int32_t temp; if (read(&temp, 4)) { return temp; } }
    else if (_width == 16) { int16_t temp; if (read(&temp, 2)) { return temp; } }
    else                   { uint8_t temp; if (read(&temp, 1)) { return temp; } }

    return 0;
}

int I2SClass::read(void* buffer, size_t size)
{
    uint32_t xf_offset, xf_index, xf_count;

    if (_state != I2S_STATE_RECEIVE) {
	if (!((_state == I2S_STATE_READY) || (_state == I2S_STATE_RECEIVE))) {
	    return 0;
	}
	
	_state = I2S_STATE_RECEIVE;
    }

    if (!_xf_active)
    {
	xf_offset = _xf_offset;
	xf_index  = xf_offset >> 31;
	xf_count  = xf_offset & 0x7fffffff;

	if (xf_count == 0)
	{
	    if (stm32l4_sai_done(_sai))
	    {
		if (_xf_pending)
		{
		    _xf_pending = false;
		    xf_count = I2S_BUFFER_SIZE;
		}
		
		_xf_active = true;
		_xf_offset = ((xf_index ^ 1) << 31) | xf_count;
		
		stm32l4_sai_receive(_sai, (uint8_t*)&_xf_data[xf_index][0], I2S_BUFFER_SIZE);
	    }
	}
    }

    xf_offset = _xf_offset;
    xf_index  = xf_offset >> 31;
    xf_count  = xf_offset & 0x7fffffff;

    if (size > xf_count) {
	size = xf_count;
    }

    if (size)
    {
	xf_count -= size;

	memcpy(buffer, &(((uint8_t*)&_xf_data[xf_index][0])[xf_count]), size);
    }

    _xf_offset = (xf_index << 31) | xf_count;

    if (!_xf_active)
    {
	xf_offset = _xf_offset;
	xf_index  = xf_offset >> 31;
	xf_count  = xf_offset & 0x7fffffff;

	if (xf_count == 0)
	{
	    if (stm32l4_sai_done(_sai))
	    {
		if (_xf_pending)
		{
		    _xf_pending = false;
		    xf_count = I2S_BUFFER_SIZE;
		}
		
		_xf_active = true;
		_xf_offset = ((xf_index ^ 1) << 31) | xf_count;
		
		stm32l4_sai_receive(_sai, (uint8_t*)&_xf_data[xf_index][0], I2S_BUFFER_SIZE);
	    }
	}
    }

    return size;
}

void I2SClass::flush()
{
}

size_t I2SClass::availableForWrite()
{
    if (!((_state == I2S_STATE_READY) || (_state == I2S_STATE_TRANSMIT))) {
	return 0;
    }

    _state = I2S_STATE_TRANSMIT;

    return I2S_BUFFER_SIZE - (_xf_offset & 0x7fffffff);
}

size_t I2SClass::write(uint8_t data)
{
    return write((int32_t)data);
}

size_t I2SClass::write(const uint8_t *buffer, size_t size)
{
    return write((const void*)buffer, size);
}

size_t I2SClass::write(int sample)
{
    return write((int32_t)sample);
}

size_t I2SClass::write(int32_t sample)
{
    if (_state != I2S_STATE_TRANSMIT) {
	if (!((_state == I2S_STATE_READY) || (_state == I2S_STATE_TRANSMIT))) {
	    return 0;
	}
	
	_state = I2S_STATE_TRANSMIT;
    }

    while (!write((const void*)&sample, (_width / 8))) {
	armv7m_core_yield();
    }

    return 1;
}

size_t I2SClass::write(const void *buffer, size_t size)
{
    uint32_t xf_offset, xf_index, xf_count;

    if (_state != I2S_STATE_TRANSMIT) {
	if (!((_state == I2S_STATE_READY) || (_state == I2S_STATE_TRANSMIT))) {
	    return 0;
	}
	
	_state = I2S_STATE_TRANSMIT;
    }

    if      (_width == 32) { size &= ~3; }
    else if (_width == 16) { size &= ~1; }
	
    xf_offset = _xf_offset;
    xf_index  = xf_offset >> 31;
    xf_count  = xf_offset & 0x7fffffff;
    
    if ((xf_count + size) > I2S_BUFFER_SIZE) {
	size = I2S_BUFFER_SIZE - xf_count;
    }

    if (size)
    {
        memcpy(&(((uint8_t*)&_xf_data[xf_index][0])[xf_count]), buffer, size);

	xf_count += size;
    }

    _xf_offset = (xf_index << 31) | xf_count;

    if (!_xf_active)
    {
	xf_offset = _xf_offset;
	xf_index  = xf_offset >> 31;
	xf_count  = xf_offset & 0x7fffffff;

	if (xf_count == I2S_BUFFER_SIZE)
	{
	    if (stm32l4_sai_done(_sai))
	    {
		_xf_active = true;
		_xf_offset = (xf_index ^ 1) << 31;
		
		stm32l4_sai_transmit(_sai, (const uint8_t*)&_xf_data[xf_index][0], I2S_BUFFER_SIZE);
	    }
	}
    }

    return size;
}

void I2SClass::onReceive(void(*callback)(void))
{
    _receiveCallback = callback;
}

void I2SClass::onTransmit(void(*callback)(void))
{
    _transmitCallback = callback;
}

void I2SClass::EventCallback(uint32_t events)
{
    uint32_t xf_offset, xf_index, xf_count;

    xf_offset = _xf_offset;
    xf_index  = xf_offset >> 31;
    xf_count  = xf_offset & 0x7fffffff;

    if (events & SAI_EVENT_RECEIVE_REQUEST)
    {
	if (xf_count == 0)
	{
	    _xf_offset = ((xf_index ^ 1) << 31) | I2S_BUFFER_SIZE;

	    stm32l4_sai_receive(_sai, (uint8_t*)&_xf_data[xf_index][0], I2S_BUFFER_SIZE);
	}
	else
	{
	    _xf_pending = true;
	    _xf_active = false;
	}

	if (_receiveCallback)
	{
	    armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)_receiveCallback, NULL, 0);
	}
    }

    if (events & SAI_EVENT_TRANSMIT_REQUEST)
    {
	if (xf_count == I2S_BUFFER_SIZE)
	{
	    _xf_offset = (xf_index ^ 1) << 31;

	    stm32l4_sai_transmit(_sai, (const uint8_t*)&_xf_data[xf_index][0], I2S_BUFFER_SIZE);
	}
	else
	{
	    _xf_active = false;
	}

	if (_transmitCallback)
	{
	    armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)_transmitCallback, NULL, 0);
	}
    }
}

void I2SClass::_eventCallback(void *context, uint32_t events)
{
    reinterpret_cast<class I2SClass*>(context)->EventCallback(events);
}

#if I2S_INTERFACES_COUNT > 0

extern const stm32l4_sai_pins_t g_SAIPins;
extern const unsigned int g_SAIInstance;
extern const unsigned int g_SAIMode;

static stm32l4_sai_t _SAI;

I2SClass I2S(&_SAI, g_SAIInstance, &g_SAIPins, STM32L4_SAI_IRQ_PRIORITY, g_SAIMode);

#endif

#if I2S_INTERFACES_COUNT > 1

extern const stm32l4_sai_pins_t g_SAI1Pins;
extern const unsigned int g_SAI1Instance;
extern const unsigned int g_SAI1Mode;

static stm32l4_sai_t _SAI1;

I2SClass I2S1(&_SAI1, g_SAI1Instance, &g_SAI1Pins, STM32L4_SAI_IRQ_PRIORITY, g_SAI1Mode);

#endif
