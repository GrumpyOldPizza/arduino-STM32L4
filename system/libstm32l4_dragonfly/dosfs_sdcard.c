/*
 * Copyright (c) 2014-2016 Thomas Roell.  All rights reserved.
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

#include "dosfs_sdcard.h"
#include "dosfs_port.h"

#include "armv7m_systick.h"

static dosfs_sdcard_t dosfs_sdcard;

#if (DOSFS_CONFIG_SDCARD_SIMULATE == 0)

bool stm32l4_sdcard_spi_init(dosfs_sdcard_t *sdcard)
{
    stm32l4_spi_pins_t pins;
    uint32_t clock, divide;

    pins.mosi = GPIO_PIN_PB5_SPI1_MOSI;
    pins.miso = GPIO_PIN_PB4_SPI1_MISO;
    pins.sck  = GPIO_PIN_PB3_SPI1_SCK;
    pins.ss   = GPIO_PIN_NONE;

    stm32l4_gpio_pin_configure(GPIO_PIN_PA8, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));

    stm32l4_spi_create(&sdcard->spi, SPI_INSTANCE_SPI1, &pins, 2, SPI_MODE_RX_DMA | SPI_MODE_TX_DMA | SPI_MODE_RX_DMA_SECONDARY | SPI_MODE_TX_DMA_SECONDARY);
    stm32l4_spi_enable(&sdcard->spi, NULL, NULL, 0);
    
    clock = stm32l4_spi_clock(&sdcard->spi) / 2;
    divide = 0;

    while ((clock > 400000) && (divide < 7))
    {
	clock /= 2;
	divide++;
    }

    sdcard->control = SPI_CONTROL_MODE_3 | (divide << SPI_CONTROL_DIV_SHIFT);
    sdcard->pin_cs = GPIO_PIN_PA8;

    return true;
}

bool stm32l4_sdcard_spi_present(dosfs_sdcard_t *sdcard)
{
  // return !!stm32l4_gpio_pin_read(sdcard->pin_cs);
  return true;
}

uint32_t stm32l4_sdcard_spi_mode(dosfs_sdcard_t *sdcard, int mode)
{
    uint32_t n, speed, clock, divide;

    if (mode == DOSFS_SDCARD_MODE_NONE)
    {
	speed = 0;

	/* Switch CS_SDCARD to be input
	 */
	stm32l4_gpio_pin_configure(sdcard->pin_cs, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));
    }
    else
    {
	if (mode == DOSFS_SDCARD_MODE_IDENTIFY)
	{
	    speed = 400000;

	    /* Switch CS_SDCARD to be output
	     */

	    stm32l4_gpio_pin_configure(sdcard->pin_cs, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_OUTPUT));
	    stm32l4_gpio_pin_write(sdcard->pin_cs, 1);
	}
	else
	{
	    // speed = 20000000;
	    // speed = 10000000;
	    speed = 5000000;
	    
	    if (mode == DOSFS_SDCARD_MODE_DATA_TRANSFER)
	    {
		speed = 25000000;
	    }
	    else
	    {
		speed = 50000000;
	    }

	    // speed = 5000000;

	    stm32l4_sdcard_spi_deselect(sdcard);
	}

	clock = stm32l4_spi_clock(&sdcard->spi) / 2;
	divide = 0;
	
	while ((clock > 400000) && (divide < 7))
	{
	    clock /= 2;
	    divide++;
	}
	
	sdcard->control = SPI_CONTROL_MODE_3 | (divide << SPI_CONTROL_DIV_SHIFT);

	if (mode == DOSFS_SDCARD_MODE_IDENTIFY)
	{
	    stm32l4_spi_select(&sdcard->spi, sdcard->control);

	    stm32l4_gpio_pin_configure(sdcard->spi.pins.mosi, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_OUTPUT));
	    stm32l4_gpio_pin_write(sdcard->spi.pins.mosi, 1);

	    /* Here CS/MOSI are driven both to H.
	     *
	     * Specs says to issue 74 clock cycles in SPI mode while CS/MOSI are H,
	     * so simply send 10 bytes over the clock line.
	     */
    
	    for (n = 0; n < 10; n++)
	    {
		stm32l4_spi_exchange8(&sdcard->spi, 0xff);
	    }
    
	    stm32l4_gpio_pin_configure(sdcard->spi.pins.mosi, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));

	    stm32l4_spi_unselect(&sdcard->spi);
	}

	stm32l4_sdcard_spi_select(sdcard);
    }
    
    return speed;
}

void stm32l4_sdcard_spi_select(dosfs_sdcard_t *sdcard)
{
    stm32l4_spi_select(&sdcard->spi, sdcard->control);

    /* CS output, drive CS to L */
    stm32l4_gpio_pin_write(sdcard->pin_cs, 0);

    /* The card will not drive DO for one more clock
     * after CS goes L, but will accept data right away.
     * The first thing after a select will be always
     * either a command (send_command), or a "Stop Token".
     * In both cases there will be a byte over the
     * bus, and hence DO will be stable.
     */
}

void stm32l4_sdcard_spi_deselect(dosfs_sdcard_t *sdcard)
{
    /* CS is output, drive CS to H */
    stm32l4_gpio_pin_write(sdcard->pin_cs, 1);

    /* The card drives the DO line at least one more
     * clock cycle after CS goes H. Hence send
     * one extra byte over the bus.
     */
    
    stm32l4_sdcard_spi_send(sdcard, 0x00);

    stm32l4_spi_unselect(&sdcard->spi);
}

void stm32l4_sdcard_spi_send(dosfs_sdcard_t *sdcard, uint8_t data)
{
    stm32l4_spi_exchange8(&sdcard->spi, data);
}

uint8_t stm32l4_sdcard_spi_receive(dosfs_sdcard_t *sdcard)
{
    return stm32l4_spi_exchange8(&sdcard->spi, 0xff);
}

void stm32l4_sdcard_spi_send_block(dosfs_sdcard_t *sdcard, const uint8_t *data)
{
    stm32l4_spi_transmit(&sdcard->spi, data, 512, SPI_CONTROL_CRC16);

    while (!stm32l4_spi_done(&sdcard->spi))
    {
	armv7m_core_yield();
    }
}

uint32_t stm32l4_sdcard_spi_receive_block(dosfs_sdcard_t *sdcard, uint8_t *data)
{
    stm32l4_spi_receive(&sdcard->spi, data, 512, SPI_CONTROL_CRC16);
  
    while (!stm32l4_spi_done(&sdcard->spi))
    {
	armv7m_core_yield();
    }

    return stm32l4_spi_crc16(&sdcard->spi);
}

#define FALSE  0
#define TRUE   1

#if !defined(NULL)
#define NULL   ((void*)0)
#endif

static int dosfs_sdcard_send_command(dosfs_sdcard_t *sdcard, uint8_t index, const uint32_t argument, uint32_t count);
static int dosfs_sdcard_send_data(dosfs_sdcard_t *sdcard, uint8_t token, const uint8_t *data, uint32_t count, unsigned int *p_retries);
static int dosfs_sdcard_receive_data(dosfs_sdcard_t *sdcard, uint8_t *data, uint32_t count, unsigned int *p_retries);
static int dosfs_sdcard_reset(dosfs_sdcard_t *sdcard);
static int dosfs_sdcard_lock(dosfs_sdcard_t *sdcard, int state, uint32_t address);
static int dosfs_sdcard_unlock(dosfs_sdcard_t *sdcard, int status);


#define SD_CMD_GO_IDLE_STATE           (0)
#define SD_CMD_SEND_OP_COND            (1)
#define SD_CMD_SWITCH_FUNC             (6)
#define SD_CMD_SEND_IF_COND            (8)
#define SD_CMD_SEND_CSD                (9)
#define SD_CMD_SEND_CID                (10)
#define SD_CMD_STOP_TRANSMISSION       (12)
#define SD_CMD_SEND_STATUS             (13)
#define SD_CMD_SET_BLOCKLEN            (16)
#define SD_CMD_READ_SINGLE_BLOCK       (17)
#define SD_CMD_READ_MULTIPLE_BLOCK     (18)
#define SD_CMD_WRITE_SINGLE_BLOCK      (24)
#define SD_CMD_WRITE_MULTIPLE_BLOCK    (25)
#define SD_CMD_PROGRAM_CSD             (27)
#define SD_CMD_ERASE_WR_BLK_START_ADDR (32)
#define SD_CMD_ERASE_WR_BLK_END_ADDR   (33)
#define SD_CMD_ERASE                   (38)
#define SD_CMD_APP_CMD                 (55)
#define SD_CMD_READ_OCR                (58)
#define SD_CMD_CRC_ON_OFF              (59)

