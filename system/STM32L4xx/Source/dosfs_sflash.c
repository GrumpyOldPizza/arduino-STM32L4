/*
 * Copyright (c) 2016-2017 Thomas Roell.  All rights reserved.
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

#if defined(STM32L476xx) || defined(STM32L496xx)

#include "dosfs_sflash.h"

#include <stdio.h>

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
#include <assert.h>
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

dosfs_sflash_t dosfs_sflash;

static uint32_t dosfs_sflash_cache[2 * (DOSFS_SFLASH_BLOCK_SIZE / sizeof(uint32_t))];

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
static uint8_t sflash_data_shadow[DOSFS_SFLASH_DATA_SIZE]; /* 16 MB */
static uint16_t sflash_xlate_shadow[DOSFS_SFLASH_DATA_SIZE / DOSFS_SFLASH_BLOCK_SIZE];
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

#if (DOSFS_CONFIG_SFLASH_SIMULATE == 0)

#include "stm32l4_gpio.h"

static uint32_t dosfs_sflash_nor_identify(dosfs_sflash_t *sflash)
{
    uint8_t temp[2], rx_data[256];
    uint32_t address, address_bits, data_size;
    unsigned int n, nph, length, revision;
    stm32l4_qspi_pins_t qspi_pins;

    data_size = 0;

#if 0
    qspi_pins.clk = GPIO_PIN_PE10_QUADSPI_CLK;
    qspi_pins.ncs = GPIO_PIN_PE11_QUADSPI_BK1_NCS;
    qspi_pins.io0 = GPIO_PIN_PE12_QUADSPI_BK1_IO0;
    qspi_pins.io1 = GPIO_PIN_PE13_QUADSPI_BK1_IO1;
    qspi_pins.io2 = GPIO_PIN_PE14_QUADSPI_BK1_IO2;
    qspi_pins.io3 = GPIO_PIN_PE15_QUADSPI_BK1_IO3;
#endif

    qspi_pins.clk = GPIO_PIN_PB10_QUADSPI_CLK;
    qspi_pins.ncs = GPIO_PIN_PB11_QUADSPI_BK1_NCS;
    qspi_pins.io0 = GPIO_PIN_PB1_QUADSPI_BK1_IO0;
    qspi_pins.io1 = GPIO_PIN_PB0_QUADSPI_BK1_IO1;
    qspi_pins.io2 = GPIO_PIN_PA7_QUADSPI_BK1_IO2;
    qspi_pins.io3 = GPIO_PIN_PA6_QUADSPI_BK1_IO3;

    // ### FIXME: IRQ LEVEL needs to be adjusted !!!
    stm32l4_qspi_create(&sflash->qspi, QSPI_INSTANCE_QUADSPI, &qspi_pins, 11, QSPI_MODE_DMA);
    stm32l4_qspi_enable(&sflash->qspi, 40000000, QSPI_OPTION_MODE_3, NULL, NULL, 0);

    stm32l4_qspi_select(&sflash->qspi);
    stm32l4_qspi_mode(&sflash->qspi, 0);

    stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDP, 0);
    
    stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDID, 0x000000, &rx_data[0], 3, 0);

    sflash->mid = rx_data[0];
    sflash->did = (rx_data[1] << 8) | (rx_data[2] << 0);

    switch (sflash->mid) {
    case 0x01:  /* SPANSION */
	sflash->features |= DOSFS_SFLASH_FEATURE_RDSR;
	break;
	
    case 0x20:  /* MICRON */
	sflash->features |= DOSFS_SFLASH_FEATURE_RDFS;
	break;
	
    case 0xc2:  /* MACRONIX */
	sflash->features |= DOSFS_SFLASH_FEATURE_RDSCUR;
	break;
	
    default:
	break;
    }
	  
    stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSFDP, 0x000000, &rx_data[0], 8, 0);

    if ((rx_data[0] == 0x53) && (rx_data[1] == 0x46) && (rx_data[2] == 0x44) && (rx_data[3] == 0x50))
    {
	nph = rx_data[6];

	revision = 0;
	length = 0;
	address = 0;

	for (n = 0; n <= nph; n++)
	{
	    stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSFDP, 0x000008 + (n * 8), &rx_data[0], 8, 0);

	    if ((rx_data[0] == 0x00) && (rx_data[7] == 0xff) &&  (rx_data[2] == 0x01) && (rx_data[1] >= revision) && (rx_data[3] != 0x00))
	    {
		revision = rx_data[1];
		length = rx_data[3];
		address = (rx_data[4] << 0) | (rx_data[5] << 8) | (rx_data[6] << 16);
	    }
	}

	if (length != 0)
	{
	    stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSFDP, address, &rx_data[0], length * 4, 0);

	    if (length >= 9)
	    {
		/* SDFP JESD216, 9 DWORDs
		 */
			  
		if ((rx_data[2] & 0x06) == 0x04)
		{
		    address_bits = QSPI_COMMAND_ADDRESS_32BIT;
		}
		else
		{
		    address_bits = QSPI_COMMAND_ADDRESS_24BIT;
		}

		if (!(rx_data[0] & 0x10))
		{
		    /* Need to issue a 0x50 before updating volatile status bits.
		     * That seems to be needed for WINBOND to update at least the
		     * QE bit.
		     */
		    sflash->features |= DOSFS_SFLASH_FEATURE_SR_VOLATILE_SELECT;
		}

		if (length >= 16)
		{
		    if (rx_data[52] & 0x08)
		    {
			sflash->features |= DOSFS_SFLASH_FEATURE_RDFS;
		    }

		    if (rx_data[58] & 0x80)
		    {
			sflash->features |= DOSFS_SFLASH_FEATURE_HOLD_RESET_DISABLE;
		    }

		    switch (rx_data[58] & 0x70) {
		    case 0x00:
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_AUTO;
			break;

		    case 0x10:
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE_RESET;
			break;

		    case 0x20:
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_SR_6;
			break;

		    case 0x30:
			/* I could not locate any datasheet that would list any
			 * device doing this ...
			 * 
			 * sflash_features |= SFLASH_FEATURE_QE_CR_7;
			 */
			break;

		    case 0x40:
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE;
			break;

		    case 0x50:
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_CR_1;
			break;

		    default:
			break;
		    }
		}
		else
		{
		    switch (sflash->mid) {
		    case 0x20:  /* MICRON */
			sflash->features |= (DOSFS_SFLASH_FEATURE_HOLD_RESET_DISABLE | DOSFS_SFLASH_FEATURE_QE_AUTO | DOSFS_SFLASH_FEATURE_RDFS);
			break;
			  
		    case 0xc2:  /* MACRONIX */
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_SR_6;
			break;
			  
		    case 0xef:  /* WINBOND */
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE;
			break;

		    case 0x01:  /* SPANSION */
		    case 0xbf:  /* MICROCHIP */
			sflash->features |= DOSFS_SFLASH_FEATURE_QE_CR_1;
			break;
			  
		    default:
			break;
		    }
		}

		/* Here the chip might still be in a PROGRAM/ERASE operation. So wait till that
		 * is finished before setting up before setting up the rest.
		 */

		if (sflash->features & DOSFS_SFLASH_FEATURE_RDFS)
		{
		    do 
		    {
			stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDFS, 0x0000000, &temp[0], 1, 0);
		    }
		    while (!(temp[0] & 0x80));
		}
		else
		{
		    do 
		    {
			stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x0000000, &temp[0], 1, 0);
		    }
		    while (temp[0] & 0x01);
		}

		if (sflash->features & (DOSFS_SFLASH_FEATURE_HOLD_RESET_DISABLE | DOSFS_SFLASH_FEATURE_QE_AUTO | DOSFS_SFLASH_FEATURE_QE_SR_6 | DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE_RESET | DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE | DOSFS_SFLASH_FEATURE_QE_CR_1))
		{
		    if (sflash->features & DOSFS_SFLASH_FEATURE_HOLD_RESET_DISABLE)
		    {
			stm32l4_qspi_receive(&sflash->qspi, (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x65), 0x000000, &temp[0], 1, 0);
			  
			temp[0] &= ~0x10;
			  
			stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN, 0);
			stm32l4_qspi_transmit(&sflash->qspi, (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x61), 0x000000, &temp[0], 1, 0);
		    }


		    if (sflash->features & DOSFS_SFLASH_FEATURE_QE_SR_6)
		    {
			stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x000000, &temp[0], 1, 0);

			if (!(temp[0] & 0x40))
			{
			    temp[0] |= 0x40;
			      
			    stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN, 0);
			    stm32l4_qspi_transmit(&sflash->qspi, DOSFS_SFLASH_COMMAND_WRSR, 0x000000, &temp[0], 1, 0);

			    do
			    {
				stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x000000, &temp[0], 1, 0);
			    }
			    while (temp[0] & 0x01);
			}
		    }

		    if (sflash->features & (DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE_RESET | DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE))
		    {
			stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x000000, &temp[0], 1, 0);
			stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDCR, 0x000000, &temp[1], 1, 0);
			  
			temp[1] |= 0x02;
			      
			stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN_VOLATILE, 0);
			stm32l4_qspi_transmit(&sflash->qspi, DOSFS_SFLASH_COMMAND_WRSR, 0x000000, &temp[0], 2, 0);
		    }

		    if (sflash->features & DOSFS_SFLASH_FEATURE_QE_CR_1)
		    {
			stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x000000, &temp[0], 1, 0);
			stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDCR, 0x000000, &temp[1], 1, 0);

			if (!(temp[1] & 0x02))
			{
			    temp[1] |= 0x02;
			      
			    stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN, 0);
			    stm32l4_qspi_transmit(&sflash->qspi, DOSFS_SFLASH_COMMAND_WRSR, 0x000000, &temp[0], 2, 0);

			    do
			    {
				stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x000000, &temp[0], 1, 0);
			    }
			    while (temp[0] & 0x01);
			}
		    }
		}
		else
		{
		    rx_data[2] &= ~0x60;
		}

		/* SDFP does not address the presence of 0x32 (or 0x38), nor does it contain any way to
		 * expose a 1_1_4 or 1_4_4 page program command. The table below is expirically collected
		 * by looking at all the vendors datasheet and verify that if SFDP and QUAD mode are present
		 * then always 0x32/0x38 are present, too.
		 *
		 * Spansion for example has devices that do not support 0x32, while others do.
		 */

		if (rx_data[2] & 0x60)
		{
		    switch (sflash->mid) {
		    case 0x20:  /* MICRON */
		    case 0xef:  /* WINBOND */
			sflash->command_program = (address_bits | QSPI_COMMAND_DATA_QUAD | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x32);
			break;
			      
		    case 0xc2:  /* MACRONIX */
			sflash->command_program = (address_bits | QSPI_COMMAND_DATA_QUAD | QSPI_COMMAND_ADDRESS_QUAD | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x38);
			break;
			      
		    default:
			break;
		    }
		}

		if (!sflash->command_program)
		{
		    sflash->command_program = (address_bits | QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x02);
		}

		if ((rx_data[2] & 0x20) && !sflash->command_read)
		{
		    /* FAST READ 1-4-4
		     */
			  
		    if (rx_data[8] & 0xe0)
		    {
			if ((rx_data[8] & 0xe0) <= 0x40)
			{
			    sflash->command_read = ((((rx_data[8] & 0x1f) + (rx_data[8] >> 5) -2) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						    address_bits |
						    QSPI_COMMAND_DATA_QUAD | QSPI_COMMAND_MODE_QUAD | QSPI_COMMAND_ADDRESS_QUAD | QSPI_COMMAND_INSTRUCTION_SINGLE |
						    rx_data[9]);
			}
		    }
		    else
		    {
			sflash->command_read = (((rx_data[8] & 0x1f) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						address_bits |
						QSPI_COMMAND_DATA_QUAD | QSPI_COMMAND_ADDRESS_QUAD | QSPI_COMMAND_INSTRUCTION_SINGLE |
						rx_data[9]);
		    }
		}

		if ((rx_data[2] & 0x40) && !sflash->command_read)
		{
		    /* FAST READ 1-1-4
		     */

		    if (rx_data[10] & 0xe0)
		    {
			sflash->command_read = ((((rx_data[10] & 0x1f) + (rx_data[10] >> 5) -8) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						address_bits |
						QSPI_COMMAND_DATA_QUAD | QSPI_COMMAND_MODE_SINGLE | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE |
						rx_data[11]);
		    }
		    else
		    {
			sflash->command_read = (((rx_data[10] & 0x1f) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						address_bits |
						QSPI_COMMAND_DATA_QUAD | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE |
						rx_data[11]);
		    }
			  
		}

		if ((rx_data[2] & 0x10) && !sflash->command_read)
		{
		    /* FAST READ 1-2-2
		     */
			  
		    if (rx_data[14] & 0xe0)
		    {
			if ((rx_data[14] & 0xe0) <= 0x80)
			{
			    sflash->command_read = ((((rx_data[14] & 0x1f) + (rx_data[14] >> 5) -4) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						    address_bits |
						    QSPI_COMMAND_DATA_DUAL | QSPI_COMMAND_MODE_DUAL | QSPI_COMMAND_ADDRESS_DUAL | QSPI_COMMAND_INSTRUCTION_SINGLE |
						    rx_data[15]);
			}
		    }
		    else
		    {
			sflash->command_read = (((rx_data[14] & 0x1f) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						address_bits |
						QSPI_COMMAND_DATA_DUAL | QSPI_COMMAND_ADDRESS_DUAL | QSPI_COMMAND_INSTRUCTION_SINGLE |
						rx_data[15]);
		    }
		}

		if ((rx_data[2] & 0x01) && !sflash->command_read)
		{
		    /* FAST READ 1-1-2
		     */

		    if (rx_data[12] & 0xe0)
		    {
			sflash->command_read = ((((rx_data[12] & 0x1f) + (rx_data[12] >> 5) -8) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						address_bits |
						QSPI_COMMAND_DATA_DUAL | QSPI_COMMAND_MODE_SINGLE | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE |
						rx_data[13]);
		    }
		    else
		    {
			sflash->command_read = (((rx_data[12] & 0x1f) << QSPI_COMMAND_WAIT_STATES_SHIFT) |
						address_bits |
						QSPI_COMMAND_DATA_DUAL | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE |
						rx_data[13]);
		    }
		}
		      
		if (!sflash->command_read)
		{
		    /* FAST READ 1-1-1
		     */

		    sflash->command_read = ((8 << QSPI_COMMAND_WAIT_STATES_SHIFT) |
					    address_bits |
					    QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x0b);
		}

		if (rx_data[28] == 0x10)
		{
		    sflash->command_erase = (QSPI_COMMAND_ADDRESS_SINGLE | address_bits | QSPI_COMMAND_INSTRUCTION_SINGLE | rx_data[29]);
		}

		if (rx_data[30] == 0x10)
		{
		    sflash->command_erase = (QSPI_COMMAND_ADDRESS_SINGLE | address_bits | QSPI_COMMAND_INSTRUCTION_SINGLE | rx_data[31]);
		}

		if (rx_data[32] == 0x10)
		{
		    sflash->command_erase = (QSPI_COMMAND_ADDRESS_SINGLE | address_bits | QSPI_COMMAND_INSTRUCTION_SINGLE | rx_data[33]);
		}

		if (rx_data[34] == 0x10)
		{
		    sflash->command_erase = (QSPI_COMMAND_ADDRESS_SINGLE | address_bits | QSPI_COMMAND_INSTRUCTION_SINGLE | rx_data[35]);
		}

		if (rx_data[7] & 0x80)
		{
		    data_size = 0x00000001 << (rx_data[4] - 3);
		}
		else
		{
		    data_size = (((rx_data[4] << 0) | (rx_data[5] << 8) | (rx_data[6] << 16) | (rx_data[7] << 24)) >> 3) + 1;
		}
	    }
	}
    }

    sflash->features &= ~DOSFS_SFLASH_FEATURE_RDFS;

#if 0
    sflash->command_program = (QSPI_COMMAND_ADDRESS_24BIT | QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x02);

    sflash->command_read = ((8 << QSPI_COMMAND_WAIT_STATES_SHIFT) |
			    QSPI_COMMAND_ADDRESS_24BIT |
			    QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x0b);
#endif

    stm32l4_qspi_unselect(&sflash->qspi);

    return data_size;
}

static bool dosfs_sflash_nor_erase(dosfs_sflash_t *sflash, uint32_t address)
{
    uint8_t temp[1];

    if (sflash->address != (address >> 24))
    {
	sflash->address = (address >> 24);

	DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_address);
    }

    DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_erase);
    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_nor_erase, DOSFS_SFLASH_ERASE_SIZE);

    if (sflash->features & DOSFS_SFLASH_FEATURE_RDFS)
    {
	stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_CLFS, 0);

	stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN, 0);

	stm32l4_qspi_command(&sflash->qspi, sflash->command_erase, address);

	stm32l4_qspi_wait(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDFS, 0x000000, &temp[0], 1, 0x80, 0x80, QSPI_CONTROL_ASYNC);

	while (!stm32l4_qspi_done(&sflash->qspi)) { }

	if (temp[0] & 0x20)
	{
	    DOSFS_SFLASH_STATISTICS_COUNT(sflash_nor_efail);
	}

	return (!(temp[0] & 0x20));
    }
    else
    {
	stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN, 0);

	stm32l4_qspi_command(&sflash->qspi, sflash->command_erase, address);

	stm32l4_qspi_wait(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x000000, &temp[0], 1, 0x01, 0x00, QSPI_CONTROL_ASYNC);

	while (!stm32l4_qspi_done(&sflash->qspi)) { }

	if (sflash->features & DOSFS_SFLASH_FEATURE_RDSR)
	{
	    return (!(temp[0] & 0x20));
	}
	else
	{
	    if (sflash->features & DOSFS_SFLASH_FEATURE_RDSCUR)
	    {
		stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSCUR, 0x000000, &temp[0], 1, 0);
		
		return (!(temp[0] & 0x40));
	    }
	    else
	    {
		return true;
	    }
	}
    }
}

