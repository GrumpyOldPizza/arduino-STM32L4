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
#include "Wire.h"

TwoWire::TwoWire(struct _stm32l4_i2c_t *i2c, unsigned int instance, const struct _stm32l4_i2c_pins_t *pins, unsigned int priority, unsigned int mode)
{
    _i2c = i2c;

    _clock = TWI_CLOCK;
    _option = 0;

    stm32l4_i2c_create(i2c, instance, pins, priority, mode);

    _rx_read = 0;
    _rx_write = 0;

    _tx_write = 0;
    _tx_active = false;

    _completionCallback = NULL;
    _requestCallback = NULL;
    _receiveCallback = NULL;
}

void TwoWire::begin()
{
    _option = I2C_OPTION_RESET;

    stm32l4_i2c_enable(_i2c, _clock, _option, TwoWire::_eventCallback, (void*)this, (I2C_EVENT_ADDRESS_NACK | I2C_EVENT_DATA_NACK | I2C_EVENT_ARBITRATION_LOST | I2C_EVENT_BUS_ERROR | I2C_EVENT_OVERRUN | I2C_EVENT_RECEIVE_DONE | I2C_EVENT_TRANSMIT_DONE | I2C_EVENT_TRANSFER_DONE));
}

void TwoWire::begin(uint8_t address) 
{
    _option = I2C_OPTION_RESET | (address << I2C_OPTION_ADDRESS_SHIFT);

    stm32l4_i2c_enable(_i2c, _clock, _option, TwoWire::_eventCallback, (void*)this, (I2C_EVENT_RECEIVE_REQUEST | I2C_EVENT_RECEIVE_DONE | I2C_EVENT_TRANSMIT_REQUEST));
}

void TwoWire::setClock(uint32_t clock) 
{
    _clock = clock;
  
    stm32l4_i2c_configure(_i2c, _clock, _option);
}

void TwoWire::end() 
{
    stm32l4_i2c_disable(_i2c);
}

uint8_t TwoWire::requestFrom(uint8_t address, size_t quantity, bool stopBit)
{
    if (__get_IPSR() != 0) {
	return 0;
    }

    if (quantity == 0) {
	return 0;
    }

    if (quantity > BUFFER_LENGTH) {
	quantity = BUFFER_LENGTH;
    }

    while (!stm32l4_i2c_done(_i2c)) {
	armv7m_core_yield();
    }

    if (!stm32l4_i2c_receive(_i2c, address, &_rx_data[0], quantity, (stopBit ? 0 : I2C_CONTROL_RESTART))) {
	return 0;
    }    

    while (!stm32l4_i2c_done(_i2c)) {
	armv7m_core_yield();
    }

    if (stm32l4_i2c_status(_i2c)) {
	return 0;
    }

    _rx_read = 0;
    _rx_write = quantity;

    return quantity;
}

void TwoWire::beginTransmission(uint8_t address)
{
    if (__get_IPSR() != 0) {
	return;
    }

    _tx_write = 0;
    _tx_address = address;
    _tx_active = true;
}

// Errors:
//  0 : Success
//  1 : Data too long
//  2 : NACK on transmit of address
//  3 : NACK on transmit of data
//  4 : Other error
uint8_t TwoWire::endTransmission(bool stopBit)
{
    unsigned int status;

    if (!_tx_active) {
	return 4;
    }

    while (!stm32l4_i2c_done(_i2c)) {
	armv7m_core_yield();
    }

    if (!stm32l4_i2c_transmit(_i2c, _tx_address, &_tx_data[0], _tx_write, (stopBit ? 0 : I2C_CONTROL_RESTART))) {
	_tx_active = false;

	return 4;
    }

    while (!stm32l4_i2c_done(_i2c)) {
	armv7m_core_yield();
    }

    status = stm32l4_i2c_status(_i2c);

    _tx_active = false;

    if (status == 0) {
	return 0;
    }

    if (status & I2C_STATUS_ADDRESS_NACK) {
	return 2;
    }

    else if (status & I2C_STATUS_DATA_NACK) {
	return 3;
    }

    else {
	return 4;
    }
}

size_t TwoWire::write(uint8_t data)
{
    if (!_tx_active) {
	return 0;
    }

    if (_tx_write >= BUFFER_LENGTH) {
	return 0;
    }

    _tx_data[_tx_write++] = data;

    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
    size_t count;

    if (!_tx_active) {
	return 0;
    }

    for (count = 0; count < quantity; count++) {
	if (_tx_write >= BUFFER_LENGTH) {
	    return count;
	}

	_tx_data[_tx_write++] = *data++;
    }

    return quantity;
}

