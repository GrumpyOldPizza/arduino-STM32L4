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

#include "Uart.h"
#include "Arduino.h"
#include "wiring_private.h"

#define UART_TX_PACKET_SIZE 16

Uart::Uart(struct _stm32l4_uart_t *uart, unsigned int instance, const struct _stm32l4_uart_pins_t *pins, unsigned int priority, unsigned int mode, bool serialEvent)
{
    _uart = uart;

    _blocking = true;

    _rx_read = 0;
    _rx_write = 0;
    _rx_count = 0;
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
    begin(baudrate, SERIAL_8N1);
}

void Uart::begin(unsigned long baudrate, uint16_t config)
{
    if (_uart->state == UART_STATE_INIT) {
	stm32l4_uart_enable(_uart, &_rx_fifo[0], sizeof(_rx_fifo), baudrate, config, Uart::_event_callback, (void*)this, (UART_EVENT_RECEIVE | UART_EVENT_TRANSMIT));
    } else {
	flush();

	stm32l4_uart_configure(_uart, baudrate, config);
    }
}

void Uart::end()
{
    flush();

    stm32l4_uart_disable(_uart);
}

int Uart::available()
{
    return _rx_count;
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
    if (_rx_count == 0) {
	return -1;
    }

    return _rx_data[_rx_read];
}

int Uart::read()
{
    unsigned int rx_read;
    uint8_t data;

    if (_rx_count == 0) {
	return -1;
    }

    rx_read = _rx_read;

    data = _rx_data[rx_read];
    
    _rx_read = (unsigned int)(rx_read + 1) & (UART_RX_BUFFER_SIZE -1);

    armv7m_atomic_sub(&_rx_count, 1);
  
    return data;
}

size_t Uart::read(uint8_t *buffer, size_t size)
{
    unsigned int rx_read, rx_count;
    size_t count;

    count = 0;

    while (count < size) {

	rx_count = _rx_count;

	if (rx_count == 0) {
	    break;
	}

	rx_read = _rx_read;

	if (rx_count > (UART_RX_BUFFER_SIZE - rx_read)) {
	    rx_count = (UART_RX_BUFFER_SIZE - rx_read);
	}

	memcpy(&buffer[count], &_rx_data[rx_read], rx_count);
	count += rx_count;
      
	_rx_read = (rx_read + rx_count) & (UART_RX_BUFFER_SIZE -1);

	armv7m_atomic_sub(&_rx_count, rx_count);
    }

    return count;
}