static bool dosfs_sflash_nor_write(dosfs_sflash_t *sflash, uint32_t address, uint32_t count, const uint8_t *data)
{
    uint8_t temp[1];

    if (sflash->address != (address >> 24))
    {
	sflash->address = (address >> 24);

	DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_address);
    }

    DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_write);
    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_nor_write, count);

    if (sflash->features & DOSFS_SFLASH_FEATURE_RDFS)
    {
	stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_CLFS, 0);

	stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN, 0);

	if (count > 8)
	{
	    stm32l4_qspi_transmit(&sflash->qspi, sflash->command_program, address, data, count, QSPI_CONTROL_ASYNC);    

	    while (!stm32l4_qspi_done(&sflash->qspi)) { }
	}
	else
	{
	    stm32l4_qspi_transmit(&sflash->qspi, sflash->command_program, address, data, count, 0);    
	}
	
	stm32l4_qspi_wait(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDFS, 0x000000, &temp[0], 1, 0x80, 0x80, QSPI_CONTROL_ASYNC);

	while (!stm32l4_qspi_done(&sflash->qspi)) { }

	if (temp[0] & 0x10)
	{
	    DOSFS_SFLASH_STATISTICS_COUNT(sflash_nor_pfail);
	}

	return (!(temp[0] & 0x10));
    }
    else
    {
	stm32l4_qspi_command(&sflash->qspi, DOSFS_SFLASH_COMMAND_WREN, 0);

	if (count > 8)
	{
	    stm32l4_qspi_transmit(&sflash->qspi, sflash->command_program, address, data, count, QSPI_CONTROL_ASYNC);    

	    while (!stm32l4_qspi_done(&sflash->qspi)) { }
	}
	else
	{
	    stm32l4_qspi_transmit(&sflash->qspi, sflash->command_program, address, data, count, 0);    
	}

	stm32l4_qspi_wait(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSR, 0x000000, &temp[0], 1, 0x01, 0x00, QSPI_CONTROL_ASYNC);

	while (!stm32l4_qspi_done(&sflash->qspi)) { }

	if (sflash->features & DOSFS_SFLASH_FEATURE_RDSR)
	{
	    return (!(temp[0] & 0x40));
	}
	else
	{
	    if (sflash->features & DOSFS_SFLASH_FEATURE_RDSCUR)
	    {
		stm32l4_qspi_receive(&sflash->qspi, DOSFS_SFLASH_COMMAND_RDSCUR, 0x000000, &temp[0], 1, 0);
		
		return (!(temp[0] & 0x20));
	    }
	    else
	    {
		return true;
	    }
	}
    }
}

static void dosfs_sflash_nor_read(dosfs_sflash_t *sflash, uint32_t address, uint32_t count, uint8_t *data)
{
    if (sflash->address != (address >> 24))
    {
	sflash->address = (address >> 24);

	DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_address);
    }

    DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_read);
    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_nor_read, count);

    if (count > 8)
    {
	stm32l4_qspi_receive(&sflash->qspi, sflash->command_read, address, data, count, QSPI_CONTROL_ASYNC);    

	while (!stm32l4_qspi_done(&sflash->qspi)) { }
    }
    else
    {
	stm32l4_qspi_receive(&sflash->qspi, sflash->command_read, address, data, count, 0);
    }
}

#else /* (DOSFS_CONFIG_SFLASH_SIMULATE == 0) */

static uint32_t dosfs_sflash_nor_identify(dosfs_sflash_t *sflash)
{
    if (sflash->image == NULL)
    {
	sflash->image = (uint8_t*)malloc(DOSFS_CONFIG_SFLASH_SIMULATE_DATA_SIZE);
    }

    sflash->address  = 0;

    return sflash->image ? DOSFS_CONFIG_SFLASH_SIMULATE_DATA_SIZE : 0;
}

static void dosfs_sflash_nor_erase(dosfs_sflash_t *sflash, uint32_t address)
{
    if (sflash->address != (address >> 24))
    {
	sflash->address = (address >> 24);

	DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_address);
    }

    DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_erase);
    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_nor_erase, DOSFS_SFLASH_ERASE_SIZE);

    memset(&sflash->image[address & 0x01ff0000], 0xff, 65536);
}

static void dosfs_sflash_nor_write(dosfs_sflash_t *sflash, uint32_t address, uint32_t count, const uint8_t *data)
{
    unsigned int i;

    if (sflash->address != (address >> 24))
    {
	sflash->address = (address >> 24);

	DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_address);
    }

    DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_write);
    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_nor_write, count);

    for (i = 0; i < count; i++)
    {
        sflash->image[(address & 0x01ffff00) | ((address + i) & 0x000000ff)] &= data[i];
#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	assert(sflash->image[(address & 0x01ffff00) | ((address + i) & 0x000000ff)] == data[i]);
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */
    }
}

static void dosfs_sflash_nor_read(dosfs_sflash_t *sflash, uint32_t address, uint32_t count, uint8_t *data)
{
    unsigned int i;

    if (sflash->address != (address >> 24))
    {
	sflash->address = (address >> 24);

	DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_address);
    }

    DOSFS_SFLASH_STATISTICS_COUNT(sflash_command_read);
    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_nor_read, count);

    for (i = 0; i < count; i++)
    {
	data[i] = sflash->image[(address + i) & 0x01ffffff];
    }
}

#endif /* DOSFS_CONFIG_SFLASH_SIMULATE != 0 */

#if (DOSFS_SFLASH_DATA_SIZE == 0x02000000)

static inline uint32_t dosfs_sflash_ftl_sector_lookup(dosfs_sflash_t *sflash, uint32_t index)
{
    return (uint32_t)sflash->sector_table[index] + (((sflash->sector_mask[index / 32] >> (index & 31)) & 1) << 8);
}

