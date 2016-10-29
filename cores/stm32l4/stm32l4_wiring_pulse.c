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

#include "Arduino.h"
#include "stm32l4_wiring_private.h"

/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse. */
uint32_t pulseIn(uint32_t pin, uint32_t state, uint32_t timeout)
{
    if (g_APinDescription[pin].GPIO == NULL)
    {
	return 0;
    }

  // cache the port and bit of the pin in order to speed up the
  // pulse width measuring loop and achieve finer resolution.  calling
  // digitalRead() instead yields much coarser resolution.
  GPIO_TypeDef *GPIO = g_APinDescription[pin].GPIO;
  uint32_t bit = 1 << g_APinDescription[pin].bit;
  uint32_t stateMask = state ? bit : 0;

  
  // convert the timeout from microseconds to a number of times through
  // the initial loop; it takes (roughly) 13 clock cycles per iteration.
  uint32_t maxcycles = microsecondsToClockCycles(timeout);
  uint32_t cycles;

  cycles = DWT->CYCCNT;

  // wait for any previous pulse to end
  while ((GPIO->IDR & bit) == stateMask)
  {
      if ((DWT->CYCCNT - cycles) >= maxcycles)
      {
          return 0;
      }
  }

  cycles = DWT->CYCCNT;

  // wait for the pulse to start
  while ((GPIO->IDR & bit) != stateMask)
  {
      if ((DWT->CYCCNT - cycles) >= maxcycles)
      {
          return 0;
      }
  }

  cycles = DWT->CYCCNT;

  // wait for the pulse to stop
  while ((GPIO->IDR & bit) == stateMask)
  {
      if ((DWT->CYCCNT - cycles) >= maxcycles)
      {
          return 0;
      }
  }

  cycles = DWT->CYCCNT - cycles;

  if (cycles)
    return clockCyclesToMicroseconds(cycles);
  else
    return 0;
}

