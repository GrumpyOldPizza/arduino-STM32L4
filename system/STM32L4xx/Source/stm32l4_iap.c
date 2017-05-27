/*
 * Copyright (c) 2017 Thomas Roell.  All rights reserved.
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

#include "stm32l4_system.h"
#include "stm32l4_iap.h"

extern const stm32l4_iap_prefix_t stm32l4_iap_prefix;

#if defined(STM32L432xx)
static const char stm32l4_iap_signature[11] = "STM32L432xx";
#endif
#if defined(STM32L433xx)
static const char stm32l4_iap_signature[11] = "STM32L433xx";
#endif
#if defined(STM32L476xx)
static const char stm32l4_iap_signature[11] = "STM32L476xx";
#endif
#if defined(STM32L496xx)
static const char stm32l4_iap_signature[11] = "STM32L496xx";
#endif

/*
 * void stm32l4_iap_do_erase(void)
 * {
 *     uint32_t flash_sr;
 * 
 *     FLASH->CR |= FLASH_CR_STRT;
 * 
 *     do
 *     {
 *         flash_sr = FLASH->SR;
 *     }
 *     while (flash_sr & FLASH_SR_BSY);
 * 
 *     FLASH->CR = 0;
 * }
 */

#define STM32L4_IAP_DO_ERASE_SIZE 18

static const uint16_t stm32l4_iap_do_erase[STM32L4_IAP_DO_ERASE_SIZE] = {
    0xf44f, 0x5300,  //  mov.w   r3, #8192       ; 0x2000
    0xf2c4, 0x0302,  //  movt    r3, #16386      ; 0x4002
    0x695a,          //  ldr     r2, [r3, #20]
    0xf442, 0x3280,  //  orr.w   r2, r2, #65536  ; 0x10000
    0x615a,          //  str     r2, [r3, #20]
    0xf44f, 0x5300,  //  mov.w   r3, #8192       ; 0x2000
    0xf2c4, 0x0302,  //  movt    r3, #16386      ; 0x4002
    0x691a,          //  ldr     r2, [r3, #16]
    0xf412, 0x3280,  //  ands.w  r2, r2, #65536  ; 0x10000
    0xd1f7,          //  bne.n   10 <stm32l4_iap_do_erase+0x10>
    0x615a,          //  str     r2, [r3, #20]
    0x4770,          //  bx      lr
};

/*
 * void stm32l4_iap_do_program(volatile uint32_t *flash, const uint8_t *data, const uint8_t *data_e)
 * {
 *     uint32_t flash_sr;
 * 
 *     do
 *     {
 *         FLASH->CR = FLASH_CR_PG;
 * 
 *         flash[0] = ((const uint32_t*)((const void*)data))[0];
 *         flash[1] = ((const uint32_t*)((const void*)data))[1];
 * 
 *         flash += 2;
 *         data  += 8;
 * 
 *         __DMB();
 * 
 *         do
 *         {
 *             flash_sr = FLASH->SR;
 *         }
 *         while (flash_sr & FLASH_SR_BSY); 
 *             
 *         if (flash_sr & (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR))
 *         {
 *             FLASH->CR = 0;
 * 
 *             return;
 *         }
 *     }
 *     while (data != data_e);
 * 
 *     FLASH->CR = 0;
 * }
 */

#define STM32L4_IAP_DO_PROGRAM_SIZE 34

