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

#ifdef __cplusplus
extern "C" {
#endif

stm32l4_exti_t stm32l4_exti;


void HardFault_Handler(void)
{
    while (1)
    {
	OTG_FS_IRQHandler();
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
	OTG_FS_IRQHandler();
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
	OTG_FS_IRQHandler();
    }
}

void init( void )
{
  stm32l4_system_configure(F_CPU, F_CPU, F_CPU/2, F_CPU/2);

  armv7m_svcall_initialize();
  armv7m_pendsv_initialize();
  armv7m_systick_initialize(STM32L4_SYSTICK_IRQ_PRIORITY);
  armv7m_timer_initialize();

  stm32l4_exti_create(&stm32l4_exti, STM32L4_EXTI_IRQ_PRIORITY);
  stm32l4_exti_enable(&stm32l4_exti);

#if 0
  // Setup all pins (digital and analog) in INPUT mode (default is nothing)
  for ( pin = 0 ; pin < NUM_TOTAL_PINS ; pin++ )
  {
      pinMode( pin, INPUT ) ;
  }
#endif
}

#ifdef __cplusplus
}
#endif