static inline void dosfs_sflash_ftl_sector_assign(dosfs_sflash_t *sflash, uint32_t index, uint32_t sector)
{
    sflash->sector_table[index] = sector;

    if (sector & 0x100)
    {
	sflash->sector_mask[index / 32] |= (1ul << (index & 31));
    }
    else
    {
	sflash->sector_mask[index / 32] &= ~(1ul << (index & 31));
    }
}

#else /* DOSFS_SFLASH_DATA_SIZE */

static inline uint32_t dosfs_sflash_ftl_sector_lookup(dosfs_sflash_t *sflash, uint32_t index)
{
    return sflash->sector_table[index];
}

static inline void dosfs_sflash_ftl_sector_assign(dosfs_sflash_t *sflash, uint32_t index, uint32_t sector)
{
    sflash->sector_table[index] = sector;
}

#endif /* DOSFS_SFLASH_DATA_SIZE */

static uint32_t dosfs_sflash_ftl_translate(dosfs_sflash_t *sflash, uint32_t logical)
{
    return ((dosfs_sflash_ftl_sector_lookup(sflash, (logical >> DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT)) << DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT) + (logical & DOSFS_SFLASH_LOGICAL_BLOCK_MASK)) * DOSFS_SFLASH_BLOCK_SIZE;
}

static void dosfs_sflash_ftl_info_merge(uint32_t *info, uint32_t index, uint32_t data)
{
    info[(index*4)+0]  = (info[(index*4)+0] & ~DOSFS_SFLASH_INFO_EXTENDED_MASK) | (((data >> 0)  << DOSFS_SFLASH_INFO_EXTENDED_SHIFT) & DOSFS_SFLASH_INFO_EXTENDED_MASK);
    info[(index*4)+1]  = (info[(index*4)+1] & ~DOSFS_SFLASH_INFO_EXTENDED_MASK) | (((data >> 8)  << DOSFS_SFLASH_INFO_EXTENDED_SHIFT) & DOSFS_SFLASH_INFO_EXTENDED_MASK);
    info[(index*4)+2]  = (info[(index*4)+2] & ~DOSFS_SFLASH_INFO_EXTENDED_MASK) | (((data >> 16) << DOSFS_SFLASH_INFO_EXTENDED_SHIFT) & DOSFS_SFLASH_INFO_EXTENDED_MASK);
    info[(index*4)+3]  = (info[(index*4)+3] & ~DOSFS_SFLASH_INFO_EXTENDED_MASK) | (((data >> 24) << DOSFS_SFLASH_INFO_EXTENDED_SHIFT) & DOSFS_SFLASH_INFO_EXTENDED_MASK);
}

static uint32_t dosfs_sflash_ftl_info_extract(const uint32_t *info, uint32_t index)
{
    return ((((info[(index*4)+0] & DOSFS_SFLASH_INFO_EXTENDED_MASK) >> DOSFS_SFLASH_INFO_EXTENDED_SHIFT) <<  0) |
	    (((info[(index*4)+1] & DOSFS_SFLASH_INFO_EXTENDED_MASK) >> DOSFS_SFLASH_INFO_EXTENDED_SHIFT) <<  8) |
	    (((info[(index*4)+2] & DOSFS_SFLASH_INFO_EXTENDED_MASK) >> DOSFS_SFLASH_INFO_EXTENDED_SHIFT) << 16) |
	    (((info[(index*4)+3] & DOSFS_SFLASH_INFO_EXTENDED_MASK) >> DOSFS_SFLASH_INFO_EXTENDED_SHIFT) << 24));
}

static void dosfs_sflash_ftl_info_type(dosfs_sflash_t *sflash, uint32_t logical, uint32_t info)
{
    dosfs_sflash_nor_write(sflash, (dosfs_sflash_ftl_translate(sflash, logical) & ~(DOSFS_SFLASH_ERASE_SIZE-1)) + ((logical & DOSFS_SFLASH_LOGICAL_BLOCK_MASK) * 4) +2, 1, (const uint8_t*)&info + 2);
}

static void dosfs_sflash_ftl_info_entry(dosfs_sflash_t *sflash, uint32_t logical, uint32_t info)
{
    dosfs_sflash_nor_write(sflash, (dosfs_sflash_ftl_translate(sflash, logical) & ~(DOSFS_SFLASH_ERASE_SIZE-1)) + ((logical & DOSFS_SFLASH_LOGICAL_BLOCK_MASK) * 4), 3, (const uint8_t*)&info);
}

static uint32_t dosfs_sflash_ftl_victim(dosfs_sflash_t *sflash)
{
    uint32_t index, victim_index, victim_score, score;

#if 0
    {
	sflash->xlate_logical = DOSFS_SFLASH_BLOCK_RESERVED;
	sflash->xlate2_logical = DOSFS_SFLASH_BLOCK_RESERVED;

	for (index = 0; index < ((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1); index++)
	{
	    unsigned int erase_count, victim_offset, free_total, n;
	    
	    free_total = 0;
	    
	    victim_offset = dosfs_sflash_ftl_sector_lookup(sflash, index) * DOSFS_SFLASH_ERASE_SIZE;
	    
	    dosfs_sflash_nor_read(sflash, victim_offset, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)&sflash_block_cache[0][0]);
	    sflash_read_count -= DOSFS_SFLASH_BLOCK_SIZE;
	    
	    erase_count = dosfs_sflash_ftl_info_extract(&sflash_block_cache[0][0], 0);
	    
	    for (n = 1; n < DOSFS_SFLASH_BLOCK_INFO_ENTRIES; n++)
	    {
		if (sflash_block_cache[0][n] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO)
		{
		    free_total++;
		}
	    }
	    
	    // printf("%d: %d %d (%d)\n", index, sflash->victim_score[index], sflash->victim_delta[index], erase_count);
	    
#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	    assert(erase_count == (sflash->erase_count_max - sflash->victim_delta[index]));
	    assert(!!(sflash->victim_score[index] & DOSFS_SFLASH_VICTIM_ALLOCATED_MASK) == (free_total == 0));
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */
	}
    }
#endif

    victim_index = 0;
    victim_score = 0;

    index = sflash->victim_sector;
	
    do
    {
	score = sflash->victim_delta[index] + sflash->victim_score[index];
	
	if (victim_score < score)
	{
	    victim_index = index;
	    victim_score = score;
	}
	
	index++;
	
	if (index == ((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1))
	{
	    index  = 0;
	}
    }
    while (index != sflash->victim_sector);
	
    sflash->victim_sector = victim_index +1;
	
    if (sflash->victim_sector == ((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1))
    {
	sflash->victim_sector = 0;
    }

    return dosfs_sflash_ftl_sector_lookup(sflash, victim_index) * DOSFS_SFLASH_ERASE_SIZE;
}
    
static void dosfs_sflash_ftl_swap(dosfs_sflash_t *sflash, uint32_t victim_offset, uint32_t victim_sector, uint32_t victim_erase_count, uint32_t reclaim_offset)
{
    uint32_t erase_info[4];
    uint32_t *cache;

    cache = sflash->cache[0];

    dosfs_sflash_nor_erase(sflash, victim_offset);

    sflash->xlate_logical = DOSFS_SFLASH_BLOCK_RESERVED;

    memset(cache, 0xff, DOSFS_SFLASH_BLOCK_SIZE);

    cache[0] &= ~DOSFS_SFLASH_INFO_NOT_WRITTEN_TO;

    erase_info[0] = victim_erase_count;
    erase_info[1] = 0xffffffff;
    erase_info[2] = DOSFS_SFLASH_SECTOR_IDENT_0;
    erase_info[3] = DOSFS_SFLASH_SECTOR_IDENT_1;

    dosfs_sflash_ftl_info_merge(cache, 0, erase_info[0]);
    dosfs_sflash_ftl_info_merge(cache, 1, erase_info[1]);
    dosfs_sflash_ftl_info_merge(cache, 2, erase_info[2]);
    dosfs_sflash_ftl_info_merge(cache, 3, erase_info[3]);

    dosfs_sflash_nor_write(sflash, victim_offset, DOSFS_SFLASH_INFO_EXTENDED_TOTAL, (const uint8_t*)cache);

    /* Now the old VICTIM has been erase, so just flip the RECLAIM sector into an ERASE sector (RECLAIM -> ERASE).
     */

    dosfs_sflash_ftl_sector_assign(sflash, victim_sector, (reclaim_offset / DOSFS_SFLASH_ERASE_SIZE));

    dosfs_sflash_ftl_info_type(sflash, (victim_sector << DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT), (DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_ERASE | victim_sector));

    sflash->reclaim_offset = victim_offset;
    sflash->reclaim_erase_count = victim_erase_count;
}

static void dosfs_sflash_ftl_reclaim(dosfs_sflash_t *sflash, uint32_t victim_offset)
{
    uint32_t index, victim_sector, victim_delta, victim_erase_count, erase_info[4], n;
    uint32_t *cache, *data;

    /* Mark the reclaim erase unit as reclaim type so that upon
     * a crash the data can be recovered.
     *
     * N.b. that during the copy operation the reclaim_victim
     * and reclaim_erase_count will be 0xffffffff.
     */

    cache = sflash->cache[0];
    data = sflash->cache[1];

    sflash->xlate_logical = DOSFS_SFLASH_BLOCK_RESERVED;
    
    dosfs_sflash_nor_read(sflash, victim_offset, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)cache);

    victim_sector = cache[0] & DOSFS_SFLASH_INFO_DATA_MASK;
    victim_erase_count = dosfs_sflash_ftl_info_extract(cache, 0);

    // printf("==== RECLAIM ==== %08x (%d=%d (%d)), %d\n", victim_offset, victim_sector, (sflash->victim_delta[victim_sector] + sflash->victim_score[victim_sector]), victim_erase_count, sflash->alloc_free);

    
    /* Mark the victim sector as VICTIM and record the "reclaim_offset" (ERASE -> VICTIM).
     */
    cache[0] = (cache[0] & ~DOSFS_SFLASH_INFO_TYPE_MASK) | DOSFS_SFLASH_INFO_TYPE_VICTIM;
	
    erase_info[0] = victim_erase_count;
    erase_info[1] = sflash->reclaim_erase_count + 1;
    erase_info[2] = DOSFS_SFLASH_SECTOR_IDENT_0;
    erase_info[3] = DOSFS_SFLASH_SECTOR_IDENT_1;
    
    dosfs_sflash_ftl_info_merge(cache, 0, erase_info[0]);
    dosfs_sflash_ftl_info_merge(cache, 1, erase_info[1]);
    dosfs_sflash_ftl_info_merge(cache, 2, erase_info[2]);
    dosfs_sflash_ftl_info_merge(cache, 3, erase_info[3]);

    dosfs_sflash_nor_write(sflash, victim_offset, DOSFS_SFLASH_INFO_EXTENDED_TOTAL, (const uint8_t*)cache);

    sflash->alloc_sector = victim_sector;
    
    {
	sflash->xlate_logical = DOSFS_SFLASH_BLOCK_RESERVED;

	sflash->alloc_count   = 0;
	sflash->alloc_index   = 0;
	sflash->alloc_mask[0] = 0;
	sflash->alloc_mask[1] = 0;
	sflash->alloc_mask[2] = 0;
	sflash->alloc_mask[3] = 0;

	/* Here the reclaim sector stays in uncommitted state.
	 */

	n = 0;

	for (index = 1; index < DOSFS_SFLASH_BLOCK_INFO_ENTRIES; index++)
	{
	    if (cache[index] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO)
	    {
		if (sflash->alloc_count == 0)
		{
		    sflash->alloc_index = index;
		}
	  
		sflash->alloc_mask[index / 32] |= (1ul << (index & 31));
		sflash->alloc_count++;
	    }
	    else
	    {
		if ((cache[index] & DOSFS_SFLASH_INFO_TYPE_MASK) == DOSFS_SFLASH_INFO_TYPE_DELETED)
		{
		    n++;

		    cache[index] = DOSFS_SFLASH_INFO_ERASED;

		    if (sflash->alloc_count == 0)
		    {
			sflash->alloc_index = index;
		    }

		    sflash->alloc_mask[index / 32] |= (1ul << (index & 31));
		    sflash->alloc_count++;
		    sflash->alloc_free++;
		}
		else
		{
		    sflash->xlate2_logical = DOSFS_SFLASH_BLOCK_RESERVED;

		    dosfs_sflash_nor_read(sflash, victim_offset + (index * DOSFS_SFLASH_BLOCK_SIZE), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)data);

		    dosfs_sflash_nor_write(sflash, sflash->reclaim_offset + (index * DOSFS_SFLASH_BLOCK_SIZE), DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)data);
		    dosfs_sflash_nor_write(sflash, sflash->reclaim_offset + (index * DOSFS_SFLASH_BLOCK_SIZE) + DOSFS_SFLASH_PAGE_SIZE, DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)data + DOSFS_SFLASH_PAGE_SIZE);
		}
	    }
	}

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	assert(n == ((sflash->victim_score[victim_sector] & DOSFS_SFLASH_VICTIM_DELETED_MASK) >> DOSFS_SFLASH_VICTIM_DELETED_SHIFT));
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */
    }

    victim_erase_count++;

    /* Write the update info header to the reclaim sector (2nd page first). This commits the reclaim operation (UNCOMITTED -> RECLAIM).
     */
    cache[0] = (cache[0]  & ~DOSFS_SFLASH_INFO_TYPE_MASK) | DOSFS_SFLASH_INFO_TYPE_RECLAIM;

    erase_info[0] = sflash->reclaim_erase_count;
    erase_info[1] = 0xffffffff;
    erase_info[2] = DOSFS_SFLASH_SECTOR_IDENT_0;
    erase_info[3] = DOSFS_SFLASH_SECTOR_IDENT_1;
    
    dosfs_sflash_ftl_info_merge(cache, 0, erase_info[0]);
    dosfs_sflash_ftl_info_merge(cache, 1, erase_info[1]);
    dosfs_sflash_ftl_info_merge(cache, 2, erase_info[2]);
    dosfs_sflash_ftl_info_merge(cache, 3, erase_info[3]);
    
    dosfs_sflash_nor_write(sflash, sflash->reclaim_offset + DOSFS_SFLASH_PAGE_SIZE, DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)cache + DOSFS_SFLASH_PAGE_SIZE);
    dosfs_sflash_nor_write(sflash, sflash->reclaim_offset, DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)cache);

    /* Here a valid VICTIM and a valid RECLAIM header exist. Erase the victim and set it up as new reclaim.
     */

    dosfs_sflash_ftl_swap(sflash, victim_offset, victim_sector, victim_erase_count, sflash->reclaim_offset);

    if (sflash->erase_count_max < victim_erase_count)
    {
	sflash->erase_count_max = victim_erase_count;
    }

    victim_delta = sflash->victim_delta[victim_sector];

    sflash->victim_delta[victim_sector] = sflash->victim_delta[sflash->data_size / DOSFS_SFLASH_ERASE_SIZE -1];

    if (victim_delta == 0)
    {
	for (index = 0; index < ((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1); index++)
	{
#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	    assert(sflash->victim_delta[index] != 255);
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

	    sflash->victim_delta[index] += 1;
	}

	sflash->victim_delta[sflash->data_size / DOSFS_SFLASH_ERASE_SIZE -1] = 0;
    }
    else
    {
	sflash->victim_delta[sflash->data_size / DOSFS_SFLASH_ERASE_SIZE -1] = victim_delta -1;
    }

    sflash->victim_score[victim_sector] = (sflash->alloc_count ? 0 : DOSFS_SFLASH_VICTIM_ALLOCATED_MASK);
}


