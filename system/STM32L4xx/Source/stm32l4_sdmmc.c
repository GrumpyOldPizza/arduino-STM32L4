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

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)

#include "stm32l4_sdmmc.h"
#include "stm32l4_system.h"

#include "armv7m_systick.h"

static stm32l4_sdmmc_t stm32l4_sdmmc;

#define SDMMC_DCTRL_DBLOCKSIZE_1B            (0u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_2B            (1u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_4B            (2u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_8B            (3u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_16B           (4u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_32B           (5u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_64B           (6u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_128B          (7u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_256B          (8u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_512B          (9u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_1024B         (10u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_2048B         (11u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_4096B         (12u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_8192B         (13u << SDMMC_DCTRL_DBLOCKSIZE_Pos)
#define SDMMC_DCTRL_DBLOCKSIZE_16384B        (14u << SDMMC_DCTRL_DBLOCKSIZE_Pos)

#define SD_CMD_GO_IDLE_STATE           (0)
#define SD_CMD_SEND_ALL_CID            (2)
#define SD_CMD_SEND_RELATIVE_ADDR      (3)
#define SD_CMD_SET_DSR                 (4)
#define SD_CMD_SWITCH_FUNC             (6)
#define SD_CMD_SELECT_DESELECT_CARD    (7)
#define SD_CMD_SEND_IF_COND            (8)
#define SD_CMD_SEND_CSD                (9)
#define SD_CMD_SEND_CID                (10)
#define SD_CMD_VOLTAGE_SWITCH          (11)
#define SD_CMD_STOP_TRANSMISSION       (12)
#define SD_CMD_SEND_STATUS             (13)
#define SD_CMD_GO_INACTIVE_STATE       (15)
#define SD_CMD_SET_BLOCKLEN            (16)
#define SD_CMD_READ_SINGLE_BLOCK       (17)
#define SD_CMD_READ_MULTIPLE_BLOCK     (18)
#define SD_CMD_SPEED_CLASS_CONTROL     (20)
#define SD_CMD_SET_BLOCK_COUNT         (23)
#define SD_CMD_WRITE_SINGLE_BLOCK      (24)
#define SD_CMD_WRITE_MULTIPLE_BLOCK    (25)
#define SD_CMD_PROGRAM_CSD             (27)
#define SD_CMD_ERASE_WR_BLK_START_ADDR (32)
#define SD_CMD_ERASE_WR_BLK_END_ADDR   (33)
#define SD_CMD_ERASE                   (38)
#define SD_CMD_APP_CMD                 (55)

#define SD_ACMD_SET_BUS_WIDTH          (6)
#define SD_ACMD_SD_STATUS              (13)
#define SD_ACMD_SEND_NUM_WR_BLOCKS     (22)
#define SD_ACMD_SET_WR_BLK_ERASE_COUNT (23)
#define SD_ACMD_SD_SEND_OP_COND        (41)
#define SD_ACMD_SET_CLR_CARD_DETECT    (42)
#define SD_ACMD_SEND_SCR               (51)

#define SD_R1_AKE_SEQ_ERROR            0x00000008
#define SD_R1_ADD_CMD                  0x00000020
#define SD_R1_READY_FOR_DATA           0x00000100
#define SD_R1_CURRENT_STATE_MASK       0x00001e00
#define SD_R1_CURRENT_STATE_IDLE       0x00000000
#define SD_R1_CURRENT_STATE_READY      0x00000200
#define SD_R1_CURRENT_STATE_IDENT      0x00000400
#define SD_R1_CURRENT_STATE_STBY       0x00000600
#define SD_R1_CURRENT_STATE_TRAN       0x00000800
#define SD_R1_CURRENT_STATE_DATA       0x00000a00
#define SD_R1_CURRENT_STATE_RCV        0x00000c00
#define SD_R1_CURRENT_STATE_PRG        0x00000e00
#define SD_R1_CURRENT_STATE_DIS        0x00001000
#define SD_R1_ERASE_RESET              0x00002000
#define SD_R1_CARD_ECC_DISABLED        0x00004000
#define SD_R1_WP_ERASE_SKIP            0x00008000
#define SD_R1_CSD_OVERWRITE            0x00010000
#define SD_R1_ERROR                    0x00080000
#define SD_R1_CC_ERROR                 0x00100000
#define SD_R1_CARD_ECC_FAILED          0x00200000
#define SD_R1_ILLEGAL_COMMAND          0x00400000
#define SD_R1_COM_CRC_ERROR            0x00800000
#define SD_R1_LOCK_UNLOCK_FAILED       0x01000000
#define SD_R1_CARD_IS_LOCKED           0x02000000
#define SD_R1_WP_VIOLATION             0x04000000
#define SD_R1_ERASE_PARAM              0x08000000
#define SD_R1_ERASE_SEQ_ERROR          0x10000000
#define SD_R1_BLOCK_LEN_ERROR          0x20000000
#define SD_R1_ADDRESS_ERROR            0x40000000
#define SD_R1_OUT_OF_RANGE             0x80000000
#define SD_R1_PREVIOUS_ERROR_MASK      0x00c00000
#define SD_R1_CURRENT_ERROR_MASK       0xfd398008

#define SD_R6_CURRENT_STATE_MASK       0x00001e00
#define SD_R6_CURRENT_STATE_IDLE       0x00000000
#define SD_R6_CURRENT_STATE_READY      0x00000200
#define SD_R6_CURRENT_STATE_IDENT      0x00000400
#define SD_R6_CURRENT_STATE_STBY       0x00000600
#define SD_R6_CURRENT_STATE_TRAN       0x00000800
#define SD_R6_CURRENT_STATE_DATA       0x00000a00
#define SD_R6_CURRENT_STATE_RCV        0x00000c00
#define SD_R6_CURRENT_STATE_PRG        0x00000e00
#define SD_R6_CURRENT_STATE_DIS        0x00001000
#define SD_R6_ERROR                    0x00002000
#define SD_R6_ILLEGAL_COMMAND          0x00004000
#define SD_R6_COM_CRC_ERROR            0x00008000

#define SD_WAIT_RESPONSE_NONE          (0)
#define SD_WAIT_RESPONSE_SHORT         (SDMMC_CMD_WAITRESP_0)
#define SD_WAIT_RESPONSE_LONG          (SDMMC_CMD_WAITRESP_0 | SDMMC_CMD_WAITRESP_1)

#define STM32L4_SDMMC_CONTROL_STOP     0x00000001
#define STM32L4_SDMMC_CONTROL_PREFETCH 0x00000002
#define STM32L4_SDMMC_CONTROL_CONTINUE 0x00000004

static inline __attribute__((optimize("O3"),always_inline)) uint32_t stm32l4_sdmmc_slice(const uint8_t *data, uint32_t size, uint32_t start, uint32_t width)
{
    uint32_t mask, shift;

    data += ((size >> 3) -1);
    data -= (start >> 3);

    shift = start & 7;
    mask = (data[0] >> shift);

    if ((width + shift) > 8)
    {
	mask |= (data[-1] << (8 - shift));

	if ((width + shift) > 16)
	{
	    mask |= (data[-2] << (16 - shift));

	    if ((width + shift) > 24)
	    {
		mask |= (data[-3] << (24 - shift));

		if ((width + shift) > 32)
		{
		    mask |= (data[-4] << (32 - shift));
		}
	    }
	}
    }

    return mask & (0xffffffff >> (32 - width));
}

static bool stm32l4_sdmmc_detect(stm32l4_sdmmc_t *sdmmc);
static void stm32l4_sdmmc_select(stm32l4_sdmmc_t *sdmmc);
static void stm32l4_sdmmc_deselect(stm32l4_sdmmc_t *sdmmc);
static void stm32l4_sdmmc_mode(stm32l4_sdmmc_t *sdmmc, uint32_t mode);
static int stm32l4_sdmmc_command(stm32l4_sdmmc_t *sdmmc, uint8_t index, uint32_t argument, uint32_t wait);
static int stm32l4_sdmmc_receive(stm32l4_sdmmc_t *sdmmc, uint8_t index, uint32_t argument, uint8_t *data, uint32_t count, uint32_t *p_count, uint32_t control);
static int stm32l4_sdmmc_transmit(stm32l4_sdmmc_t *sdmmc, const uint8_t *data, uint32_t count, bool *p_check, bool stop);
static int stm32l4_sdmmc_read_stop(stm32l4_sdmmc_t *sdmmc);
static int stm32l4_sdmmc_write_stop(stm32l4_sdmmc_t *sdmmc);
static int stm32l4_sdmmc_write_sync(stm32l4_sdmmc_t *sdmmc);

