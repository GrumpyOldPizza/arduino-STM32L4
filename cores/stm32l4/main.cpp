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

#define ARDUINO_MAIN
#include "Arduino.h"
#include "HardwareSerial.h"


void (*serialEventCallback)(void) = NULL;

// SerialEvent functions are weak, so when the user doesn't define them,
// the linker just sets their address to 0 (which is checked below).
// The Serialx_available is just a wrapper around Serialx.available(),
// but we can refer to it weakly so we don't pull in the entire
// HardwareSerial instance if the user doesn't also refer to it.

void serialEvent() __attribute__((weak));
bool Serial_empty() __attribute__((weak));

#if SERIAL_INTERFACES_COUNT > 1

void serialEvent1() __attribute__((weak));
bool Serial1_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 2

void serialEvent2() __attribute__((weak));
bool Serial2_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 3

void serialEvent3() __attribute__((weak));
bool Serial3_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 4

void serialEvent4() __attribute__((weak));
bool Serial4_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 5

void serialEvent5() __attribute__((weak));
bool Serial5_empty() __attribute__((weak));

#endif

#if SERIAL_INTERFACES_COUNT > 6

void serialEvent6() __attribute__((weak));
bool Serial6_empty() __attribute__((weak));

#endif

void serialEventDispatch(void)
{
    if (serialEvent  && Serial_empty  && !Serial_empty())  { serialEvent();  }
#if SERIAL_INTERFACES_COUNT > 1
    if (serialEvent1 && Serial1_empty && !Serial1_empty()) { serialEvent1(); }
#endif
#if SERIAL_INTERFACES_COUNT > 2
    if (serialEvent2 && Serial2_empty && !Serial2_empty()) { serialEvent2(); }
#endif
#if SERIAL_INTERFACES_COUNT > 3
    if (serialEvent3 && Serial3_empty && !Serial3_empty()) { serialEvent3(); }
#endif
#if SERIAL_INTERFACES_COUNT > 4
    if (serialEvent4 && Serial4_empty && !Serial4_empty()) { serialEvent4(); }
#endif
#if SERIAL_INTERFACES_COUNT > 5
    if (serialEvent5 && Serial5_empty && !Serial5_empty()) { serialEvent5(); }
#endif
#if SERIAL_INTERFACES_COUNT > 6
    if (serialEvent6 && Serial6_empty && !Serial6_empty()) { serialEvent6(); }
#endif
}

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

// Initialize C library
extern "C" void __libc_init_array(void);

/*
 * \brief Main entry point of Arduino application
 */
extern "C" int main( void )
{
    init();

    __libc_init_array();

    initVariant();

    delay(1);

#if defined(USBCON)
    if (SystemCoreClock >= 16000000)
    {
	USBDevice.init();
	USBDevice.attach();
    }
#endif /* USBCON */

    setup();

    for (;;)
    {
	loop();
	if (serialEventCallback) (*serialEventCallback)();
    }

    return 0;
}