static void dosfs_sflash_ftl_format(dosfs_sflash_t *sflash)
{
    uint32_t offset, erase_count, erase_info[8];
    uint32_t *cache;

    stm32l4_gpio_pin_configure(GPIO_PIN_PB2, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_OUTPUT));
    stm32l4_gpio_pin_configure(GPIO_PIN_PA10, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_OUTPUT));

    cache = sflash->cache[0];

    for (offset = 0; offset < sflash->data_size; offset += DOSFS_SFLASH_ERASE_SIZE)
    {
	stm32l4_gpio_pin_write(GPIO_PIN_PB2, !(offset & DOSFS_SFLASH_ERASE_SIZE));
	stm32l4_gpio_pin_write(GPIO_PIN_PA10, !!(offset & DOSFS_SFLASH_ERASE_SIZE));

	erase_count = 1;

	dosfs_sflash_nor_read(sflash, offset, DOSFS_SFLASH_INFO_EXTENDED_TOTAL, (uint8_t*)cache);

	if (!(cache[0] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO))
	{
	    erase_info[0] = dosfs_sflash_ftl_info_extract(cache, 0);
	    erase_info[1] = dosfs_sflash_ftl_info_extract(cache, 1);
	    erase_info[2] = dosfs_sflash_ftl_info_extract(cache, 2);
	    erase_info[3] = dosfs_sflash_ftl_info_extract(cache, 3);

	    if ((erase_info[2] == DOSFS_SFLASH_SECTOR_IDENT_0) && (erase_info[3] != DOSFS_SFLASH_SECTOR_IDENT_1))
	    {
		erase_count = erase_info[0] + 1;
	    }
	}

	dosfs_sflash_nor_erase(sflash, offset);

	memset(cache, 0xff, DOSFS_SFLASH_BLOCK_SIZE);

	cache[0] &= ~DOSFS_SFLASH_INFO_NOT_WRITTEN_TO;

	erase_info[0] = erase_count;
	erase_info[1] = 0xffffffff;
	erase_info[2] = DOSFS_SFLASH_SECTOR_IDENT_0;
	erase_info[3] = DOSFS_SFLASH_SECTOR_IDENT_1;

	dosfs_sflash_ftl_info_merge(cache, 0, erase_info[0]);
	dosfs_sflash_ftl_info_merge(cache, 1, erase_info[1]);
	dosfs_sflash_ftl_info_merge(cache, 2, erase_info[2]);
	dosfs_sflash_ftl_info_merge(cache, 3, erase_info[3]);
	
	if (offset != (sflash->data_size - DOSFS_SFLASH_ERASE_SIZE))
	{
	    cache[0] = (cache[0] & DOSFS_SFLASH_INFO_EXTENDED_MASK) | DOSFS_SFLASH_INFO_TYPE_ERASE | (offset / DOSFS_SFLASH_ERASE_SIZE);
	}

	dosfs_sflash_nor_write(sflash, offset, DOSFS_SFLASH_INFO_EXTENDED_TOTAL, (const uint8_t*)cache);
    }

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
    memset(&sflash_data_shadow, 0xff, sizeof(sflash_data_shadow));
    memset(&sflash_xlate_shadow, 0xff, sizeof(sflash_xlate_shadow));
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

    stm32l4_gpio_pin_configure(GPIO_PIN_PB2, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));
    stm32l4_gpio_pin_configure(GPIO_PIN_PA10, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));
}

/* Modify XLATE/XLATE_SECONDARY mappings. Assumption is that XLATE/XLATE_SECONDARY already
 * have been setup to have a slot that can be written. Also there is a DATA_WRITTEN block
 * that contains valid data.
 */

static void dosfs_sflash_ftl_modify(dosfs_sflash_t *sflash, uint32_t address, uint32_t write_logical)
{
    uint32_t xlate_segment, xlate_index, read_logical;
    uint16_t *xlate_cache, *xlate2_cache;

    xlate_cache   = (uint16_t*)sflash->cache[0];
    xlate2_cache  = (uint16_t*)sflash->cache[1];

    xlate_segment = (address - DOSFS_SFLASH_XLATE_OFFSET) >> DOSFS_SFLASH_XLATE_SEGMENT_SHIFT;
    xlate_index   = (address - DOSFS_SFLASH_XLATE_OFFSET) & DOSFS_SFLASH_XLATE_INDEX_MASK;

    if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
    {
        DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_miss);

	sflash->xlate_logical = sflash->xlate_table[xlate_segment];
	
	dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
    }
    else
    {
        DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_hit);
    }

    read_logical = xlate_cache[xlate_index];
    
    if (read_logical == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
    {
#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	assert(read_logical == sflash_xlate_shadow[address]);
	sflash_xlate_shadow[address] = write_logical;
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */
	
	xlate_cache[xlate_index] = write_logical;
	
	dosfs_sflash_nor_write(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical) + (xlate_index * 2), 2, (const uint8_t*)&xlate_cache[xlate_index]);
	
	dosfs_sflash_ftl_info_type(sflash, write_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED | address);
    }
    else
    {
	if (sflash->xlate2_logical != sflash->xlate2_table[xlate_segment])
	{
	    DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_miss);

	    sflash->xlate2_logical = sflash->xlate2_table[xlate_segment];
	    
	    dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate2_cache);
	}
	else
	{
	    DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_hit);
	}

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	assert(read_logical == sflash_xlate_shadow[address]);
	sflash_xlate_shadow[address] = write_logical;
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */
	
	xlate2_cache[xlate_index] = write_logical;
	
	dosfs_sflash_nor_write(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical) + (xlate_index * 2), 2, (const uint8_t*)&xlate2_cache[xlate_index]);
	
	dosfs_sflash_ftl_info_type(sflash, write_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED | address);
	
	if (read_logical != DOSFS_SFLASH_BLOCK_DELETED)
	{
	    xlate_cache[xlate_index] = DOSFS_SFLASH_BLOCK_DELETED;
	    
	    dosfs_sflash_nor_write(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical) + (xlate_index * 2), 2, (const uint8_t*)&xlate_cache[xlate_index]);
	    
	    dosfs_sflash_ftl_info_type(sflash, read_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DELETED);
	    
	    sflash->victim_score[read_logical >> DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;
	}
    }
}

static void dosfs_sflash_ftl_trim(dosfs_sflash_t *sflash, uint32_t address, uint32_t delete_logical)
{
    uint32_t xlate_segment, xlate_index;
    uint16_t *xlate_cache, *xlate2_cache;

    xlate_cache   = (uint16_t*)sflash->cache[0];
    xlate2_cache  = (uint16_t*)sflash->cache[1];

    xlate_segment = (address - DOSFS_SFLASH_XLATE_OFFSET) >> DOSFS_SFLASH_XLATE_SEGMENT_SHIFT;
    xlate_index   = (address - DOSFS_SFLASH_XLATE_OFFSET) & DOSFS_SFLASH_XLATE_INDEX_MASK;

    if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
    {
	DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_miss);

	sflash->xlate_logical = sflash->xlate_table[xlate_segment];
	
	dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
    }
    else
    {
	DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_hit);;
    }

    if (xlate_cache[xlate_index] != DOSFS_SFLASH_BLOCK_DELETED)
    {
#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	sflash_xlate_shadow[address] = DOSFS_SFLASH_BLOCK_DELETED;
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

	xlate_cache[xlate_index] = DOSFS_SFLASH_BLOCK_DELETED;
				
	dosfs_sflash_nor_write(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical) + (xlate_index * 2), 2, (const uint8_t*)&xlate_cache[xlate_index]);
    }
    else
    {
	if (sflash->xlate2_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	{
	    if (sflash->xlate2_logical != sflash->xlate2_table[xlate_segment])
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_miss);

		sflash->xlate2_logical = sflash->xlate2_table[xlate_segment];
		
		dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate2_cache);
	    }
	    else
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_hit);
	    }

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	    sflash_xlate_shadow[address] = DOSFS_SFLASH_BLOCK_DELETED;
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

	    xlate2_cache[xlate_index] = DOSFS_SFLASH_BLOCK_DELETED;
	    
	    dosfs_sflash_nor_write(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical) + (xlate_index * 2), 2, (const uint8_t*)&xlate2_cache[xlate_index]);
	}
    }

    dosfs_sflash_ftl_info_type(sflash, delete_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DELETED);

    sflash->victim_score[delete_logical >> DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;
}