/* Add a 0x40 to ACMD, so that it's clear what is meant. 
 * In dosfs_sdcard_send_command() a 0x40 will set anyway.
 */
#define SD_ACMD_SD_STATUS              (0x40+13)
#define SD_ACMD_SEND_NUM_WR_BLOCKS     (0x40+22)
#define SD_ACMD_SET_WR_BLK_ERASE_COUNT (0x40+23)
#define SD_ACMD_SD_SEND_OP_COND        (0x40+41)
#define SD_ACMD_SET_CLR_CARD_DETECT    (0x40+42)
#define SD_ACMD_SEND_SCR               (0x40+51)

#define SD_R1_VALID_MASK               0x80
#define SD_R1_VALID_DATA               0x00
#define SD_R1_IN_IDLE_STATE            0x01
#define SD_R1_ERASE_RESET              0x02
#define SD_R1_ILLEGAL_COMMAND          0x04
#define SD_R1_COM_CRC_ERROR            0x08
#define SD_R1_ERASE_SEQUENCE_ERROR     0x10
#define SD_R1_ADDRESS_ERROR            0x20
#define SD_R1_PARAMETER_ERROR          0x40

#define SD_R2_CARD_IS_LOCKED           0x01
#define SD_R2_WP_ERASE_SKIP            0x02
#define SD_R2_LOCK_UNLOCK_FAILED       0x02
#define SD_R2_EXECUTION_ERROR          0x04
#define SD_R2_CC_ERROR                 0x08
#define SD_R2_CARD_ECC_FAILED          0x10
#define SD_R2_WP_VIOLATION             0x20
#define SD_R2_ERASE_PARAM              0x40
#define SD_R2_OUT_OF_RANGE             0x80
#define SD_R2_CSD_OVERWRITE            0x80

#define SD_READY_TOKEN                 0xff    /* host -> card, card -> host */
#define SD_START_READ_TOKEN            0xfe    /* card -> host */
#define SD_START_WRITE_SINGLE_TOKEN    0xfe    /* host -> card */
#define SD_START_WRITE_MULTIPLE_TOKEN  0xfc    /* host -> card */
#define SD_STOP_TRANSMISSION_TOKEN     0xfd    /* host -> card */

/* data response token for WRITE */
#define SD_DATA_RESPONSE_MASK          0x1f
#define SD_DATA_RESPONSE_ACCEPTED      0x05
#define SD_DATA_RESPONSE_CRC_ERROR     0x0b
#define SD_DATA_RESPONSE_WRITE_ERROR   0x0d

/* data error token for READ */
#define SD_DATA_ERROR_TOKEN_VALID_MASK 0xf0
#define SD_DATA_ERROR_TOKEN_VALID_DATA 0x00
#define SD_DATA_ERROR_EXECUTION_ERROR  0x01
#define SD_DATA_ERROR_CC_ERROR         0x02
#define SD_DATA_ERROR_CARD_ECC_FAILED  0x04
#define SD_DATA_ERROR_OUT_OF_RANGE     0x08

#if 1 || (DOSFS_CONFIG_SDCARD_CRC == 1)

const uint8_t dosfs_crc7_table[256]= {
    0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
    0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
    0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
    0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
    0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
    0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
    0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
    0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
    0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
    0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
    0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
    0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
    0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
    0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
    0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
    0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
    0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
    0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
    0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
    0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
    0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
    0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
    0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
    0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
    0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
    0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
    0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
    0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
    0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
    0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
    0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
    0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

const uint16_t dosfs_crc16_table[256]= {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49, 
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067, 
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

uint8_t dosfs_compute_crc7(const uint8_t *data, uint32_t count)
{
    unsigned int n;
    uint8_t crc7 = 0;

    for (n = 0; n < count; n++)
    {
	DOSFS_UPDATE_CRC7(crc7, data[n]);
    }
    
    return crc7;
}

uint16_t dosfs_compute_crc16(const uint8_t *data, uint32_t count)
{
    unsigned int n;
    uint16_t crc16 = 0;

    for (n = 0; n < count; n++)
    {
	DOSFS_UPDATE_CRC16(crc16, data[n]);
    }
    
    return crc16;
}

#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */

/*
 * dosfs_sdcard_wait_ready(dosfs_sdcard_t *sdcard)
 *
 * Wait till DO transitions from BUSY (0x00) to READY (0xff).
 */

static int dosfs_sdcard_wait_ready(dosfs_sdcard_t *sdcard)
{
    int status = F_NO_ERROR;
    unsigned int n, millis;
    uint8_t response;

    /* While waiting for non busy (not 0x00) the host can
     * release the CS line to let somebody else access the
     * bus. However there needs to be an extra clock
     * cycle after driving CS to H for the card to release DO,
     * as well as one extra clock cycle after driving CS to L
     * before the data is valid again.
     */

    for (n = 0; n < 64; n++)
    {
        response = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);

        if (response == SD_READY_TOKEN)
        {
            break;
        }
    }

    if (response != SD_READY_TOKEN)
    {
        millis = armv7m_systick_millis();

	do
	{
	    response = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	
	    if (response != SD_READY_TOKEN)
	    {
		if ((armv7m_systick_millis() - millis) > 250)
		{
		    break;
		}

#if defined(DOSFS_PORT_SDCARD_YIELD)
		DOSFS_PORT_SDCARD_SPI_DESELECT(sdcard);

		DOSFS_PORT_SDCARD_UNLOCK();

		DOSFS_PORT_SDCARD_YIELD();

		status = DOSFS_PORT_SDCARD_LOCK();

		if (status == F_NO_ERROR)
		{
		    DOSFS_PORT_SDCARD_SPI_SELECT(sdcard);
		}
#endif /* DOSFS_PORT_SDCARD_SPI_YIELD */
	    }
	}
	while ((status == F_NO_ERROR) && (response != SD_READY_TOKEN));
    }

    sdcard->flags &= ~DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT;

    if (response != SD_READY_TOKEN)
    {
	status = F_ERR_ONDRIVE;
    }

    return status;
}

static int dosfs_sdcard_send_command(dosfs_sdcard_t *sdcard, uint8_t index, const uint32_t argument, uint32_t count)
{
    int status = F_NO_ERROR;
    unsigned int n;
    uint8_t data[5], response, crc7;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_COMMAND_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0) */
    unsigned int retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0) */

    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_send_command);

    data[0] = 0x40 | index;
    data[1] = argument >> 24;
    data[2] = argument >> 16;
    data[3] = argument >> 8;
    data[4] = argument >> 0;

