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

#include "Arduino.h"
#include "stm32l4_wiring_private.h"

#ifdef __cplusplus
 extern "C" {
#endif

void pinMode( uint32_t ulPin, uint32_t ulMode )
{
    // Handle the case the pin isn't usable as PIO
    if ( g_APinDescription[ulPin].GPIO == NULL )
    {
	return ;
    }

    // Set pin mode according to chapter '22.6.3 I/O Pin Configuration'
    switch ( ulMode )
    {
    case INPUT:
	// Set pin to input mode
	stm32l4_gpio_pin_configure(g_APinDescription[ulPin].pin, (GPIO_PUPD_NONE | GPIO_OSPEED_MEDIUM | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));
	break ;

    case INPUT_PULLUP:
	// Set pin to input mode with pull-up resistor enabled
	stm32l4_gpio_pin_configure(g_APinDescription[ulPin].pin, (GPIO_PUPD_PULLUP | GPIO_OSPEED_MEDIUM | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));
	break ;

    case INPUT_PULLDOWN:
	// Set pin to input mode with pull-down resistor enabled
	stm32l4_gpio_pin_configure(g_APinDescription[ulPin].pin, (GPIO_PUPD_PULLDOWN | GPIO_OSPEED_MEDIUM | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));
	break ;

    case INPUT_ANALOG:
	// Set pin to analog mode
	stm32l4_gpio_pin_configure(g_APinDescription[ulPin].pin, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
	break ;

    case OUTPUT:
	// Set pin to output mode
	stm32l4_gpio_pin_configure(g_APinDescription[ulPin].pin, (GPIO_PUPD_NONE | GPIO_OSPEED_MEDIUM | GPIO_OTYPE_PUSHPULL | GPIO_MODE_OUTPUT));
	break ;

    default:
	// do nothing
	break ;
    }
}

void digitalWrite( uint32_t ulPin, uint32_t ulVal )
{
    // Handle the case the pin isn't usable as PIO
    if ( g_APinDescription[ulPin].GPIO == NULL )
    {
	return ;
    }

    GPIO_TypeDef *GPIO = g_APinDescription[ulPin].GPIO;
    uint32_t bit = g_APinDescription[ulPin].bit;

    if (ulVal == 0)
    {
	GPIO->BRR = bit;
    }
    else
    {
	GPIO->BSRR = bit;
    }
}

int digitalRead( uint32_t ulPin )
{
    // Handle the case the pin isn't usable as PIO
    if ( g_APinDescription[ulPin].GPIO == NULL )
    {
	return LOW ;
    }

    GPIO_TypeDef *GPIO = g_APinDescription[ulPin].GPIO;
    uint32_t bit = g_APinDescription[ulPin].bit;

    return !!(GPIO->IDR & bit);
}

#ifdef __cplusplus
}
#endif

