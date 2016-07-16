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

#include <stdlib.h>

#include "armv7m.h"

#include "stm32l4_flash.h"

static __attribute__((optimize("O3"), section(".ramfunc"), long_call)) void stm32l4_flash_erase_page(void)
{
    uint32_t primask;

    primask = __get_PRIMASK();

    __disable_irq();

    FLASH->CR |= FLASH_CR_STRT;

    while (FLASH->SR & FLASH_SR_BSY) 
    {
	continue;
    }

    __set_PRIMASK(primask);
}

static __attribute__((optimize("O3"), section(".ramfunc"), long_call)) void stm32l4_flash_program_standard(volatile uint32_t *flash, uint32_t count, const uint8_t *data)
{
    uint32_t primask;
    const uint8_t *data_e;

    data_e = data + count;

    do
    {
	primask = __get_PRIMASK();
	
	__disable_irq();

	FLASH->CR = FLASH_CR_PG;

	flash[0] = ((const uint32_t*)((const void*)data))[0];
	flash[1] = ((const uint32_t*)((const void*)data))[1];

	flash += 2;
	data  += 8;

	__DMB();

	while (FLASH->SR & FLASH_SR_BSY) 
	{
	    continue;
	}

	__set_PRIMASK(primask);

	if (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR))
	{
	    return;
	}
    }
    while (data != data_e);
}

static __attribute__((optimize("O3"), section(".ramfunc"), long_call)) void stm32l4_flash_program_fast(volatile uint32_t *flash, uint32_t count, const uint8_t *data)
{
    uint32_t primask;
    const uint8_t *data_r, *data_e;
    
    data_e = data + count;

    do
    {
	data_r = data + 256;

	primask = __get_PRIMASK();

	__disable_irq();

	FLASH->CR = FLASH_CR_FSTPG;

	do
	{
	    flash[0] = ((const uint32_t*)((const void*)data))[0];
	    flash[1] = ((const uint32_t*)((const void*)data))[1];

	    flash += 2;
	    data  += 8;

	    __DMB();
	}
	while (data != data_r);

	while (FLASH->SR & FLASH_SR_BSY) 
	{
	    continue;
	}

	__set_PRIMASK(primask);

	if (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR))
	{
	    return;
	}
    }
    while (data != data_e);
}

uint32_t stm32l4_flash_size(void)
{
    return *((volatile uint16_t*)0x1fff75e0) * 1024;
}

bool stm32l4_flash_unlock(void)
{
    uint32_t primask;

    if (!(FLASH->CR & FLASH_CR_LOCK))
    {
	return true;
    }

    primask = __get_PRIMASK();

    __disable_irq();

    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xcdef89ab;

    __set_PRIMASK(primask);

    return !!(FLASH->CR & FLASH_CR_LOCK);
}

void stm32l4_flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

bool stm32l4_flash_erase(uint32_t address, uint32_t count)
{
    uint32_t split;
    bool success = true;

    if (FLASH->CR & FLASH_CR_LOCK)
    {
	return false;
    }

    split = FLASH_BASE + (stm32l4_flash_size() >> 1);

    do
    {
	if (address < split)
	{
	    FLASH->CR = FLASH_CR_PER | ((((address - FLASH_BASE) / 2048) << 3) & FLASH_CR_PNB);
	}
	else
	{
	    FLASH->CR = FLASH_CR_PER | FLASH_CR_BKER | ((((address - split) / 2048) << 3) & FLASH_CR_PNB);
	}
	
	stm32l4_flash_erase_page();
	
	FLASH->CR = 0;
	
	if (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR))
	{
	    success = false;
	    
	    break;
	}
	
	address += 2048;
	count   -= 2048;
    }
    while (count);

    FLASH->SR = (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR);
    
    return success;
}

bool stm32l4_flash_program(uint32_t address, uint32_t count, const uint8_t *data)
{
    bool success = true;

    if (FLASH->CR & FLASH_CR_LOCK)
    {
	return false;
    }

    if ((address & 255) || (count & 255))
    {
	stm32l4_flash_program_standard((volatile uint32_t *)address, count, data);
    }
    else
    {
	stm32l4_flash_program_fast((volatile uint32_t *)address, count, data);
    }

    if (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR))
    {
	success = false;
    }
    
    FLASH->SR = (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR);
    
    return success;
}

