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

#include "Arduino.h"
#include "stm32l4_wiring_private.h"
#include "Uart.h"

#define UART_TX_PACKET_SIZE 16

Uart::Uart(struct _stm32l4_uart_t *uart, unsigned int instance, const struct _stm32l4_uart_pins_t *pins, unsigned int priority, unsigned int mode, bool serialEvent)
{
    _uart = uart;

    _blocking = true;

    _tx_read = 0;
    _tx_write = 0;
    _tx_count = 0;
    _tx_size = 0;

    _tx_data2 = NULL;
    _tx_size2 = 0;
  
    _completionCallback = NULL;
    _receiveCallback = NULL;

    stm32l4_uart_create(uart, instance, pins, priority, mode);

    if (serialEvent) {
	serialEventCallback = serialEventDispatch;
    }
}

void Uart::begin(unsigned long baudrate)
{
    begin(baudrate, SERIAL_8N1, &_rx_data[0], sizeof(_rx_data));
}

void Uart::begin(unsigned long baudrate, uint32_t config)
{
    begin(baudrate, config, &_rx_data[0], sizeof(_rx_data));
}

void Uart::begin(unsigned long baudrate, uint8_t *buffer, size_t size)
{
    begin(baudrate, SERIAL_8N1, buffer, size);
}

void Uart::begin(unsigned long baudrate, uint32_t config, uint8_t *buffer, size_t size)
{
    if (_uart->state != UART_STATE_INIT) {
	flush();
	stm32l4_uart_disable(_uart);
    }

    stm32l4_uart_enable(_uart, buffer, size, baudrate, config, Uart::_event_callback, (void*)this, (UART_EVENT_RECEIVE | UART_EVENT_TRANSMIT));
}

void Uart::end()
{
    flush();

    stm32l4_uart_disable(_uart);
}

int Uart::available()
{
    return stm32l4_uart_count(_uart);
}

int Uart::availableForWrite()
{
    if (_uart->state < UART_STATE_READY) {
	return 0;
    }

    if (_tx_size2 != 0) {
	return 0;
    }

    return UART_TX_BUFFER_SIZE - _tx_count;
}

int Uart::peek()
{
    return stm32l4_uart_peek(_uart);
}

int Uart::read()
{
    uint8_t data;

    if (!stm32l4_uart_count(_uart)) {
	return -1;
    }

    stm32l4_uart_receive(_uart, &data, 1);

    return data;
}

size_t Uart::read(uint8_t *buffer, size_t size)
{
    return stm32l4_uart_receive(_uart, buffer, size);
}

void Uart::flush()
{
    if (armv7m_core_priority() <= STM32L4_UART_IRQ_PRIORITY) {
	while ((_tx_count != 0) || (_tx_size2 != 0) || !stm32l4_uart_done(_uart)) {
	    stm32l4_uart_poll(_uart);
	}
    } else {
	while ((_tx_count != 0) || (_tx_size2 != 0) || !stm32l4_uart_done(_uart)) {
	    armv7m_core_yield();
	}
    }
}

size_t Uart::write(const uint8_t data)
{
    return write(&data, 1);
}