static int stm32l4_sdmmc_idle(stm32l4_sdmmc_t *sdmmc, uint32_t *p_media);
static int stm32l4_sdmmc_reset(stm32l4_sdmmc_t *sdmmc, uint32_t media);
static int stm32l4_sdmmc_lock(stm32l4_sdmmc_t *sdmmc, int state, uint32_t address);
static int stm32l4_sdmmc_unlock(stm32l4_sdmmc_t *sdmmc, int status);


static __attribute__((optimize("O3"))) uint32_t stm32l4_sdmmc_read_fifo(const stm32l4_sdmmc_t *sdmmc, uint8_t *data, uint32_t count, uint32_t *p_count)
{
    uint32_t *data32, *data32_e;
    uint32_t sdmmc_sta;

    if (count <= 64)
    {
	data32 = (uint32_t*)((void*)data);
	data32_e = (uint32_t*)((void*)(data + count));

	do
	{
	    sdmmc_sta = SDMMC1->STA;
	}
	while (!(sdmmc_sta & (SDMMC_STA_DBCKEND | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT)));
	
	if (sdmmc_sta & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT))
	{
	    goto failure;
	}
	
	do
	{
	    data32[0] = SDMMC1->FIFO;
	    
	    data32 += 1;
	}
	while (data32 != data32_e);
    }
    else
    {
	/* This code below is somewhat iffy. Problem at hand is that SDMMC_STA_DBCKEND
	 * is per block, not per block sequence. So the idea is to clear this flags
	 * in the middle of the last block. Since hardware control flow is in use,
	 * the FIFO cannot overrun, and the flag is relieable cleared in the last
	 * block.
	 *
	 * The loops below are organized so that the FIFO is first drained before
	 * the DCRCFAIL/DTIMEOUT is attended to. Thus a CRCFAIL will be reported
	 * after all the data for the failing block is read.
	 */
	data32 = (uint32_t*)((void*)data);
	data32_e = (uint32_t*)((void*)(data + count)) - 64;

	do
	{
	    sdmmc_sta = SDMMC1->STA;
	    
	    if (sdmmc_sta & SDMMC_STA_RXFIFOHF)
	    {
		data32[0] = SDMMC1->FIFO;
		data32[1] = SDMMC1->FIFO;
		data32[2] = SDMMC1->FIFO;
		data32[3] = SDMMC1->FIFO;
		data32[4] = SDMMC1->FIFO;
		data32[5] = SDMMC1->FIFO;
		data32[6] = SDMMC1->FIFO;
		data32[7] = SDMMC1->FIFO;
		
		data32 += 8;
	    }
	    else
	    {
		if (sdmmc_sta & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT))
		{
		    goto failure;
		}
	    }
	}
	while (data32 != data32_e);

	SDMMC1->ICR = SDMMC_ICR_DBCKENDC;
	
	data32_e += 64;
	
	do
	{
	    sdmmc_sta = SDMMC1->STA;
	    
	    if (sdmmc_sta & SDMMC_STA_RXFIFOHF)
	    {
		data32[0] = SDMMC1->FIFO;
		data32[1] = SDMMC1->FIFO;
		data32[2] = SDMMC1->FIFO;
		data32[3] = SDMMC1->FIFO;
		data32[4] = SDMMC1->FIFO;
		data32[5] = SDMMC1->FIFO;
		data32[6] = SDMMC1->FIFO;
		data32[7] = SDMMC1->FIFO;
		
		data32 += 8;
	    }
	    else
	    {
		if (sdmmc_sta & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT))
		{
		    goto failure;
		}
	    }
	    
	}
	while (data32 != data32_e);
    }

failure:
    *p_count = (uint8_t*)data32 - data;

    return sdmmc_sta;
}

static __attribute__((optimize("O3"))) uint32_t stm32l4_sdmmc_write_fifo(const stm32l4_sdmmc_t *sdmmc, const uint8_t *data, uint32_t count)
{
    uint32_t sdmmc_sta;
    const uint32_t *data32, *data32_e;

    data32 = (const uint32_t*)((const void*)data);
    
    do
    {
	data32_e = data32 + 128;
	
	do
	{
	    sdmmc_sta = SDMMC1->STA;
	    
	    if (sdmmc_sta & SDMMC_STA_DTIMEOUT)
	    {
		goto failure;
	    }
	}
	while (sdmmc_sta & SDMMC_STA_TXACT);
	
	SDMMC1->DTIMER = sdmmc->write_timeout;
	// SDMMC1->DTIMER = sdmmc->speed / (1000 / 250); /* 250ms timeout for SDHC */
	SDMMC1->DLEN = 512;
	SDMMC1->DCTRL = SDMMC_DCTRL_DBLOCKSIZE_512B | SDMMC_DCTRL_DTEN;
	
	do
	{
	    sdmmc_sta = SDMMC1->STA;

	    if (sdmmc_sta & SDMMC_STA_TXFIFOHE)
	    {
		SDMMC1->FIFO = data32[0];
		SDMMC1->FIFO = data32[1];
		SDMMC1->FIFO = data32[2];
		SDMMC1->FIFO = data32[3];
		SDMMC1->FIFO = data32[4];
		SDMMC1->FIFO = data32[5];
		SDMMC1->FIFO = data32[6];
		SDMMC1->FIFO = data32[7];
		
		data32 += 8;
	    }
	    else
	    {
		if (sdmmc_sta & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT))
		{
		    goto failure;
		}
	    }
	}
	while (data32 != data32_e);
	
	do
	{
	    sdmmc_sta = SDMMC1->STA;

	    if (sdmmc_sta & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT))
	    {
		goto failure;
	    }
	}
	while (!(sdmmc_sta & SDMMC_STA_DBCKEND));
	
	SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;
	
	count -= 512;
    }
    while (count);
    
failure:
    return sdmmc_sta;
}

static bool stm32l4_sdmmc_detect(stm32l4_sdmmc_t *sdmmc)
{
    bool detect;

    /* This below is kind of fragile. The idea is to first set CS/DAT3 to 0, wait a tad
     * till the input settles to a 0. The switch the mode to input, while will make
     * the external pullup on CS/DAT3 take effect. The delays are required to let
     * the signal on CS/DAT3 settle.
     */

    stm32l4_gpio_pin_output(GPIO_PIN_PC11);
    stm32l4_gpio_pin_write(GPIO_PIN_PC11, 0);
    armv7m_core_udelay(2);
    stm32l4_gpio_pin_input(GPIO_PIN_PC11);
    armv7m_core_udelay(2);
    detect = !!stm32l4_gpio_pin_read(GPIO_PIN_PC11);
    stm32l4_gpio_pin_alternate(GPIO_PIN_PC11);

    return detect;
}

static void stm32l4_sdmmc_select(stm32l4_sdmmc_t *sdmmc)
{
    SDMMC1->CLKCR &= ~SDMMC_CLKCR_PWRSAV;
}

static void stm32l4_sdmmc_deselect(stm32l4_sdmmc_t *sdmmc)
{
    if (sdmmc->state != STM32L4_SDMMC_STATE_READ_MULTIPLE)
    {
	SDMMC1->CLKCR |= SDMMC_CLKCR_PWRSAV;
    }
}