#if (DOSFS_CONFIG_SDCARD_CRC == 1)
    crc7 = (dosfs_compute_crc7(data, 5) << 1) | 0x01;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) */
    if (index == SD_CMD_GO_IDLE_STATE)
    {
        crc7 = 0x95;
    }
    else if (index == SD_CMD_SEND_IF_COND)
    {
        crc7 = 0x87;
    } 
    else
    {
        crc7 = 0x01;
    }
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */

    do
    {
	/* A command needs at least one back to back idle cycle. Unless of 
	 * course it's known to have this extra byte seen already.
	 */
	
	if (sdcard->flags & DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT)
	{
	    DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	}

	sdcard->flags |= DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT;
	
	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, data[0]);
	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, data[1]);
	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, data[2]);
	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, data[3]);
	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, data[4]);
	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, crc7);

	/* NCR is 1..8 bytes, so simply always discard the first byte,
	 * and then read up to 8 bytes or till a vaild response
	 * was seen. N.b that STOP_TRANSMISSION specifies that
	 * the first byte on DataOut after reception of the command
	 * is a stuffing byte that has to be ignored. The discard
	 * takes care of that here.
	 */

	DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);

	for (n = 0; n < 8; n++)
	{
	    response = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
        
	    if (!(response & 0x80))
	    {
		/*
		 * A STOP_TRANSMISSION can be issued before the card
		 * had send a "Data Error Token" for that last block
		 * that we are not really intrested in. This could result
		 * in a "Parameter Error" if the next block is out of bounds.
		 * Due to the way CMD_STOP_TRANSMISSION gets send there is
		 * also the chance it gets an "Illegal Command" error ...
		 */
		if (index == SD_CMD_STOP_TRANSMISSION)
		{
		    response &= ~(SD_R1_ILLEGAL_COMMAND | SD_R1_PARAMETER_ERROR);
		}
		break;
	    }
	}

	sdcard->response[0] = response;
	
	/* This below is somewhat tricky. There can be multiple communication
	 * failures:
	 *
	 * - card not responding
	 * - noise on the DATA line
	 * - noise on the CLK line
	 *
	 * If the card does not respond there should be 1xxxxxxxb as a response.
	 * If there is noise on the DATA line a CRC error is detect with a
	 * 0xxx1xxxx response.
	 *
	 * If there is noise on the CLK line, bits will be missing. The loop
	 * up there will throw out anything with a 1xxxxxxxb pattern. Hence if
	 * bits are missing the possible replies are (n.b. that the CRC error
	 * bit will be set, and the missing bits will be filled in as ones
	 * from the right):
	 *
	 * 00010001
	 * 00100011
	 * 01000111
	 * 00111111
	 * 01111111
	 *
	 * So in a nutshell, bit 0 will be always set, and at least one other bit
	 * will be set. This means 0x01 is a legal pattern. N.b. that without
	 * CRC only the "card not responding" case can be addressed.
	 *
	 * If such errors occur, a retry process is used, whereby the CS line
	 * is toggled to allow the card to resync.
	 */

	if (
#if (DOSFS_CONFIG_SDCARD_CRC == 1)
	    ((response != 0x01) && (response & 0x89)) 
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) */
	    (response & 0x80)
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */
	    )
	{
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_COMMAND_RETRIES != 0)
	    if (retries > 1)
	    {
		DOSFS_PORT_SDCARD_SPI_DESELECT(sdcard);

		DOSFS_PORT_SDCARD_SPI_SELECT(sdcard);

		sdcard->flags &= ~DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT;

		DOSFS_SDCARD_STATISTICS_COUNT(sdcard_send_command_retry);
		    
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
	    /* Always read the rest of the data to avoid synchronization
	     * issues. Worst case the data read is random.
	     */
	    for (n = 1; n <= count; n++)
	    {
		sdcard->response[n] = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	    } 

	    if (response != 0x00)
	    {
		if (sdcard->state != DOSFS_SDCARD_STATE_RESET)
		{
		    status = F_ERR_ONDRIVE;
		}
	    }

	    retries = 0;
	}
    }
    while (retries != 0);

    if (status != F_NO_ERROR)
    {
	DOSFS_SDCARD_STATISTICS_COUNT(sdcard_send_command_fail);
    }

    return status;
}

static int dosfs_sdcard_send_data(dosfs_sdcard_t *sdcard, uint8_t token, const uint8_t *data, uint32_t count, unsigned int *p_retries)
{
    int status = F_NO_ERROR;
    unsigned int retries = *p_retries;
    uint8_t response;
#if !defined(DOSFS_PORT_SDCARD_SPI_SEND_BLOCK)
    unsigned int n;
    uint32_t crc16;
#endif /* !DOSFS_PORT_SDCARD_SPI_SEND_BLOCK */

    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_send_data);

    DOSFS_PORT_SDCARD_SPI_SEND(sdcard, token);

#if defined(DOSFS_PORT_SDCARD_SPI_SEND_BLOCK)

    DOSFS_PORT_SDCARD_SPI_SEND_BLOCK(sdcard, data);

#else /* DOSFS_PORT_SDCARD_SPI_SEND_BLOCK */

    crc16 = 0;

    for (n = 0; n < count; n++)
    {
#if (DOSFS_CONFIG_SDCARD_CRC == 1)
	DOSFS_UPDATE_CRC16(crc16, data[n]);
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */

	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, data[n]);
    } 

    DOSFS_PORT_SDCARD_SPI_SEND(sdcard, crc16 >> 8);
    DOSFS_PORT_SDCARD_SPI_SEND(sdcard, crc16);
    
#endif /* DOSFS_PORT_SDCARD_SPI_SEND_BLOCK */

    /* At last read back the "Data Response Token":
     *
     * 0x05 No Error
     * 0x0b CRC Error
     * 0x0d Write Error
     */

    response = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard) & SD_DATA_RESPONSE_MASK;

    if (response == SD_DATA_RESPONSE_ACCEPTED)
    {
	retries = 0;
    }
    else
    {
#if (DOSFS_CONFIG_SDCARD_CRC == 1)
	/* But then it get's interesting. It's possible that the start token
	 * morphed into a STOP_TRANSMISSION token, or got lost all together.
	 * In that case there would be a BUSY state. So one had to wait till
	 * that was done before issuing the CMD_STOP_TRANSMISSION later on.
	 * The other case is where clock pulses were lost. In that cases the
	 * toggle of CS line will resync the clock, and we'd end up in
	 * a READY state after one dummy byte. But there might be still data 
	 * left over. This left over data will predictably end up in a 
	 * DATA_RESPONSE_CRC_ERROR, which shares R1_COM_CRC_ERROR with a normal
	 * command response. Hence the subsequent CMDP_STOP_TRANSMISSION will
	 * consume that left over data as 0xff or 0x0b ... and due to the retry
	 * mechanism resync.
	 */
	DOSFS_PORT_SDCARD_SPI_DESELECT(sdcard);
	
	DOSFS_PORT_SDCARD_SPI_SELECT(sdcard);
	
	status = dosfs_sdcard_wait_ready(sdcard);

	if (status == F_NO_ERROR)
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */
	{
	    /* A non-accepted "Data Response Token" in a write multiple block
	     * operation shall be followed by a CMD_STOP_TRANSMISSION
	     * (not a "Stop Token").
	     */
	    
	    status = dosfs_sdcard_send_command(sdcard, SD_CMD_STOP_TRANSMISSION, 0, 0);
	    
	    if (status == F_NO_ERROR)
	    {
		status = dosfs_sdcard_wait_ready(sdcard);
	    
		if (status == F_NO_ERROR)
		{
		    sdcard->state = DOSFS_SDCARD_STATE_READY;

		    status = dosfs_sdcard_send_command(sdcard, SD_CMD_SEND_STATUS, 0, 1);

		    if (status == F_NO_ERROR)
		    {
			if (sdcard->response[0] != 0x00)
			{
			    /* Here the complete error response is available, so one can find out the
			     * true cause for the write error.
			     */
			
			    if (sdcard->response[1] & (SD_R2_CARD_IS_LOCKED | SD_R2_WP_ERASE_SKIP | SD_R2_EXECUTION_ERROR | SD_R2_CC_ERROR | SD_R2_ERASE_PARAM | SD_R2_OUT_OF_RANGE))
			    {
				status = F_ERR_ONDRIVE;
			    }
			    else if (sdcard->response[1] & SD_R2_WP_VIOLATION)
			    {
				status = F_ERR_WRITEPROTECT;
			    }
			    else
			    {
				/* CARD_ECC_FAILED */
				status = F_ERR_INVALIDSECTOR;
			    }

			    retries = 0;
			}
			else
			{
			    /* If there was not error posted, it must have been a CRC error, so
			     * let's restart the operatio if possible ...
			     */
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
			    if (retries > 1)
			    {
				DOSFS_SDCARD_STATISTICS_COUNT(sdcard_send_data_retry);
			    
				retries--;
			    }
			    else
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
			    {
				status = F_ERR_WRITE;
			    
				retries = 0;
			    }
			}
		    }
		}
	    }
	}
    }

    if (status != F_NO_ERROR)
    {
	DOSFS_SDCARD_STATISTICS_COUNT(sdcard_send_data_fail);
    }

#if (DOSFS_CONFIG_SDCARD_DATA_RETRIES == 0)

    /* There are paths throu the code when "status" gets set,
     * but "retries" does not get touched. So if "retries"
     * is known to be 0, make it so.
     */

    retries = 0;

#endif /* (DOSFS_CONFIG_SDCARD_DATA_RETRIES == 0) */

    *p_retries = retries;

    return status;
}

