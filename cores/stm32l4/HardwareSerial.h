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

#ifndef HardwareSerial_h
#define HardwareSerial_h

#include <inttypes.h>

#include "Stream.h"

#define HARDSER_PARITY_NONE     (0x0000ul)
#define HARDSER_PARITY_ODD      (0x0001ul)
#define HARDSER_PARITY_EVEN     (0x0002ul)
#define HARDSER_PARITY_MASK     (0x000Ful)

#define HARDSER_STOP_BIT_1      (0x0000ul)
#define HARDSER_STOP_BIT_1_5	(0x0010ul)
#define HARDSER_STOP_BIT_2	(0x0020ul)
#define HARDSER_STOP_BIT_MASK	(0x00F0ul)

#define HARDSER_DATA_7	 	(0x0000ul)
#define HARDSER_DATA_8	 	(0x0100ul)
#define HARDSER_DATA_9	 	(0x0200ul)
#define HARDSER_DATA_MASK	(0x0F00ul)

#define SERIAL_7N1	(HARDSER_STOP_BIT_1 | HARDSER_PARITY_NONE | HARDSER_DATA_7)
#define SERIAL_8N1	(HARDSER_STOP_BIT_1 | HARDSER_PARITY_NONE | HARDSER_DATA_8)
#define SERIAL_9N1	(HARDSER_STOP_BIT_1 | HARDSER_PARITY_NONE | HARDSER_DATA_9)
#define SERIAL_7N2	(HARDSER_STOP_BIT_2 | HARDSER_PARITY_NONE | HARDSER_DATA_7)
#define SERIAL_8N2	(HARDSER_STOP_BIT_2 | HARDSER_PARITY_NONE | HARDSER_DATA_8)
#define SERIAL_9N2	(HARDSER_STOP_BIT_2 | HARDSER_PARITY_NONE | HARDSER_DATA_9)
#define SERIAL_7E1	(HARDSER_STOP_BIT_1 | HARDSER_PARITY_EVEN | HARDSER_DATA_7)
#define SERIAL_8E1	(HARDSER_STOP_BIT_1 | HARDSER_PARITY_EVEN | HARDSER_DATA_8)
#define SERIAL_7E2	(HARDSER_STOP_BIT_2 | HARDSER_PARITY_EVEN | HARDSER_DATA_7)
#define SERIAL_8E2	(HARDSER_STOP_BIT_2 | HARDSER_PARITY_EVEN | HARDSER_DATA_8)
#define SERIAL_7O1	(HARDSER_STOP_BIT_1 | HARDSER_PARITY_ODD  | HARDSER_DATA_7)
#define SERIAL_8O1	(HARDSER_STOP_BIT_1 | HARDSER_PARITY_ODD  | HARDSER_DATA_8)
#define SERIAL_7O2	(HARDSER_STOP_BIT_2 | HARDSER_PARITY_ODD  | HARDSER_DATA_7)
#define SERIAL_8O2	(HARDSER_STOP_BIT_2 | HARDSER_PARITY_ODD  | HARDSER_DATA_8)

class HardwareSerial : public Stream
{
  public:
    virtual void begin(unsigned long);
    virtual void begin(unsigned long baudrate, uint32_t config);
    virtual void end();
    virtual int available(void) = 0;
    virtual int peek(void) = 0;
    virtual int read(void) = 0;
    virtual void flush(void) = 0;
    virtual size_t write(uint8_t) = 0;
    using Print::write; // pull in write(str) and write(buf, size) from Print
    virtual operator bool() = 0;
};

extern void (*serialEventCallback)(void);
extern void serialEventDispatch(void);

#endif