static void stm32l4_sdmmc_mode(stm32l4_sdmmc_t *sdmmc, uint32_t mode)
{
    uint32_t speed, omask, omode;

    if (mode == STM32L4_SDMMC_MODE_NONE)
    {
	stm32l4_gpio_pin_configure(GPIO_PIN_PC8_SDMMC1_D0,  (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | GPIO_OSPEED_LOW | GPIO_MODE_INPUT));
	stm32l4_gpio_pin_configure(GPIO_PIN_PC9_SDMMC1_D1,  (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | GPIO_OSPEED_LOW | GPIO_MODE_INPUT));
	stm32l4_gpio_pin_configure(GPIO_PIN_PC10_SDMMC1_D2, (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | GPIO_OSPEED_LOW | GPIO_MODE_INPUT));
	stm32l4_gpio_pin_configure(GPIO_PIN_PC11_SDMMC1_D3, (GPIO_PUPD_NONE   | GPIO_OTYPE_PUSHPULL | GPIO_OSPEED_LOW | GPIO_MODE_INPUT));
	stm32l4_gpio_pin_configure(GPIO_PIN_PC12_SDMMC1_CK, (GPIO_PUPD_NONE   | GPIO_OTYPE_PUSHPULL | GPIO_OSPEED_LOW | GPIO_MODE_INPUT));
	stm32l4_gpio_pin_configure(GPIO_PIN_PD2_SDMMC1_CMD, (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | GPIO_OSPEED_LOW | GPIO_MODE_INPUT));

	SDMMC1->CLKCR = 0;

	armv7m_core_udelay(5);

	SDMMC1->POWER = 0;

	armv7m_core_udelay(5);

	stm32l4_system_periph_disable(SYSTEM_PERIPH_SDMMC1);

	stm32l4_system_clk48_release(SYSTEM_CLK48_REFERENCE_SDMMC1);

	sdmmc->speed = 0;
    }
    else
    {
	stm32l4_system_clk48_acquire(SYSTEM_CLK48_REFERENCE_SDMMC1);

	stm32l4_system_periph_enable(SYSTEM_PERIPH_SDMMC1);

	armv7m_core_udelay(5);

	SDMMC1->POWER = SDMMC_POWER_PWRCTRL;

	armv7m_core_udelay(5);

	if (mode == STM32L4_SDMMC_MODE_IDENTIFY)
	{
	    stm32l4_gpio_pin_configure(GPIO_PIN_PC11, (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | GPIO_OSPEED_LOW | GPIO_MODE_OUTPUT));
	    stm32l4_gpio_pin_write(GPIO_PIN_PC11, 1);

	    armv7m_core_udelay(5);
	    
	    SDMMC1->CLKCR = (120 - 2) | SDMMC_CLKCR_CLKEN | SDMMC_CLKCR_HWFC_EN;

	    armv7m_core_udelay(250);

	    speed = 400000;

	    omask = 0x31;
	    omode = GPIO_OSPEED_LOW | GPIO_MODE_ALTERNATE;
	}
	else if (mode == STM32L4_SDMMC_MODE_DATA_TRANSFER)
	{
	    SDMMC1->CLKCR = SDMMC_CLKCR_CLKEN | SDMMC_CLKCR_HWFC_EN;

	    speed = 24000000;

	    omask = 0x31;
	    omode = GPIO_OSPEED_MEDIUM | GPIO_MODE_ALTERNATE;
	}
	else if (mode == STM32L4_SDMMC_MODE_DATA_TRANSFER_WIDE)
	{
	    SDMMC1->CLKCR = SDMMC_CLKCR_CLKEN | SDMMC_CLKCR_WIDBUS_0 | SDMMC_CLKCR_HWFC_EN;

	    speed = 24000000;

	    omask = 0x3f;
	    omode = GPIO_OSPEED_MEDIUM | GPIO_MODE_ALTERNATE;
	}
	else
	{
	    SDMMC1->CLKCR = SDMMC_CLKCR_CLKEN | SDMMC_CLKCR_BYPASS | SDMMC_CLKCR_WIDBUS_0 | SDMMC_CLKCR_HWFC_EN;

	    speed = 48000000;

	    omask = 0x3f;
	    omode = GPIO_OSPEED_FAST | GPIO_MODE_ALTERNATE;
	}

	stm32l4_gpio_pin_configure(GPIO_PIN_PC8_SDMMC1_D0,  (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | ((omask & 0x01) ? omode : (GPIO_OSPEED_LOW | GPIO_MODE_INPUT))));
	stm32l4_gpio_pin_configure(GPIO_PIN_PC9_SDMMC1_D1,  (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | ((omask & 0x02) ? omode : (GPIO_OSPEED_LOW | GPIO_MODE_INPUT))));
	stm32l4_gpio_pin_configure(GPIO_PIN_PC10_SDMMC1_D2, (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | ((omask & 0x04) ? omode : (GPIO_OSPEED_LOW | GPIO_MODE_INPUT))));

	if (mode != STM32L4_SDMMC_MODE_IDENTIFY)
	{
	    stm32l4_gpio_pin_configure(GPIO_PIN_PC11_SDMMC1_D3, (GPIO_PUPD_NONE | GPIO_OTYPE_PUSHPULL | ((omask & 0x08) ? omode : (GPIO_OSPEED_LOW | GPIO_MODE_INPUT))));
	}

	stm32l4_gpio_pin_configure(GPIO_PIN_PC12_SDMMC1_CK, (GPIO_PUPD_NONE   | GPIO_OTYPE_PUSHPULL | ((omask & 0x10) ? omode : (GPIO_OSPEED_LOW | GPIO_MODE_INPUT))));
	stm32l4_gpio_pin_configure(GPIO_PIN_PD2_SDMMC1_CMD, (GPIO_PUPD_PULLUP | GPIO_OTYPE_PUSHPULL | ((omask & 0x20) ? omode : (GPIO_OSPEED_LOW | GPIO_MODE_INPUT))));

	sdmmc->speed = speed;
	sdmmc->read_timeout  = (speed / (1000 / 100)) + 4114;
	sdmmc->write_timeout = (speed / (1000 / 250)) + 4114;

    }
}

static int stm32l4_sdmmc_command(stm32l4_sdmmc_t *sdmmc, uint8_t index, const uint32_t argument, uint32_t wait)
{
    int status = F_NO_ERROR;
    uint32_t sdmmc_sta;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_COMMAND_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0) */
    unsigned int retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0) */

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_command);

    do
    {
	SDMMC1->ARG = argument;
	SDMMC1->CMD = index | wait | SDMMC_CMD_CPSMEN;
    
	if (wait == SD_WAIT_RESPONSE_NONE)
	{
	    do
	    {
		sdmmc_sta = SDMMC1->STA;
	    }
	    while (!(sdmmc_sta & (SDMMC_STA_CMDSENT | SDMMC_STA_CTIMEOUT)));
	}
	else
	{
	    do
	    {
		sdmmc_sta = SDMMC1->STA;
	    }
	    while (!(sdmmc_sta & (SDMMC_STA_CMDREND | SDMMC_STA_CCRCFAIL | SDMMC_STA_CTIMEOUT)));
	}

	SDMMC1->ICR = SDMMC_ICR_CMDSENTC | SDMMC_ICR_CMDRENDC | SDMMC_ICR_CTIMEOUTC | SDMMC_ICR_CCRCFAILC;

	/* Response R3 (OCR register) has no CRC7, hence filter out a CRC error.
	 */
	if (index == SD_ACMD_SD_SEND_OP_COND)
	{
	    sdmmc_sta &= ~SDMMC_STA_CCRCFAIL;
	}

	if (sdmmc_sta & SDMMC_STA_CTIMEOUT)
	{
	    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_command_timeout);

#if (DOSFS_CONFIG_STATISTICS == 1)
	    sdmmc->statistics.sdcard_command_timeout_2[index]++;
#endif

#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0)
	    if (retries > 1)
	    {
		STM32L4_SDMMC_STATISTICS_COUNT(sdcard_command_retry);
		retries--;
	    }
	    else
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0) */
	    {
		status = F_ERR_ONDRIVE;

		retries = 0;
	    }
	}
	else
	{
	    if (wait != SD_WAIT_RESPONSE_NONE)
	    {
		if (sdmmc_sta & SDMMC_STA_CCRCFAIL)
		{
		    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_command_crcfail);

#if (DOSFS_CONFIG_STATISTICS == 1)
		    sdmmc->statistics.sdcard_command_crcfail_2[index]++;
#endif

		    status = F_ERR_ONDRIVE;
		}
		else
		{
		    if ((index == SD_CMD_SEND_ALL_CID) || (index == SD_CMD_SEND_CSD) || (index == SD_CMD_SEND_CID) || (index == SD_ACMD_SD_SEND_OP_COND))
		    {
			if (SDMMC1->RESPCMD != 0x3f)
			{
			    status = F_ERR_ONDRIVE;
			}
		    }
		    else
		    {
			if (SDMMC1->RESPCMD != index)
			{
			    status = F_ERR_ONDRIVE;
			}
			else
			{
			    if (index == SD_CMD_SEND_RELATIVE_ADDR)
			    {
				if (SDMMC1->RESP1 & SD_R6_ERROR)
				{
				    status = F_ERR_ONDRIVE;
				}
			    }
			    else
			    {
				if (!((index == SD_CMD_SEND_IF_COND) || (index == SD_CMD_STOP_TRANSMISSION) || (index == SD_CMD_SEND_STATUS)))
				{
				    if (SDMMC1->RESP1 & SD_R1_CURRENT_ERROR_MASK)
				    {
					status = F_ERR_ONDRIVE;
				    }
				}
			    }
			}
		    }
		}
	    }

	    retries = 0;
	}
    }
    while (retries != 0);

    if (status != F_NO_ERROR)
    {
	STM32L4_SDMMC_STATISTICS_COUNT(sdcard_command_fail);
    }

    return status;
}

