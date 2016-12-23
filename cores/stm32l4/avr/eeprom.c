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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stm32l4xx.h"
#include "stm32l4_flash.h"

#include "avr/eeprom.h"

#define EEPROM_FLASH_MAGIC_1 0xaa55ee77
#define EEPROM_FLASH_MAGIC_2 0x77eeaa55

#define EEPROM_FLASH_SIZE    1024
#define EEPROM_FLASH_BANK    8192

static const uint8_t *eeprom_flash_data = NULL;

uint32_t eeprom_flash_bottom;
uint32_t eeprom_flash_top;
uint32_t eeprom_flash_slot;
uint32_t eeprom_flash_limit;


static void eeprom_flash_initialize(void)
{
    uint32_t sequence_1, sequence_2;
    uint8_t wdata[16];

    eeprom_flash_data = (const uint8_t*)FLASH_BASE + stm32l4_flash_size() - 2*EEPROM_FLASH_BANK;

    sequence_1 = 0;
    sequence_2 = 0;

    if ((eeprom_flash_data[1*EEPROM_FLASH_BANK -8] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  0)) &&
	(eeprom_flash_data[1*EEPROM_FLASH_BANK -7] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  8)) &&
	(eeprom_flash_data[1*EEPROM_FLASH_BANK -6] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 16)) &&
	(eeprom_flash_data[1*EEPROM_FLASH_BANK -5] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 24)) &&
	(eeprom_flash_data[1*EEPROM_FLASH_BANK -4] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  0)) &&
	(eeprom_flash_data[1*EEPROM_FLASH_BANK -3] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  8)) &&
	(eeprom_flash_data[1*EEPROM_FLASH_BANK -2] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 16)) &&
	(eeprom_flash_data[1*EEPROM_FLASH_BANK -1] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 24)))
    {
	sequence_1 = ((eeprom_flash_data[1*EEPROM_FLASH_BANK -16] <<  0) |
		      (eeprom_flash_data[1*EEPROM_FLASH_BANK -15] <<  8) |
		      (eeprom_flash_data[1*EEPROM_FLASH_BANK -14] << 16) |
		      (eeprom_flash_data[1*EEPROM_FLASH_BANK -13] << 24));
    }

    if ((eeprom_flash_data[2*EEPROM_FLASH_BANK -8] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  0)) &&
	(eeprom_flash_data[2*EEPROM_FLASH_BANK -7] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  8)) &&
	(eeprom_flash_data[2*EEPROM_FLASH_BANK -6] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 16)) &&
	(eeprom_flash_data[2*EEPROM_FLASH_BANK -5] == (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 24)) &&
	(eeprom_flash_data[2*EEPROM_FLASH_BANK -4] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  0)) &&
	(eeprom_flash_data[2*EEPROM_FLASH_BANK -3] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  8)) &&
	(eeprom_flash_data[2*EEPROM_FLASH_BANK -2] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 16)) &&
	(eeprom_flash_data[2*EEPROM_FLASH_BANK -1] == (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 24)))
    {
	sequence_2 = ((eeprom_flash_data[2*EEPROM_FLASH_BANK -16] <<  0) |
		      (eeprom_flash_data[2*EEPROM_FLASH_BANK -15] <<  8) |
		      (eeprom_flash_data[2*EEPROM_FLASH_BANK -14] << 16) |
		      (eeprom_flash_data[2*EEPROM_FLASH_BANK -13] << 24));
    }

    if ((sequence_1 == 0) && (sequence_2 == 0))
    {
	stm32l4_flash_unlock();

        stm32l4_flash_erase((uint32_t)eeprom_flash_data, EEPROM_FLASH_BANK);

	wdata[ 0] = (uint8_t)(1 >>  0);
	wdata[ 1] = (uint8_t)(1 >>  8);
	wdata[ 2] = (uint8_t)(1 >> 16);
	wdata[ 3] = (uint8_t)(1 >> 24);
	wdata[ 4] = 0x00;
	wdata[ 5] = 0x00;
	wdata[ 6] = 0x00;
	wdata[ 7] = 0x00;
	wdata[ 8] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  0);
	wdata[ 9] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  8);
	wdata[10] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 16);
	wdata[11] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 24);
	wdata[12] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  0);
	wdata[13] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  8);
	wdata[14] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 16);
	wdata[15] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 24);

	stm32l4_flash_program((uint32_t)eeprom_flash_data + 1*EEPROM_FLASH_BANK - 16, &wdata[0], 16);

	stm32l4_flash_lock();

	eeprom_flash_bottom = 0;
	eeprom_flash_top = EEPROM_FLASH_SIZE;
	eeprom_flash_slot = EEPROM_FLASH_SIZE;
	eeprom_flash_limit = EEPROM_FLASH_BANK - 16;
    }
    else
    {
	if (sequence_1 > sequence_2)
	{
	    eeprom_flash_bottom = 0;
	    eeprom_flash_top = EEPROM_FLASH_SIZE;
	    eeprom_flash_slot = EEPROM_FLASH_SIZE;
	    eeprom_flash_limit = EEPROM_FLASH_BANK - 16;
	}
	else
	{
	    eeprom_flash_bottom = EEPROM_FLASH_BANK + 0;
	    eeprom_flash_top = EEPROM_FLASH_BANK + EEPROM_FLASH_SIZE;
	    eeprom_flash_slot = EEPROM_FLASH_BANK + EEPROM_FLASH_SIZE;
	    eeprom_flash_limit = EEPROM_FLASH_BANK + EEPROM_FLASH_BANK - 16;
	}

	while (eeprom_flash_slot < eeprom_flash_limit)
	{
	    if (eeprom_flash_data[eeprom_flash_slot + 7] == 0xff)
	    {
		break;
	    }

	    eeprom_flash_slot += 8;
	}
    }
}

