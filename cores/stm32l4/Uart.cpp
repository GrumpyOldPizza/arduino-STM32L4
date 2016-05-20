/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Uart.h"
#include "Arduino.h"
#include "wiring_private.h"

#define UART_TX_PACKET_SIZE 16

#define UART_RX_BUFFER_MASK ((UART_RX_BUFFER_SIZE<<1)-1)
#define UART_RX_INDEX_MASK  (UART_RX_BUFFER_SIZE-1)

#define UART_TX_BUFFER_MASK ((UART_TX_BUFFER_SIZE<<1)-1)
#define UART_TX_INDEX_MASK  (UART_TX_BUFFER_SIZE-1)

Uart::Uart(struct _stm32l4_uart_t *uart, unsigned int instance, const struct _stm32l4_uart_pins_t *pins, unsigned int priority, unsigned int mode)
{
  _uart = uart;

  _rx_read = 0;
  _rx_write = 0;
  _tx_read = 0;
  _tx_write = 0;
  _tx_count = 0;
  
  _completionCallback = NULL;

  stm32l4_uart_create(uart, instance, pins, priority, mode);
}

void Uart::begin(unsigned long baudrate)
{
  begin(baudrate, (uint8_t)SERIAL_8N1);
}

void Uart::begin(unsigned long baudrate, uint16_t config)
{
  stm32l4_uart_enable(_uart, &_rx_fifo[0], sizeof(_rx_fifo), baudrate, config, Uart::_event_callback, (void*)this, (UART_EVENT_RECEIVE | UART_EVENT_TRANSMIT));
}

void Uart::end()
{
  flush();

  stm32l4_uart_disable(_uart);
}

int Uart::available( void )
{
  unsigned int rx_read, rx_write;

  if (_uart->state < UART_STATE_READY)
    return 0;

  rx_read = _rx_read;
  rx_write = _rx_write;

  if ((rx_read ^ rx_write) == 0) {
    return 0;
  } else {
    rx_read &= UART_RX_INDEX_MASK;
    rx_write &= UART_RX_INDEX_MASK;

    if (rx_write <= rx_read) {
      return UART_RX_BUFFER_SIZE - rx_read + rx_write;
    } else {
      return rx_write - rx_read;
    }
  }
}

int Uart::availableForWrite(void)
{
  unsigned int tx_read, tx_write;

  if (_uart->state < UART_STATE_READY)
    return 0;

  tx_read = _tx_read;
  tx_write = _tx_write;

  if ((tx_read ^ tx_write) == UART_TX_BUFFER_SIZE) {
    return 0;
  } else {
    tx_read &= UART_TX_INDEX_MASK;
    tx_write &= UART_TX_INDEX_MASK;

    if (tx_read <= tx_write) {
      return UART_TX_BUFFER_SIZE - tx_write + tx_read;
    } else {
      return tx_read - tx_write;
    }
  }
}

int Uart::peek( void )
{
  unsigned int rx_read = _rx_read;

  if ((rx_read ^ _rx_write) == 0)
    return -1;

  return _rx_data[rx_read & UART_RX_INDEX_MASK];
}

int Uart::read( void )
{
  uint8_t data;
  unsigned int rx_read = _rx_read;

  if ((rx_read ^ _rx_write) == 0)
    return -1;

  data =_rx_data[rx_read & UART_RX_INDEX_MASK];

  _rx_read = (unsigned int)(_rx_read + 1) & UART_RX_BUFFER_MASK;
  
  return data;
}

size_t Uart::read(uint8_t *buffer, size_t size)
{
  unsigned int rx_read, rx_write, rx_count;
  size_t count;

  count = 0;

  while (count < size) {

    rx_read = _rx_read;
    rx_write = _rx_write;

    if ((rx_read ^ rx_write) == 0) {
      break;
    }

    rx_read &= UART_RX_INDEX_MASK;
    rx_write &= UART_RX_INDEX_MASK;
    
    if (rx_write <= rx_read) {
      rx_count = UART_RX_BUFFER_SIZE - rx_read;
    } else {
      rx_count = rx_write - rx_read;
    }

    if (rx_count > (size - count)) {
      rx_count = (size - count);
    }

    memcpy(&buffer[count], &_rx_data[rx_read], rx_count);
    count += rx_count;

    _rx_read = (_rx_read + rx_count) & UART_RX_BUFFER_MASK;
  }

  return count;
}

void Uart::flush( void )
{
  if (armv7m_core_priority() <= STM32L4_UART_IRQ_PRIORITY) {
    while (_tx_read != _tx_write) {
      stm32l4_uart_poll(_uart);
    }
    
    while (!stm32l4_uart_done(_uart)) {
      stm32l4_uart_poll(_uart);
    }
  } else {
    while (_tx_read != _tx_write) {
      armv7m_core_yield();
    }
    
    while (!stm32l4_uart_done(_uart)) {
      armv7m_core_yield();
    }
  }
}