static int stm32l4_sdmmc_receive(stm32l4_sdmmc_t *sdmmc, uint8_t index, const uint32_t argument, uint8_t *data, uint32_t count, uint32_t *p_count, uint32_t control)
{
    int status = F_NO_ERROR;
    uint32_t sdmmc_dctrl, sdmmc_sta, response, blksz, offset;

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive);

    if (control & STM32L4_SDMMC_CONTROL_CONTINUE)
    {
	blksz = 512;
    }
    else
    {
	if (count >= 512)
	{
	    blksz = 512;
	    
	    sdmmc_dctrl = SDMMC_DCTRL_DBLOCKSIZE_512B | SDMMC_DCTRL_DTDIR;
	}
	else
	{
	    blksz = count;
	    
	    if (count == 64)
	    {
		sdmmc_dctrl = SDMMC_DCTRL_DBLOCKSIZE_64B | SDMMC_DCTRL_DTDIR;
	    }
	    else if (count == 8)
	    {
		sdmmc_dctrl = SDMMC_DCTRL_DBLOCKSIZE_8B | SDMMC_DCTRL_DTDIR;
	    }
	    else
	    {
		sdmmc_dctrl = SDMMC_DCTRL_DBLOCKSIZE_4B | SDMMC_DCTRL_DTDIR;
	    }
	}
	
	SDMMC1->DTIMER = sdmmc->read_timeout; 
	SDMMC1->DLEN = ((control & STM32L4_SDMMC_CONTROL_PREFETCH) ? 0x00ffffff : count);
	SDMMC1->DCTRL = sdmmc_dctrl | SDMMC_DCTRL_DTEN;

	status = stm32l4_sdmmc_command(sdmmc, index, argument, SD_WAIT_RESPONSE_SHORT);
    }

    if (status == F_NO_ERROR)
    {
	sdmmc_sta = stm32l4_sdmmc_read_fifo(sdmmc, data, count, &offset);

	if (sdmmc_sta & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT))
	{
	    SDMMC1->DCTRL = 0;
	    SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;

	    if (sdmmc_sta & SDMMC_STA_DCRCFAIL)
	    {
		STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_crcfail);

		if (offset > blksz)
		{
		    *p_count = (offset & ~(blksz - 1)) - blksz;
		}
		else
		{
		    *p_count = 0;
		}
	    }
	    
	    if (sdmmc_sta & SDMMC_STA_DTIMEOUT)
	    {
		STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_timeout);

		*p_count = (offset & ~(blksz - 1));
	    }

	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_STOP_TRANSMISSION, 0, SD_WAIT_RESPONSE_SHORT);

	    if (status == F_NO_ERROR)
	    {
	        response = SDMMC1->RESP1;

		status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_STATUS, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
		    
		if (status == F_NO_ERROR)
		{
		    response |= SDMMC1->RESP1;
			
		    if (response & SD_R1_CURRENT_ERROR_MASK)
		    {
			if (response & (SD_R1_WP_VIOLATION | SD_R1_WP_ERASE_SKIP))
			{
			    status = F_ERR_WRITEPROTECT;
			}
			else if (response & SD_R1_CARD_ECC_FAILED)
			{
			    status = F_ERR_INVALIDSECTOR;
			}
			else
			{
			    status = F_ERR_ONDRIVE;
			}
		    }
		}
	    }

	    if (status != F_NO_ERROR)
	    {
		STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_fail);
	    }
	}
	else
	{
	    if (!(control & (STM32L4_SDMMC_CONTROL_PREFETCH | STM32L4_SDMMC_CONTROL_CONTINUE)))
	    {
		SDMMC1->DCTRL = 0;
		SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;
	    }

	    *p_count = count;
	}
    }
    else
    {
	SDMMC1->DCTRL = 0;
	SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;

	*p_count = 0;
    }
    
    return status;
}

static int stm32l4_sdmmc_transmit(stm32l4_sdmmc_t *sdmmc, const uint8_t *data, uint32_t count, bool *p_check, bool stop)
{
    int status = F_NO_ERROR;
    uint32_t sdmmc_sta, response, millis;

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit);

    *p_check = false;

    sdmmc_sta = stm32l4_sdmmc_write_fifo(sdmmc, data, count);

    if (sdmmc_sta & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT))
    {
	SDMMC1->DCTRL = 0;
	SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;

        if (sdmmc_sta & SDMMC_STA_DCRCFAIL)
	{
	    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_crcfail);

	    *p_check = true;
	}

        if (sdmmc_sta & SDMMC_STA_DTIMEOUT)
	{
	    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_timeout);

	    *p_check = true;
	}

	if (!stop)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_STOP_TRANSMISSION, 0, SD_WAIT_RESPONSE_SHORT);
	}

	if (status == F_NO_ERROR)
	{
	    sdmmc->state = STM32L4_SDMMC_STATE_WRITE_STOP;

	    response = 0;

	    millis = armv7m_systick_millis();

	    do
	    {
		status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_STATUS, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
		
		if (status == F_NO_ERROR)
		{
		    response |= SDMMC1->RESP1;

		    if ((SDMMC1->RESP1 & SD_R1_CURRENT_STATE_MASK) != SD_R1_CURRENT_STATE_PRG)
		    {
			SDMMC1->DCTRL = 0;
			SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;

			sdmmc->state = STM32L4_SDMMC_STATE_READY;

			break;
		    }
		    else
		    {
			if (((uint32_t)armv7m_systick_millis() - millis) > 250)
			{
			    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_timeout2);

			    status = F_ERR_ONDRIVE;
			}
		    }
		}
	    }
	    while (status == F_NO_ERROR);

	    if (status == F_NO_ERROR)
	    {
		if (response & SD_R1_CURRENT_ERROR_MASK)
		{
		    if (response & (SD_R1_WP_VIOLATION | SD_R1_WP_ERASE_SKIP))
		    {
			status = F_ERR_WRITEPROTECT;
		    }
		    else if (response & SD_R1_CARD_ECC_FAILED)
		    {
			status = F_ERR_INVALIDSECTOR;
		    }
		    else
		    {
			status = F_ERR_ONDRIVE;
		    }
		}
	    }
	}

	if (status != F_NO_ERROR)
	{
	    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_fail);
	}
    }
    else
    {
	if (stop)
	{
	    sdmmc->state = STM32L4_SDMMC_STATE_WRITE_STOP;
	}
    }

    return status;
}

static int stm32l4_sdmmc_read_stop(stm32l4_sdmmc_t *sdmmc)
{
    int status = F_NO_ERROR;
    uint32_t response;

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_read_stop);

    SDMMC1->DCTRL = 0;
    SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;
    
    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_STOP_TRANSMISSION, 0, SD_WAIT_RESPONSE_SHORT);

    if (status == F_NO_ERROR)
    {
	sdmmc->state = STM32L4_SDMMC_STATE_READY;

	response = SDMMC1->RESP1;

	if (response & SD_R1_CURRENT_ERROR_MASK)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_STATUS, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
				
	    if (status == F_NO_ERROR)
	    {
		response |= SDMMC1->RESP1;
				    
		if (response & (SD_R1_WP_VIOLATION | SD_R1_WP_ERASE_SKIP))
		{
		    status = F_ERR_WRITEPROTECT;
		}
		else if (response & SD_R1_CARD_ECC_FAILED)
		{
		    status = F_ERR_INVALIDSECTOR;
		}
		else
		{
		    status = F_ERR_ONDRIVE;
		}
	    }
	}
    }
				
    return status;
}

static int stm32l4_sdmmc_write_stop(stm32l4_sdmmc_t *sdmmc)
{
    int status = F_NO_ERROR;

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_write_stop);

    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_STOP_TRANSMISSION, 0, SD_WAIT_RESPONSE_SHORT);

    if (status == F_NO_ERROR)
    {
	sdmmc->state = STM32L4_SDMMC_STATE_WRITE_STOP;
    }

    return status;
}