static int dosfs_sdcard_receive_data(dosfs_sdcard_t *sdcard, uint8_t *data, uint32_t count, unsigned int *p_retries)
{
    int status = F_NO_ERROR;
    unsigned int n, millis;
    unsigned int retries = *p_retries;
    uint8_t token;
#if (DOSFS_CONFIG_SDCARD_CRC == 1)
    uint32_t crc16;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */

    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_receive_data);

    /* Before we waited for a non 0xff token.
     * If a "Start Block Token" (0xfe) zips by then
     * a data block will follow. Otherwise a "Data Error Token"
     * shows up:
     *
     * 0x01 Execution Error
     * 0x02 CC Error
     * 0x04 Card ECC failed
     * 0x08 Out of Range
     *
     * The maximum documented timeout is 100ms for SDHC.
     */

    for (n = 0; n < 64; n++)
    {
        token = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);

        if (token != SD_READY_TOKEN)
        {
            break;
        }
    }

    if (token == SD_READY_TOKEN)
    {
        millis = armv7m_systick_millis();

	do
	{
	    token = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	    
	    if (token == SD_READY_TOKEN)
	    {
		if ((armv7m_systick_millis() - millis) > 100)
		{
		    break;
		}
	    }
	}
	while (token == SD_READY_TOKEN);
    }

    if (token != SD_START_READ_TOKEN)
    {
	/* On an invalid token a toggle of the CS signal
	 * will resync the clock. The stop is required,
	 * as it's unclear as to whether the transfer is 
	 * under way of not.
	 */
	
	DOSFS_PORT_SDCARD_SPI_DESELECT(sdcard);
	
	DOSFS_PORT_SDCARD_SPI_SELECT(sdcard);
	
	sdcard->flags &= ~DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT;
	
	status = dosfs_sdcard_send_command(sdcard, SD_CMD_STOP_TRANSMISSION, 0, 0);

	if (status == F_NO_ERROR)
	{
	    sdcard->state = DOSFS_SDCARD_STATE_READY;
	    
	    if ((token & SD_DATA_ERROR_TOKEN_VALID_MASK) != SD_DATA_ERROR_TOKEN_VALID_DATA)
	    {
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
		if (retries > 1)
		{
		    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_receive_data_retry);
		
		    retries--;
		}
		else
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
		{
		    status = F_ERR_READ;
		    
		    retries = 0;
		}
	    }
	    else
	    {
		if (token & (SD_DATA_ERROR_EXECUTION_ERROR | SD_DATA_ERROR_CC_ERROR | SD_DATA_ERROR_OUT_OF_RANGE))
		{
		    status = F_ERR_ONDRIVE;
		}
		else
		{
		    /* CARD_ECC_FAILED */
		    status = F_ERR_INVALIDSECTOR;
		}
		
		retries = 0;
	    }
	}
	else 
	{ 
	    retries = 0;
	}
    }
    else
    {
#if (DOSFS_CONFIG_SDCARD_CRC == 1)

#if defined(DOSFS_PORT_SDCARD_SPI_RECEIVE_BLOCK)
	if (count == DOSFS_BLK_SIZE)
	{
	    crc16 = DOSFS_PORT_SDCARD_SPI_RECEIVE_BLOCK(sdcard, data);
	}
	else
#endif /* DOSFS_PORT_SDCARD_SPI_RECEIVE_BLOCK */
	{
	    crc16 = 0;

	    for (n = 0; n < count; n++)
	    {
		data[n] = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
		
		DOSFS_UPDATE_CRC16(crc16, data[n]);
	    } 

	    crc16 ^= (DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard) << 8);
	    crc16 ^= DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	}

	if (crc16 != 0)
	{
	    /* On an invalid token a toggle of the CS signal
	     * will resync the clock. The stop is required,
	     * as it's unclear as to whether the tranfer is 
	     * under way of not.
	     */
	
	    DOSFS_PORT_SDCARD_SPI_DESELECT(sdcard);

	    DOSFS_PORT_SDCARD_SPI_SELECT(sdcard);

	    sdcard->flags &= ~DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT;
	
	    status = dosfs_sdcard_send_command(sdcard, SD_CMD_STOP_TRANSMISSION, 0, 0);
	    
	    if (status == F_NO_ERROR)
	    {
		sdcard->state = DOSFS_SDCARD_STATE_READY;
	    
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
		if (retries > 1)
		{
		    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_receive_data_retry);

		    retries--;
		}
		else
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
		{
		    status = F_ERR_READ;
		    
		    retries = 0;
		}
	    }
	    else
	    {
		retries = 0;
	    }
	}
	else
	{
	    retries = 0;
	}

#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) */

#if defined(DOSFS_PORT_SDCARD_SPI_RECEIVE_BLOCK)
	if (count == DOSFS_BLK_SIZE)
	{
	    DOSFS_PORT_SDCARD_SPI_RECEIVE_BLOCK(sdcard, data);
	}
	else
#endif /* DOSFS_PORT_SDCARD_SPI_RECEIVE_BLOCK */
	{
	    for (n = 0; n < count; n++)
	    {
		data[n] = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	    } 
    
	    DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	    DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	}

	retries = 0;
    
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */
    }

    if (status != F_NO_ERROR)
    {
	DOSFS_SDCARD_STATISTICS_COUNT(sdcard_receive_data_fail);
    }

#if (DOSFS_CONFIG_SDCARD_DATA_RETRIES == 0)

    /* There are paths throu the code when "status" gets set,
     * but "retries" does not get touched. So if "retries"
     * is known to be 0, make it so.
     */

    retries = 0;

#endif /* (DOSFS_CONFIG_SDCARD_DATA_RETRIES == 0) */

    *p_retries = retries;
    
    return status;
}

