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
#include "wiring_private.h"

#include <string.h>

/*
 * \brief Specifies a named Interrupt Service Routine (ISR) to call when an interrupt occurs.
 *        Replaces any previous function that was attached to the interrupt.
 */
void attachInterrupt(uint32_t pin, voidFuncPtr callback, uint32_t mode)
{
  if (!(g_APinDescription[pin].attr & PIN_ATTR_EXTI))
    return;

  switch (mode) {
    
  case CHANGE:
    stm32l4_exti_notify(&stm32l4_exti, g_APinDescription[pin].pin, EXTI_CONTROL_BOTH_EDGES, (stm32l4_exti_callback_t)callback, NULL);
    break;
    
  case FALLING:
    stm32l4_exti_notify(&stm32l4_exti, g_APinDescription[pin].pin, EXTI_CONTROL_FALLING_EDGE, (stm32l4_exti_callback_t)callback, NULL);
    break;
    
  case RISING:
    stm32l4_exti_notify(&stm32l4_exti, g_APinDescription[pin].pin, EXTI_CONTROL_RISING_EDGE, (stm32l4_exti_callback_t)callback, NULL);
    break;
  }
}

/*
 * \brief Turns off the given interrupt.
 */
void detachInterrupt(uint32_t pin)
{
  if (!(g_APinDescription[pin].attr & PIN_ATTR_EXTI))
    return;

  stm32l4_exti_notify(&stm32l4_exti, g_APinDescription[pin].pin, EXTI_CONTROL_DISABLE, NULL, NULL);
}