static int stm32l4_sdmmc_write_sync(stm32l4_sdmmc_t *sdmmc)
{
    int status = F_NO_ERROR;
    uint32_t response, millis;

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_write_sync);

    response = 0;

    millis = armv7m_systick_millis();

    do
    {
	status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_STATUS, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
	
	if (status == F_NO_ERROR)
	{
	    response |= SDMMC1->RESP1;
		    
	    if ((SDMMC1->RESP1 & SD_R1_CURRENT_STATE_MASK) != SD_R1_CURRENT_STATE_PRG)
	    {
		while (SDMMC1->STA & SDMMC_STA_TXACT)
		{
		}
		
		SDMMC1->DCTRL = 0;
		SDMMC1->ICR = SDMMC_ICR_DBCKENDC | SDMMC_ICR_DATAENDC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DCRCFAILC;
		
		sdmmc->state = STM32L4_SDMMC_STATE_READY;
		
		break;
	    }
	    else
	    {
		if (((uint32_t)armv7m_systick_millis() - millis) > 250)
		{
		    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_timeout2);
		    
		    status = F_ERR_ONDRIVE;
		}
	    }
	}
    }
    while (status == F_NO_ERROR);

    if (status == F_NO_ERROR)
    {
	if (response & SD_R1_CURRENT_ERROR_MASK)
	{
	    if (response & (SD_R1_WP_VIOLATION | SD_R1_WP_ERASE_SKIP | SD_R1_CARD_ECC_FAILED))
	    {
		if (response & (SD_R1_WP_VIOLATION | SD_R1_WP_ERASE_SKIP))
		{
		    status = F_ERR_WRITEPROTECT;
		}
		else
		{
		    status = F_ERR_INVALIDSECTOR;
		}

		if (sdmmc->p_status != NULL)
		{
		    if (*sdmmc->p_status == F_NO_ERROR)
		    {
			*sdmmc->p_status = status;
		    }
			
		    status = F_NO_ERROR;
		}
	    }
	    else
	    {
		status = F_ERR_ONDRIVE;
	    }
	}
	    
    }

    sdmmc->p_status = NULL;
				
    return status;
}

static int stm32l4_sdmmc_idle(stm32l4_sdmmc_t *sdmmc, uint32_t *p_media)
{
    int status = F_NO_ERROR;
    uint32_t media, retries;

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_idle);

    media = DOSFS_MEDIA_NONE;
    
    stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_IDENTIFY);

    for (retries = 0; retries < 4; retries++)
    {
	/* Apply an initial CMD_GO_IDLE_STATE, so that the card is out of
	 * data read/write mode, and can properly respond.
	 */
	
	status = stm32l4_sdmmc_command(sdmmc, SD_CMD_GO_IDLE_STATE, 0, SD_WAIT_RESPONSE_NONE);

	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_IF_COND, 0x000001aa, SD_WAIT_RESPONSE_SHORT);
	    
	    if (status == F_NO_ERROR)
	    {
		if (SDMMC1->RESP1 == 0x000001aa)
		{
		    media = DOSFS_MEDIA_SDHC;
		}
		else
		{
		    status = F_ERR_INVALIDMEDIA;
		}
	    }
	    else
	    {
		media = DOSFS_MEDIA_SDSC;
		
		status = stm32l4_sdmmc_command(sdmmc, SD_CMD_GO_IDLE_STATE, 0, SD_WAIT_RESPONSE_NONE);
	    }
	    
	    if (status == F_NO_ERROR)
	    {
		/* Send ACMD_SD_SEND_OP_COND of the inquiry variant to get the OCR (and the supported voltage ranges. This
		 * command must succeed of the SDCARD is not preset.
		 */

		status = stm32l4_sdmmc_command(sdmmc, SD_CMD_APP_CMD, 0, SD_WAIT_RESPONSE_SHORT);
	      
		if (status == F_NO_ERROR)
		{
		    status = stm32l4_sdmmc_command(sdmmc, SD_ACMD_SD_SEND_OP_COND, 0x00000000, SD_WAIT_RESPONSE_SHORT);
		    
		    if (status == F_NO_ERROR)
		    {
			sdmmc->OCR = SDMMC1->RESP1;

			if (sdmmc->OCR & 0x00300000)
			{
			    break;
			}

			status = F_ERR_INVALIDMEDIA;
		    }
		}
	    }
	}
    }

    if ((p_media == NULL) || (status != F_NO_ERROR))
    {
	stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_NONE);
    }
    
    if (p_media)
    {
	*p_media = media;
    }

    return status;
}

static uint32_t stm32l4_sdmmc_au_size_table[16] = {
    0,
    32,      /* 16KB  */
    64,      /* 32KB  */
    128,     /* 64KB  */
    256,     /* 128KB */
    512,     /* 256KB */
    1024,    /* 512KB */
    2048,    /* 1MB   */
    4096,    /* 2MB   */
    8192,    /* 4MB   */
    16384,   /* 8MB   */
    24576,   /* 12MB  */
    32768,   /* 16MB  */
    49152,   /* 24MB  */
    65536,   /* 32MB  */
    131072,  /* 64MB  */
};

static int stm32l4_sdmmc_reset(stm32l4_sdmmc_t *sdmmc, uint32_t media)
{
    int status = F_NO_ERROR;
    uint32_t millis, count;
    uint8_t data[64];

    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_reset);

    /* Send ACMD_SD_SEND_OP_COND, and wait till the initialization process is done. The SDCARD has
     * 1000ms to complete this initialization process.
     */

    millis = armv7m_systick_millis();
    
    do
    {
	status = stm32l4_sdmmc_command(sdmmc, SD_CMD_APP_CMD, 0, SD_WAIT_RESPONSE_SHORT);
	    
	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_ACMD_SD_SEND_OP_COND, ((media == DOSFS_MEDIA_SDHC) ? 0x40300000 : 0x00300000), SD_WAIT_RESPONSE_SHORT);
		
	    if (status == F_NO_ERROR)
	    {
		sdmmc->OCR = SDMMC1->RESP1;

		if (sdmmc->OCR & 0x80000000)
		{
		    if (media == DOSFS_MEDIA_SDHC)
		    {
			if (sdmmc->OCR & 0x40000000)
			{
			    media = DOSFS_MEDIA_SDHC;
			}
			else
			{
			    media = DOSFS_MEDIA_SDSC;
			}
		    }
		    break;
		}
		    
		if (((uint32_t)armv7m_systick_millis() - millis) > 1000)
		{
		    status = F_ERR_ONDRIVE;
		}
	    }
	}
    }
    while (status == F_NO_ERROR);
    
    if (status == F_NO_ERROR)
    {
	status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_ALL_CID, 0x00000000, SD_WAIT_RESPONSE_LONG);
	
	if (status == F_NO_ERROR)
	{
	    sdmmc->CID[0] = SDMMC1->RESP1 >> 24;
	    sdmmc->CID[1] = SDMMC1->RESP1 >> 16;
	    sdmmc->CID[2] = SDMMC1->RESP1 >> 8;
	    sdmmc->CID[3] = SDMMC1->RESP1 >> 0;
	    sdmmc->CID[4] = SDMMC1->RESP2 >> 24;
	    sdmmc->CID[5] = SDMMC1->RESP2 >> 16;
	    sdmmc->CID[6] = SDMMC1->RESP2 >> 8;
	    sdmmc->CID[7] = SDMMC1->RESP2 >> 0;
	    sdmmc->CID[8] = SDMMC1->RESP3 >> 24;
	    sdmmc->CID[9] = SDMMC1->RESP3 >> 16;
	    sdmmc->CID[10] = SDMMC1->RESP3 >> 8;
	    sdmmc->CID[11] = SDMMC1->RESP3 >> 0;
	    sdmmc->CID[12] = SDMMC1->RESP4 >> 24;
	    sdmmc->CID[13] = SDMMC1->RESP4 >> 16;
	    sdmmc->CID[14] = SDMMC1->RESP4 >> 8;
	    sdmmc->CID[15] = SDMMC1->RESP4 >> 0;
	    
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_RELATIVE_ADDR, 0x00000000, SD_WAIT_RESPONSE_SHORT);
	    
	    if (status == F_NO_ERROR)
	    {
		sdmmc->RCA = SDMMC1->RESP1 & 0xffff0000;
		
		if (sdmmc->RCA == 0x00000000)
		{
		    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_RELATIVE_ADDR, 0x00000000, SD_WAIT_RESPONSE_SHORT);
		    
		    if (status == F_NO_ERROR)
		    {
			sdmmc->RCA = SDMMC1->RESP1 & 0xffff0000;
		    }
		}
	    }
	}
    }

    /* If we got here without a hitch then it's time to enter transfer mode
     * and setup buswidth and speed.
     */

    if (status == F_NO_ERROR)
    {
	stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_DATA_TRANSFER);
	
	status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_CSD, sdmmc->RCA, SD_WAIT_RESPONSE_LONG);
	
	if (status == F_NO_ERROR)
	{
	    sdmmc->CSD[0] = SDMMC1->RESP1 >> 24;
	    sdmmc->CSD[1] = SDMMC1->RESP1 >> 16;
	    sdmmc->CSD[2] = SDMMC1->RESP1 >> 8;
	    sdmmc->CSD[3] = SDMMC1->RESP1 >> 0;
	    sdmmc->CSD[4] = SDMMC1->RESP2 >> 24;
	    sdmmc->CSD[5] = SDMMC1->RESP2 >> 16;
	    sdmmc->CSD[6] = SDMMC1->RESP2 >> 8;
	    sdmmc->CSD[7] = SDMMC1->RESP2 >> 0;
	    sdmmc->CSD[8] = SDMMC1->RESP3 >> 24;
	    sdmmc->CSD[9] = SDMMC1->RESP3 >> 16;
	    sdmmc->CSD[10] = SDMMC1->RESP3 >> 8;
	    sdmmc->CSD[11] = SDMMC1->RESP3 >> 0;
	    sdmmc->CSD[12] = SDMMC1->RESP4 >> 24;
	    sdmmc->CSD[13] = SDMMC1->RESP4 >> 16;
	    sdmmc->CSD[14] = SDMMC1->RESP4 >> 8;
	    sdmmc->CSD[15] = SDMMC1->RESP4 >> 0;
	}

	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SELECT_DESELECT_CARD, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
	}
	
	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_APP_CMD, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
		
	    if (status == F_NO_ERROR)
	    {
		status = stm32l4_sdmmc_receive(sdmmc, SD_ACMD_SEND_SCR, 0, &sdmmc->SCR[0], 8, &count, STM32L4_SDMMC_CONTROL_STOP);
		
		if ((status == F_NO_ERROR) && (count != 8))
		{
		    status = F_ERR_READ;
		}
	    }
	}
	
	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_APP_CMD, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
		
	    if (status == F_NO_ERROR)
	    {
		status = stm32l4_sdmmc_receive(sdmmc, SD_ACMD_SD_STATUS, 0, &sdmmc->SSR[0], 64, &count, STM32L4_SDMMC_CONTROL_STOP);
		
		if ((status == F_NO_ERROR) && (count != 64))
		{
		    status = F_ERR_READ;
		}
	    }
	}

