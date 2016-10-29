/*
  HardwareSerial.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus
  Modified 3 December 2013 by Matthijs Kooijman
*/

#include "Arduino.h"
#include "HardwareSerial.h"

void (*serialEventCallback)(void) = NULL;

// SerialEvent functions are weak, so when the user doesn't define them,
// the linker just sets their address to 0 (which is checked below).
// The Serialx_available is just a wrapper around Serialx.available(),
// but we can refer to it weakly so we don't pull in the entire
// HardwareSerial instance if the user doesn't also refer to it.

#if defined(STM32L4_CONFIG_USBD_CDC)

void serialEvent() __attribute__((weak));
bool SerialUSB_empty() __attribute__((weak));

void serialEvent1() __attribute__((weak));
bool Serial1_empty() __attribute__((weak));

#if SERIAL_INTERFACES_COUNT > 1

void serialEvent2() __attribute__((weak));
bool Serial2_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 2

void serialEvent3() __attribute__((weak));
bool Serial3_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 3

void serialEvent4() __attribute__((weak));
bool Serial4_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 4

void serialEvent5() __attribute__((weak));
bool Serial5_empty() __attribute__((weak));

#endif

void serialEventDispatch(void)
{
    if (serialEvent && SerialUSB_empty && !SerialUSB_empty()) { serialEvent(); }

    if (serialEvent1 && Serial1_empty && !Serial1_empty()) { serialEvent1(); }
#if SERIAL_INTERFACES_COUNT > 1
    if (serialEvent2 && Serial2_empty && !Serial2_empty()) { serialEvent2(); }
#endif
#if SERIAL_INTERFACES_COUNT > 2
    if (serialEvent3 && Serial3_empty && !Serial3_empty()) { serialEvent3(); }
#endif
#if SERIAL_INTERFACES_COUNT > 3
    if (serialEvent4 && Serial4_empty && !Serial4_empty()) { serialEvent4(); }
#endif
#if SERIAL_INTERFACES_COUNT > 4
    if (serialEvent5 && Serial5_empty && !Serial5_empty()) { serialEvent5(); }
#endif
#if SERIAL_INTERFACES_COUNT > 5
    if (serialEvent6 && Serial6_empty && !Serial6_empty()) { serialEvent6(); }
#endif
}

#else /* STM32L4_CONFIG_USBD_CDC */

void serialEvent() __attribute__((weak));
bool Serial_empty() __attribute__((weak));

void serialEventDispatch(void)
{
    if (serialEvent && Serial_empty && !Serial_empty()) { serialEvent(); }
}

#endif /* STM32L4_CONFIG_USBD_CDC */

