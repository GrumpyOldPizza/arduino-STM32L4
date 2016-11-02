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
#include "dosfs_api.h"

#ifdef __cplusplus
extern "C" {
#endif

stm32l4_adc_t stm32l4_adc;
stm32l4_exti_t stm32l4_exti;

void HardFault_Handler(void)
{
    while (1)
    {
#if defined(STM32L4_CONFIG_USBD_CDC)
#if defined(STM32L476xx)
	OTG_FS_IRQHandler();
#else
	USB_IRQHandler();
#endif
#endif
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
#if defined(STM32L4_CONFIG_USBD_CDC)
#if defined(STM32L476xx)
	OTG_FS_IRQHandler();
#else
	USB_IRQHandler();
#endif
#endif
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
#if defined(STM32L4_CONFIG_USBD_CDC)
#if defined(STM32L476xx)
	OTG_FS_IRQHandler();
#else
	USB_IRQHandler();
#endif
#endif
    }
}

void init( void )
{
#if defined(STM32L4_CONFIG_USBD_CDC) || defined(STM32L4_CONFIG_USBD_MSC)
    stm32l4_gpio_pin_configure(STM32L4_CONFIG_USBD_VUSB, (GPIO_PUPD_PULLDOWN | GPIO_OSPEED_LOW | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));

    armv7m_clock_spin(2000);

    if ((_SYSTEM_CORE_CLOCK_ < 16000000) && stm32l4_gpio_pin_read(STM32L4_CONFIG_USBD_VUSB))
    {
	stm32l4_system_configure(16000000, 8000000, 8000000);
    }
    else
#endif
    {
	stm32l4_system_configure(_SYSTEM_CORE_CLOCK_, _SYSTEM_CORE_CLOCK_/2, _SYSTEM_CORE_CLOCK_/2);
    }

    armv7m_svcall_initialize();
    armv7m_pendsv_initialize();
    armv7m_systick_initialize(STM32L4_SYSTICK_IRQ_PRIORITY);
    armv7m_timer_initialize();

    stm32l4_rtc_configure(STM32L4_RTC_IRQ_PRIORITY);

    stm32l4_exti_create(&stm32l4_exti, STM32L4_EXTI_IRQ_PRIORITY);
    stm32l4_exti_enable(&stm32l4_exti);

#if defined(STM32L4_CONFIG_USBD_CDC) || defined(STM32L4_CONFIG_USBD_MSC)
    if (stm32l4_system_hclk() >= 16000000)
    {
	USBD_Attach(STM32L4_CONFIG_USBD_VUSB, STM32L4_USB_IRQ_PRIORITY);
    }
#endif /* STM32L4_CONFIG_USBD_CDC */

#if defined(STM32L4_CONFIG_DOSFS_SDCARD)
    f_initvolume(&dosfs_sdcard_init, 0);
    f_checkvolume();
#endif /* STM32L4_CONFIG_DOSFS_SDCARD */
#if defined(STM32L4_CONFIG_DOSFS_SFLASH)
    f_initvolume(&dosfs_sflash_init, 0);
    f_checkvolume();
#endif /* STM32L4_CONFIG_DOSFS_SFLASH */
}

#ifdef __cplusplus
}
#endif