#if 0	
	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_APP_CMD, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
	    
	    if (status == F_NO_ERROR)
	    {
		status = stm32l4_sdmmc_command(sdmmc, SD_ACMD_SET_CLR_CARD_DETECT, 0x00000000, SD_WAIT_RESPONSE_SHORT);
	    }
	}
#endif

	if (status == F_NO_ERROR)
	{
	    if (stm32l4_sdmmc_slice(sdmmc->SCR, 64, 50, 1))
	    {
		if ((sdmmc->option & STM32L4_SDMMC_OPTION_HIGH_SPEED) && (stm32l4_sdmmc_slice(sdmmc->SCR, 64, 56, 4) >= 1) && stm32l4_sdmmc_slice(sdmmc->CSD, 128, 94, 1))
		{
		    status = stm32l4_sdmmc_receive(sdmmc, SD_CMD_SWITCH_FUNC, 0x80fffff1, data, 64, &count, STM32L4_SDMMC_CONTROL_STOP);
		    
		    if ((status == F_NO_ERROR) && (count == 64) && stm32l4_sdmmc_slice(data, 512, 496, 16))
		    {
			stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_DATA_TRANSFER_WIDE_HS);
		    }
		    else
		    {
			stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_DATA_TRANSFER_WIDE);
		    }
		}
		else
		{
		    stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_DATA_TRANSFER_WIDE);
		}
		
		status = stm32l4_sdmmc_command(sdmmc, SD_CMD_APP_CMD, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
		
		if (status == F_NO_ERROR)
		{
		    status = stm32l4_sdmmc_command(sdmmc, SD_ACMD_SET_BUS_WIDTH, 0x00000002, SD_WAIT_RESPONSE_SHORT);
		}
	    }
	}
	
	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SET_BLOCKLEN, 512, SD_WAIT_RESPONSE_SHORT);
	}
    }

    if (status == F_NO_ERROR)
    {
	sdmmc->state = STM32L4_SDMMC_STATE_READY;

	sdmmc->media = media;
	sdmmc->shift = 9;
	sdmmc->au_size = 0;
	sdmmc->erase_size = 0;
	sdmmc->erase_timeout = 0;
	sdmmc->erase_offset = 0;

	if (media == DOSFS_MEDIA_SDHC)
	{
	    sdmmc->shift = 0;
	    sdmmc->au_size = stm32l4_sdmmc_au_size_table[stm32l4_sdmmc_slice(sdmmc->SSR, 512, 428, 4)];
	    sdmmc->erase_size = stm32l4_sdmmc_slice(sdmmc->SSR, 512, 408, 16);
	    sdmmc->erase_timeout = stm32l4_sdmmc_slice(sdmmc->SSR, 512, 402, 6) * 1000;
	    sdmmc->erase_offset = stm32l4_sdmmc_slice(sdmmc->SSR, 512, 400, 2) * 1000;

	    if (!sdmmc->au_size || !sdmmc->erase_size || !sdmmc->erase_timeout)
	    {
		sdmmc->erase_size = 0;
		sdmmc->erase_timeout = 0;
		sdmmc->erase_offset = 0;
	    }
	}
    }
    else
    {
	sdmmc->state = STM32L4_SDMMC_STATE_RESET;
	
	stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_NONE);

	status = F_ERR_CARDREMOVED;
    }
    
    sdmmc->p_status = NULL;

    return status;
}

static int stm32l4_sdmmc_lock(stm32l4_sdmmc_t *sdmmc, int state, uint32_t address)
{
    int status = F_NO_ERROR;
    uint32_t media;

#if defined(DOSFS_PORT_SDCARD_LOCK)
    status = DOSFS_PORT_SDCARD_LOCK();
    
    if (status == F_NO_ERROR)
#endif /* DOSFS_PORT_SDCARD_LOCK */
    {
	if (sdmmc->state == STM32L4_SDMMC_STATE_INIT)
	{
	    if (stm32l4_sdmmc_detect(sdmmc))
	    {
		sdmmc->state = STM32L4_SDMMC_STATE_RESET;
	    }
	    else
	    {
		status = F_ERR_CARDREMOVED;
	    }
	}
	
	if (status == F_NO_ERROR)
	{
	    if (sdmmc->state == STM32L4_SDMMC_STATE_RESET)
	    {
		status = stm32l4_sdmmc_idle(sdmmc, &media);

		if (status == F_NO_ERROR)
		{
		    status = stm32l4_sdmmc_reset(sdmmc, media);
		}
		else
		{
		    status = F_ERR_CARDREMOVED;
		}
	    }
	    else
	    {
		stm32l4_sdmmc_select(sdmmc);

		if (sdmmc->state == STM32L4_SDMMC_STATE_READ_MULTIPLE)
		{
		    if ((state != STM32L4_SDMMC_STATE_READ_MULTIPLE) || (sdmmc->address != address))
		    {
			status = stm32l4_sdmmc_read_stop(sdmmc);
		    }
		}

		if (sdmmc->state == STM32L4_SDMMC_STATE_WRITE_MULTIPLE)
		{
		    if ((state != STM32L4_SDMMC_STATE_WRITE_MULTIPLE) || (sdmmc->address != address))
		    {
			status = stm32l4_sdmmc_write_stop(sdmmc);
		    }
		}

		if (sdmmc->state == STM32L4_SDMMC_STATE_WRITE_STOP)
		{
		    if (state != STM32L4_SDMMC_STATE_WRITE_STOP)
		    {
			status = stm32l4_sdmmc_write_sync(sdmmc);
		    }
		}
	    }
	}
    }
    
    if (status != F_NO_ERROR)
    {
	status = stm32l4_sdmmc_unlock(sdmmc, status);
    }

    return status;
}

static int stm32l4_sdmmc_unlock(stm32l4_sdmmc_t *sdmmc, int status)
{
    if ((sdmmc->state != STM32L4_SDMMC_STATE_INIT) && (sdmmc->state != STM32L4_SDMMC_STATE_RESET))
    {
	stm32l4_sdmmc_deselect(sdmmc);
    }

    if (status == F_ERR_ONDRIVE)
    {
	if (stm32l4_sdmmc_idle(sdmmc, NULL) == F_NO_ERROR)
	{
	    sdmmc->state = STM32L4_SDMMC_STATE_INIT;
	}
	else
	{
	    sdmmc->state = STM32L4_SDMMC_STATE_RESET;

	    status = F_ERR_CARDREMOVED;
	}
    }

#if defined(DOSFS_PORT_SDCARD_UNLOCK)
    DOSFS_PORT_SDCARD_UNLOCK();
#endif /* DOSFS_PORT_SDCARD_UNLOCK */

    return status;
}