#define DOSFS_SFLASH_FTL_CHECK() while (1) { }

static void dosfs_sflash_ftl_check(dosfs_sflash_t *sflash)
{
    uint32_t offset, index, sector, address, info_logical, cache_logical;
    uint32_t *cache, temp[512 / 4], erase_info[4];
    uint32_t read_logical, xlate_segment, xlate_index;
    uint16_t *xlate_cache, *xlate2_cache;

    return;

    cache = &temp[0];

    for (offset = 0; offset < sflash->data_size; offset += DOSFS_SFLASH_ERASE_SIZE)
    {
	dosfs_sflash_nor_read(sflash, offset, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)cache);
	    
	if (cache[0] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO)
	{
	    DOSFS_SFLASH_FTL_CHECK();
	}
	else
	{
	    erase_info[0] = dosfs_sflash_ftl_info_extract(cache, 0);
	    erase_info[1] = dosfs_sflash_ftl_info_extract(cache, 1);
	    erase_info[2] = dosfs_sflash_ftl_info_extract(cache, 2);
	    erase_info[3] = dosfs_sflash_ftl_info_extract(cache, 3);
		
	    if ((erase_info[2] != DOSFS_SFLASH_SECTOR_IDENT_0) || (erase_info[3] != DOSFS_SFLASH_SECTOR_IDENT_1))
	    {
		DOSFS_SFLASH_FTL_CHECK();
	    }

	    switch (cache[0] & DOSFS_SFLASH_INFO_TYPE_MASK) {
	    case DOSFS_SFLASH_INFO_TYPE_DELETED:
	    case DOSFS_SFLASH_INFO_TYPE_XLATE:
	    case DOSFS_SFLASH_INFO_TYPE_XLATE_SECONDARY:
	    case DOSFS_SFLASH_INFO_TYPE_DATA_DELETED:
	    case DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED:
	    case DOSFS_SFLASH_INFO_TYPE_DATA_WRITTEN:
		DOSFS_SFLASH_FTL_CHECK();
		break;

	    case DOSFS_SFLASH_INFO_TYPE_VICTIM:
		DOSFS_SFLASH_FTL_CHECK();
		break;

	    case DOSFS_SFLASH_INFO_TYPE_ERASE:
		sector = (cache[0] & DOSFS_SFLASH_INFO_DATA_MASK);

		if (dosfs_sflash_ftl_sector_lookup(sflash, sector) != (offset / DOSFS_SFLASH_ERASE_SIZE))
		{
		    DOSFS_SFLASH_FTL_CHECK();
		}

		for (index = 1; index < DOSFS_SFLASH_BLOCK_INFO_ENTRIES; index++)
		{
		    if (cache[index] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO)
		    {
			if ((cache[index] & DOSFS_SFLASH_INFO_TYPE_MASK) != DOSFS_SFLASH_INFO_TYPE_RESERVED)
			{
			    DOSFS_SFLASH_FTL_CHECK();
			}
		    }
		    else
		    {
			info_logical = (sector << DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT) + index;

			switch (cache[index] & DOSFS_SFLASH_INFO_TYPE_MASK) {
			case DOSFS_SFLASH_INFO_TYPE_VICTIM:
			case DOSFS_SFLASH_INFO_TYPE_ERASE:
			case DOSFS_SFLASH_INFO_TYPE_RECLAIM:
			    DOSFS_SFLASH_FTL_CHECK();
			    break;

			case DOSFS_SFLASH_INFO_TYPE_DELETED:
			    break;

			case DOSFS_SFLASH_INFO_TYPE_DATA_DELETED:
			    DOSFS_SFLASH_FTL_CHECK();
			    break;
		
			case DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED:
			    address = cache[index] & DOSFS_SFLASH_INFO_DATA_MASK;

			    if (address < DOSFS_SFLASH_XLATE_OFFSET)
			    {
				read_logical = sflash->block_table[address];
			    }
			    else
			    {
				xlate_cache   = (uint16_t*)sflash->cache[0];
				xlate2_cache  = (uint16_t*)sflash->cache[1];
				
				xlate_segment = (address - DOSFS_SFLASH_XLATE_OFFSET) >> DOSFS_SFLASH_XLATE_SEGMENT_SHIFT;
				xlate_index   = (address - DOSFS_SFLASH_XLATE_OFFSET) & DOSFS_SFLASH_XLATE_INDEX_MASK;
				
				if (sflash->xlate_table[xlate_segment] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
				{
				    read_logical = DOSFS_SFLASH_BLOCK_NOT_ALLOCATED;
				}
				else
				{
				    if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
				    {
					sflash->xlate_logical = sflash->xlate_table[xlate_segment];

					dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
				    }

				    read_logical = xlate_cache[xlate_index];
				    
				    if (read_logical == DOSFS_SFLASH_BLOCK_DELETED)
				    {
					if (sflash->xlate2_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
					{
					    if (sflash->xlate2_logical != sflash->xlate2_table[xlate_segment])
					    {
						sflash->xlate2_logical = sflash->xlate2_table[xlate_segment];
			
						dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate2_cache);
					    }

					    if (xlate2_cache[xlate_index] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
					    {
						read_logical = xlate2_cache[xlate_index];
					    }
					}
				    }
				}
			    }
			
			    if (read_logical != info_logical)
			    {
				DOSFS_SFLASH_FTL_CHECK();
			    }
			    break;

			case DOSFS_SFLASH_INFO_TYPE_DATA_WRITTEN:
			    DOSFS_SFLASH_FTL_CHECK();
			    break;

			case DOSFS_SFLASH_INFO_TYPE_XLATE:
			    if (sflash->xlate_table[cache[index] & DOSFS_SFLASH_INFO_DATA_MASK] != info_logical)
			    {
				DOSFS_SFLASH_FTL_CHECK();
			    }
			    break;

			case DOSFS_SFLASH_INFO_TYPE_XLATE_SECONDARY:
			    if (sflash->xlate2_table[cache[index] & DOSFS_SFLASH_INFO_DATA_MASK] != info_logical)
			    {
				DOSFS_SFLASH_FTL_CHECK();
			    }
			    break;

			case DOSFS_SFLASH_INFO_TYPE_RESERVED:
			    break;
			}
		    }
		}
		break;

	    case DOSFS_SFLASH_INFO_TYPE_RECLAIM:
		DOSFS_SFLASH_FTL_CHECK();
		break;
		    
	    case DOSFS_SFLASH_INFO_TYPE_RESERVED:
		if (sflash->reclaim_offset != offset)
		{
		    DOSFS_SFLASH_FTL_CHECK();
		}
		break;
	    }
	}
    }

    xlate_cache   = (uint16_t*)sflash->cache[0];
    xlate2_cache  = (uint16_t*)sflash->cache[1];
    
    cache_logical = DOSFS_SFLASH_BLOCK_NOT_ALLOCATED;

    for (xlate_segment = 0; xlate_segment < sflash->xlate_count; xlate_segment++)
    {
	if (sflash->xlate_table[xlate_segment] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	{
	    if (sflash->xlate2_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	    {
		DOSFS_SFLASH_FTL_CHECK();
	    }
	}
	else
	{
	    for (xlate_index = 0; xlate_index < DOSFS_SFLASH_XLATE_ENTRIES; xlate_index++)
	    {
		address = ((xlate_segment << DOSFS_SFLASH_XLATE_SEGMENT_SHIFT) | xlate_index) + DOSFS_SFLASH_XLATE_OFFSET;

		if (sflash->xlate_table[xlate_segment] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
		{
		    read_logical = DOSFS_SFLASH_BLOCK_NOT_ALLOCATED;
		}
		else
		{
		    if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
		    {
			sflash->xlate_logical = sflash->xlate_table[xlate_segment];

			dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
		    }

		    read_logical = xlate_cache[xlate_index];

		    if (read_logical == DOSFS_SFLASH_BLOCK_DELETED)
		    {
			if (sflash->xlate2_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
			{
			    if (sflash->xlate2_logical != sflash->xlate2_table[xlate_segment])
			    {
				sflash->xlate2_logical = sflash->xlate2_table[xlate_segment];
			
				dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate2_cache);
			    }
			    
			    if (xlate2_cache[xlate_index] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
			    {
				read_logical = xlate2_cache[xlate_index];
			    }
			}
		    }
		}

		if ((read_logical != DOSFS_SFLASH_BLOCK_DELETED) && (read_logical != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED))
		{
		    /* Here we have a "address" and it's "read_logical" translation. Check whether the corresponding info header says the same.
		     */

		    if (cache_logical != (read_logical & ~DOSFS_SFLASH_LOGICAL_BLOCK_MASK))
		    {
			cache_logical = (read_logical & ~DOSFS_SFLASH_LOGICAL_BLOCK_MASK);
			
			dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, cache_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)cache);
		    }
		    
		    if ((address | DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED) != (cache[read_logical & DOSFS_SFLASH_LOGICAL_BLOCK_MASK] & (DOSFS_SFLASH_INFO_TYPE_MASK | DOSFS_SFLASH_INFO_NOT_WRITTEN_TO | DOSFS_SFLASH_INFO_DATA_MASK)))
		    {
			DOSFS_SFLASH_FTL_CHECK();
		    }
		}
	    }
	}
    }
}

/* A earse unit header has been read into the cache. Merge
 * the entries into the tables ...
 */

static bool dosfs_sflash_ftl_collect(dosfs_sflash_t *sflash, const uint32_t *cache, uint32_t offset, uint32_t sector, uint32_t *p_data_written, uint32_t *p_data_deleted)
{
    unsigned int index, free_total;
    uint32_t info_logical;

    free_total = 0;

    dosfs_sflash_ftl_sector_assign(sflash, sector, (offset / DOSFS_SFLASH_ERASE_SIZE));

    sflash->victim_score[sector] = DOSFS_SFLASH_VICTIM_ALLOCATED_MASK;

    for (index = 1; index < DOSFS_SFLASH_BLOCK_INFO_ENTRIES; index++)
    {
	if (cache[index] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO)
	{
	    sflash->victim_score[sector] &= ~DOSFS_SFLASH_VICTIM_ALLOCATED_MASK;

	    free_total++;
	}
	else
	{
	    info_logical = (sector << DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT) + index;

	    switch (cache[index] & DOSFS_SFLASH_INFO_TYPE_MASK) {
	    case DOSFS_SFLASH_INFO_TYPE_VICTIM:
	    case DOSFS_SFLASH_INFO_TYPE_ERASE:
	    case DOSFS_SFLASH_INFO_TYPE_RECLAIM:
		return false;

	    case DOSFS_SFLASH_INFO_TYPE_DELETED:
		sflash->victim_score[sector] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;
		break;

	    case DOSFS_SFLASH_INFO_TYPE_DATA_DELETED:
		p_data_deleted[0] = (cache[index] & DOSFS_SFLASH_INFO_DATA_MASK);
		p_data_deleted[1] = info_logical;
		break;
		
	    case DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED:
		if ((cache[index] & DOSFS_SFLASH_INFO_DATA_MASK) < DOSFS_SFLASH_XLATE_OFFSET)
		{
		    if (sflash->block_table[cache[index] & DOSFS_SFLASH_INFO_DATA_MASK] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
		    {
			sflash->block_table[cache[index] & DOSFS_SFLASH_INFO_DATA_MASK] = info_logical;
		    }
		    else
		    {
			dosfs_sflash_ftl_info_type(sflash, info_logical, (DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DELETED | DOSFS_SFLASH_INFO_DATA_MASK));

			sflash->victim_score[sector] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;
		    }
		}
		break;

	    case DOSFS_SFLASH_INFO_TYPE_DATA_WRITTEN:
		p_data_written[0] = (cache[index] & DOSFS_SFLASH_INFO_DATA_MASK);
		p_data_written[1] = info_logical;
		break;

	    case DOSFS_SFLASH_INFO_TYPE_XLATE:
		if ((cache[index] & DOSFS_SFLASH_INFO_DATA_MASK) >= DOSFS_SFLASH_XLATE_COUNT)
		{
		    return false;
		}

		sflash->xlate_table[cache[index] & DOSFS_SFLASH_INFO_DATA_MASK] = info_logical;
		break;

	    case DOSFS_SFLASH_INFO_TYPE_XLATE_SECONDARY:
		if ((cache[index] & DOSFS_SFLASH_INFO_DATA_MASK) >= DOSFS_SFLASH_XLATE_COUNT)
		{
		    return false;
		}

		sflash->xlate2_table[cache[index] & DOSFS_SFLASH_INFO_DATA_MASK] = info_logical;
		break;

	    case DOSFS_SFLASH_INFO_TYPE_RESERVED:
		dosfs_sflash_ftl_info_type(sflash, info_logical, (DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DELETED));

		sflash->victim_score[sector] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;
		break;
	    }
	}
    }

    if (free_total && !sflash->alloc_free)
    {
	sflash->alloc_count = free_total;
	sflash->alloc_sector = sector;
    }

    sflash->alloc_free += free_total;

    return true;
}

static bool dosfs_sflash_ftl_mount(dosfs_sflash_t *sflash)
{
    uint32_t offset, erase_offset, reclaim_offset, victim_offset, victim_sector, victim_erase_count, victim_index, erase_count, erase_count_min, erase_count_max, erase_info[8];
    uint32_t xlate_segment, xlate2_logical, data_written[2], data_deleted[2];
    uint32_t *cache;

    // printf("==== MOUNT %d ====\n", sizeof(dosfs_sflash_t));

    memset(&sflash->sector_table[0], 0xff, sizeof(sflash->sector_table));
#if (DOSFS_SFLASH_DATA_SIZE == 0x02000000)
    memset(&sflash->sector_mask[0], 0xff, sizeof(sflash->sector_mask));
#endif /* DOSFS_SFLASH_DATA_SIZE */
    memset(&sflash->block_table[0], 0xff, sizeof(sflash->block_table));
    memset(&sflash->xlate_table[0], 0xff, sizeof(sflash->xlate_table));
    memset(&sflash->xlate2_table[0], 0xff, sizeof(sflash->xlate2_table));
    memset(&sflash->victim_delta[0], 0x00, sizeof(sflash->victim_delta));
    memset(&sflash->victim_score[0], 0x00, sizeof(sflash->victim_score));

    sflash->xlate_logical = DOSFS_SFLASH_BLOCK_RESERVED;
    sflash->xlate2_logical = DOSFS_SFLASH_BLOCK_RESERVED;

    sflash->reclaim_offset = DOSFS_SFLASH_PHYSICAL_ILLEGAL;
    sflash->reclaim_erase_count = 0xffffffff;

    sflash->alloc_sector = 0;
    sflash->alloc_index = 0;
    sflash->alloc_count = 0;
    sflash->alloc_free = 0;

    data_written[0] = DOSFS_SFLASH_BLOCK_NOT_ALLOCATED;
    data_written[1] = 0;

    data_deleted[0] = DOSFS_SFLASH_BLOCK_NOT_ALLOCATED;
    data_deleted[1] = 0;

    erase_count_min = 0xffffffff;
    erase_count_max = 0x00000000;

    erase_offset = DOSFS_SFLASH_PHYSICAL_ILLEGAL;
    reclaim_offset = DOSFS_SFLASH_PHYSICAL_ILLEGAL;
    victim_offset = DOSFS_SFLASH_PHYSICAL_ILLEGAL;

    cache = sflash->cache[0];

    for (offset = 0; offset < sflash->data_size; offset += DOSFS_SFLASH_ERASE_SIZE)
    {
	dosfs_sflash_nor_read(sflash, offset, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)cache);

	if (cache[0] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO)
	{
	    /* A crash happened during the reclaim process. 
	     */

	    if (erase_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
	    {
		return false;
	    }

	    erase_offset = offset;
	}
	else
	{
	    erase_info[0] = dosfs_sflash_ftl_info_extract(cache, 0);
	    erase_info[1] = dosfs_sflash_ftl_info_extract(cache, 1);
	    erase_info[2] = dosfs_sflash_ftl_info_extract(cache, 2);
	    erase_info[3] = dosfs_sflash_ftl_info_extract(cache, 3);

	    if ((erase_info[2] != DOSFS_SFLASH_SECTOR_IDENT_0) || (erase_info[3] != DOSFS_SFLASH_SECTOR_IDENT_1))
	    {
		return false;
	    }

	    erase_count = erase_info[0];

	    switch (cache[0] & DOSFS_SFLASH_INFO_TYPE_MASK) {
	    case DOSFS_SFLASH_INFO_TYPE_DELETED:
	    case DOSFS_SFLASH_INFO_TYPE_XLATE:
	    case DOSFS_SFLASH_INFO_TYPE_XLATE_SECONDARY:
	    case DOSFS_SFLASH_INFO_TYPE_DATA_DELETED:
	    case DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED:
	    case DOSFS_SFLASH_INFO_TYPE_DATA_WRITTEN:
		break;

	    case DOSFS_SFLASH_INFO_TYPE_VICTIM:
		/* A crash happened during the reclaim process. 
		 */

		if (victim_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
		{
		    return false;
		}

		victim_offset = offset;
		/* FALLTHROU */

	    case DOSFS_SFLASH_INFO_TYPE_ERASE:
		if (erase_count_min > erase_count)
		{
		    /* Track the sector with the lowest erase count the the most likely
		     * last reclaimed sector.
		     */
		    sflash->victim_sector = (cache[0] & DOSFS_SFLASH_INFO_DATA_MASK);
		}

		if (!dosfs_sflash_ftl_collect(sflash, cache, offset, (cache[0] & DOSFS_SFLASH_INFO_DATA_MASK), &data_written[0], &data_deleted[0]))
		{
		    return false;
		}
		break;

	    case DOSFS_SFLASH_INFO_TYPE_RECLAIM:
		/* A crash happened during the reclaim process. 
		 */

		if (reclaim_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
		{
		    return false;
		}

		reclaim_offset = offset;
		break;

	    case DOSFS_SFLASH_INFO_TYPE_RESERVED:
		/* This is really the case where there is a unallocated free
		 * free erase unit ... So pick the one with the lowest erase
		 * count as the reclaim erase unit.
		 *
		 * There should be only one, and that is reclaim sector.
		 */

		if (sflash->reclaim_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
		{
		    return false;
		}

		sflash->reclaim_offset = offset;
		sflash->reclaim_erase_count = erase_count;
		break;
	    }

	    if (erase_count_min > erase_count)
	    {
		erase_count_min = erase_count;
	    }
		
	    if (erase_count_max < erase_count)
	    {
		erase_count_max = erase_count;
	    }
	}
    }

    /* Check for the case where a crash happened during the reclaim process.
     * In that case, erase the victim and update the wear level info
     */

    if (victim_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
    {
	dosfs_sflash_nor_read(sflash, victim_offset, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)cache);
    
	victim_sector = cache[0] & DOSFS_SFLASH_INFO_DATA_MASK;
	victim_erase_count = dosfs_sflash_ftl_info_extract(cache, 0);

	if (reclaim_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
	{
	    /* If there is a victim and a reclaim sector, That means everything got
	     * copied, and just the victim sector needs to be erased and made
	     * the new reclaim sector.
	     */

	    victim_erase_count++;
	    
	    dosfs_sflash_ftl_swap(sflash, victim_offset, victim_sector, victim_erase_count, reclaim_offset);

	    if (erase_count_max < victim_erase_count)
	    {
		erase_count_max = victim_erase_count;
	    }
	}
	else
	{
	    /* Here a crash happened while copying data from the victim sector to the reclaim sector.
	     * Hence erase the reclaim sector and try again with the reclaim.
	     *
	     * If there is a valid erase_offset, there was a double crash before, so assume that
	     * the newly earase sector was the reclaim sector, and the victim sector contains
	     * the proper erase_count.
	     */

	    if (erase_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
	    {
		sflash->reclaim_erase_count = dosfs_sflash_ftl_info_extract(cache, 1);
		sflash->reclaim_offset = erase_offset;
	    }
	    else
	    {
		sflash->reclaim_erase_count++;

		dosfs_sflash_nor_erase(sflash, sflash->reclaim_offset);

		if (erase_count_max < sflash->reclaim_erase_count)
		{
		    erase_count_max = sflash->reclaim_erase_count;
		}
	    }

	    memset(cache, 0xff, DOSFS_SFLASH_BLOCK_SIZE);

	    cache[0] &= ~DOSFS_SFLASH_INFO_NOT_WRITTEN_TO;

	    erase_info[0] = sflash->reclaim_erase_count;
	    erase_info[1] = 0xffffffff;
	    erase_info[2] = DOSFS_SFLASH_SECTOR_IDENT_0;
	    erase_info[3] = DOSFS_SFLASH_SECTOR_IDENT_1;
	    
	    dosfs_sflash_ftl_info_merge(cache, 0, erase_info[0]);
	    dosfs_sflash_ftl_info_merge(cache, 1, erase_info[1]);
	    dosfs_sflash_ftl_info_merge(cache, 2, erase_info[2]);
	    dosfs_sflash_ftl_info_merge(cache, 3, erase_info[3]);

	    dosfs_sflash_nor_write(sflash, sflash->reclaim_offset, DOSFS_SFLASH_INFO_EXTENDED_TOTAL, (const uint8_t*)cache);

	    dosfs_sflash_ftl_reclaim(sflash, victim_offset);
	}
    }
    else
    {
	if (reclaim_offset != DOSFS_SFLASH_PHYSICAL_ILLEGAL)
	{
	    /* Here we crashed before marking the reclaim sector as normal erase sector.
	     */
	    dosfs_sflash_nor_read(sflash, reclaim_offset, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)cache);

	    cache[0] = (cache[0] & ~DOSFS_SFLASH_INFO_TYPE_MASK) | DOSFS_SFLASH_INFO_TYPE_ERASE;

	    dosfs_sflash_nor_write(sflash, reclaim_offset, 4, (const uint8_t*)cache); /* RECLAIM -> ERASE */

	    if (!dosfs_sflash_ftl_collect(sflash, cache, reclaim_offset, (cache[0] & DOSFS_SFLASH_INFO_DATA_MASK), &data_written[0], &data_deleted[0]))
	    {
		return false;
	    }
	}
    }

    /* Scan xlate_table and xlate2_table for the case where there is a xlate2 entry but
     * no xlate entry. This can occure while xlate2 is converted to an xlate entry,
     * which means the xlate2 entry is really an xlate entry.
     *
     * Also scan xlate/xlate2 for entries where UNCOMITTED is set, but BLOCK_MASK is not all 1s.
     * If the block pointed to is DELETED, then also delete it in xlate/xlate2, otherwise
     * clear the UNCOMITTED bit. This recovers from a crash during a write operation.
     */

    for (xlate_segment = 0; xlate_segment < sflash->xlate_count; xlate_segment++)
    {
	if ((sflash->xlate_table[xlate_segment] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED) && (sflash->xlate2_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED))
	{
	    xlate2_logical = sflash->xlate2_table[xlate_segment];
	    
	    dosfs_sflash_ftl_info_type(sflash, xlate2_logical, (DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_XLATE | DOSFS_SFLASH_INFO_DATA_MASK));
	    
	    sflash->xlate_table[xlate_segment] = xlate2_logical;
	    sflash->xlate2_table[xlate_segment] = DOSFS_SFLASH_BLOCK_NOT_ALLOCATED;
	}
    }

    if (data_written[0] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
    {
	dosfs_sflash_ftl_modify(sflash, data_written[0], data_written[1]);
    }

    if (data_deleted[0] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
    {
	dosfs_sflash_ftl_trim(sflash, data_written[0], data_written[1]);
    }

    for (offset = 0; offset < sflash->data_size; offset += DOSFS_SFLASH_ERASE_SIZE)
    {
	dosfs_sflash_nor_read(sflash, offset, DOSFS_SFLASH_INFO_EXTENDED_SIZE, (uint8_t*)cache);

	if (!(cache[0] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO))
	{
	    if ((cache[0] & DOSFS_SFLASH_INFO_TYPE_MASK) == DOSFS_SFLASH_INFO_TYPE_ERASE)
	    {
		victim_index = cache[0] & DOSFS_SFLASH_INFO_DATA_MASK;
	    }
	    else
	    {
		victim_index = (sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1;
	    }

	    erase_count = dosfs_sflash_ftl_info_extract(cache, 0);

	    if ((erase_count_max - erase_count) > 255)
	    {
		sflash->victim_delta[victim_index] = 255;
	    }
	    else
	    {
		sflash->victim_delta[victim_index] = (erase_count_max - erase_count);
	    }
	}
    }

    sflash->erase_count_max = erase_count_max;

    sflash->alloc_count = 0;
    
    dosfs_sflash_ftl_check(sflash);

    return true;
}

static uint32_t dosfs_sflash_ftl_allocate(dosfs_sflash_t *sflash)
{
    uint32_t index, logical;
    uint32_t *cache;

    cache = sflash->cache[1];

    while (1)
    {
	if (sflash->alloc_count == 0)
	{
	    if (!(sflash->victim_score[sflash->alloc_sector] & DOSFS_SFLASH_VICTIM_ALLOCATED_MASK))
	    {
		sflash->alloc_index   = 0;
		sflash->alloc_mask[0] = 0;
		sflash->alloc_mask[1] = 0;
		sflash->alloc_mask[2] = 0;
		sflash->alloc_mask[3] = 0;

		sflash->xlate2_logical = DOSFS_SFLASH_BLOCK_RESERVED;

		// printf("ALLOCATE %d\n", sflash->alloc_sector);

		dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_sector_lookup(sflash, sflash->alloc_sector) * DOSFS_SFLASH_ERASE_SIZE, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)cache);

		for (index = 1; index < DOSFS_SFLASH_BLOCK_INFO_ENTRIES; index++)
		{
		    if (cache[index] & DOSFS_SFLASH_INFO_NOT_WRITTEN_TO)
		    {
			sflash->alloc_mask[index / 32] |= (1ul << (index & 31));
			sflash->alloc_count++;

			if (sflash->alloc_index == 0)
			{
			    sflash->alloc_index = index;
			}
		    }
		}
	    }
	}

	if (sflash->alloc_count == 0)
	{
	    sflash->victim_score[sflash->alloc_sector] |= DOSFS_SFLASH_VICTIM_ALLOCATED_MASK;

	    sflash->alloc_sector++;
	    
	    if (sflash->alloc_sector == ((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1))
	    {
		sflash->alloc_sector = 0;
	    }
	}
	else
	{
	    while (1)
	    {
		if (sflash->alloc_mask[sflash->alloc_index / 32] & (1ul << (sflash->alloc_index & 31)))
		{
		    logical = (sflash->alloc_sector << DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT) + sflash->alloc_index;

		    sflash->alloc_mask[sflash->alloc_index / 32] &= ~(1ul << (sflash->alloc_index & 31));
		    sflash->alloc_free--;
		    sflash->alloc_count--;
		    sflash->alloc_index++;

		    if (sflash->alloc_count == 0)
		    {
			sflash->victim_score[sflash->alloc_sector] |= DOSFS_SFLASH_VICTIM_ALLOCATED_MASK;

			sflash->alloc_sector++;
			
			if (sflash->alloc_sector == ((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1))
			{
			    sflash->alloc_sector = 0;
			}
		    }

		    return logical;
		}

		sflash->alloc_index++;
	    }
	}
    }
}

static void dosfs_sflash_ftl_write(dosfs_sflash_t *sflash, uint32_t address, const uint8_t *data)
{
    uint32_t victim_offset, read_logical, write_logical, write_offset, xlate_segment, xlate_index, index;
    uint16_t *xlate_cache, *xlate2_cache;

    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_ftl_write, 512);

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
    memcpy(&sflash_data_shadow[address * 512], data, 512);
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */
 
    /* Make sure there are enough free blocksto one new block and an xlate/xlate2 block.
     */

    while (sflash->alloc_free <= 16)
    {
	victim_offset = dosfs_sflash_ftl_victim(sflash);

	dosfs_sflash_ftl_reclaim(sflash, victim_offset);
    }

    /* Prepare xlate/xlate2 so that the logical mapping can be updated.
     */

    xlate_cache   = (uint16_t*)sflash->cache[0];
    xlate2_cache  = (uint16_t*)sflash->cache[1];

    xlate_segment = (address - DOSFS_SFLASH_XLATE_OFFSET) >> DOSFS_SFLASH_XLATE_SEGMENT_SHIFT;
    xlate_index   = (address - DOSFS_SFLASH_XLATE_OFFSET) & DOSFS_SFLASH_XLATE_INDEX_MASK;

    if (address >= DOSFS_SFLASH_XLATE_OFFSET)
    {
	if (sflash->xlate_table[xlate_segment] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	{
	    sflash->xlate_logical = dosfs_sflash_ftl_allocate(sflash);
	    sflash->xlate_table[xlate_segment] = sflash->xlate_logical;

	    dosfs_sflash_ftl_info_entry(sflash, sflash->xlate_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_XLATE | xlate_segment);

	    memset(xlate_cache, 0xff, DOSFS_SFLASH_BLOCK_SIZE);
	}
	else 
	{
	    if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_miss);

		sflash->xlate_logical = sflash->xlate_table[xlate_segment];

		dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
	    }
	    else
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_hit);;
	    }

	    if (xlate_cache[xlate_index] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	    {
		if (sflash->xlate2_table[xlate_segment] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
		{
		    sflash->xlate2_logical = dosfs_sflash_ftl_allocate(sflash);
		    sflash->xlate2_table[xlate_segment] = sflash->xlate2_logical;
		    
		    dosfs_sflash_ftl_info_entry(sflash, sflash->xlate2_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_XLATE_SECONDARY | xlate_segment);
		    
		    memset(xlate2_cache, 0xff, DOSFS_SFLASH_BLOCK_SIZE);
		}
		else
		{
		    if (sflash->xlate2_logical != sflash->xlate2_table[xlate_segment])
		    {
			DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_miss);

			sflash->xlate2_logical = sflash->xlate2_table[xlate_segment];
			
			dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate2_cache);
		    }
		    else
		    {
			DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_hit);
		    }

		    if (xlate2_cache[xlate_index] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
		    {
			/* Swap xlate & xlate2. Backprop all changes into xlate_cache, and then write
			 * xlate2 with the merged data back. Next allocate a new xlate2 and properly
			 * add the next logical mapping.
			 *
			 * Make sure the upper allocate did not blow away the xlate_cache ...
			 */

			if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
			{
			    DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_miss);

			    sflash->xlate_logical = sflash->xlate_table[xlate_segment];
			    
			    dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
			}
			else
			{
			    DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_hit);;
			}

			for (index = 0; index < DOSFS_SFLASH_XLATE_ENTRIES; index++)
			{
			    if (xlate2_cache[index] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
			    {
				xlate_cache[index] = xlate2_cache[index];
			    }
			}

			dosfs_sflash_nor_write(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)xlate_cache);
			dosfs_sflash_nor_write(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical) + DOSFS_SFLASH_PAGE_SIZE, DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)xlate_cache + DOSFS_SFLASH_PAGE_SIZE);

			/* Change xlate2 to xlate, and delete the old xlate entry.
			 */
			
			dosfs_sflash_ftl_info_type(sflash, sflash->xlate_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DELETED | xlate_segment);

			sflash->victim_score[sflash->xlate_logical >> DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;

			dosfs_sflash_ftl_info_type(sflash, sflash->xlate2_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_XLATE | xlate_segment);

			sflash->xlate_logical = sflash->xlate2_logical;
			sflash->xlate_table[xlate_segment] = sflash->xlate_logical;

			/* Allocate a new xlate2 and modify the target entry.
			 */

			sflash->xlate2_logical = dosfs_sflash_ftl_allocate(sflash);
			sflash->xlate2_table[xlate_segment] = sflash->xlate2_logical;

			dosfs_sflash_ftl_info_entry(sflash, sflash->xlate2_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_XLATE_SECONDARY | xlate_segment);

			memset(xlate2_cache, 0xff, DOSFS_SFLASH_BLOCK_SIZE);
		    }
		}
	    }
	}
    }

    /* Allocate a new block and write the data to it.
     */

    write_logical = dosfs_sflash_ftl_allocate(sflash);

    // ###
    dosfs_sflash_ftl_info_entry(sflash, write_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_RESERVED | address);

    write_offset = dosfs_sflash_ftl_translate(sflash, write_logical);
    
    dosfs_sflash_nor_write(sflash, write_offset, DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)data);
    dosfs_sflash_nor_write(sflash, write_offset + DOSFS_SFLASH_PAGE_SIZE, DOSFS_SFLASH_PAGE_SIZE, (const uint8_t*)data + DOSFS_SFLASH_PAGE_SIZE);



    /* Now we have a valid new block, and allocated xlate/xlate2 entries. So simply plug in the new
     * mapping.
     */

    if (address < DOSFS_SFLASH_XLATE_OFFSET)
    {
	dosfs_sflash_ftl_info_type(sflash, write_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED | address);

	read_logical = sflash->block_table[address];

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	assert(read_logical == sflash_xlate_shadow[address]);
	sflash_xlate_shadow[address] = write_logical;
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

	sflash->block_table[address] = write_logical;

	if (read_logical != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	{
	    dosfs_sflash_ftl_info_type(sflash, read_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DELETED);

	    sflash->victim_score[read_logical >> DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;
	}
    }
    else
    {
	dosfs_sflash_ftl_info_type(sflash, write_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DATA_WRITTEN | address);

	dosfs_sflash_ftl_modify(sflash, address, write_logical);
    }
}

static void dosfs_sflash_ftl_read(dosfs_sflash_t *sflash, uint32_t address, uint8_t *data)
{
    uint32_t read_logical, read_offset, xlate_segment, xlate_index;
    uint16_t *xlate_cache, *xlate2_cache;

    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_ftl_read, 512);

    if (address < DOSFS_SFLASH_XLATE_OFFSET)
    {
	read_logical = sflash->block_table[address];
    }
    else
    {
	xlate_cache   = (uint16_t*)sflash->cache[0];
	xlate2_cache  = (uint16_t*)sflash->cache[1];

	xlate_segment = (address - DOSFS_SFLASH_XLATE_OFFSET) >> DOSFS_SFLASH_XLATE_SEGMENT_SHIFT;
	xlate_index   = (address - DOSFS_SFLASH_XLATE_OFFSET) & DOSFS_SFLASH_XLATE_INDEX_MASK;

	if (sflash->xlate_table[xlate_segment] == DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	{
	    read_logical = DOSFS_SFLASH_BLOCK_NOT_ALLOCATED;
	}
	else
	{
	    if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_miss);

		sflash->xlate_logical = sflash->xlate_table[xlate_segment];

		dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
	    }
	    else
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_hit);;
	    }

	    read_logical = xlate_cache[xlate_index];

	    if (read_logical == DOSFS_SFLASH_BLOCK_DELETED)
	    {
		if (sflash->xlate2_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
		{
		    if (sflash->xlate2_logical != sflash->xlate2_table[xlate_segment])
		    {
			DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_miss);

			sflash->xlate2_logical = sflash->xlate2_table[xlate_segment];
			
			dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate2_cache);
		    }
		    else
		    {
			DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_hit);
		    }

		    if (xlate2_cache[xlate_index] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
		    {
			read_logical = xlate2_cache[xlate_index];
		    }
		}
	    }
	}
    }

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
    assert(read_logical == sflash_xlate_shadow[address]);
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

    if ((read_logical != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED) && (read_logical != DOSFS_SFLASH_BLOCK_DELETED))
    {
	read_offset = dosfs_sflash_ftl_translate(sflash, read_logical);
    
	dosfs_sflash_nor_read(sflash, read_offset, DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)data);
    }
    else
    {
	memset(data, 0xff, DOSFS_SFLASH_BLOCK_SIZE);
    }

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
    assert (!memcmp(&sflash_data_shadow[address * 512], data, 512));
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */
}