void Uart::flush()
{
    if (armv7m_core_priority() <= STM32L4_UART_IRQ_PRIORITY) {
	while (_tx_count != 0) {
	    stm32l4_uart_poll(_uart);
	}

	while (_tx_size2 != 0) {
	    stm32l4_uart_poll(_uart);
	}
    
	while (!stm32l4_uart_done(_uart)) {
	    stm32l4_uart_poll(_uart);
	}
    } else {
	while (_tx_count != 0) {
	    armv7m_core_yield();
	}

	while (_tx_size2 != 0) {
	    armv7m_core_yield();
	}
    
	while (!stm32l4_uart_done(_uart)) {
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

    // If there is an async write pending, wait till it's done
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
	if (_tx_size2 != 0) {
	    stm32l4_uart_transmit(_uart, _tx_data2, _tx_size2);
	}
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

void Uart::onReceive(void(*callback)(int))
{
    _receiveCallback = callback;
}

void Uart::blockOnOverrun(bool block)
{
    _blocking = block;
}

bool Uart::isEnabled()
{
    return (_uart->state >= UART_STATE_READY);
}

void Uart::EventCallback(uint32_t events)
{
    unsigned int rx_write, rx_count, rx_size, count;
    unsigned int tx_read, tx_size;
    bool empty;
    void(*callback)(void);

    if (events & UART_EVENT_RECEIVE) {
	while (_rx_count != UART_RX_BUFFER_SIZE) {
	    empty = (_rx_count == 0);

	    count = 0;

	    do {
		rx_count = UART_RX_BUFFER_SIZE - _rx_count;

		if (rx_count == 0) {
		    break;
		}
	      
		rx_write = _rx_write;

		if (rx_count > (UART_RX_BUFFER_SIZE - rx_write)) {
		    rx_count = (UART_RX_BUFFER_SIZE - rx_write);
		}
	      
		rx_size = stm32l4_uart_receive(_uart, &_rx_data[rx_write], rx_count);
	      
		_rx_write = (rx_write + rx_size) & (UART_RX_BUFFER_SIZE -1);
	      
		armv7m_atomic_add(&_rx_count, rx_size);
	      
		count += rx_size;
	      
	    } while (rx_size);
	  
	    if (empty && _receiveCallback) {
		(*_receiveCallback)(count);
	    }

	    if (!rx_size) {
		break;
	    }
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

	    callback = _completionCallback;
	    _completionCallback = NULL;

	    if (callback) {
		(*callback)();
	    }
	}
    }
}

void Uart::_event_callback(void *context, uint32_t events)
{
  reinterpret_cast<class Uart*>(context)->EventCallback(events);
}

bool Serial1_empty() { return !Serial1.available(); }

extern void serialEvent1() __attribute__((weak));

static stm32l4_uart_t stm32l4_usart3;

static const stm32l4_uart_pins_t stm32l4_usart3_pins = { GPIO_PIN_PC5_USART3_RX, GPIO_PIN_PC4_USART3_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial1(&stm32l4_usart3, UART_INSTANCE_USART3, &stm32l4_usart3_pins, STM32L4_UART_IRQ_PRIORITY, UART_MODE_RX_DMA | UART_MODE_TX_DMA, (serialEvent1 != NULL));

#if SERIAL_INTERFACES_COUNT > 1

bool Serial2_empty() { return !Serial2.available(); }

extern void serialEvent2() __attribute__((weak));

static stm32l4_uart_t stm32l4_uart4;

static const stm32l4_uart_pins_t stm32l4_uart4_pins = { GPIO_PIN_PA1_UART4_RX, GPIO_PIN_PA0_UART4_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial2(&stm32l4_uart4, UART_INSTANCE_UART4, &stm32l4_uart4_pins, STM32L4_UART_IRQ_PRIORITY, UART_MODE_RX_DMA | UART_MODE_RX_DMA_SECONDARY, (serialEvent2 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 2

bool Serial3_empty() { return !Serial3.available(); }

extern void serialEvent3() __attribute__((weak));

static stm32l4_uart_t stm32l4_usart2;

static const stm32l4_uart_pins_t stm32l4_usart2_pins = { GPIO_PIN_PA3_USART2_RX, GPIO_PIN_PA2_USART2_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial3(&stm32l4_usart2, UART_INSTANCE_USART2, &stm32l4_usart2_pins, STM32L4_UART_IRQ_PRIORITY, 0, (serialEvent3 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 3

bool Serial4_empty() { return !Serial4.available(); }

extern void serialEvent4() __attribute__((weak));

static stm32l4_uart_t stm32l4_uart5;

static const stm32l4_uart_pins_t stm32l4_uart5_pins = { GPIO_PIN_PD2_UART5_RX, GPIO_PIN_PC12_UART5_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial4(&stm32l4_uart5, UART_INSTANCE_UART5, &stm32l4_uart5_pins, STM32L4_UART_IRQ_PRIORITY, 0, (serialEvent4 != NULL));

#endif

#if SERIAL_INTERFACES_COUNT > 4

bool Serial5_empty() { return !Serial5.available(); }

extern void serialEvent5() __attribute__((weak));

static stm32l4_uart_t stm32l4_usart1;

static const stm32l4_uart_pins_t stm32l4_usart1_pins = { GPIO_PIN_PB7_USART1_RX, GPIO_PIN_PB6_USART1_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial5(&stm32l4_usart1, UART_INSTANCE_USART1, &stm32l4_usart1_pins, STM32L4_UART_IRQ_PRIORITY, 0, (serialEvent5 != NULL));

#endif

