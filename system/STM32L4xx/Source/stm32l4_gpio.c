/*
 * Copyright (c) 2014,2017 Thomas Roell.  All rights reserved.
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

#include "stm32l4_gpio.h"

#include "armv7m.h"

void stm32l4_gpio_pin_configure(unsigned int pin, unsigned int mode)
{
    GPIO_TypeDef *GPIO;
    uint32_t group, index, afsel;

    afsel = (pin >> 8) & 15;
    group = (pin >> 4) & 7;
    index = (pin >> 0) & 15;

    GPIO = (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * group);

    armv7m_atomic_or(&RCC->AHB2ENR, (RCC_AHB2ENR_GPIOAEN << group));

    /* First switch the pin back to analog mode */
    armv7m_atomic_or(&GPIO->MODER, (0x00000003 << (index << 1)));

#if defined(STM32L476xx)
    armv7m_atomic_and(&GPIO->ASCR, ~(0x00000001 << index));
#endif /* defined(STM32L476xx) */

    /* Set OPTYPER */
    armv7m_atomic_modify(&GPIO->OTYPER, (0x00000001 << index), (((mode & GPIO_OTYPE_MASK) >> GPIO_OTYPE_SHIFT) << index));

    /* Set OPSPEEDR */
    armv7m_atomic_modify(&GPIO->OSPEEDR, (0x00000003 << (index << 1)), (((mode & GPIO_OSPEED_MASK) >> GPIO_OSPEED_SHIFT) << (index << 1)));

    /* Set PUPD */
    armv7m_atomic_modify(&GPIO->PUPDR, (0x00000003 << (index << 1)), (((mode & GPIO_PUPD_MASK) >> GPIO_PUPD_SHIFT) << (index << 1)));

    /* Set AFRL/AFRH */
    armv7m_atomic_modify(&GPIO->AFR[index >> 3], (0x0000000f << ((index & 7) << 2)), (afsel << ((index & 7) << 2)));

    if ((mode & GPIO_MODE_MASK) == GPIO_MODE_ANALOG)
    {
#if defined(STM32L476xx)
	if (mode & GPIO_ANALOG_SWITCH)
	{
	    armv7m_atomic_or(&GPIO->ASCR, (0x00000001 << index));
	}
#endif /* defined(STM32L476xx) */
    }
    else
    {
	armv7m_atomic_modify(&GPIO->MODER, (0x00000003 << (index << 1)), (((mode & GPIO_MODE_MASK) >> GPIO_MODE_SHIFT) << (index << 1)));
    }
}

void stm32l4_gpio_pin_input(unsigned int pin)
{
    GPIO_TypeDef *GPIO;
    uint32_t group, index;

    group = (pin >> 4) & 7;
    index = (pin >> 0) & 15;

    GPIO = (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * group);

    armv7m_atomic_modify(&GPIO->MODER, (0x00000003 << (index << 1)), ((GPIO_MODE_INPUT >> GPIO_MODE_SHIFT) << (index << 1)));
}

void stm32l4_gpio_pin_output(unsigned int pin)
{
    GPIO_TypeDef *GPIO;
    uint32_t group, index;

    group = (pin >> 4) & 7;
    index = (pin >> 0) & 15;

    GPIO = (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * group);

    armv7m_atomic_modify(&GPIO->MODER, (0x00000003 << (index << 1)), ((GPIO_MODE_OUTPUT >> GPIO_MODE_SHIFT) << (index << 1)));
}

void stm32l4_gpio_pin_alternate(unsigned int pin)
{
    GPIO_TypeDef *GPIO;
    uint32_t group, index;

    group = (pin >> 4) & 7;
    index = (pin >> 0) & 15;

    GPIO = (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * group);

    armv7m_atomic_modify(&GPIO->MODER, (0x00000003 << (index << 1)), ((GPIO_MODE_ALTERNATE >> GPIO_MODE_SHIFT) << (index << 1)));
}
