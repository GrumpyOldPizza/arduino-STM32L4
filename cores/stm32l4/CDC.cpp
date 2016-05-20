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

#include "CDC.h"
#include "Arduino.h"
#include "wiring_private.h"

/* STM32L4x5/STM32L4x6 have USB_OTG_FS with a multi-packet FIFO. However 
 * to avoid sending ZLP packets, the CDC_TX_PACKET_SIZE is one byte
 * less than the maximum FIFO size in terms of 64 byte packets.
 */
#define CDC_TX_PACKET_SIZE (((USBD_CDC_FIFO_SIZE + 63) & ~63) -1)

#define CDC_RX_BUFFER_MASK ((CDC_RX_BUFFER_SIZE<<1)-1)
#define CDC_RX_INDEX_MASK  (CDC_RX_BUFFER_SIZE-1)

#define CDC_TX_BUFFER_MASK ((CDC_TX_BUFFER_SIZE<<1)-1)
#define CDC_TX_INDEX_MASK  (CDC_TX_BUFFER_SIZE-1)

stm32l4_usbd_cdc_t stm32l4_usbd_cdc;

CDC::CDC(struct _stm32l4_usbd_cdc_t *usbd_cdc)
{
  _usbd_cdc = usbd_cdc;

  _rx_read = 0;
  _rx_write = 0;
  _tx_read = 0;
  _tx_write = 0;
  _tx_count = 0;

  _completionCallback = NULL;

  stm32l4_usbd_cdc_create(usbd_cdc);
}

void CDC::begin(unsigned long baudrate)
{
  begin(baudrate, (uint8_t)SERIAL_8N1);
}

void CDC::begin(unsigned long baudrate, uint16_t config)
{
  /* If USBD_CDC has already been enabled/initialized by STDIO, just add the notify.
   */
  if (_usbd_cdc->state == USBD_CDC_STATE_INIT) {
    stm32l4_usbd_cdc_enable(_usbd_cdc, 0, CDC::_event_callback, (void*)this, (USBD_CDC_EVENT_RECEIVE | USBD_CDC_EVENT_TRANSMIT));
  } else {
    stm32l4_usbd_cdc_notify(_usbd_cdc, CDC::_event_callback, (void*)this, (USBD_CDC_EVENT_RECEIVE | USBD_CDC_EVENT_TRANSMIT));
  }
}

void CDC::end()
{
  flush();

  stm32l4_usbd_cdc_disable(_usbd_cdc);
}

int CDC::available( void )
{
  unsigned int rx_read, rx_write;

  if (_usbd_cdc->state < USBD_CDC_STATE_READY)
    return 0;

  rx_read = _rx_read;
  rx_write = _rx_write;

  if ((rx_read ^ rx_write) == 0) {
    return 0;
  } else {
    rx_read &= CDC_RX_INDEX_MASK;
    rx_write &= CDC_RX_INDEX_MASK;

    if (rx_write <= rx_read) {
      return CDC_RX_BUFFER_SIZE - rx_read + rx_write;
    } else {
      return rx_write - rx_read;
    }
  }
}

int CDC::availableForWrite(void)
{
  unsigned int tx_read, tx_write;

  if (_usbd_cdc->state < USBD_CDC_STATE_READY)
    return 0;

  tx_read = _tx_read;
  tx_write = _tx_write;

  if ((tx_read ^ tx_write) == CDC_TX_BUFFER_SIZE) {
    return 0;
  } else {
    tx_read &= CDC_TX_INDEX_MASK;
    tx_write &= CDC_TX_INDEX_MASK;

    if (tx_read <= tx_write) {
      return CDC_TX_BUFFER_SIZE - tx_write + tx_read;
    } else {
      return tx_read - tx_write;
    }
  }
}

int CDC::peek( void )
{
  unsigned int rx_read = _rx_read;

  if ((rx_read ^ _rx_write) == 0)
    return -1;

  return _rx_data[rx_read & CDC_RX_INDEX_MASK];
}

int CDC::read( void )
{
  uint8_t data;
  unsigned int rx_read = _rx_read;

  if ((rx_read ^ _rx_write) == 0)
    return -1;

  data =_rx_data[rx_read & CDC_RX_INDEX_MASK];

  _rx_read = (unsigned int)(_rx_read + 1) & CDC_RX_BUFFER_MASK;
  
  return data;
}

size_t CDC::read(uint8_t *buffer, size_t size)
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

    rx_read &= CDC_RX_INDEX_MASK;
    rx_write &= CDC_RX_INDEX_MASK;
    
    if (rx_write <= rx_read) {
      rx_count = CDC_RX_BUFFER_SIZE - rx_read;
    } else {
      rx_count = rx_write - rx_read;
    }

    if (rx_count > (size - count)) {
      rx_count = (size - count);
    }

    memcpy(&buffer[count], &_rx_data[rx_read], rx_count);
    count += rx_count;

    _rx_read = (_rx_read + rx_count) & CDC_RX_BUFFER_MASK;
  }

  return count;
}

