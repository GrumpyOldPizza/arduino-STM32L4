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

extern "C" {
#include <string.h>
}

#include <Arduino.h>
#include <wiring_private.h>

#include "Wire.h"

TwoWire::TwoWire(struct _stm32l4_i2c_t *i2c, unsigned int instance, const struct _stm32l4_i2c_pins_t *pins, unsigned int priority, unsigned int mode)
{
  _i2c = i2c;

  stm32l4_i2c_create(i2c, instance, pins, priority, mode);

  _rx_read = 0;
  _rx_write = 0;

  _tx_write = 0;
  _tx_active = false;

  _completionCallback = NULL;
  _requestCallback = NULL;
  _receiveCallback = NULL;
}

void TwoWire::begin(uint8_t address, uint32_t clock, bool alternate) {
  _option = I2C_OPTION_RESET;

  if (alternate) {
    _option |= I2C_OPTION_ALTERNATE;
  }

  if (address) {
    _option |= (address << I2C_OPTION_ADDRESS_SHIFT);

    stm32l4_i2c_enable(_i2c, clock, _option, TwoWire::_eventCallback, (void*)this, (I2C_EVENT_RECEIVE_REQUEST | I2C_EVENT_RECEIVE_DONE | I2C_EVENT_TRANSMIT_REQUEST));
  } else {
    stm32l4_i2c_enable(_i2c, clock, _option, TwoWire::_eventCallback, (void*)this, (I2C_EVENT_ADDRESS_NACK | I2C_EVENT_DATA_NACK | I2C_EVENT_ARBITRATION_LOST | I2C_EVENT_BUS_ERROR | I2C_EVENT_OVERRUN | I2C_EVENT_RECEIVE_DONE | I2C_EVENT_TRANSMIT_DONE | I2C_EVENT_TRANSFER_DONE));
  }
}

void TwoWire::setClock(uint32_t clock) {
  stm32l4_i2c_configure(_i2c, clock, _option);
}

void TwoWire::end() {
  stm32l4_i2c_disable(_i2c);
}

uint8_t TwoWire::requestFrom(uint8_t address, size_t quantity, bool stopBit)
{
  if (quantity == 0)
    return 0;
  
  if (quantity > TWI_RX_BUFFER_SIZE)
    quantity = TWI_RX_BUFFER_SIZE;

  if (!stm32l4_i2c_done(_i2c)) {
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
      do {
	stm32l4_i2c_poll(_i2c);
      } while (!stm32l4_i2c_done(_i2c));
    } else {
      do {
	armv7m_core_yield();
      } while (!stm32l4_i2c_done(_i2c));
    }
  }

  if (!stm32l4_i2c_receive(_i2c, address, &_rx_data[0], quantity, (stopBit ? 0 : I2C_CONTROL_RESTART))) {
    return 0;
  }    

  if (!stm32l4_i2c_done(_i2c)) {
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
      do {
	stm32l4_i2c_poll(_i2c);
      } while (!stm32l4_i2c_done(_i2c));
    } else {
      do {
	armv7m_core_yield();
      } while (!stm32l4_i2c_done(_i2c));
    }
  }

  if (stm32l4_i2c_status(_i2c)) {
    return 0;
  }

  _rx_read = 0;
  _rx_write = quantity;

  return quantity;
}

uint8_t TwoWire::requestFrom(uint8_t address, size_t quantity)
{
  return requestFrom(address, quantity, true);
}

