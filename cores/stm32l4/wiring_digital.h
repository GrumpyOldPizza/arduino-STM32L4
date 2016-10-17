/*
 * Copyright (c) 2016 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#ifndef _WIRING_DIGITAL_
#define _WIRING_DIGITAL_

#ifdef __cplusplus
 extern "C" {
#endif

#include "WVariant.h"

/**
 * \brief Configures the specified pin to behave either as an input or an output. See the description of digital pins for details.
 *
 * \param ulPin The number of the pin whose mode you wish to set
 * \param ulMode Can be INPUT, OUTPUT, INPUT_PULLUP or INPUT_PULLDOWN
 */
extern void pinMode( uint32_t dwPin, uint32_t dwMode ) ;

/**
 * \brief Write a HIGH or a LOW value to a digital pin.
 *
 * If the pin has been configured as an OUTPUT with pinMode(), its voltage will be set to the
 * corresponding value: 5V (or 3.3V on 3.3V boards) for HIGH, 0V (ground) for LOW.
 *
 * If the pin is configured as an INPUT, writing a HIGH value with digitalWrite() will enable an internal
 * 20K pullup resistor (see the tutorial on digital pins). Writing LOW will disable the pullup. The pullup
 * resistor is enough to light an LED dimly, so if LEDs appear to work, but very dimly, this is a likely
 * cause. The remedy is to set the pin to an output with the pinMode() function.
 *
 * \note Digital pin PIN_LED is harder to use as a digital input than the other digital pins because it has an LED
 * and resistor attached to it that's soldered to the board on most boards. If you enable its internal 20k pull-up
 * resistor, it will hang at around 1.7 V instead of the expected 5V because the onboard LED and series resistor
 * pull the voltage level down, meaning it always returns LOW. If you must use pin PIN_LED as a digital input, use an
 * external pull down resistor.
 *
 * \param dwPin the pin number
 * \param dwVal HIGH or LOW
 */
extern void digitalWrite( uint32_t dwPin, uint32_t dwVal ) ;

/**
 * \brief Reads the value from a specified digital pin, either HIGH or LOW.
 *
 * \param ulPin The number of the digital pin you want to read (int)
 *
 * \return HIGH or LOW
 */
extern int digitalRead( uint32_t ulPin ) ;

#ifdef __cplusplus
}
#endif

#endif /* _WIRING_DIGITAL_ */