static void eeprom_flash_read(uint32_t offset, uint8_t *data)
{
    uint32_t index, slot;

    offset &= (EEPROM_FLASH_SIZE-1);

    if (eeprom_flash_data == NULL)
    {
	eeprom_flash_initialize();
    }

    for (slot = eeprom_flash_slot - 8; slot >= eeprom_flash_top; slot -= 8)
    {
	index = ((eeprom_flash_data[slot +4] << 0) | (eeprom_flash_data[slot +5] << 8));

	if (index == offset)
	{
	    data[0] = eeprom_flash_data[slot +0];
	    data[1] = eeprom_flash_data[slot +1];
	    data[2] = eeprom_flash_data[slot +2];
	    data[3] = eeprom_flash_data[slot +3];

	    return;
	}
    }

    data[0] = eeprom_flash_data[eeprom_flash_bottom +offset +0];
    data[1] = eeprom_flash_data[eeprom_flash_bottom +offset +1];
    data[2] = eeprom_flash_data[eeprom_flash_bottom +offset +2];
    data[3] = eeprom_flash_data[eeprom_flash_bottom +offset +3];
}

static void eeprom_flash_write(uint32_t offset, const uint8_t *data)
{
    uint32_t soffset, sbottom, sequence;
    uint8_t wdata[16];

    offset &= (EEPROM_FLASH_SIZE-1);

    if (eeprom_flash_data == NULL)
    {
	eeprom_flash_initialize();
    }

    stm32l4_flash_unlock();

    if (eeprom_flash_slot != eeprom_flash_limit)
    {
	wdata[0] = data[0];
	wdata[1] = data[1];
	wdata[2] = data[2];
	wdata[3] = data[3];
	wdata[4] = (uint8_t)(offset >> 0);
	wdata[5] = (uint8_t)(offset >> 8);
	wdata[6] = 0x00;
	wdata[7] = 0x00;

	stm32l4_flash_program((uint32_t)eeprom_flash_data + eeprom_flash_slot, &wdata[0], 8);

	eeprom_flash_slot += 8;
    }
    else
    {
	sbottom = eeprom_flash_bottom ^ EEPROM_FLASH_BANK;

	stm32l4_flash_erase((uint32_t)eeprom_flash_data + sbottom, EEPROM_FLASH_BANK);

	for (soffset = 0; soffset < EEPROM_FLASH_SIZE; soffset += 8)
	{
	    if (soffset == offset)
	    {
		wdata[0] = data[0];
		wdata[1] = data[1];
		wdata[2] = data[2];
		wdata[3] = data[3];
	    }
	    else
	    {
		eeprom_flash_read(soffset, &wdata[0]);
	    }

	    if ((soffset + 4) == offset)
	    {
		wdata[4] = data[0];
		wdata[5] = data[1];
		wdata[6] = data[2];
		wdata[7] = data[3];
	    }
	    else
	    {
		eeprom_flash_read(soffset +4, &wdata[4]);
	    }
	    
	    stm32l4_flash_program((uint32_t)eeprom_flash_data +sbottom +soffset, &wdata[0], 8);
	}

	sequence = ((eeprom_flash_data[eeprom_flash_limit +0] <<  0) |
		    (eeprom_flash_data[eeprom_flash_limit +1] <<  8) |
		    (eeprom_flash_data[eeprom_flash_limit +2] << 16) |
		    (eeprom_flash_data[eeprom_flash_limit +3] << 24));

	sequence++;

	wdata[ 0] = (uint8_t)(sequence >>  0);
	wdata[ 1] = (uint8_t)(sequence >>  8);
	wdata[ 2] = (uint8_t)(sequence >> 16);
	wdata[ 3] = (uint8_t)(sequence >> 24);
	wdata[ 4] = 0x00;
	wdata[ 5] = 0x00;
	wdata[ 6] = 0x00;
	wdata[ 7] = 0x00;
	wdata[ 8] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  0);
	wdata[ 9] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >>  8);
	wdata[10] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 16);
	wdata[11] = (uint8_t)(EEPROM_FLASH_MAGIC_1 >> 24);
	wdata[12] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  0);
	wdata[13] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >>  8);
	wdata[14] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 16);
	wdata[15] = (uint8_t)(EEPROM_FLASH_MAGIC_2 >> 24);

	stm32l4_flash_program((uint32_t)eeprom_flash_data +sbottom +EEPROM_FLASH_BANK - 16, &wdata[0], 16);

	eeprom_flash_bottom = sbottom;
	eeprom_flash_top = sbottom + EEPROM_FLASH_SIZE;
	eeprom_flash_slot = sbottom + EEPROM_FLASH_SIZE;
	eeprom_flash_limit = sbottom + EEPROM_FLASH_BANK - 16;
    }

    stm32l4_flash_lock();
}