void CDC::flush( void )
{
  if (armv7m_core_priority() <= STM32L4_USB_IRQ_PRIORITY) {
    while (_tx_read != _tx_write) {
      stm32l4_usbd_cdc_poll(_usbd_cdc);
    }
    
    while (!stm32l4_usbd_cdc_done(_usbd_cdc)) {
      stm32l4_usbd_cdc_poll(_usbd_cdc);
    }
  } else {
    while (_tx_read != _tx_write) {
      armv7m_core_yield();
    }
    
    while (!stm32l4_usbd_cdc_done(_usbd_cdc)) {
      armv7m_core_yield();
    }
  }
}

size_t CDC::write( const uint8_t data )
{
  unsigned int tx_read, tx_write;

  if ((_usbd_cdc->state < USBD_CDC_STATE_READY) || !(stm32l4_usbd_cdc_info.lineState & 2))
    return 0;

  while (1) {
    
    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) == CDC_TX_BUFFER_SIZE) {
      if (stm32l4_usbd_cdc_done(_usbd_cdc)) {

	tx_read &= CDC_TX_INDEX_MASK;
	tx_write &= CDC_TX_INDEX_MASK;
	
	if (tx_write <= tx_read) {
	  _tx_count = CDC_TX_BUFFER_SIZE - tx_read;
	} else {
	  _tx_count = tx_write - tx_read;
	}
	
	if (_tx_count >= CDC_TX_PACKET_SIZE) {
	  _tx_count = CDC_TX_PACKET_SIZE;
	}

	stm32l4_usbd_cdc_transmit(_usbd_cdc, &_tx_data[tx_read], _tx_count);
      }

      if (armv7m_core_priority() <= STM32L4_USB_IRQ_PRIORITY) {
	tx_read = _tx_read;
	do {
	  stm32l4_usbd_cdc_poll(_usbd_cdc);
	} while (tx_read == _tx_read);
      } else {
	armv7m_core_yield();
      }
    } else {
      tx_write &= CDC_TX_INDEX_MASK;

      _tx_data[tx_write] = data;
      _tx_write = (unsigned int)(_tx_write + 1) & CDC_TX_BUFFER_MASK;

      break;
    }
  }

  if (stm32l4_usbd_cdc_done(_usbd_cdc)) {

    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) != 0) {
      tx_read &= CDC_TX_INDEX_MASK;
      tx_write &= CDC_TX_INDEX_MASK;
      
      if (tx_write <= tx_read) {
	_tx_count = CDC_TX_BUFFER_SIZE - tx_read;
      } else {
	_tx_count = tx_write - tx_read;
      }
      
      if (_tx_count >= CDC_TX_PACKET_SIZE) {
	_tx_count = CDC_TX_PACKET_SIZE;
      }
      
      stm32l4_usbd_cdc_transmit(_usbd_cdc, &_tx_data[tx_read], _tx_count);
    }
  }

  return 1;
}

size_t CDC::write(const uint8_t *buffer, size_t size)
{
  unsigned int tx_read, tx_write, tx_count;
  size_t count;

  if ((_usbd_cdc->state < USBD_CDC_STATE_READY) || !(stm32l4_usbd_cdc_info.lineState & 2))
    return 0;

  count = 0;

  while (count < size) {
    
    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) == CDC_TX_BUFFER_SIZE) {
      if (stm32l4_usbd_cdc_done(_usbd_cdc)) {

	tx_read &= CDC_TX_INDEX_MASK;
	tx_write &= CDC_TX_INDEX_MASK;
	
	if (tx_write <= tx_read) {
	  _tx_count = CDC_TX_BUFFER_SIZE - tx_read;
	} else {
	  _tx_count = tx_write - tx_read;
	}
	
	if (_tx_count >= CDC_TX_PACKET_SIZE) {
	  _tx_count = CDC_TX_PACKET_SIZE;
	}
	
	stm32l4_usbd_cdc_transmit(_usbd_cdc, &_tx_data[tx_read], _tx_count);
      }

      if (armv7m_core_priority() <= STM32L4_USB_IRQ_PRIORITY) {
	tx_read = _tx_read;
	do {
	  stm32l4_usbd_cdc_poll(_usbd_cdc);
	} while (tx_read == _tx_read);
      } else {
	armv7m_core_yield();
      }
    } else {

      tx_read &= CDC_TX_INDEX_MASK;
      tx_write &= CDC_TX_INDEX_MASK;

      if (tx_read <= tx_write) {
	tx_count = CDC_TX_BUFFER_SIZE - tx_write;
      } else {
	tx_count = tx_read - tx_write;
      }

      if (tx_count >= (size - count)) {
	tx_count = (size - count);
      }

      memcpy(&_tx_data[tx_write], &buffer[count], tx_count);
      count += tx_count;
      
      _tx_write = (unsigned int)(_tx_write + tx_count) & CDC_TX_BUFFER_MASK;
    }
  }

  if (stm32l4_usbd_cdc_done(_usbd_cdc)) {

    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) != 0) {

      tx_read &= CDC_TX_INDEX_MASK;
      tx_write &= CDC_TX_INDEX_MASK;
      
      if (tx_write <= tx_read) {
	_tx_count = CDC_TX_BUFFER_SIZE - tx_read;
      } else {
	_tx_count = tx_write - tx_read;
      }
      
      if (_tx_count >= CDC_TX_PACKET_SIZE) {
	_tx_count = CDC_TX_PACKET_SIZE;
      }
      
      stm32l4_usbd_cdc_transmit(_usbd_cdc, &_tx_data[tx_read], _tx_count);
    }
  }

  return size;
}

