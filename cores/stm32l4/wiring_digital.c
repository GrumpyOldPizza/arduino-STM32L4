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

