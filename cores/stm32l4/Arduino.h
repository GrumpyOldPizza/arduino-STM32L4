/*
  Arduino.h - Main include file for the Arduino SDK
  Copyright (c) 2014 Arduino LLC.  All right reserved.

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
*/

#ifndef Arduino_h
#define Arduino_h

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef bool boolean;
typedef uint8_t byte;
typedef uint16_t word;

// some libraries and sketches depend on this AVR stuff,
// assuming Arduino.h or WProgram.h automatically includes it...
//
#include "avr/pgmspace.h"
#include "avr/interrupt.h"
#include "avr/io.h"

#include "binary.h"
#include "itoa.h"

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#define LOW             (0x0)
#define HIGH            (0x1)

#define INPUT           (0x0)
#define OUTPUT          (0x1)
#define INPUT_PULLUP    (0x2)
#define INPUT_PULLDOWN  (0x3)
#define INPUT_ANALOG    (0x4)

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define SERIAL  0x0
#define DISPLAY 0x1

enum BitOrder {
	LSBFIRST = 0,
	MSBFIRST = 1
};

//      LOW 0
//      HIGH 1
#define CHANGE 2
#define FALLING 3
#define RISING 4

#define DEFAULT 1
#define EXTERNAL 0

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( ((a) * 1000L) / (F_CPU / 1000L) )
#define microsecondsToClockCycles(a) ( (a) * (F_CPU / 1000000L) )

#define ARM_MATH_CM4
#include "stm32l4xx.h"
#undef STM32L4
#undef RTC
#undef DAC1
#undef SPI1
#undef SPI2
#include "stm32l4_wiring.h"

void yield( void ) ;

/* sketch */
void setup( void ) ;
void loop( void ) ;

#ifdef __cplusplus
} // extern "C"
#endif

// The following headers are for C++ only compilation
#ifdef __cplusplus
  #include "WCharacter.h"
  #include "WString.h"
  #include "WMath.h"
  #include "HardwareSerial.h"
  #include "STM32.h"
#endif

// Include board variant
#include "variant.h"

// undefine stdlib's abs if encountered
#ifdef abs
#undef abs
#endif // abs

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define bit(b) (1UL << (b))

#define _BV(bit) (1 << (bit))

#if (ARDUINO_STM32L4_VARIANT_COMPLIANCE >= 10606)
// Interrupts
#define digitalPinToInterrupt(P)   ( P )
#endif

#endif // Arduino_h
