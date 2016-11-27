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
#include <stdio.h>

static inline __attribute__((optimize("O3"),always_inline)) uint32_t countPulseInline(const volatile uint32_t *port, uint32_t bit, uint32_t stateMask, unsigned long maxloops)
{
    uint32_t micros;

    // wait for any previous pulse to end
    while ((*port & bit) == stateMask)
    {
	if (--maxloops == 0)
	{
	    return 0;
	}
    }
    
    // wait for the pulse to start
    while ((*port & bit) != stateMask)
    {
	if (--maxloops == 0)
	{
	    return 0;
	}
    }

    micros = armv7m_systick_micros();

    // wait for the pulse to stop
    while ((*port & bit) == stateMask)
    {
	if (--maxloops == 0)
	{
	    return 0;
	}
    }
    
    return (uint32_t)armv7m_systick_micros() - micros;
}

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
  uint32_t bit = g_APinDescription[pin].bit;
  uint32_t stateMask = state ? bit : 0;

  // convert the timeout from microseconds to a number of times through
  // the initial loop; it takes (roughly) 8 clock cycles per iteration.
  uint32_t maxloops = microsecondsToClockCycles(timeout) / 8;

  return countPulseInline(&GPIO->IDR, bit, stateMask, maxloops);
}