uint8_t eeprom_read_byte(const uint8_t *address)
{
    uint8_t data;

    eeprom_read_block(&data, address, sizeof(data));

    return data;
}

uint16_t eeprom_read_word(const uint16_t *address)
{
    uint16_t data;

    eeprom_read_block(&data, address, sizeof(data));

    return data;
}

uint32_t eeprom_read_dword(const uint32_t *address)
{
    uint32_t data;

    eeprom_read_block(&data, address, sizeof(data));

    return data;
}

float eeprom_read_float(const float *address)
{
    float data;

    eeprom_read_block(&data, address, sizeof(data));

    return data;
}

void eeprom_read_block(void *data, const void *address, uint32_t count)
{
    uint32_t offset, index;
    uint8_t rdata[4];
    uint8_t *d;

    d      = (uint8_t*)data;
    offset = (uint32_t)address & ~3ul;
    index  = (uint32_t)address & 3ul;

    eeprom_flash_read(offset, &rdata[0]);

    while (count--)
    {
	if (index == 4)
	{
	    offset += 4;
	    index = 0;

	    eeprom_flash_read(offset, &rdata[0]);
	}

	*d++ = rdata[index++];
    }
}

void eeprom_write_byte(uint8_t *address, uint8_t data)
{
    eeprom_write_block(&data, address, sizeof(data));
}

void eeprom_write_word(uint16_t *address, uint16_t data)
{
    eeprom_write_block(&data, address, sizeof(data));
}

void eeprom_write_dword(uint32_t *address, uint32_t data)
{
    eeprom_write_block(&data, address, sizeof(data));
}

void eeprom_write_float(float *address, float data)
{
    eeprom_write_block(&data, address, sizeof(data));
}

void eeprom_write_block(const void *data, void *address, uint32_t count)
{
    uint32_t offset, index;
    uint8_t wdata[4];
    const uint8_t *s;
    bool update;

    s      = (const uint8_t*)data;
    offset = (uint32_t)address & ~3ul;
    index  = (uint32_t)address & 3ul;

    eeprom_flash_read(offset, &wdata[0]);

    update = false;

    while (count--)
    {
	if (index == 4)
	{
	    if (update)
	    {
		eeprom_flash_write(offset, &wdata[0]);

		update = false;
	    }

	    offset += 4;
	    index = 0;

	    eeprom_flash_read(offset, &wdata[0]);
	}

	if (wdata[index] != *s)
	{
	    wdata[index] = *s;
	  
	    update = true;
	}

	s++;
	index++;
    }

    if (update)
    {
	eeprom_flash_write(offset, &wdata[0]);
    }
}

int eeprom_is_ready(void)
{
    return 1;
}