int TwoWire::available(void)
{
    return (_rx_write - _rx_read);
}

int TwoWire::read(void)
{
    if (_rx_read >= _rx_write) {
	return -1;
    }

    return _rx_data[_rx_read++];
}

int TwoWire::peek(void)
{
    if (_rx_read >= _rx_write) {
	return -1;
    }

    return _rx_data[_rx_read];
}

void TwoWire::flush(void)
{
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
	while (!stm32l4_i2c_done(_i2c)) {
	    stm32l4_i2c_poll(_i2c);
	}
    } else {
	while (!stm32l4_i2c_done(_i2c)) {
	    armv7m_core_yield();
	}
    }
}
uint8_t TwoWire::transfer(uint8_t address, const uint8_t *txBuffer, size_t txSize, uint8_t *rxBuffer, size_t rxSize, bool stopBit)
{
    unsigned int status;

    if (__get_IPSR() != 0) {
	return 4;
    }

    if (_tx_active)  {
	return 4;
    }

    if ((txSize > 1024) || (rxSize > 1024))  {
	return 4;
    }

    while (!stm32l4_i2c_done(_i2c)) {
	armv7m_core_yield();
    }

    if (rxSize) {
	if (txSize) {
	    if (!stm32l4_i2c_transfer(_i2c, address, txBuffer, txSize, rxBuffer, rxSize, (stopBit ? 0 : I2C_CONTROL_RESTART)))
		return 4;
	} else {
	    if (!stm32l4_i2c_receive(_i2c, address, rxBuffer, rxSize, (stopBit ? 0 : I2C_CONTROL_RESTART)))
		return 4;
	}
    } else {
	if (!stm32l4_i2c_transmit(_i2c, address, txBuffer, txSize, (stopBit ? 0 : I2C_CONTROL_RESTART)))
	    return 4;
    }

    while (!stm32l4_i2c_done(_i2c)) {
	armv7m_core_yield();
    }

    status = stm32l4_i2c_status(_i2c);

    if (status == 0) {
	return 0;
    }

    if (status & I2C_STATUS_ADDRESS_NACK) {
	return 2;
    }

    else if (status & I2C_STATUS_DATA_NACK) {
	return 3;
    }

    else {
	return 4;
    }
}

bool TwoWire::transfer(uint8_t address, const uint8_t *txBuffer, size_t txSize, uint8_t *rxBuffer, size_t rxSize, bool stopBit, void(*callback)(uint8_t))
{
    if (_tx_active) {
	return false;
    }

    if ((txSize > 1024) || (rxSize > 1024)) {
	return false;
    }

    if (!stm32l4_i2c_done(_i2c)) {
	return false;
    }

    _completionCallback = callback;
  
    if (rxSize) {
	if (txSize) {
	    if (!stm32l4_i2c_transfer(_i2c, address, txBuffer, txSize, rxBuffer, rxSize, (stopBit ? 0 : I2C_CONTROL_RESTART))) {
		_completionCallback = NULL;
		return false;
	    }
	} else {
	    if (!stm32l4_i2c_receive(_i2c, address, rxBuffer, rxSize, (stopBit ? 0 : I2C_CONTROL_RESTART))) {
		_completionCallback = NULL;
		return false;
	    }
	}
    } else {
	if (!stm32l4_i2c_transmit(_i2c, address, txBuffer, txSize, (stopBit ? 0 : I2C_CONTROL_RESTART))) {
	    _completionCallback = NULL;
	    return false;
	}
    }

    return true;
}

bool TwoWire::done(void)
{
    return stm32l4_i2c_done(_i2c);
}

uint8_t TwoWire::status(void)
{
    unsigned int status = stm32l4_i2c_status(_i2c);

    if (status == 0) {
	return 0;
    }

    if (status & I2C_STATUS_ADDRESS_NACK) {
	return 2;
    }

    else if (status & I2C_STATUS_DATA_NACK) {
	return 3;
    }

    else {
	return 4;
    }
}

bool TwoWire::isEnabled(void)
{
    return (_i2c->state >= I2C_STATE_READY);
}

void TwoWire::reset(void)
{
    stm32l4_i2c_reset(_i2c);
}

void TwoWire::onReceive(void(*callback)(int))
{
    _receiveCallback = callback;
}

void TwoWire::onRequest(void(*callback)(void))
{
    _requestCallback = callback;
}