size_t Uart::write( const uint8_t data )
{
  unsigned int tx_read, tx_write;

  if (_uart->state < UART_STATE_READY)
    return 0;

  while (1) {
    
    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) == UART_TX_BUFFER_SIZE) {
      if (stm32l4_uart_done(_uart)) {

	tx_read &= UART_TX_INDEX_MASK;
	tx_write &= UART_TX_INDEX_MASK;
	
	if (tx_write <= tx_read) {
	  _tx_count = UART_TX_BUFFER_SIZE - tx_read;
	} else {
	  _tx_count = tx_write - tx_read;
	}
	
	if (_tx_count >= UART_TX_PACKET_SIZE) {
	  _tx_count = UART_TX_PACKET_SIZE;
	}
	
	stm32l4_uart_transmit(_uart, &_tx_data[tx_read], _tx_count);
      }

      if (armv7m_core_priority() <= STM32L4_UART_IRQ_PRIORITY) {
	tx_read = _tx_read;
	do {
	  stm32l4_uart_poll(_uart);
	} while (tx_read == _tx_read);
      } else {
	armv7m_core_yield();
      }
    } else {
      tx_write &= UART_TX_INDEX_MASK;

      _tx_data[tx_write] = data;
      _tx_write = (unsigned int)(_tx_write + 1) & UART_TX_BUFFER_MASK;

      break;
    }
  }

  if (stm32l4_uart_done(_uart)) {

    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) != 0) {
      tx_read &= UART_TX_INDEX_MASK;
      tx_write &= UART_TX_INDEX_MASK;
      
      if (tx_write <= tx_read) {
	_tx_count = UART_TX_BUFFER_SIZE - tx_read;
      } else {
	_tx_count = tx_write - tx_read;
      }
      
      if (_tx_count >= UART_TX_PACKET_SIZE) {
	_tx_count = UART_TX_PACKET_SIZE;
      }
      
      stm32l4_uart_transmit(_uart, &_tx_data[tx_read], _tx_count);
    }
  }

  return 1;
}

size_t Uart::write(const uint8_t *buffer, size_t size)
{
  unsigned int tx_read, tx_write, tx_count;
  size_t count;

  if (_uart->state < UART_STATE_READY)
    return 0;

  count = 0;

  while (count < size) {
    
    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) == UART_TX_BUFFER_SIZE) {
      if (stm32l4_uart_done(_uart)) {

	tx_read &= UART_TX_INDEX_MASK;
	tx_write &= UART_TX_INDEX_MASK;
	
	if (tx_write <= tx_read) {
	  _tx_count = UART_TX_BUFFER_SIZE - tx_read;
	} else {
	  _tx_count = tx_write - tx_read;
	}
	
	if (_tx_count >= UART_TX_PACKET_SIZE) {
	  _tx_count = UART_TX_PACKET_SIZE;
	}
	
	stm32l4_uart_transmit(_uart, &_tx_data[tx_read], _tx_count);
      }

      if (armv7m_core_priority() <= STM32L4_UART_IRQ_PRIORITY) {
	tx_read = _tx_read;
	do {
	  stm32l4_uart_poll(_uart);
	} while (tx_read == _tx_read);
      } else {
	armv7m_core_yield();
      }
    } else {

      tx_read &= UART_TX_INDEX_MASK;
      tx_write &= UART_TX_INDEX_MASK;

      if (tx_read <= tx_write) {
	tx_count = UART_TX_BUFFER_SIZE - tx_write;
      } else {
	tx_count = tx_read - tx_write;
      }

      if (tx_count >= (size - count)) {
	tx_count = (size - count);
      }
      
      memcpy(&_tx_data[tx_write], &buffer[count], tx_count);
      count += tx_count;
      
      _tx_write = (unsigned int)(_tx_write + tx_count) & UART_TX_BUFFER_MASK;
    }
  }

  if (stm32l4_uart_done(_uart)) {

    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) != 0) {

      tx_read &= UART_TX_INDEX_MASK;
      tx_write &= UART_TX_INDEX_MASK;
      
      if (tx_write <= tx_read) {
	_tx_count = UART_TX_BUFFER_SIZE - tx_read;
      } else {
	_tx_count = tx_write - tx_read;
      }
      
      if (_tx_count >= UART_TX_PACKET_SIZE) {
	_tx_count = UART_TX_PACKET_SIZE;
      }
      
      stm32l4_uart_transmit(_uart, &_tx_data[tx_read], _tx_count);
    }
  }

  return size;
}

bool Uart::write(const uint8_t *buffer, size_t size, void(*callback)(void))
{
  if (_uart->state < UART_STATE_READY)
    return false;
  
  if (!stm32l4_uart_done(_uart)) 
    return false;

  _completionCallback = callback;

  if (!stm32l4_uart_transmit(_uart, buffer, size)) {
    _completionCallback = NULL;
    return false;
  }

  return true;
}

bool Uart::done( void )
{
  if (_tx_read != _tx_write)
    return false;

  if (!stm32l4_uart_done(_uart))
    return false;

  return true;
}

void Uart::onReceive(void(*callback)(int))
{
  _receiveCallback = callback;
}