static void dosfs_sflash_ftl_discard(dosfs_sflash_t *sflash, uint32_t address)
{
    uint32_t delete_logical, xlate_segment, xlate_index;
    uint16_t *xlate_cache, *xlate2_cache;

    DOSFS_SFLASH_STATISTICS_COUNT_N(sflash_ftl_delete, 512);

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
    memset(&sflash_data_shadow[address * 512], 0xff, 512);
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

    if (address < DOSFS_SFLASH_XLATE_OFFSET)
    {
	delete_logical = sflash->block_table[address];

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	assert(delete_logical == sflash_xlate_shadow[address]);
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

	if ((delete_logical != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED) && (delete_logical != DOSFS_SFLASH_BLOCK_DELETED))
	{
#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	    sflash_xlate_shadow[address] = DOSFS_SFLASH_BLOCK_DELETED;
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

	    sflash->block_table[address] = DOSFS_SFLASH_BLOCK_DELETED;

	    dosfs_sflash_ftl_info_type(sflash, delete_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DELETED);
		
	    sflash->victim_score[delete_logical >> DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT] += DOSFS_SFLASH_VICTIM_DELETED_INCREMENT;
	}
    }
    else
    {
	xlate_cache   = (uint16_t*)sflash->cache[0];
	xlate2_cache  = (uint16_t*)sflash->cache[1];

	xlate_segment = (address - DOSFS_SFLASH_XLATE_OFFSET) >> DOSFS_SFLASH_XLATE_SEGMENT_SHIFT;
	xlate_index   = (address - DOSFS_SFLASH_XLATE_OFFSET) & DOSFS_SFLASH_XLATE_INDEX_MASK;

	if (sflash->xlate_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	{
	    if (sflash->xlate_logical != sflash->xlate_table[xlate_segment])
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_miss);

		sflash->xlate_logical = sflash->xlate_table[xlate_segment];

		dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate_cache);
	    }
	    else
	    {
		DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate_hit);;
	    }

	    delete_logical = xlate_cache[xlate_index];

	    if (delete_logical != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
	    {
		if (delete_logical == DOSFS_SFLASH_BLOCK_DELETED)
		{
		    if (sflash->xlate2_table[xlate_segment] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
		    {
			if (sflash->xlate2_logical != sflash->xlate2_table[xlate_segment])
			{
			    DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_miss);
			    
			    sflash->xlate2_logical = sflash->xlate2_table[xlate_segment];
			
			    dosfs_sflash_nor_read(sflash, dosfs_sflash_ftl_translate(sflash, sflash->xlate2_logical), DOSFS_SFLASH_BLOCK_SIZE, (uint8_t*)xlate2_cache);
			}
			else
			{
			    DOSFS_SFLASH_STATISTICS_COUNT(sflash_ftl_xlate2_hit);
			}

			if (xlate2_cache[xlate_index] != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED)
			{
			    delete_logical = xlate2_cache[xlate_index];
			}
		    }
		}
	    }

#if (DOSFS_CONFIG_SFLASH_DEBUG == 1)
	    assert(delete_logical == sflash_xlate_shadow[address]);
#endif /* DOSFS_CONFIG_SFLASH_DEBUG == 1 */

	    if ((delete_logical != DOSFS_SFLASH_BLOCK_NOT_ALLOCATED) && (delete_logical != DOSFS_SFLASH_BLOCK_DELETED))
	    {
		dosfs_sflash_ftl_info_type(sflash, delete_logical, DOSFS_SFLASH_INFO_EXTENDED_MASK | DOSFS_SFLASH_INFO_TYPE_DATA_DELETED | address);
		
		dosfs_sflash_ftl_trim(sflash, address, delete_logical);
	    }
	}
    }
}