static int stm32l4_sdmmc_release(void *context)
{
    stm32l4_sdmmc_t *sdmmc = (stm32l4_sdmmc_t*)context;
    int status = F_NO_ERROR;

    if ((sdmmc->state != STM32L4_SDMMC_STATE_INIT) && (sdmmc->state != STM32L4_SDMMC_STATE_RESET))
    {
	status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READY, 0);
	
	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_unlock(sdmmc, status);

	    stm32l4_sdmmc_mode(sdmmc, STM32L4_SDMMC_MODE_NONE);

	    sdmmc->state = STM32L4_SDMMC_STATE_INIT;
	}
    }

    return status;
}

static int stm32l4_sdmmc_info(void *context, uint8_t *p_media, uint8_t *p_write_protected, uint32_t *p_block_count, uint32_t *p_au_size, uint32_t *p_serial)
{
    stm32l4_sdmmc_t *sdmmc = (stm32l4_sdmmc_t*)context;
    int status = F_NO_ERROR;
    uint32_t c_size, c_size_mult, read_bl_len;

    status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READY, 0);

    if (status == F_NO_ERROR)
    {
	*p_media = sdmmc->media;
	*p_write_protected = false;

	/* PERM_WRITE_PROTECT.
	 * TMP_WRITE_PROTECT
	 */
	if (stm32l4_sdmmc_slice(sdmmc->CSD, 128, 12, 2))
	{
	    *p_write_protected = true;
	}
	
	if ((sdmmc->CSD[0] & 0xc0) == 0)
	{
	    /* SDSC */
	    
	    read_bl_len = stm32l4_sdmmc_slice(sdmmc->CSD, 128, 80, 4);
	    c_size = stm32l4_sdmmc_slice(sdmmc->CSD, 128, 62, 12);
	    c_size_mult = stm32l4_sdmmc_slice(sdmmc->CSD, 128, 47, 3);
	    
	    *p_block_count = ((c_size + 1) << (c_size_mult + 2)) << (read_bl_len - 9);
	}
	else
	{
	    /* SDHC */

	    c_size = stm32l4_sdmmc_slice(sdmmc->CSD, 128, 48, 22);

	    *p_block_count = (c_size + 1) << (19 - 9);
	}

	*p_au_size = sdmmc->au_size;

	*p_serial = stm32l4_sdmmc_slice(sdmmc->CID, 128, 24, 32);

	status = stm32l4_sdmmc_unlock(sdmmc, status);
    }

    return status;
}


static int stm32l4_sdmmc_format(void *context)
{
    return F_NO_ERROR;
}

static int stm32l4_sdmmc_erase(void *context, uint32_t address, uint32_t length)
{
    stm32l4_sdmmc_t *sdmmc = (stm32l4_sdmmc_t*)context;
    int status = F_NO_ERROR;
    uint32_t au_start, au_end, timeout, response, millis;

    if (sdmmc->erase_size)
    {
	au_start = address / sdmmc->au_size;
	au_end   = ((address + length + (sdmmc->au_size -1)) / sdmmc->au_size);

	timeout = ((sdmmc->erase_timeout * (au_end - au_start)) / sdmmc->erase_size) + sdmmc->erase_offset;

	if (timeout < 1000)
	{
	    timeout = 1000;
	}

	if (address != (au_start * sdmmc->au_size))
	{
	    timeout += 250;
	}

	if ((address + length) != (au_end * sdmmc->au_size))
	{
	    timeout += 250;
	}
	
	status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READY, 0);
    
	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_ERASE_WR_BLK_START_ADDR, (address << sdmmc->shift), SD_WAIT_RESPONSE_SHORT);
	
	    if (status == F_NO_ERROR)
	    {
		status = stm32l4_sdmmc_command(sdmmc, SD_CMD_ERASE_WR_BLK_END_ADDR, ((address + length -1) << sdmmc->shift), SD_WAIT_RESPONSE_SHORT);
	
		if (status == F_NO_ERROR)
		{
		    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_ERASE, 0, SD_WAIT_RESPONSE_SHORT);

		    if (status == F_NO_ERROR)
		    {
			response = 0;

			millis = armv7m_systick_millis();
		    
			do
			{
			    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_SEND_STATUS, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
		
			    if (status == F_NO_ERROR)
			    {
				response |= SDMMC1->RESP1;
			    
				if ((SDMMC1->RESP1 & SD_R1_CURRENT_STATE_MASK) != SD_R1_CURRENT_STATE_PRG)
				{
				    break;
				}
				else
				{
				    if (((uint32_t)armv7m_systick_millis() - millis) > timeout)
				    {
					STM32L4_SDMMC_STATISTICS_COUNT(sdcard_erase_timeout);

					status = F_ERR_ONDRIVE;
				    }
				}
			    }
			}
			while (status == F_NO_ERROR);

			if (status == F_NO_ERROR)
			{
			    if (response & SD_R1_CURRENT_ERROR_MASK)
			    {
				if (response & (SD_R1_WP_VIOLATION | SD_R1_WP_ERASE_SKIP))
				{
				    status = F_ERR_WRITEPROTECT;
				}
				else if (response & SD_R1_CARD_ECC_FAILED)
				{
				    status = F_ERR_INVALIDSECTOR;
				}
				else
				{
				    status = F_ERR_ONDRIVE;
				}
			    }
			    else
			    {
				STM32L4_SDMMC_STATISTICS_COUNT_N(sdcard_erase, length);
			    }
			}
		    }
		}
	    }

	    status = stm32l4_sdmmc_unlock(sdmmc, status);
	}
    }

    return status;
}

static int stm32l4_sdmmc_discard(void *context, uint32_t address, uint32_t length)
{
    return F_NO_ERROR;
}

static int stm32l4_sdmmc_read(void *context, uint32_t address, uint8_t *data, uint32_t length, bool prefetch)
{
    stm32l4_sdmmc_t *sdmmc = (stm32l4_sdmmc_t*)context;
    int status = F_NO_ERROR;
    uint32_t count;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

    if (!prefetch && (length == 1))
    {
	status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READY, 0);

	if (status == F_NO_ERROR)
	{
	    do
	    {
		status = stm32l4_sdmmc_receive(sdmmc, SD_CMD_READ_SINGLE_BLOCK, (address << sdmmc->shift), data, DOSFS_BLK_SIZE, &count, STM32L4_SDMMC_CONTROL_STOP);
	    
		if (status == F_NO_ERROR)
		{
		    if (count != DOSFS_BLK_SIZE)
		    {
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
			if (retries)
			{
			    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_retry);

			    retries--;
			
			    continue;
			}
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

			STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_fail);
		    
			status = F_ERR_READ;
		    }
		    else
		    {
			STM32L4_SDMMC_STATISTICS_COUNT(sdcard_read_single);

			data += DOSFS_BLK_SIZE;

			address++;
			length--;
		    }
		}
	    }
	    while ((status == F_NO_ERROR) && length);

	    status = stm32l4_sdmmc_unlock(sdmmc, status);
	}
    }
    else
    {
	if (prefetch)
	{
	    status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READ_MULTIPLE, address);
	}
	else
	{
	    status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READY, 0);
	}

	if (status == F_NO_ERROR)
	{
	    do
	    {
		if (prefetch)
		{
		    if (sdmmc->state != STM32L4_SDMMC_STATE_READ_MULTIPLE)
		    {
			status = stm32l4_sdmmc_receive(sdmmc, SD_CMD_READ_MULTIPLE_BLOCK, (address << sdmmc->shift), data, DOSFS_BLK_SIZE * length, &count, STM32L4_SDMMC_CONTROL_PREFETCH);

			if (status == F_NO_ERROR)
			{
			    sdmmc->state = STM32L4_SDMMC_STATE_READ_MULTIPLE;
			    sdmmc->address = address;
			    sdmmc->count = 0;
			}
		    }
		    else
		    {
			status = stm32l4_sdmmc_receive(sdmmc, SD_CMD_READ_MULTIPLE_BLOCK, (address << sdmmc->shift), data, DOSFS_BLK_SIZE * length, &count, STM32L4_SDMMC_CONTROL_CONTINUE);
		    }
		}
		else
		{
		    status = stm32l4_sdmmc_receive(sdmmc, SD_CMD_READ_MULTIPLE_BLOCK, (address << sdmmc->shift), data, DOSFS_BLK_SIZE * length, &count, 0);
		}
	    
		if (status == F_NO_ERROR)
		{
		    count /= DOSFS_BLK_SIZE;
		    
		    STM32L4_SDMMC_STATISTICS_COUNT_N(sdcard_read_multiple, count);

		    if (length != count)
		    {
			/* If not all blocks got read the receive is terminated early,
			 * hence switch back to READY state. N.b. this is not easily
			 * doable in stm32l4_sdmmc_receive().
			 */
			sdmmc->state = STM32L4_SDMMC_STATE_READY;

#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
			data += (DOSFS_BLK_SIZE * count);

			address += count;
			length -= count;

			if (count)
			{
			    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_retry);

			    retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES;

			    continue;
			}
			else
			{
			    if (retries)
			    {
				STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_retry);
				
				retries--;
				
				continue;
			    }
			}
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

			STM32L4_SDMMC_STATISTICS_COUNT(sdcard_receive_fail);
		    
			status = F_ERR_READ;
		    }
		    else
		    {
			if (prefetch)
			{
			    sdmmc->address += length;
			    sdmmc->count += length;

			    if (sdmmc->count >= 16384)
			    {
			        STM32L4_SDMMC_STATISTICS_COUNT(sdcard_read_stop);

				prefetch = false;
			    }
			}

			length = 0;

			if (!prefetch)
			{
			    status = stm32l4_sdmmc_read_stop(sdmmc);
			}
		    }
		}
	    }
	    while ((status == F_NO_ERROR) && length);

	    status = stm32l4_sdmmc_unlock(sdmmc, status);
	}
    }

    return status;
}