void Uart::EventCallback( uint32_t events )
{
  unsigned int rx_read, rx_write, rx_count, rx_size, count;
  unsigned int tx_read, tx_write;
  void(*callback)(void);
  bool empty = false;

  if (events & UART_EVENT_RECEIVE) {

    count = 0;

    do {
      rx_read = _rx_read;
      rx_write = _rx_write;

      if ((rx_read ^ rx_write) == 0) {
	empty = true;
      }

      if ((rx_read ^ rx_write) == UART_TX_BUFFER_SIZE) {
	break;
      }

      rx_read &= UART_RX_INDEX_MASK;
      rx_write &= UART_RX_INDEX_MASK;

      if (rx_read <= rx_write) {
	rx_count = UART_RX_BUFFER_SIZE - rx_write;
      } else {
	rx_count = rx_read - rx_write;
      }

      rx_size = stm32l4_uart_receive(_uart, &_rx_data[rx_write], rx_count);
      _rx_write = (_rx_write + rx_size) & UART_RX_BUFFER_MASK;

      count += rx_size;

    } while (rx_size);

    if (empty && _receiveCallback) {
      (*_receiveCallback)(count);
    }
  }

  if (events & UART_EVENT_TRANSMIT) {

    _tx_read = (_tx_read + _tx_count) & UART_TX_BUFFER_MASK;
    _tx_count = 0;

    callback = _completionCallback;
    _completionCallback = NULL;

    if (callback) {
      (*callback)();
    }
      
    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) != 0) {

      tx_read &= UART_TX_INDEX_MASK;
      tx_write &= UART_TX_INDEX_MASK;
      
      if (tx_write <= tx_read) {
	_tx_count = UART_TX_BUFFER_SIZE - tx_read;
      } else {
	_tx_count = tx_write - tx_read;
      }
      
      if (_tx_count >= UART_TX_PACKET_SIZE) {
	_tx_count = UART_TX_PACKET_SIZE;
      }
      
      stm32l4_uart_transmit(_uart, &_tx_data[tx_read], _tx_count);
    }
  }
}

void Uart::_event_callback(void *context, uint32_t events)
{
  reinterpret_cast<class Uart*>(context)->EventCallback(events);
}

static stm32l4_uart_t stm32l4_usart3;

static const stm32l4_uart_pins_t stm32l4_usart3_pins = { GPIO_PIN_PC5_USART3_RX, GPIO_PIN_PC4_USART3_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial1(&stm32l4_usart3, UART_INSTANCE_USART3, &stm32l4_usart3_pins, STM32L4_UART_IRQ_PRIORITY, UART_MODE_RX_DMA | UART_MODE_TX_DMA);

bool Serial1_empty() { return Serial1.empty(); }

#if SERIAL_INTERFACES_COUNT > 1

static stm32l4_uart_t stm32l4_uart4;

static const stm32l4_uart_pins_t stm32l4_uart4_pins = { GPIO_PIN_PA1_UART4_RX, GPIO_PIN_PA0_UART4_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial2(&stm32l4_uart4, UART_INSTANCE_UART4, &stm32l4_uart4_pins, STM32L4_UART_IRQ_PRIORITY, UART_MODE_RX_DMA | UART_MODE_RX_DMA_SECONDARY);

bool Serial2_empty() { return Serial2.empty(); }

#endif

#if SERIAL_INTERFACES_COUNT > 2

static stm32l4_uart_t stm32l4_usart2;

static const stm32l4_uart_pins_t stm32l4_usart2_pins = { GPIO_PIN_PA3_USART2_RX, GPIO_PIN_PA2_USART2_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial3(&stm32l4_usart2, UART_INSTANCE_USART2, &stm32l4_usart2_pins, STM32L4_UART_IRQ_PRIORITY, 0);

bool Serial3_empty() { return Serial3.empty(); }

#endif

#if SERIAL_INTERFACES_COUNT > 3

static stm32l4_uart_t stm32l4_uart5;

static const stm32l4_uart_pins_t stm32l4_uart5_pins = { GPIO_PIN_PD2_UART5_RX, GPIO_PIN_PC12_UART5_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial4(&stm32l4_uart5, UART_INSTANCE_UART5, &stm32l4_uart5_pins, STM32L4_UART_IRQ_PRIORITY, 0);

bool Serial4_empty() { return Serial4.empty(); }

#endif

#if SERIAL_INTERFACES_COUNT > 4

static stm32l4_uart_t stm32l4_usart1;

static const stm32l4_uart_pins_t stm32l4_usart1_pins = { GPIO_PIN_PB7_USART1_RX, GPIO_PIN_PB6_USART1_TX, GPIO_PIN_NONE, GPIO_PIN_NONE };

Uart __attribute__((weak)) Serial5(&stm32l4_usart1, UART_INSTANCE_USART1, &stm32l4_usart1_pins, STM32L4_UART_IRQ_PRIORITY, 0);

bool Serial5_empty() { return Serial5.empty(); }

#endif