bool CDC::write(const uint8_t *buffer, size_t size, void(*callback)(void))
{
  if ((_usbd_cdc->state < USBD_CDC_STATE_READY) || !(stm32l4_usbd_cdc_info.lineState & 2))
    return false;
  
  if (!stm32l4_usbd_cdc_done(_usbd_cdc)) 
    return false;

  _completionCallback = callback;

  if (!stm32l4_usbd_cdc_transmit(_usbd_cdc, buffer, size)) {
    _completionCallback = NULL;
    return false;
  }

  return true;
}

bool CDC::done( void )
{
  if (_tx_read != _tx_write)
    return false;

  if (!stm32l4_usbd_cdc_done(_usbd_cdc))
    return false;

  return true;
}

void CDC::EventCallback( uint32_t events )
{
  unsigned int rx_read, rx_write, rx_count, rx_size, count;
  unsigned int tx_read, tx_write;
  void(*callback)(void);
  bool empty = false;

  if (events & USBD_CDC_EVENT_RECEIVE) {

    count = 0;

    do {
      rx_read = _rx_read;
      rx_write = _rx_write;

      if ((rx_read ^ rx_write) == 0) {
	empty = true;
      }

      if ((rx_read ^ rx_write) == CDC_TX_BUFFER_SIZE) {
	break;
      }

      rx_read &= CDC_RX_INDEX_MASK;
      rx_write &= CDC_RX_INDEX_MASK;

      if (rx_read <= rx_write) {
	rx_count = CDC_RX_BUFFER_SIZE - rx_write;
      } else {
	rx_count = rx_read - rx_write;
      }

      rx_size = stm32l4_usbd_cdc_receive(_usbd_cdc, &_rx_data[rx_write], rx_count);
      _rx_write = (_rx_write + rx_size) & CDC_RX_BUFFER_MASK;

      count += rx_size;

    } while (rx_size);

    if (empty && _receiveCallback) {
      (*_receiveCallback)(count);
    }
  }

  if (events & USBD_CDC_EVENT_TRANSMIT) {

    _tx_read = (_tx_read + _tx_count) & CDC_TX_BUFFER_MASK;
    _tx_count = 0;

    callback = _completionCallback;
    _completionCallback = NULL;

    if (callback) {
      (*callback)();
    }
      
    tx_read = _tx_read;
    tx_write = _tx_write;
    
    if ((tx_read ^ tx_write) != 0) {

      tx_read &= CDC_TX_INDEX_MASK;
      tx_write &= CDC_TX_INDEX_MASK;
      
      if (tx_write <= tx_read) {
	_tx_count = CDC_TX_BUFFER_SIZE - tx_read;
      } else {
	_tx_count = tx_write - tx_read;
      }
      
      if (_tx_count >= CDC_TX_PACKET_SIZE) {
	_tx_count = CDC_TX_PACKET_SIZE;
      }
      
      stm32l4_usbd_cdc_transmit(_usbd_cdc, &_tx_data[tx_read], _tx_count);
    }
  }
}

void CDC::_event_callback(void *context, uint32_t events)
{
  reinterpret_cast<class CDC*>(context)->EventCallback(events);
}

CDC::operator bool()
{
  return stm32l4_usbd_cdc_connected(_usbd_cdc);
}

unsigned long CDC::baud() {
  return stm32l4_usbd_cdc_info.dwDTERate;
}

uint8_t CDC::stopbits() {
  return stm32l4_usbd_cdc_info.bCharFormat;
}

uint8_t CDC::paritytype() {
  return stm32l4_usbd_cdc_info.bParityType;
}

uint8_t CDC::numbits() {
  return stm32l4_usbd_cdc_info.bDataBits;
}

bool CDC::dtr() {
  return stm32l4_usbd_cdc_info.lineState & 1;
}

bool CDC::rts() {
  return stm32l4_usbd_cdc_info.lineState & 2;
}

CDC SerialUSB(&stm32l4_usbd_cdc);

bool SerialUSB_empty() { return SerialUSB.empty(); }