static const uint16_t stm32l4_iap_do_program[STM32L4_IAP_DO_PROGRAM_SIZE] = {

    0xb430,          //  push    {r4, r5}
    0xf44f, 0x5300,  //  mov.w   r3, #8192       ; 0x2000
    0xf2c4, 0x0302,  //  movt    r3, #16386      ; 0x4002
    0x2401,          //  movs    r4, #1
    0x615c,          //  str     r4, [r3, #20]
    0x680b,          //  ldr     r3, [r1, #0]
    0x6003,          //  str     r3, [r0, #0]
    0x684b,          //  ldr     r3, [r1, #4]
    0x6043,          //  str     r3, [r0, #4]
    0x3108,          //  adds    r1, #8
    0x3008,          //  adds    r0, #8
    0xf3bf, 0x8f5f,  //  dmb     sy
    0xf44f, 0x5400,  //  mov.w   r4, #8192       ; 0x2000
    0xf2c4, 0x0402,  //  movt    r4, #16386      ; 0x4002
    0x6923,          //  ldr     r3, [r4, #16]
    0xf413, 0x3580,  //  ands.w  r5, r3, #65536  ; 0x10000
    0xd1f7,          //  bne.n   1e <stm32l4_iap_do_program+0x1e>
    0xf013, 0x03f8,  //  ands.w  r3, r3, #248    ; 0xf8
    0xd104,          //  bne.n   3e <stm32l4_iap_do_program+0x3e>
    0x4291,          //  cmp     r1, r2
    0xd1e4,          //  bne.n   2 <stm32l4_iap_do_program+0x2>
    0x6163,          //  str     r3, [r4, #20]
    0xbc30,          //  pop     {r4, r5}
    0x4770,          //  bx      lr
    0x6165,          //  str     r5, [r4, #20]
    0xbc30,          //  pop     {r4, r5}
    0x4770,          //  bx      lr
};

typedef void (*stm32l4_iap_do_erase_t)(void);
typedef void (*stm32l4_iap_do_program_t)(volatile uint32_t*, const uint32_t*, const uint32_t*);

static void stm32l4_iap_erase(uint32_t address, uint32_t address_e)
{
    unsigned int i;
    const uint32_t flash_base = FLASH_BASE;
#if defined(STM32L476xx) || defined(STM32L496xx)
    const uint32_t flash_size = (*((volatile uint16_t*)0x1fff75e0) * 1024);
    const uint32_t flash_split = (flash_base + (flash_size >> 1));
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
    uint16_t iap_do_erase[STM32L4_IAP_DO_ERASE_SIZE];

    for (i = 0; i < STM32L4_IAP_DO_ERASE_SIZE; i++)
    {
	iap_do_erase[i] = stm32l4_iap_do_erase[i];
    }

    FLASH->SR = (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR);
    
    do
    {
#if defined(STM32L476xx) || defined(STM32L496xx)
	if (address >= flash_split)
	{
	    FLASH->CR = FLASH_CR_PER | FLASH_CR_BKER | ((((address - flash_split) / 2048) << 3) & FLASH_CR_PNB);
	}
	else
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
	{
	    FLASH->CR = FLASH_CR_PER | ((((address - flash_base) / 2048) << 3) & FLASH_CR_PNB);
	}
	
	/* The "+1" is needed to force a branch to thumb code */
	(*((stm32l4_iap_do_erase_t)(((uint32_t)&iap_do_erase[0])+1)))();

	/* Hang on erase error. */
	while (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR))
	{
	    continue;
	}
	
	address += 2048;
    }
    while (address != address_e);
}

static void stm32l4_iap_program(uint32_t address, uint32_t address_e, const uint32_t *data)
{
    unsigned int i, chunk;
    uint16_t iap_do_program[STM32L4_IAP_DO_PROGRAM_SIZE];
    uint32_t iap_data[512];

    for (i = 0; i < STM32L4_IAP_DO_PROGRAM_SIZE; i++)
    {
	iap_do_program[i] = stm32l4_iap_do_program[i];
    }

    FLASH->SR = (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR);
    
    do
    {
	chunk = (address_e - address) / 4;

	if (chunk > 512)
	{
	    chunk = 512;
	}

	for (i = 0; i < chunk; i++)
	{
	    iap_data[i] = *data++;
	}

	/* The "+1" is needed to force a branch to thumb code */
	(*((stm32l4_iap_do_program_t)(((uint32_t)&iap_do_program[0])+1)))((volatile uint32_t*)address, &iap_data[0], &iap_data[chunk]);

	/* Hang on program error. */
	while (FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR))
	{
	    continue;
	}
	
	address += (chunk * 4);
    }
    while (address != address_e);
}