size_t Uart::write(const uint8_t *buffer, size_t size)
{
    unsigned int tx_read, tx_write, tx_count, tx_size;
    size_t count;

    if (_uart->state < UART_STATE_READY) {
	return 0;
    }

    if (size == 0) {
	return 0;
    }

    if (_tx_size2 != 0) {
	if (!_blocking || (__get_IPSR() != 0)) {
	    return 0;
	}
	
	while (_tx_size2 != 0) {
	    armv7m_core_yield();
	}
    }
      
    count = 0;

    while (count < size) {

	tx_count = UART_TX_BUFFER_SIZE - _tx_count;

	if (tx_count == 0) {

	    if (!_blocking || (__get_IPSR() != 0)) {
		break;
	    }

	    if (stm32l4_uart_done(_uart)) {
		tx_size = _tx_count;
		tx_read = _tx_read;

		if (tx_size > (UART_TX_BUFFER_SIZE - tx_read)) {
		    tx_size = (UART_TX_BUFFER_SIZE - tx_read);
		}
		
		if (tx_size > UART_TX_PACKET_SIZE) {
		    tx_size = UART_TX_PACKET_SIZE;
		}
		
		_tx_size = tx_size;
		
		stm32l4_uart_transmit(_uart, &_tx_data[tx_read], tx_size);
	    }

	    while (UART_TX_BUFFER_SIZE == _tx_count) {
		armv7m_core_yield();
	    }

	    tx_count = UART_TX_BUFFER_SIZE - _tx_count;
	}

	tx_write = _tx_write;

	if (tx_count > (UART_TX_BUFFER_SIZE - tx_write)) {
	    tx_count = (UART_TX_BUFFER_SIZE - tx_write);
	}

	if (tx_count > (size - count)) {
	    tx_count = (size - count);
	}

	memcpy(&_tx_data[tx_write], &buffer[count], tx_count);
	count += tx_count;
      
	_tx_write = (unsigned int)(tx_write + tx_count) & (UART_TX_BUFFER_SIZE -1);

	armv7m_atomic_add(&_tx_count, tx_count);
    }

    if (stm32l4_uart_done(_uart)) {
	tx_size = _tx_count;
	tx_read = _tx_read;
	
	if (tx_size) {
	    if (tx_size > (UART_TX_BUFFER_SIZE - tx_read)) {
		tx_size = (UART_TX_BUFFER_SIZE - tx_read);
	    }
	    
	    if (tx_size > UART_TX_PACKET_SIZE) {
		tx_size = UART_TX_PACKET_SIZE;
	    }
	    
	    _tx_size = tx_size;
	    
	    stm32l4_uart_transmit(_uart, &_tx_data[tx_read], tx_size);
	}
    }

    return count;
}

bool Uart::write(const uint8_t *buffer, size_t size, void(*callback)(void))
{
    if (_uart->state < UART_STATE_READY) {
	return false;
    }

    if (size == 0) {
	return false;
    }

    if (_tx_size2 != 0) {
	return false;
    }

    _completionCallback = callback;
    _tx_data2 = buffer;
    _tx_size2 = size;

    if (stm32l4_uart_done(_uart)) {
	stm32l4_uart_transmit(_uart, _tx_data2, _tx_size2);
    }

    return true;
}

bool Uart::done()
{
    if (_tx_count) {
	return false;
    }

    if (_tx_size2) {
	return false;
    }

    if (!stm32l4_uart_done(_uart)) {
	return false;
    }

    return true;
}

void Uart::onReceive(void(*callback)(void))
{
    _receiveCallback = callback;
}

void Uart::blockOnOverrun(bool block)
{
    _blocking = block;
}

void Uart::EventCallback(uint32_t events)
{
    unsigned int tx_read, tx_size;

    if (events & UART_EVENT_RECEIVE) {
	if (_receiveCallback) {
	    armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)_receiveCallback, NULL, 0);
	}
    }

    if (events & UART_EVENT_TRANSMIT) {

	tx_size = _tx_size;

	if (tx_size != 0) {
	    _tx_read = (_tx_read + tx_size) & (UART_TX_BUFFER_SIZE -1);
      
	    armv7m_atomic_sub(&_tx_count, tx_size);
      
	    _tx_size = 0;

	    if (_tx_count != 0) {
		tx_size = _tx_count;
		tx_read = _tx_read;

		if (tx_size > (UART_TX_BUFFER_SIZE - tx_read)) {
		    tx_size = (UART_TX_BUFFER_SIZE - tx_read);
		}
	  
		if (tx_size > UART_TX_PACKET_SIZE) {
		    tx_size = UART_TX_PACKET_SIZE;
		}
	  
		_tx_size = tx_size;
	  
		stm32l4_uart_transmit(_uart, &_tx_data[tx_read], tx_size);
	    } else {
		if (_tx_size2 != 0) {
		    stm32l4_uart_transmit(_uart, _tx_data2, _tx_size2);
		}
	    }
	} else {
	    _tx_size2 = 0;
	    _tx_data2 = NULL;

	    if (_completionCallback) {
		armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)_completionCallback, NULL, 0);

		_completionCallback = NULL;
	    }
	}
    }
}