void TwoWire::EventCallback(uint32_t events)
{
    unsigned int status;
    void(*callback)(uint8_t);

    if (_option & I2C_OPTION_ADDRESS_MASK) {
	if (events & I2C_EVENT_RECEIVE_DONE) {
	    _rx_read = 0;
	    _rx_write = stm32l4_i2c_count(_i2c);
	  
	    if(_receiveCallback) {
		(*_receiveCallback)(_rx_write);
	    }
      
	    stm32l4_i2c_service(_i2c, &_rx_data[0], BUFFER_LENGTH);
	}
    
	if (events & I2C_EVENT_RECEIVE_REQUEST) {
	    stm32l4_i2c_service(_i2c, &_rx_data[0], BUFFER_LENGTH);
	}
    
	if (events & I2C_EVENT_TRANSMIT_REQUEST) {
	    _tx_write = 0;
	    _tx_active = true;

	    if(_requestCallback) {
		(*_requestCallback)();
	    }

	    _tx_active = false;
      
	    stm32l4_i2c_service(_i2c, &_tx_data[0], _tx_write);
	}
    } else {
	if (events & (I2C_EVENT_ADDRESS_NACK | I2C_EVENT_DATA_NACK | I2C_EVENT_ARBITRATION_LOST | I2C_EVENT_BUS_ERROR | I2C_EVENT_OVERRUN | I2C_EVENT_RECEIVE_DONE | I2C_EVENT_TRANSMIT_DONE | I2C_EVENT_TRANSFER_DONE)) {
	    callback = _completionCallback;
	    _completionCallback = NULL;

	    if (callback) {
		if (!(events & (I2C_EVENT_ADDRESS_NACK | I2C_EVENT_DATA_NACK | I2C_EVENT_ARBITRATION_LOST | I2C_EVENT_BUS_ERROR | I2C_EVENT_OVERRUN))) {
		    status = 0;
		} else {
		    if (events & I2C_EVENT_ADDRESS_NACK) {
			status = 2;
		    } else if (events & I2C_EVENT_DATA_NACK) {
			status = 3;
		    } else {
			status = 4;
		    }
		} 
	
		(*callback)(status);
	    }
	}
    }
}

void TwoWire::_eventCallback(void *context, uint32_t events)
{
    reinterpret_cast<class TwoWire*>(context)->EventCallback(events);
}

void TwoWireEx::begin(TwoWireExPins pins) 
{
    _option = I2C_OPTION_RESET;

    if (pins != TWI_PINS_20_21) {
	_option |= I2C_OPTION_ALTERNATE;
    }

    stm32l4_i2c_enable(_i2c, _clock, _option, TwoWire::_eventCallback, (void*)this, (I2C_EVENT_ADDRESS_NACK | I2C_EVENT_DATA_NACK | I2C_EVENT_ARBITRATION_LOST | I2C_EVENT_BUS_ERROR | I2C_EVENT_OVERRUN | I2C_EVENT_RECEIVE_DONE | I2C_EVENT_TRANSMIT_DONE | I2C_EVENT_TRANSFER_DONE));
}

void TwoWireEx::begin(uint8_t address, TwoWireExPins pins) 
{
    _option = I2C_OPTION_RESET | (address << I2C_OPTION_ADDRESS_SHIFT);

    if (pins != TWI_PINS_20_21) {
	_option |= I2C_OPTION_ALTERNATE;
    }

    stm32l4_i2c_enable(_i2c, _clock, _option, TwoWire::_eventCallback, (void*)this, (I2C_EVENT_RECEIVE_REQUEST | I2C_EVENT_RECEIVE_DONE | I2C_EVENT_TRANSMIT_REQUEST));
}

#if WIRE_INTERFACES_COUNT > 0

extern const stm32l4_i2c_pins_t g_WirePins;
extern const unsigned int g_WireInstance;
extern const unsigned int g_WireMode;

static stm32l4_i2c_t _Wire;

TwoWireEx Wire(&_Wire, g_WireInstance, &g_WirePins, STM32L4_I2C_IRQ_PRIORITY, g_WireMode);

#endif

#if WIRE_INTERFACES_COUNT > 1

extern const stm32l4_i2c_pins_t g_Wire1Pins;
extern const unsigned int g_Wire1Instance;
extern const unsigned int g_Wire1Mode;

static stm32l4_i2c_t _Wire1;

TwoWire Wire1(&_Wire1, g_Wire1Instance, &g_Wire1Pins, STM32L4_I2C_IRQ_PRIORITY, g_Wire1Mode);

#endif

#if WIRE_INTERFACES_COUNT > 2

extern const stm32l4_i2c_pins_t g_Wire2Pins;
extern const unsigned int g_Wire2Instance;
extern const unsigned int g_Wire2Mode;

static stm32l4_i2c_t _Wire2;

TwoWire Wire2(&_Wire2, g_Wire2Instance, &g_Wire2Pins, STM32L4_I2C_IRQ_PRIORITY, g_Wire2Mode);

#endif