static int dosfs_sdcard_reset(dosfs_sdcard_t *sdcard)
{
    int status = F_NO_ERROR;
    unsigned int n, millis, media, mode;
#if (DOSFS_CONFIG_SDCARD_HIGH_SPEED == 1)
    unsigned int retries;
    uint8_t data[64];
#endif /* (DOSFS_CONFIG_SDCARD_HIGH_SPEED == 1) */

    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_reset);

    media = DOSFS_MEDIA_NONE;
    
    sdcard->speed = DOSFS_PORT_SDCARD_SPI_MODE(sdcard, DOSFS_SDCARD_MODE_IDENTIFY);

    /* Apply an initial CMD_GO_IDLE_STATE, so that the card is out of
     * data read/write mode, and can properly respond.
     */

    dosfs_sdcard_send_command(sdcard, SD_CMD_GO_IDLE_STATE, 0, 0);

    if (sdcard->response[0] != 0x01)
    {
	/* There could be 2 error scenarios. One is that the
	 * CMD_GO_IDLE_STATE was send while the card was waiting
	 * for the next "Start Block" / "Stop Transmission" token.
	 * The card will answer that normally by signaling BUSY,
	 * when means there would be a 0x00 response.
	 * The other case is that there was a reset request while
	 * being in the middle of a write transaction. As a result
	 * the card will send back a stream of non-zero values
	 * (READY or a "Data Response Token"), which can be handled
	 * by simple flushing the transaction (CRC will take care of
	 * rejecting the data ... without CRC the last write will be
	 * garbage.
	 *
	 * Hence first drain a possible write command, and then
	 * rety CMD_GO_IDLE_STATE after a dosfs_sdcard_wait_ready().
	 * If the card is still in BUSY mode, then retry the
	 * CMD_GO_IDLE_STATE again.
	 */

	if (sdcard->response[0] != 0x00)
	{
	    for (n = 0; n < 1024; n++)
	    {
		DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
	    }

	    dosfs_sdcard_send_command(sdcard, SD_CMD_GO_IDLE_STATE, 0, 0);
	}

	dosfs_sdcard_wait_ready(sdcard);
	    
	dosfs_sdcard_send_command(sdcard, SD_CMD_GO_IDLE_STATE, 0, 0);

	if (sdcard->response[0] == 0x00)
	{
	    dosfs_sdcard_wait_ready(sdcard);
	    
	    dosfs_sdcard_send_command(sdcard, SD_CMD_GO_IDLE_STATE, 0, 0);
	}
    }

    if (sdcard->response[0] == 0x01)
    {
	dosfs_sdcard_send_command(sdcard, SD_CMD_SEND_IF_COND, 0x000001aa, 4);

	if (sdcard->response[0] == 0x01)
	{
	    if ((sdcard->response[1] == 0x00) &&
		(sdcard->response[2] == 0x00) &&
		(sdcard->response[3] == 0x01) &&
		(sdcard->response[4] == 0xaa))
	    {
		media = DOSFS_MEDIA_SDHC;
	    }
	    else
	    {
		media = DOSFS_MEDIA_NONE;
	    }
	}
	else
	{
	    media = DOSFS_MEDIA_SDSC;
	}
		
	if (media != DOSFS_MEDIA_NONE)
	{
	    /* Send ACMD_SD_SEND_OP_COND, and wait till the initialization process is done. The SDCARD has 1000ms to
	     * respond to that with 0x00.
	     */
	    
	    millis = armv7m_systick_millis();

	    do
	    {
		dosfs_sdcard_send_command(sdcard, SD_CMD_APP_CMD, 0, 0);
		
		if (sdcard->response[0] > 0x01)
		{
		    media = DOSFS_MEDIA_NONE;
		}
		else
		{
		    dosfs_sdcard_send_command(sdcard, SD_ACMD_SD_SEND_OP_COND, ((media == DOSFS_MEDIA_SDHC) ? 0x40000000 : 0x00000000), 0);

		    if (sdcard->response[0] > 0x01)
		    {
			media = DOSFS_MEDIA_NONE;
		    }
		}
	    }
	    while ((media != DOSFS_MEDIA_NONE) && (sdcard->response[0] == 0x01) && !((armv7m_systick_millis() - millis) > 1000));
	}

	if (sdcard->response[0] != 0x00)
	{
	    /* The card did not respond, so it's not an SDCARD.
	     */
	    media = DOSFS_MEDIA_NONE;
	}
	else
	{
	    /* Here there is a SDSC or SDHC card. Hence switch the state to DOSFS_SDCARD_STATE_READY,
	     * so that dosfs_sdcard_send_command() does the full error checking.
	     */
	    
	    sdcard->state = DOSFS_SDCARD_STATE_READY;
	    
	    if (media == DOSFS_MEDIA_SDHC)
	    {
		/* Now it's time to find out whether we really have a SDHC, or a SDSC supporting V2.
		 */
		status = dosfs_sdcard_send_command(sdcard, SD_CMD_READ_OCR, 0x00000000, 4);
			    
		if (status == F_NO_ERROR)
		{
		    if (!(sdcard->response[1] & 0x40))
		    {
			media = DOSFS_MEDIA_SDSC;
		    }
		}
	    }

#if (DOSFS_CONFIG_SDCARD_CRC == 1)
	    if (status == F_NO_ERROR)
	    {
		status  = dosfs_sdcard_send_command(sdcard, SD_CMD_CRC_ON_OFF, 1, 0);
	    }
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */
			
	    if (status == F_NO_ERROR)
	    {
		status = dosfs_sdcard_send_command(sdcard, SD_CMD_SET_BLOCKLEN, 512, 0);
	    }

	    /* If there was no error up to here the "media" is valid and the card is
	     * ready for use.
	     */
	    if (status == F_NO_ERROR)
	    {
		sdcard->media = media;
		sdcard->flags = 0;
		sdcard->shift = (media == DOSFS_MEDIA_SDHC) ? 0 : 9;

		mode = DOSFS_SDCARD_MODE_DATA_TRANSFER;

#if (DOSFS_CONFIG_SDCARD_HIGH_SPEED == 1)
		if (media == DOSFS_MEDIA_SDHC)
		{
		    dosfs_sdcard_send_command(sdcard, SD_CMD_SWITCH_FUNC, 0x80ffff01, 0);

		    if (sdcard->response[0] == 0x00)
		      {
			dosfs_sdcard_receive_data(sdcard, data, 64, &retries);
		        
			mode = DOSFS_SDCARD_MODE_DATA_TRANSFER_HS;
		    }
		}
#endif /* (DOSFS_CONFIG_SDCARD_HIGH_SPEED == 1) */


		sdcard->speed = DOSFS_PORT_SDCARD_SPI_MODE(sdcard, mode);
	    }
	}

	if (media == DOSFS_MEDIA_NONE)
	{
	    if (status == F_NO_ERROR)
	    {
		status = F_ERR_INVALIDMEDIA;
	    }
	}
    }
    else
    {
	status = F_ERR_CARDREMOVED;
    }

    if (status != F_NO_ERROR)
    {
	sdcard->state = DOSFS_SDCARD_STATE_RESET;
	sdcard->speed = DOSFS_PORT_SDCARD_SPI_MODE(sdcard, DOSFS_SDCARD_MODE_NONE);
    }
    
    return status;
}

static int dosfs_sdcard_write_stop(dosfs_sdcard_t *sdcard)
{
    int status = F_NO_ERROR;
    uint8_t response;

    status = dosfs_sdcard_wait_ready(sdcard);
    
    if (status == F_NO_ERROR)
    {
	/* There need to be 8 clocks before the "Stop Transfer Token",
	 * and 8 clocks after it, before the card signals busy properly.
	 * The 8 clocks before are covered by dosfs_sdcard_wait_ready().
	 */
	
	DOSFS_PORT_SDCARD_SPI_SEND(sdcard, SD_STOP_TRANSMISSION_TOKEN);
	
	status = dosfs_sdcard_wait_ready(sdcard);

	if (status == F_NO_ERROR)
	{
	    /* Here it's getting a tad interesting. The spec says that
	     * an error that occured after the STOP_TRANSMISSION_TOKEN
	     * occured will be forwarded to the next command. So here
	     * one really has to send a CMD_SEND_STATUS to find out.
	     *
	     * But there is the chance that the STOP_TRANSMISSION_TOKEN
	     * was lost. If so, the CMD_SEND_STATUS will transition to BUSY
	     * state. When this is detected, one has to wait for READY and
	     * then reissue CMD_SEND_STATUS.
	     */
	    
	    status = dosfs_sdcard_send_command(sdcard, SD_CMD_SEND_STATUS, 0, 1);
	    
	    if (status == F_NO_ERROR)
	    {
		response = DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
		
		sdcard->flags &= ~DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT;
		
		if (response != SD_READY_TOKEN)
		{
		    status = dosfs_sdcard_wait_ready(sdcard);
		    
		    if (status == F_NO_ERROR)
		    {
			status = dosfs_sdcard_send_command(sdcard, SD_CMD_SEND_STATUS, 0, 1);
		    }
		}
	    }
	    
	    if (status == F_NO_ERROR)
	    {
		sdcard->state = DOSFS_SDCARD_STATE_READY;

		if (sdcard->response[1] != 0x00)
		{
		    if (sdcard->response[1] & (SD_R2_CARD_IS_LOCKED | SD_R2_WP_ERASE_SKIP | SD_R2_CC_ERROR | SD_R2_ERASE_PARAM))
		    {
			status = F_ERR_ONDRIVE;
		    }
		    else
		    {
			if (sdcard->response[1] & (SD_R2_EXECUTION_ERROR | SD_R2_OUT_OF_RANGE))
			{
			    status = F_ERR_ONDRIVE;
			}
			else if (sdcard->response[1] & SD_R2_WP_VIOLATION)
			{
			    status = F_ERR_WRITEPROTECT;
			}
			else
			{
			    /* CARD_ECC_FAILED */
			    status = F_ERR_INVALIDSECTOR;
			}
			
			if (sdcard->p_status != NULL)
			{
			    if (*sdcard->p_status == F_NO_ERROR)
			    {
				*sdcard->p_status = status;
			    }
			    
			    status = F_NO_ERROR;
			}
		    }
		}
	    }
	}
    }

    sdcard->p_status = NULL;
				
    return status;
}

/* 
 * int dosfs_sdcard_lock(dosfs_sdcard_t *sdcard, int state, uint32_t address)
 *
 * Lock the sdcard device, pull CS low, and process "state" (READY, READ, WRITE).
 * Unless there is an error, the sdcard will be in READY state, locked and selected.
 */