void Uart::_event_callback(void *context, uint32_t events)
{
  reinterpret_cast<class Uart*>(context)->EventCallback(events);
}

#if !defined(USBCON)

bool Serial_empty() { return !Serial.available(); }

extern void serialEvent() __attribute__((weak));

extern const stm32l4_uart_pins_t g_SerialPins;
extern const unsigned int g_SerialInstance;
extern const unsigned int g_SerialMode;

static stm32l4_uart_t _Serial;

Uart Serial(&_Serial, g_SerialInstance, &g_SerialPins, STM32L4_UART_IRQ_PRIORITY, g_SerialMode, (serialEvent != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 1

bool Serial1_empty() { return !Serial1.available(); }

extern void serialEvent1() __attribute__((weak));

extern const stm32l4_uart_pins_t g_Serial1Pins;
extern const unsigned int g_Serial1Instance;
extern const unsigned int g_Serial1Mode;

static stm32l4_uart_t _Serial1;

Uart Serial1(&_Serial1, g_Serial1Instance, &g_Serial1Pins, STM32L4_UART_IRQ_PRIORITY, g_Serial1Mode, (serialEvent1 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 2

bool Serial2_empty() { return !Serial2.available(); }

extern void serialEvent2() __attribute__((weak));

extern const stm32l4_uart_pins_t g_Serial2Pins;
extern const unsigned int g_Serial2Instance;
extern const unsigned int g_Serial2Mode;

static stm32l4_uart_t _Serial2;

Uart Serial2(&_Serial2, g_Serial2Instance, &g_Serial2Pins, STM32L4_UART_IRQ_PRIORITY, g_Serial2Mode, (serialEvent2 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 3

bool Serial3_empty() { return !Serial3.available(); }

extern void serialEvent3() __attribute__((weak));

extern const stm32l4_uart_pins_t g_Serial3Pins;
extern const unsigned int g_Serial3Instance;
extern const unsigned int g_Serial3Mode;

static stm32l4_uart_t _Serial3;

Uart Serial3(&_Serial3, g_Serial3Instance, &g_Serial3Pins, STM32L4_UART_IRQ_PRIORITY, g_Serial3Mode, (serialEvent3 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 4

bool Serial4_empty() { return !Serial4.available(); }

extern void serialEvent4() __attribute__((weak));

extern const stm32l4_uart_pins_t g_Serial4Pins;
extern const unsigned int g_Serial4Instance;
extern const unsigned int g_Serial4Mode;

static stm32l4_uart_t _Serial4;

Uart Serial4(&_Serial4, g_Serial4Instance, &g_Serial4Pins, STM32L4_UART_IRQ_PRIORITY, g_Serial4Mode, (serialEvent4 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 5

bool Serial5_empty() { return !Serial5.available(); }

extern void serialEvent5() __attribute__((weak));

extern const stm32l4_uart_pins_t g_Serial5Pins;
extern const unsigned int g_Serial5Instance;
extern const unsigned int g_Serial5Mode;

static stm32l4_uart_t _Serial5;

Uart Serial5(&_Serial5, g_Serial5Instance, &g_Serial5Pins, STM32L4_UART_IRQ_PRIORITY, g_Serial5Mode, (serialEvent5 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 6

bool Serial6_empty() { return !Serial6.available(); }

extern void serialEvent6() __attribute__((weak));

extern const stm32l4_uart_pins_t g_Serial6Pins;
extern const unsigned int g_Serial6Instance;
extern const unsigned int g_Serial6Mode;

static stm32l4_uart_t _Serial6;

Uart Serial6(&_Serial6, g_Serial6Instance, &g_Serial6Pins, STM32L4_UART_IRQ_PRIORITY, g_Serial6Mode, (serialEvent6 != NULL));

#endif