void stm32l4_iap(void)
{
    unsigned int i;
    uint32_t flash_acr, address, address_e, crc;
    const uint8_t *page, *page_e;
    const uint32_t *data, *data_e;
    const stm32l4_iap_info_t *info;
    const stm32l4_iap_prefix_t *prefix;
    const stm32l4_iap_suffix_t *suffix;
    const uint32_t flash_base = FLASH_BASE;
    const uint32_t flash_size = (*((volatile uint16_t*)0x1fff75e0) * 1024);

    prefix = &stm32l4_iap_prefix;

    page = (const uint8_t*)(flash_base + 4096);
    page_e = (const uint8_t*)(flash_base + flash_size);

    do
    {
	info = (const stm32l4_iap_info_t *)(page + 448);

	for (i = 0; i < 11; i++)
	{
	    if (info->signature[i] != stm32l4_iap_signature[i])
	    {
		break;
	    }
	}
	
	if ((i == 11) && 
	    (info->length == 24) &&
	    (info->address == (flash_base + 2048)) &&
	    ((info->address + info->size) <= (uint32_t)page))
	{
	    suffix = (const stm32l4_iap_suffix_t *)(page + info->size);

	    if (((prefix->bcdDevice == 0xffff) || (suffix->bcdDevice == prefix->bcdDevice)) &&
		(suffix->idProduct == prefix->idProduct) &&
		(suffix->idVendor == prefix->idVendor) &&
		(suffix->bcdDFU == 0x0100) &&
		(suffix->ucDfuSeSignature[0] == 'U') &&
		(suffix->ucDfuSeSignature[1] == 'F') &&
		(suffix->ucDfuSeSignature[2] == 'D') &&
		(suffix->bLength == 16))
	    {
		/* Here a valid iap_info and avalid iap_suffix had been found.
		 * Checksum the image and compare the value to the CRC in the
		 * suffix.
		 */

		RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

		data = (const uint32_t*)page;
		data_e = (const uint32_t*)(page + info->size + 12);

		CRC->INIT = 0xffffffff;
		CRC->POL  = 0x04c11db7;
		CRC->CR   = CRC_CR_RESET | CRC_CR_REV_IN_0 | CRC_CR_REV_IN_1 | CRC_CR_REV_OUT;

		do
		{
		    CRC->DR = *data++;
		}
		while (data != data_e);

		crc = CRC->DR;

		RCC->AHB1ENR &= ~RCC_AHB1ENR_CRCEN;

		if (suffix->dwCRC == crc)
		{
		    flash_acr = FLASH->ACR;

		    FLASH->ACR = flash_acr & ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN);
		  
		    FLASH->KEYR = 0x45670123;
		    FLASH->KEYR = 0xcdef89ab;

		    /* Erase the image address range
		     */

		    address = (flash_base + 2048);
		    address_e = (address + info->size + 2047) & ~2047;

		    stm32l4_iap_erase(address, address_e);

		    /* Program the image address range
		     */
		    
		    data = (const uint32_t*)page;
		    address = (FLASH_BASE + 2048);
		    address_e = (address + info->size + 7) & ~7;

		    stm32l4_iap_program(address, address_e, data);
		    
		    /* Erase the update address range
		     */
		    
		    address = (uint32_t)page;
		    address_e = (address + info->size + 16 + 2047) & ~2047;

		    stm32l4_iap_erase(address, address_e);
		    
		    FLASH->CR |= FLASH_CR_LOCK;

		    FLASH->ACR = (flash_acr & ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN)) | (FLASH_ACR_ICRST | FLASH_ACR_DCRST);
		    FLASH->ACR = flash_acr;
			       
		    return;
		}
	    }
	}
	
	page += 2048;
    }
    while (page != page_e);
}