void TwoWire::beginTransmission(uint8_t address) {
  if (!stm32l4_i2c_done(_i2c)) {
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
      do {
	stm32l4_i2c_poll(_i2c);
      } while (!stm32l4_i2c_done(_i2c));
    } else {
      do {
	armv7m_core_yield();
      } while (!stm32l4_i2c_done(_i2c));
    }
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

  _tx_active = false;

  if (!stm32l4_i2c_transmit(_i2c, _tx_address, &_tx_data[0], _tx_write, (stopBit ? 0 : I2C_CONTROL_RESTART))) {
    return 4;
  }

  if (!stm32l4_i2c_done(_i2c)) {
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
      do {
	stm32l4_i2c_poll(_i2c);
      } while (!stm32l4_i2c_done(_i2c));
    } else {
      do {
	armv7m_core_yield();
      } while (!stm32l4_i2c_done(_i2c));
    }
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

uint8_t TwoWire::endTransmission()
{
  return endTransmission(true);
}

size_t TwoWire::write(uint8_t data)
{
  if (!_tx_active)
    return 0;

  if (_tx_write >= TWI_TX_BUFFER_SIZE)
    return 0;

  _tx_data[_tx_write++] = data;

  return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
  size_t count;

  if (!_tx_active)
    return 0;

  for (count = 0; count < quantity; count++) {
    if (_tx_write >= TWI_TX_BUFFER_SIZE)
      return count;

    _tx_data[_tx_write++] = *data++;
  }

  return quantity;
}

int TwoWire::available(void)
{
  return _rx_write - _rx_read;
}

int TwoWire::read(void)
{
  if (_rx_read >= _rx_write)
    return -1;

  return _rx_data[_rx_read++];
}

int TwoWire::peek(void)
{
  if (_rx_read >= _rx_write)
    return -1;

  return _rx_data[_rx_read];
}

void TwoWire::flush(void)
{
  if (!stm32l4_i2c_done(_i2c)) {
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
      do {
	stm32l4_i2c_poll(_i2c);
      } while (!stm32l4_i2c_done(_i2c));
    } else {
      do {
	armv7m_core_yield();
      } while (!stm32l4_i2c_done(_i2c));
    }
  }
}

uint8_t TwoWire::transfer(uint8_t address, const uint8_t *txBuffer, size_t txSize, uint8_t *rxBuffer, size_t rxSize, bool stopBit)
{
  unsigned int status;

  if (_tx_active) 
    return 4;

  if ((txSize > 1024) || (rxSize > 1024)) 
    return 4;

  if (!stm32l4_i2c_done(_i2c)) {
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
      do {
	stm32l4_i2c_poll(_i2c);
      } while (!stm32l4_i2c_done(_i2c));
    } else {
      do {
	armv7m_core_yield();
      } while (!stm32l4_i2c_done(_i2c));
    }
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

  if (!stm32l4_i2c_done(_i2c)) {
    if (armv7m_core_priority() <= STM32L4_I2C_IRQ_PRIORITY) {
      do {
	stm32l4_i2c_poll(_i2c);
      } while (!stm32l4_i2c_done(_i2c));
    } else {
      do {
	armv7m_core_yield();
      } while (!stm32l4_i2c_done(_i2c));
    }
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
  if (_tx_active) 
    return false;

  if ((txSize > 1024) || (rxSize > 1024)) 
    return 4;

  if (!stm32l4_i2c_done(_i2c))
    return false;

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
      
      stm32l4_i2c_service(_i2c, &_rx_data[0], TWI_RX_BUFFER_SIZE);
    }
    
    if (events & I2C_EVENT_RECEIVE_REQUEST) {
      stm32l4_i2c_service(_i2c, &_rx_data[0], TWI_RX_BUFFER_SIZE);
    }
    
    if (events & I2C_EVENT_TRANSMIT_REQUEST) {
      _tx_write = 0;

      if(_requestCallback) {
	(*_requestCallback)();
      }
      
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

#if WIRE_INTERFACES_COUNT > 0

static stm32l4_i2c_t stm32l4_i2c1;

static const stm32l4_i2c_pins_t stm32l4_i2c1_pins = { GPIO_PIN_PB8_I2C1_SCL, GPIO_PIN_PB9_I2C1_SDA };

TwoWire __attribute__((weak)) Wire(&stm32l4_i2c1, I2C_INSTANCE_I2C1, &stm32l4_i2c1_pins, STM32L4_I2C_IRQ_PRIORITY, I2C_MODE_RX_DMA);

#endif

#if WIRE_INTERFACES_COUNT > 1

static stm32l4_i2c_t stm32l4_i2c3;

static const stm32l4_i2c_pins_t stm32l4_i2c3_pins = { GPIO_PIN_PC0_I2C3_SCL, GPIO_PIN_PC1_I2C3_SDA };

TwoWire __attribute__((weak)) Wire1(&stm32l4_i2c3, I2C_INSTANCE_I2C3, &stm32l4_i2c3_pins, STM32L4_I2C_IRQ_PRIORITY, 0);

#endif

#if WIRE_INTERFACES_COUNT > 2

static stm32l4_i2c_t stm32l4_i2c2;

static const stm32l4_i2c_pins_t stm32l4_i2c2_pins = { GPIO_PIN_PB13_I2C2_SCL, GPIO_PIN_PB14_I2C2_SDA };

TwoWire __attribute__((weak)) Wire2(&stm32l4_i2c2, I2C_INSTANCE_I2C2, &stm32l4_i2c2_pins, STM32L4_I2C_IRQ_PRIORITY, 0);

#endif