static int dosfs_sdcard_lock(dosfs_sdcard_t *sdcard, int state, uint32_t address)
{
    int status = F_NO_ERROR;

#if defined(DOSFS_PORT_SDCARD_LOCK)
    status = DOSFS_PORT_SDCARD_LOCK();

    if (status == F_NO_ERROR)
#endif /* DOSFS_PORT_SDCARD_LOCK */
    {
	sdcard->flags &= ~DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT;
	    
	if (sdcard->state == DOSFS_SDCARD_STATE_RESET)
	{
	    if (!DOSFS_PORT_SDCARD_SPI_PRESENT(sdcard))
	    {
		status = F_ERR_CARDREMOVED;
	    }
	    else
	    {
		status = dosfs_sdcard_reset(sdcard);
	    }
	}
	else
	{
	    DOSFS_PORT_SDCARD_SPI_SELECT(sdcard);

	    if (status == F_NO_ERROR)
	    {
		if ((sdcard->state == state) && (sdcard->address == address))
		{
		    /* Continuation of a previous read/write ... */
		}
		else
		{
		    /* No special case, move the sdcard to DOSFS_SDCARD_STATE_READY.
		     */
		
		    if (sdcard->state == DOSFS_SDCARD_STATE_READ_SEQUENTIAL)
		    {
			status = dosfs_sdcard_send_command(sdcard, SD_CMD_STOP_TRANSMISSION, 0, 0);
		    
			if (status == F_NO_ERROR)
			{
			    sdcard->state = DOSFS_SDCARD_STATE_READY;
			}
		    }

		    if (sdcard->state == DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL)
		    {
			status = dosfs_sdcard_write_stop(sdcard);
		    }
		}
	    }
	}
    }
    
    if (status != F_NO_ERROR)
    {
	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_unlock(dosfs_sdcard_t *sdcard, int status)
{
    if (sdcard->state != DOSFS_SDCARD_STATE_RESET)
    {
	DOSFS_PORT_SDCARD_SPI_DESELECT(sdcard);
    }
    
    if (status == F_ERR_ONDRIVE)
    {
	sdcard->state = DOSFS_SDCARD_STATE_RESET;
	sdcard->speed = DOSFS_PORT_SDCARD_SPI_MODE(sdcard, DOSFS_SDCARD_MODE_NONE);
    }

#if defined(DOSFS_PORT_SDCARD_UNLOCK)
    if (status != F_ERR_BUSY)
    {
	DOSFS_PORT_SDCARD_UNLOCK();
    }
#endif /* DOSFS_PORT_SDCARD_UNLOCK */

    return status;
}

static int dosfs_sdcard_release(void *context)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    if (sdcard->state != DOSFS_SDCARD_STATE_RESET)
    {
	status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);
	
	if (status == F_NO_ERROR)
	{

	    status = dosfs_sdcard_unlock(sdcard, status);
	}
    }

    sdcard->state = DOSFS_SDCARD_STATE_INIT;

    return status;
}

static int dosfs_sdcard_info(void *context, uint8_t *p_media, uint8_t *p_write_protected, uint32_t *p_block_count, uint32_t *p_au_size, uint32_t *p_serial)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;
    unsigned int retries;
    uint32_t c_size, c_size_mult, read_bl_len, au_size;
    uint8_t data[64];

    static uint32_t dosfs_au_size_table[16] = {
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

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

    if (status == F_NO_ERROR)
    {
	*p_media = sdcard->media;
	*p_write_protected = false;

#if defined(DOSFS_PORT_SDCARD_SPI_WRITE_PROTECTED)
	if (DOSFS_PORT_SDCARD_SPI_WRITE_PROTECTED(sdcard))
	{
	    *p_write_protected = true;
	}
#endif /* DOSFS_PORT_SDCARD_SPI_WRITE_PROTECTED */

#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
	retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
	retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
	
	do
	{
	    status = dosfs_sdcard_send_command(sdcard, SD_CMD_SEND_CSD, 0, 0);

	    if (status == F_NO_ERROR)
	    {
		status = dosfs_sdcard_receive_data(sdcard, data, 16, &retries);

		if ((status == F_NO_ERROR) && !retries)
		{
		    /* PERM_WRITE_PROTECT.
		     * TMP_WRITE_PROTECT
		     */
		    if (data[14] & 0x30)
		    {
			*p_write_protected = true;
		    }

		    if ((data[0] & 0xc0) == 0)
		    {
			/* SDSC */
			
			read_bl_len = (uint32_t)(data[5] & 0x0f);
			c_size = ((uint32_t)(data[6] & 0x03) << 10) | ((uint32_t)data[7] << 2) | ((uint32_t)data[8] >> 6);
			c_size_mult = ((uint32_t)(data[9] & 0x03) << 1) | ((uint32_t)(data[10] & 0x80) >> 7);
			
			*p_block_count = ((c_size + 1) << (c_size_mult + 2)) << (read_bl_len - 9);
		    }
		    else
		    {
			/* SDHC */
			
			c_size = ((uint32_t)(data[7] & 0x3f) << 16) | ((uint32_t)data[8] << 8) | ((uint32_t)data[9]);
			
			*p_block_count = (c_size + 1) << (19 - 9);
		    }
		}
	    }
	} 
	while ((status == F_NO_ERROR) && retries);
	
	if (status == F_NO_ERROR)
	{
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
	    retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
	    retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

	    do
	    {
		status = dosfs_sdcard_send_command(sdcard, SD_CMD_APP_CMD, 0, 0);
	  
		if (status == F_NO_ERROR)
		{
		    status = dosfs_sdcard_send_command(sdcard, SD_ACMD_SD_STATUS, 0, 1);
	      
		    if (status == F_NO_ERROR)
		    {
			status = dosfs_sdcard_receive_data(sdcard, data, 64, &retries);
		  
			if ((status == F_NO_ERROR) && !retries)
			{
			    /* If there is a UHS_AU_SIZE, use it, otherwise use
			     * the regular AU_SIZE. The issue at hand is that
			     * SDHC can only report up to 4MB for AU_SIZE, but
			     * will report the correct AU size in UHS_AU_SIZE.
			     */
		      
			    au_size = data[14] & 0x0f;
		      
			    if (au_size == 0)
			    {
				au_size = (data[10] & 0xf0) >> 4;
			    }
		      
			    *p_au_size = dosfs_au_size_table[au_size];
			}
		    }
		}
	    }
	    while ((status == F_NO_ERROR) && retries);
	}

	if (status == F_NO_ERROR)
	{
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
	    retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
	    retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

	    do
	    {
		status = dosfs_sdcard_send_command(sdcard, SD_CMD_SEND_CID, 0, 0);

		if (status == F_NO_ERROR)
		{
		    status = dosfs_sdcard_receive_data(sdcard, data, 16, &retries);

		    if ((status == F_NO_ERROR) && !retries)
		    {
			*p_serial = (((uint32_t)data[9]  << 24) |
				     ((uint32_t)data[10] << 16) |
				     ((uint32_t)data[11] <<  8) |
				     ((uint32_t)data[12] <<  0));
		    }
		}
	    } 
	    while ((status == F_NO_ERROR) && retries);
	}

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}


static int dosfs_sdcard_format(void *context)
{
    /* ERASE the whole SDCARD ...
     */
    return F_NO_ERROR;
}

static int dosfs_sdcard_reclaim(void *context, uint32_t size)
{
    /* Enter STREAM mode ?
     */
    return F_NO_ERROR;
}

static int dosfs_sdcard_discard(void *context, uint32_t address, uint32_t length)
{
    /* Per block erase ?
     */
    return F_NO_ERROR;
}

static int dosfs_sdcard_read(void *context, uint32_t address, uint8_t *data)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
    unsigned int retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

    if (status == F_NO_ERROR)
    {
	do
	{
	    status = dosfs_sdcard_send_command(sdcard, SD_CMD_READ_SINGLE_BLOCK, (address << sdcard->shift), 0);

	    if (status == F_NO_ERROR)
	    {
		status = dosfs_sdcard_receive_data(sdcard, data, DOSFS_BLK_SIZE, &retries);

		if ((status == F_NO_ERROR) && !retries)
		{
		    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_read_single);
		}
	    }
	} 
	while ((status == F_NO_ERROR) && retries);

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_read_sequential(void *context, uint32_t address, uint32_t length, uint8_t *data)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
    unsigned int retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READ_SEQUENTIAL, address);

    if (status == F_NO_ERROR)
    {
	do
	{
	    if (sdcard->state != DOSFS_SDCARD_STATE_READ_SEQUENTIAL)
	    {
		status = dosfs_sdcard_send_command(sdcard, SD_CMD_READ_MULTIPLE_BLOCK, (address << sdcard->shift), 0);

		if (status == F_NO_ERROR)
		{
		    sdcard->state = DOSFS_SDCARD_STATE_READ_SEQUENTIAL;
		    sdcard->address = 0;
		    sdcard->count = 0;
		}
	    }
	    
	    if (status == F_NO_ERROR)
	    {
		status = dosfs_sdcard_receive_data(sdcard, data, DOSFS_BLK_SIZE, &retries);

		if ((status == F_NO_ERROR) && !retries)
		{
		    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_read_sequential);

		    if (sdcard->address == address)
		    {
			DOSFS_SDCARD_STATISTICS_COUNT(sdcard_read_coalesce);
		    }

		    address++;
		    length--;
			
		    data += DOSFS_BLK_SIZE;

		    sdcard->address = address;
		    sdcard->count++;

		    /* A block has been sucessfully read, so the retry counter
		     * can be reset.
		     */
		    retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES;
		}
	    }
	} 
	while ((status == F_NO_ERROR) && (length != 0));

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_write(void *context, uint32_t address, const uint8_t *data)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
    unsigned int retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

    if (status == F_NO_ERROR)
    {
	do
	{
	    status = dosfs_sdcard_send_command(sdcard, SD_CMD_WRITE_SINGLE_BLOCK, (address << sdcard->shift), 0);
	    
	    if (status == F_NO_ERROR)
	    {
		/* After the CMD_WRITE_SINGLE_BLOCK there needs to be 8 clocks before sending
		 * the Start Block Token.
		 */
		DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
		    
		status = dosfs_sdcard_send_data(sdcard, SD_START_WRITE_SINGLE_TOKEN, data, DOSFS_BLK_SIZE, &retries);
	    }
	}
	while ((status == F_NO_ERROR) && retries);

	if (status == F_NO_ERROR)
	{
	    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_write_single);

	    status = dosfs_sdcard_wait_ready(sdcard);
	    
	    if (status == F_NO_ERROR)
	    {
		status = dosfs_sdcard_send_command(sdcard, SD_CMD_SEND_STATUS, 0, 1);
		
		if (status == F_NO_ERROR)
		{
		    if (sdcard->response[1] != 0x00)
		    {
			if (sdcard->response[1] & (SD_R2_CARD_IS_LOCKED | SD_R2_WP_ERASE_SKIP | SD_R2_EXECUTION_ERROR | SD_R2_CC_ERROR | SD_R2_ERASE_PARAM | SD_R2_OUT_OF_RANGE))
			{
			    status = F_ERR_ONDRIVE;
			}
			else if (sdcard->response[1] & SD_R2_WP_VIOLATION)
			{
			    status = F_ERR_WRITEPROTECT;
			}
			else
			{
			    /* CARD_ECC_FAILED */
			    status = F_ERR_INVALIDSECTOR;
			}
		    }
		}
	    }
	}

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_write_sequential(void *context, uint32_t address, uint32_t length, const uint8_t *data, volatile uint8_t *p_status)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;
    int busy;
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
    unsigned int retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
    uint32_t retry_address = address;