static int dosfs_sflash_release(void *context)
{
    int status = F_NO_ERROR;

#if (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1)
    printf("SFLASH_RELEASE\n");
#endif /* (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1) */
    
    return status;
}

static int dosfs_sflash_info(void *context, uint8_t *p_media, uint8_t *p_write_protected, uint32_t *p_block_count, uint32_t *p_au_size, uint32_t *p_serial)
{
    dosfs_sflash_t *sflash = (dosfs_sflash_t*)context;
    int status = F_NO_ERROR;
    uint32_t blkcnt;

#if (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1)
    printf("SFLASH_INFO\n");
#endif /* (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1) */

    /* One sector needs to be put away as reclaim sector. There is the starting block of each erase block.
     * There are XLATE/XLATE2 entries for each set of 128 blocks.
     */

    blkcnt = ((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) -1) * ((DOSFS_SFLASH_ERASE_SIZE / DOSFS_SFLASH_BLOCK_SIZE) -1) -2 - (2 * sflash->xlate_count);

    *p_media = DOSFS_MEDIA_SFLASH;
    *p_write_protected = false;
    *p_block_count = blkcnt;
    *p_au_size = 1;
    *p_serial = 0;

    return status;
}

static int dosfs_sflash_format(void *context)
{
    dosfs_sflash_t *sflash = (dosfs_sflash_t*)context;
    int status = F_NO_ERROR;

#if (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1)
    printf("SFLASH_FORMAT\n");
#endif /* (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1) */

    stm32l4_qspi_select(&sflash->qspi);

    dosfs_sflash_ftl_format(sflash);

    if (!dosfs_sflash_ftl_mount(sflash))
    {
	sflash->state = DOSFS_SFLASH_STATE_NOT_FORMATTED;

	status = F_ERR_ONDRIVE;
    }
    else
    {
	sflash->state = DOSFS_SFLASH_STATE_READY;
    }

    stm32l4_qspi_unselect(&sflash->qspi);

    return status;
}

