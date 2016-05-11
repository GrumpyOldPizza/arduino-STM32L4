/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Thomas Roell.  All right reserved.

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

#pragma once

#include "HardwareSerial.h"

#define CDC_RX_BUFFER_SIZE 512
#define CDC_TX_BUFFER_SIZE 512

class CDC : public HardwareSerial
{
  public:
    CDC(struct _stm32l4_usbd_cdc_t *usbd_cdc);
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

    // STM32L4 EXTENSTION: non-blocking multi-byte read
    size_t read(uint8_t *buffer, size_t size);

    // STM32L4 EXTENSTION: asynchronous write with callback
    bool write(const uint8_t *buffer, size_t size, void(*callback)(void));
    bool done(void);
    
    // STM32L4 EXTENSTION: asynchronous receive callback
    void onReceive(void(*callback)(int));

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

  private:
    struct _stm32l4_usbd_cdc_t *_usbd_cdc;
    uint8_t _rx_data[CDC_RX_BUFFER_SIZE];
    volatile uint16_t _rx_write;
    volatile uint16_t _rx_read;
    uint8_t _tx_data[CDC_TX_BUFFER_SIZE];
    volatile uint16_t _tx_write;
    volatile uint16_t _tx_read;
    uint16_t _tx_count;

    void (*_completionCallback)(void);
    void (*_receiveCallback)(int);

    static void _event_callback(void *context, uint32_t events);
    void EventCallback(uint32_t events);
};

extern CDC SerialUSB;