#else /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
    unsigned int retries = 0;
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL, address);

    if (status == F_NO_ERROR)
    {
	sdcard->p_status = p_status;

	do
	{
	    if (sdcard->state != DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL)
	    {
		if (status == F_NO_ERROR)
		{
		    status = dosfs_sdcard_send_command(sdcard, SD_CMD_WRITE_MULTIPLE_BLOCK, (address << sdcard->shift), 0);

		    if (status == F_NO_ERROR)
		    {
			/* After the CMD_WRITE_MULTIPLE_BLOCK there needs to be 8 clocks before sending
			 * the Start Block Token.
			 */
			DOSFS_PORT_SDCARD_SPI_RECEIVE(sdcard);
			
			sdcard->state = DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL;
			sdcard->address = 0;
			sdcard->count = 0;
			
			busy = FALSE;
		    }
		}
	    }
	    else
	    {
		busy = TRUE;
	    }
		    
	    if (status == F_NO_ERROR)
	    {
		if (busy)
		{
		    status = dosfs_sdcard_wait_ready(sdcard);
		}
			
		if (status == F_NO_ERROR)
		{
		    status = dosfs_sdcard_send_data(sdcard, SD_START_WRITE_MULTIPLE_TOKEN, data, DOSFS_BLK_SIZE, &retries);

		    if ((status == F_NO_ERROR) && !retries)
		    {
			DOSFS_SDCARD_STATISTICS_COUNT(sdcard_write_sequential);
			    
			if (sdcard->address == address)
			{
			    DOSFS_SDCARD_STATISTICS_COUNT(sdcard_write_coalesce);
			}
			    
			address++;
			length--;
			    
			data += DOSFS_BLK_SIZE;
			    
			sdcard->address = address;
			sdcard->count++;

#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
			if (address >= retry_address)
			{
			    /* If this write is post the previous retry address,
			     * then reset the retry counter.
			     */
			    retries = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;
			}
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
		    }
#if (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0)
		    else
		    {
			if (retries)
			{
			    /* CRC errors are retried. Hence one needs to get the number of sucessfully
			     * written block since the last CMD_WRITE_MULITPLE_BLOCK was issued.
			     */

			    uint8_t response[4];
			    uint32_t delta;
			    unsigned int retries2 = DOSFS_CONFIG_SDCARD_DATA_RETRIES +1;

			    do
			    {
				status = dosfs_sdcard_send_command(sdcard, SD_CMD_APP_CMD, 0, 0);
		
				if (status == F_NO_ERROR)
				{
				    status = dosfs_sdcard_send_command(sdcard, SD_ACMD_SEND_NUM_WR_BLOCKS, 0, 0);
					
				    if (status == F_NO_ERROR)
				    {
					status = dosfs_sdcard_receive_data(sdcard, response, 4, &retries2);
				    }
				}
			    }
			    while ((status == F_NO_ERROR) && retries2);
					    
			    if (status == F_NO_ERROR)
			    {
				delta = 
				    (sdcard->count -
				     (((uint32_t)response[0] << 24) |
				      ((uint32_t)response[1] << 16) |
				      ((uint32_t)response[2] <<  8) |
				      ((uint32_t)response[3] <<  0)));
						
				if ((address - delta) < retry_address)
				{
				    /* This here enforces that we make forward progress, and do
				     * not try to write data that is not available.
				     */

				    status = F_ERR_WRITE;
				}
				else
				{
				    address -= delta;
				    length += delta;
					    
				    data -= (delta << DOSFS_BLK_SHIFT);
					    
				    retry_address = address;
				}
			    }
			}
		    }
#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) && (DOSFS_CONFIG_SDCARD_DATA_RETRIES != 0) */
		}
	    }
	}
	while ((status == F_NO_ERROR) && (length != 0));

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_sync(void *context, volatile uint8_t *p_status)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    if ((sdcard->state == DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL) && ((p_status == NULL) || (p_status == sdcard->p_status)))
    {
	status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

	if (status == F_NO_ERROR)
	{
	    status = dosfs_sdcard_unlock(sdcard, status);
	}
    }

    return status;
}

static const F_INTERFACE dosfs_sdcard_interface = {
    dosfs_sdcard_release,
    dosfs_sdcard_info,
    dosfs_sdcard_format,
    dosfs_sdcard_reclaim,
    dosfs_sdcard_discard,
    dosfs_sdcard_read,
    dosfs_sdcard_read_sequential,
    dosfs_sdcard_write,
    dosfs_sdcard_write_sequential,
    dosfs_sdcard_sync,
};

int dosfs_sdcard_init(uint32_t param, const F_INTERFACE **p_interface, void **p_context)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)&dosfs_sdcard;
    int status = F_NO_ERROR;

    if (sdcard->state == DOSFS_SDCARD_STATE_NONE)
    {
#if defined(DOSFS_PORT_SDCARD_SPI_INIT)
	if (!DOSFS_PORT_SDCARD_SPI_INIT(sdcard))
	{
	    sdcard = NULL;

	    status = F_ERR_INVALIDMEDIA;
	}
	else
#endif /* DOSFS_PORT_SDCARD_INIT */
	{
	    sdcard->state = DOSFS_SDCARD_STATE_INIT;
	}
    }

    if (sdcard && (sdcard->state == DOSFS_SDCARD_STATE_INIT))
    {
	sdcard->state = DOSFS_SDCARD_STATE_RESET;
    }
    else
    {
	status = F_ERR_CARDREMOVED;
    }
    
    *p_interface = &dosfs_sdcard_interface;
    *p_context = (void*)sdcard;

    return status;
}

#else /* (DOSFS_CONFIG_SDCARD_SIMULATE == 0) */

/********************************************************************************************************************************************/

#include <malloc.h>
#include <stdio.h>