static int stm32l4_sdmmc_write(void *context, uint32_t address, const uint8_t *data, uint32_t length, volatile uint8_t *p_status)
{
    stm32l4_sdmmc_t *sdmmc = (stm32l4_sdmmc_t*)context;
    int status = F_NO_ERROR;
    bool check;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

    if ((p_status == NULL) && (length == 1))
    {
	status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READY, 0);

	if (status == F_NO_ERROR)
	{
	    sdmmc->p_status = p_status;

	    do
	    {
		status = stm32l4_sdmmc_command(sdmmc, SD_CMD_WRITE_SINGLE_BLOCK, (address << sdmmc->shift), SD_WAIT_RESPONSE_SHORT);
	    
		if (status == F_NO_ERROR)
		{
		    status = stm32l4_sdmmc_transmit(sdmmc, data, DOSFS_BLK_SIZE, &check, true);
		
		    if (status == F_NO_ERROR)
		    {
			if (check)
			{
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
			    if (retries)
			    {
				STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_retry);
			    
				retries--;
			    
				continue;
			    }
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
			
			    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_fail);
			
			    status = F_ERR_WRITE;
			}
			else
			{
			    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_write_single);
			
			    address++;
			    length--;

			    status = stm32l4_sdmmc_write_sync(sdmmc);
			}
		    }
		}
	    }
	    while ((status == F_NO_ERROR) && length);

	    status = stm32l4_sdmmc_unlock(sdmmc, status);
	}
    }
    else
    {
	status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_WRITE_MULTIPLE, address);

	if (status == F_NO_ERROR)
	{
	    sdmmc->p_status = p_status;

	    do
	    {
		if (sdmmc->state != STM32L4_SDMMC_STATE_WRITE_MULTIPLE)
		{
		    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_WRITE_MULTIPLE_BLOCK, (address << sdmmc->shift), SD_WAIT_RESPONSE_SHORT);
			
		    if (status == F_NO_ERROR)
		    {
			sdmmc->state = STM32L4_SDMMC_STATE_WRITE_MULTIPLE;
			sdmmc->address = address;
			sdmmc->count = 0;
		    }
		}

		if (status == F_NO_ERROR)
		{
		    status = stm32l4_sdmmc_transmit(sdmmc, data, DOSFS_BLK_SIZE * length, &check, false);

		    if (status == F_NO_ERROR)
		    {
			if (check)
			{
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
			    uint32_t count, offset;
			    uint8_t temp[4];
				
			    status = stm32l4_sdmmc_command(sdmmc, SD_CMD_APP_CMD, sdmmc->RCA, SD_WAIT_RESPONSE_SHORT);
				
			    if (status == F_NO_ERROR)
			    {
				status = stm32l4_sdmmc_receive(sdmmc, SD_ACMD_SEND_NUM_WR_BLOCKS, 0, &temp[0], 4, &count, STM32L4_SDMMC_CONTROL_STOP);
				    
				if (status == F_NO_ERROR)
				{
				    if (count == 4)
				    {
					offset = (((uint32_t)temp[0] << 24) |
						  ((uint32_t)temp[1] << 16) |
						  ((uint32_t)temp[2] <<  8) |
						  ((uint32_t)temp[3] <<  0));

					if (offset >= sdmmc->count)
					{
					    offset -= sdmmc->count;

					    STM32L4_SDMMC_STATISTICS_COUNT_N(sdcard_write_multiple, offset);
				    
					    data += (DOSFS_BLK_SIZE * offset);
				    
					    address += offset;
					    length -= offset;

					    if (offset)
					    {
						STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_retry);
						    
						retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES;
						    
						continue;
					    }
					    else
					    {
						if (retries)
						{
						    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_retry);
							
						    retries--;
							
						    continue;
						}
					    }
					}
				    }
				}
			    }
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
				
			    STM32L4_SDMMC_STATISTICS_COUNT(sdcard_transmit_fail);
			
			    status = F_ERR_WRITE;
			}
			else
			{
			    STM32L4_SDMMC_STATISTICS_COUNT_N(sdcard_write_multiple, length);

			    sdmmc->address += length;
			    sdmmc->count += length;

			    length = 0;

			    if (p_status == NULL)
			    {
				status = stm32l4_sdmmc_write_stop(sdmmc);
				
				if (status == F_NO_ERROR)
				{
				    status = stm32l4_sdmmc_write_sync(sdmmc);
				}
			    }
			}
		    }
		}
	    }
	    while ((status == F_NO_ERROR) && length);
	
	    status = stm32l4_sdmmc_unlock(sdmmc, status);
	}
    }

    return status;
}

static int stm32l4_sdmmc_sync(void *context, bool wait)
{
    stm32l4_sdmmc_t *sdmmc = (stm32l4_sdmmc_t*)context;
    int status = F_NO_ERROR;

    if (sdmmc->state >= STM32L4_SDMMC_STATE_READ_MULTIPLE)
    {
	if ((sdmmc->state == STM32L4_SDMMC_STATE_WRITE_MULTIPLE) && !wait)
	{
	    status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_WRITE_STOP, 0);
	}
	else
	{
	    status = stm32l4_sdmmc_lock(sdmmc, STM32L4_SDMMC_STATE_READY, 0);
	}

	if (status == F_NO_ERROR)
	{
	    status = stm32l4_sdmmc_unlock(sdmmc, status);
	}
    }

    return status;
}

static const dosfs_device_interface_t stm32l4_sdmmc_interface = {
    stm32l4_sdmmc_release,
    stm32l4_sdmmc_info,
    stm32l4_sdmmc_format,
    stm32l4_sdmmc_erase,
    stm32l4_sdmmc_discard,
    stm32l4_sdmmc_read,
    stm32l4_sdmmc_write,
    stm32l4_sdmmc_sync,
};

int stm32l4_sdmmc_initialize(uint32_t option)
{
    stm32l4_sdmmc_t *sdmmc = (stm32l4_sdmmc_t*)&stm32l4_sdmmc;
    int status = F_NO_ERROR;

    dosfs_device.lock = DOSFS_DEVICE_LOCK_INIT;
    dosfs_device.context = (void*)sdmmc;
    dosfs_device.interface = &stm32l4_sdmmc_interface;

    sdmmc->option = option;

    if (sdmmc->state == STM32L4_SDMMC_STATE_NONE)
    {
	/* Try GO_IDLE after a reset, as the SDCARD could be still
	 * powered during the reset.
	 */
	stm32l4_sdmmc_idle(sdmmc, NULL);

	sdmmc->state = STM32L4_SDMMC_STATE_INIT;
    }
    
    dosfs_device.lock = 0;

    return status;
}

#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