static int dosfs_sflash_erase(void *context, uint32_t address, uint32_t length)
{
    return F_NO_ERROR;
}

static int dosfs_sflash_discard(void *context, uint32_t address, uint32_t length)
{
    dosfs_sflash_t *sflash = (dosfs_sflash_t*)context;
    int status = F_NO_ERROR;

#if (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1)
    printf("SFLASH_DELETE %08x, %d\n", address, length);
#endif /* (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1) */

    if (sflash->state != DOSFS_SFLASH_STATE_READY)
    {
	status = F_ERR_ONDRIVE;
    }
    else
    {
	stm32l4_qspi_select(&sflash->qspi);

	while (length--)
	{
	    dosfs_sflash_ftl_discard(sflash, address);
	    
	    address++;
	}

	stm32l4_qspi_unselect(&sflash->qspi);
    }

    return status;
}

static int dosfs_sflash_read(void *context, uint32_t address, uint8_t *data, uint32_t length, bool prefetch)
{
    dosfs_sflash_t *sflash = (dosfs_sflash_t*)context;
    int status = F_NO_ERROR;

#if (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1)
    printf("SFLASH_READ %08x, %d\n", address, length);
#endif /* (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1) */

    if (sflash->state != DOSFS_SFLASH_STATE_READY)
    {
	status = F_ERR_ONDRIVE;
    }
    else
    {
	stm32l4_qspi_select(&sflash->qspi);

	while (length--)
	{
	    dosfs_sflash_ftl_read(sflash, address, data);
	    
	    address++;
	}

	stm32l4_qspi_unselect(&sflash->qspi);
    }

    return status;
}

static int dosfs_sflash_write(void *context, uint32_t address, const uint8_t *data, uint32_t length, volatile uint8_t *p_status)
{
    dosfs_sflash_t *sflash = (dosfs_sflash_t*)context;
    int status = F_NO_ERROR;

#if (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1)
    printf("SFLASH_WRITE %08x, %d\n", address, length);
#endif /* (DOSFS_CONFIG_SFLASH_SIMULATE_TRACE == 1) */

    if (sflash->state != DOSFS_SFLASH_STATE_READY)
    {
	status = F_ERR_ONDRIVE;
    }
    else
    {
	stm32l4_qspi_select(&sflash->qspi);

	while (length--)
	{
	    dosfs_sflash_ftl_write(sflash, address, data);
	    
	    address++;
	}

	stm32l4_qspi_unselect(&sflash->qspi);
    }

    return status;
}

static int dosfs_sflash_sync(void *context, bool wait)
{
    dosfs_sflash_t *sflash = (dosfs_sflash_t*)context;
    int status = F_NO_ERROR;

    if (sflash->state != DOSFS_SFLASH_STATE_READY)
    {
	status = F_ERR_ONDRIVE;
    }

    return status;
}

static const dosfs_device_interface_t dosfs_sflash_interface = {
    dosfs_sflash_release,
    dosfs_sflash_info,
    dosfs_sflash_format,
    dosfs_sflash_erase,
    dosfs_sflash_discard,
    dosfs_sflash_read,
    dosfs_sflash_write,
    dosfs_sflash_sync,
};

int dosfs_sflash_init(void)
{
    dosfs_sflash_t *sflash = (dosfs_sflash_t*)&dosfs_sflash;
    int status = F_NO_ERROR;
    uint8_t data[DOSFS_BLK_SIZE];

    dosfs_device.lock = DOSFS_DEVICE_LOCK_INIT;
    dosfs_device.context = (void*)sflash;
    dosfs_device.interface = &dosfs_sflash_interface;
    
    if (sflash->state == DOSFS_SFLASH_STATE_NONE)
    {
	sflash->data_size = dosfs_sflash_nor_identify(sflash);

	if (sflash->data_size == 0)
	{
	    status = F_ERR_INVALIDMEDIA;
	}
	else
	{
	    stm32l4_qspi_select(&sflash->qspi);

	    sflash->xlate_count = ((((sflash->data_size / DOSFS_SFLASH_ERASE_SIZE) * ((DOSFS_SFLASH_ERASE_SIZE / DOSFS_SFLASH_BLOCK_SIZE) -1)) -2) + (DOSFS_SFLASH_XLATE_ENTRIES -1)) / DOSFS_SFLASH_XLATE_ENTRIES;
	    
	    sflash->cache[0] = &dosfs_sflash_cache[0];
	    sflash->cache[1] = &dosfs_sflash_cache[DOSFS_SFLASH_BLOCK_SIZE / sizeof(uint32_t)];
	    
	    if (!dosfs_sflash_ftl_mount(sflash))
	    {
		stm32l4_qspi_unselect(&sflash->qspi);

		status = dosfs_device_format(&dosfs_device, data);

		if (status == F_NO_ERROR)
		{
		    sflash->state = DOSFS_SFLASH_STATE_READY;
		}
		else
		{
		    sflash->state = DOSFS_SFLASH_STATE_NOT_FORMATTED;

		    status = F_ERR_NOTFORMATTED;
		}
	    }
	    else
	    {
		stm32l4_qspi_unselect(&sflash->qspi);

		sflash->state = DOSFS_SFLASH_STATE_READY;
	    }
	}
    }

    dosfs_device.lock = 0;

    return status;
}

#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