static int dosfs_sdcard_reset(dosfs_sdcard_t *sdcard)
{
    int status = F_NO_ERROR;

    sdcard->state = DOSFS_SDCARD_STATE_RESET;
    sdcard->media = (DOSFS_CONFIG_SDCARD_SIMULATE_BLKCNT < 4209984) ? DOSFS_MEDIA_SDSC : DOSFS_MEDIA_SDHC;
    sdcard->flags = 0;
    sdcard->shift = (sdcard->media == DOSFS_MEDIA_SDHC) ? 0 : 9;
    sdcard->speed = 25000000;

    if (sdcard->image == NULL)
    {
	sdcard->image = (uint8_t*)malloc(DOSFS_CONFIG_SDCARD_SIMULATE_BLKCNT * DOSFS_BLK_SIZE);

	if (sdcard->image == NULL)
	{
	    status = F_ERR_INVALIDMEDIA;
	}
	else
	{
	    /* Simulate a compeltely erased device */
	    memset(sdcard->image, 0xff, DOSFS_CONFIG_SDCARD_SIMULATE_BLKCNT * DOSFS_BLK_SIZE);

	    sdcard->state = DOSFS_SDCARD_STATE_READY;
	}
    }

    return status;
}

static int dosfs_sdcard_lock(dosfs_sdcard_t *sdcard, int state, uint32_t address)
{
    int status = F_NO_ERROR;

    /* "state" can be READY, READ, WRITE */

    if (sdcard->state == DOSFS_SDCARD_STATE_RESET)
    {
        status = dosfs_sdcard_reset(sdcard);
    }

    if (status == F_NO_ERROR)
    {
	if ((sdcard->state == state) && (sdcard->address == address))
	{
	    /* Continuation of a previous read/write ... */
	}
	else
	{
	    /* No special case, move the sdcard to DOSFS_SDCARD_STATE_READY.
	     */
	    
	    if (sdcard->state == DOSFS_SDCARD_STATE_READ_SEQUENTIAL)
	    {
#if (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1)
		printf("SDCARD_READ_STOP\n");
#endif /* (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1) */
	    }
	    
	    if (sdcard->state == DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL)
	    {
#if (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1)
		printf("SDCARD_WRITE_STOP\n");
#endif /* (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1) */
	    }
	    
	    sdcard->state = DOSFS_SDCARD_STATE_READY;
	}
    }

    return status;
}

static int dosfs_sdcard_unlock(dosfs_sdcard_t *sdcard, int status)
{
    return status;
}

static int dosfs_sdcard_release(void *context)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    if (sdcard->state != DOSFS_SDCARD_STATE_RESET)
    {
	status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

	if (status == F_NO_ERROR)
	{
	    status = dosfs_sdcard_unlock(sdcard, status);
	}
    }

    sdcard->state = DOSFS_SDCARD_STATE_NONE;

    return status;
}

static int dosfs_sdcard_info(void *context, uint8_t *p_media, uint8_t *p_write_protected, uint32_t *p_block_count, uint32_t *p_au_size, uint32_t *p_serial)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

    if (status == F_NO_ERROR)
    {
	*p_media = sdcard->media;
	*p_write_protected = false;
	*p_block_count = DOSFS_CONFIG_SDCARD_SIMULATE_BLKCNT;
	*p_au_size = 0;
	*p_serial = 0;

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_format(void *context)
{
    return F_NO_ERROR;
}

static int dosfs_sdcard_reclaim(void *context, uint32_t size)
{
    return F_NO_ERROR;
}

static int dosfs_sdcard_discard(void *context, uint32_t address, uint32_t length)
{
    return F_NO_ERROR;
}

static int dosfs_sdcard_read(void *context, uint32_t address, uint8_t *data)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

    if (status == F_NO_ERROR)
    {
#if (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1)
        printf("SDCARD_READ %08x\n", address);
#endif /* (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1) */
	
	memcpy(data, sdcard->image + (DOSFS_BLK_SIZE * address), DOSFS_BLK_SIZE);

	DOSFS_SDCARD_STATISTICS_COUNT(sdcard_read_single);

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_read_sequential(void *context, uint32_t address, uint32_t length, uint8_t *data)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READ_SEQUENTIAL, address);

    if (status == F_NO_ERROR)
    {
	if (sdcard->state != DOSFS_SDCARD_STATE_READ_SEQUENTIAL)
	{
	    sdcard->state = DOSFS_SDCARD_STATE_READ_SEQUENTIAL;
	    sdcard->address = 0;
	    sdcard->count = 0;
	}

#if (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1)
	printf("SDCARD_READ_SEQUENTIAL %08x, %d\n", address, length);
#endif /* (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1) */
	
	memcpy(data, sdcard->image + (DOSFS_BLK_SIZE * address), (DOSFS_BLK_SIZE * length));

	DOSFS_SDCARD_STATISTICS_COUNT_N(sdcard_read_sequential, length);
	DOSFS_SDCARD_STATISTICS_COUNT_N(sdcard_read_coalesce, ((sdcard->address == address) ? length : (length -1)));

	address += length;

	sdcard->address = address;
	sdcard->count += length;

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_write(void *context, uint32_t address, const uint8_t *data)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

    if (status == F_NO_ERROR)
    {
#if (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1)
	printf("SDCARD_WRITE %08x\n", address);
#endif /* (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1) */
	
	memcpy(sdcard->image + (DOSFS_BLK_SIZE * address), data, DOSFS_BLK_SIZE);
	
	DOSFS_SDCARD_STATISTICS_COUNT(sdcard_write_single);

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_write_sequential(void *context, uint32_t address, uint32_t length, const uint8_t *data, volatile uint8_t *p_status)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL, address);

    if (status == F_NO_ERROR)
    {
	if  (sdcard->state != DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL)
	{
	    sdcard->state = DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL;
	    sdcard->address = 0;
	    sdcard->count = 0;
	}

#if (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1)
	printf("SDCARD_WRITE_SEQUENTIAL %08x, %d\n", address, length);
#endif /* (DOSFS_CONFIG_SDCARD_SIMULATE_TRACE == 1) */

	memcpy(sdcard->image + (DOSFS_BLK_SIZE * address), data, (DOSFS_BLK_SIZE * length));
	
	DOSFS_SDCARD_STATISTICS_COUNT_N(sdcard_write_sequential, length);
	DOSFS_SDCARD_STATISTICS_COUNT_N(sdcard_write_coalesce, ((sdcard->address == address) ? length : (length -1)));

	address += length;

	sdcard->address = address;
	sdcard->count += length;

	status = dosfs_sdcard_unlock(sdcard, status);
    }

    return status;
}

static int dosfs_sdcard_sync(void *context, volatile uint8_t *p_status)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)context;
    int status = F_NO_ERROR;

    if ((sdcard->state == DOSFS_SDCARD_STATE_WRITE_SEQUENTIAL) && ((p_status == NULL) || (p_status == sdcard->p_status)))
    {
	status = dosfs_sdcard_lock(sdcard, DOSFS_SDCARD_STATE_READY, 0);

	if (status == F_NO_ERROR)
	{
	    status = dosfs_sdcard_unlock(sdcard, status);
	}
    }

    return status;
}

static const F_INTERFACE dosfs_sdcard_interface = {
    dosfs_sdcard_release,
    dosfs_sdcard_info,
    dosfs_sdcard_format,
    dosfs_sdcard_reclaim,
    dosfs_sdcard_discard,
    dosfs_sdcard_read,
    dosfs_sdcard_read_sequential,
    dosfs_sdcard_write,
    dosfs_sdcard_write_sequential,
    dosfs_sdcard_sync,
};

int dosfs_sdcard_init(uint32_t param, const F_INTERFACE **p_interface, void **p_context)
{
    dosfs_sdcard_t *sdcard = (dosfs_sdcard_t*)&dosfs_sdcard;
    int status = F_NO_ERROR;

    if (sdcard->state == DOSFS_SDCARD_STATE_NONE)
    {
	sdcard->state = DOSFS_SDCARD_STATE_RESET;

#if (DOSFS_CONFIG_STATISTICS == 1)
	memset(&sdcard->statistics, 0, sizeof(sdcard->statistics));
#endif /* (DOSFS_CONFIG_STATISTICS == 1) */
    }
    else
    {
	status = F_ERR_INVALIDMEDIA;
    }
    
    *p_interface = &dosfs_sdcard_interface;
    *p_context = (void*)sdcard;

    return status;
}

#endif /* (DOSFS_CONFIG_SDCAR_SIMULATE == 0) */
