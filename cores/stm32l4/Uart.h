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

#pragma once

#include "HardwareSerial.h"

#define UART_RX_BUFFER_SIZE 64
#define UART_TX_BUFFER_SIZE 64

class Uart : public HardwareSerial
{
  public:
    Uart(struct _stm32l4_uart_t *uart, unsigned int instance, const struct _stm32l4_uart_pins_t *pins, unsigned int priority, unsigned int mode, bool serialEvent);
    void begin(unsigned long baudRate);
    void begin(unsigned long baudrate, uint16_t config);
    void end();
    int available();
    int availableForWrite(void);
    int peek();
    int read();
    void flush();
    size_t write(const uint8_t data);
    size_t write(const uint8_t *buffer, size_t size);
    using Print::write; // pull in write(str) and write(buf, size) from Print

    // STM32L4 EXTENSTION: non-blocking multi-byte read
    size_t read(uint8_t *buffer, size_t size);

    // STM32L4 EXTENSTION: asynchronous write with callback
    bool write(const uint8_t *buffer, size_t size, void(*callback)(void));
    bool done(void);

    // STM32L4 EXTENSTION: asynchronous receive callback
    void onReceive(void(*callback)(int));

    // STM32L4 EXTENSTION: quick check for empty
    inline int empty() { return (_rx_read == _rx_write); };

    operator bool() { return true; }

  private:
    struct _stm32l4_uart_t *_uart;
    uint8_t _rx_fifo[16];
    uint8_t _rx_data[UART_RX_BUFFER_SIZE];
    volatile uint8_t _rx_write;
    uint8_t _rx_read;
    uint8_t _tx_data[UART_TX_BUFFER_SIZE];
    uint8_t _tx_write;
    volatile uint8_t _tx_read;
    uint16_t _tx_count;

    void (*_completionCallback)(void);
    void (*_receiveCallback)(int);

    static void _event_callback(void *context, uint32_t events);
    void EventCallback(uint32_t events);
};
