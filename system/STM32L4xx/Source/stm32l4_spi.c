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

#include <stdio.h>

#include "stm32l4xx.h"

#include "armv7m.h"

#include "stm32l4_gpio.h"
#include "stm32l4_spi.h"
#include "stm32l4_dma.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_spi_driver_t {
    stm32l4_spi_t     *instances[SPI_INSTANCE_COUNT];
} stm32l4_spi_driver_t;

static stm32l4_spi_driver_t stm32l4_spi_driver;

#define SPI_RX_DMA_OPTION_RECEIVE_8	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |     \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_RX_DMA_OPTION_TRANSMIT_8	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |     \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_RX_DMA_OPTION_TRANSFER_8	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |     \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_RX_DMA_OPTION_RECEIVE_16	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |     \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_RX_DMA_OPTION_TRANSMIT_16	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |     \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_RX_DMA_OPTION_TRANSFER_16	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |     \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_TX_DMA_OPTION_RECEIVE_8	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_TX_DMA_OPTION_TRANSMIT_8	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_TX_DMA_OPTION_TRANSFER_8	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_TX_DMA_OPTION_RECEIVE_16	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_TX_DMA_OPTION_TRANSMIT_16	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define SPI_TX_DMA_OPTION_TRANSFER_16	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

static void stm32l4_spi_dma_callback(stm32l4_spi_t *spi, uint32_t events);

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_spi_rd8(SPI_TypeDef *SPI, void *rx_data)
{
    *((uint8_t*)rx_data) = *((volatile uint8_t*)(&SPI->DR));
}

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_spi_rd16(SPI_TypeDef *SPI, void *rx_data)
{
    *((uint16_t*)rx_data) = SPI->DR;
}

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_spi_wr8(SPI_TypeDef *SPI, const void *tx_data)
{
    *((volatile uint8_t*)(&SPI->DR)) = *((const uint8_t*)tx_data);
}

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_spi_wr16(SPI_TypeDef *SPI, const void *tx_data)
{
    SPI->DR = *((const uint16_t*)tx_data);
}

static void stm32l4_spi_start(stm32l4_spi_t *spi)
{
    stm32l4_system_periph_enable(SYSTEM_PERIPH_SPI1 + spi->instance);

    if (spi->state != SPI_STATE_BUSY)
    {
	if (spi->mode & SPI_MODE_RX_DMA)
	{
	    stm32l4_dma_enable(&spi->rx_dma, (stm32l4_dma_callback_t)stm32l4_spi_dma_callback, spi);
	}
	
	if (spi->mode & SPI_MODE_TX_DMA)
	{
	    stm32l4_dma_enable(&spi->tx_dma, NULL, NULL);
	}
    }
}

static void stm32l4_spi_stop(stm32l4_spi_t *spi)
{
    if (spi->state != SPI_STATE_BUSY)
    {
	if (spi->mode & SPI_MODE_RX_DMA)
	{
	    stm32l4_dma_enable(&spi->rx_dma, (stm32l4_dma_callback_t)stm32l4_spi_dma_callback, spi);
	}
	
	if (spi->mode & SPI_MODE_TX_DMA)
	{
	    stm32l4_dma_enable(&spi->tx_dma, NULL, NULL);
	}
    }

    stm32l4_system_periph_disable(SYSTEM_PERIPH_SPI1 + spi->instance);
}

static void stm32l4_spi_dma_callback(stm32l4_spi_t *spi, uint32_t events)
{
    SPI_TypeDef *SPI = spi->SPI;
    uint32_t spi_cr1;

    if (SPI->CR1 & SPI_CR1_CRCEN)
    {
	if (spi->xf_size != 0)
	{
	    spi->xf_count = 0;
	    spi->xf_size = 0;
	    
	    if ((spi->state == SPI_STATE_RECEIVE_8_DMA) || (spi->state == SPI_STATE_TRANSMIT_8_DMA) || (spi->state == SPI_STATE_TRANSFER_8_DMA))
	    {
		stm32l4_dma_start(&spi->rx_dma, (uint32_t)&spi->rx_crc16[0], (uint32_t)&SPI->DR, 2, SPI_RX_DMA_OPTION_RECEIVE_8);
	    }
	    else
	    {
		stm32l4_dma_start(&spi->rx_dma, (uint32_t)&spi->rx_crc16[0], (uint32_t)&SPI->DR, 1, SPI_RX_DMA_OPTION_RECEIVE_16);
	    }
	    
	    return;
	}
	else
	{
	    spi->crc16 = !!(SPI->SR & SPI_SR_CRCERR);
	    
	    spi_cr1 = SPI->CR1 & ~(SPI_CR1_CRCEN | SPI_CR1_CRCL | SPI_CR1_SPE);
	    
	    SPI->CR1 = spi_cr1 | (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    SPI->CR1 = spi_cr1;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    if ((spi->state == SPI_STATE_RECEIVE_8_DMA) || (spi->state == SPI_STATE_RECEIVE_16_DMA))
	    {
		stm32l4_gpio_pin_alternate(spi->pins.mosi);
	    }
	}
    }
    
    SPI->CR2 = spi->cr2 | (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH);
    SPI->CR1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST)) | SPI_CR1_SPE;
    
    spi->state = SPI_STATE_SELECTED;
    
    if (spi->tx_data)
    {
	if (spi->rx_data)
	{
	    events = SPI_EVENT_TRANSFER_DONE;
	}
	else
	{
	    events = SPI_EVENT_TRANSMIT_DONE;
	}
    }
    else
    {
	events = SPI_EVENT_RECEIVE_DONE;
    }
    
    events &= spi->events;

    if (events)
    {
	(*spi->callback)(spi->context, events);
    }
}

static void stm32l4_spi_finish(stm32l4_spi_t *spi, uint32_t events)
{
    SPI_TypeDef *SPI = spi->SPI;

    NVIC_DisableIRQ(spi->interrupt);
	
    SPI->CR2 = spi->cr2 | (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH);
    SPI->CR1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST)) | SPI_CR1_SPE;
    
    spi->state = SPI_STATE_SELECTED;

    events &= spi->events;

    if (events)
    {
	(*spi->callback)(spi->context, events);
    }
}

static void stm32l4_spi_interrupt(stm32l4_spi_t *spi)
{
    SPI_TypeDef *SPI = spi->SPI;
    uint32_t spi_cr1, spi_cr2;
    uint16_t rx_null; 
    uint8_t tx_crc16[2];
    const uint16_t tx_default = 0xffff;
      
    switch (spi->state) {
    case SPI_STATE_NONE:
    case SPI_STATE_INIT:
    case SPI_STATE_BUSY:
    case SPI_STATE_READY:
    case SPI_STATE_SELECTED:
    case SPI_STATE_HALFDUPLEX_8_DMA:
    case SPI_STATE_RECEIVE_8_DMA:
    case SPI_STATE_RECEIVE_16_DMA:
    case SPI_STATE_TRANSMIT_8_DMA:
    case SPI_STATE_TRANSMIT_16_DMA:
    case SPI_STATE_TRANSFER_8_DMA:
    case SPI_STATE_TRANSFER_16_DMA:
	break;

    case SPI_STATE_HALFDUPLEX_8_M:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	spi->rx_data += 1;

	if (spi->rx_data == spi->rx_data_e)
	{
	    spi->state = SPI_STATE_HALFDUPLEX_8_1;

	    SPI->CR1 &= ~SPI_CR1_SPE;
	}
	break;

    case SPI_STATE_HALFDUPLEX_8_1:
	SPI->CR2 &= ~SPI_CR2_RXNEIE;

	stm32l4_spi_rd8(SPI, spi->rx_data);

	while (SPI->SR & SPI_SR_BSY) { }

	while (SPI->SR & SPI_SR_FRLVL)
	{
	    stm32l4_spi_rd8(SPI, &rx_null);
	}
	
	SPI->CR1 &= ~SPI_CR1_BIDIMODE;
	SPI->CR1 |= SPI_CR1_SPE;

	stm32l4_spi_finish(spi, SPI_EVENT_RECEIVE_DONE);
	break;

    case SPI_STATE_RECEIVE_8_3:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	spi->rx_data += 1;

	spi->state = SPI_STATE_RECEIVE_8_2;
	break;

    case SPI_STATE_RECEIVE_8_2:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	spi->rx_data += 1;

	spi->state = SPI_STATE_RECEIVE_8_1;
	break;

    case SPI_STATE_RECEIVE_8_1:
	stm32l4_spi_rd8(SPI, spi->rx_data);

	if (SPI->CR1 & SPI_CR1_CRCEN)
	{
	    while (SPI->SR & SPI_SR_BSY) { }

	    spi->crc16 = SPI->RXCRCR;

	    spi_cr1 = SPI->CR1 & ~(SPI_CR1_CRCEN | SPI_CR1_CRCL | SPI_CR1_SPE);

	    SPI->CR1 = spi_cr1 | (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    SPI->CR1 = spi_cr1;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    stm32l4_spi_wr8(SPI, &tx_default);
	    stm32l4_spi_wr8(SPI, &tx_default);

	    spi->state = SPI_STATE_RECEIVE_8_CRC16_H;
	}
	else
	{
	    stm32l4_spi_finish(spi, SPI_EVENT_RECEIVE_DONE);
	}
	break;

    case SPI_STATE_RECEIVE_8_CRC16_H:
	stm32l4_spi_rd8(SPI, &spi->rx_crc16[0]);

	spi->state = SPI_STATE_RECEIVE_8_CRC16_L;
	break;

    case SPI_STATE_RECEIVE_8_CRC16_L:
	stm32l4_spi_rd8(SPI, &spi->rx_crc16[1]);

	spi->crc16 ^= (spi->rx_crc16[0] << 8);
	spi->crc16 ^= (spi->rx_crc16[1] << 0);
	
	stm32l4_spi_finish(spi, SPI_EVENT_RECEIVE_DONE);
	break;

    case SPI_STATE_RECEIVE_16_S:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	spi->rx_data += 1;

	spi_cr2 = SPI->CR2;

	spi_cr2 &= ~(SPI_CR2_DS | SPI_CR2_FRXTH | SPI_CR2_RXNEIE | SPI_CR2_TXEIE);
	spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXNEIE);
		
	SPI->CR2 = spi_cr2;

	stm32l4_spi_wr16(SPI, &tx_default);
	stm32l4_spi_wr16(SPI, &tx_default);

	if (spi->rx_data == spi->rx_data_e)
	{
	    spi->state = SPI_STATE_RECEIVE_16_2;
	}
	else
	{
	    spi->state = SPI_STATE_RECEIVE_16_M;
	}
	break;

    case SPI_STATE_RECEIVE_16_M:
	stm32l4_spi_rd16(SPI, spi->rx_data);
	spi->rx_data += 2;

	stm32l4_spi_wr16(SPI, &tx_default);

	if (spi->rx_data == spi->rx_data_e)
	{
	    spi->state = SPI_STATE_RECEIVE_16_2;
	}
	break;

    case SPI_STATE_RECEIVE_16_2:
	stm32l4_spi_rd16(SPI, spi->rx_data);
	spi->rx_data += 2;

	spi->state = SPI_STATE_RECEIVE_16_1;
	break;

    case SPI_STATE_RECEIVE_16_1:
	stm32l4_spi_rd16(SPI, spi->rx_data);

	if (SPI->CR1 & SPI_CR1_CRCEN)
	{
	    while (SPI->SR & SPI_SR_BSY) { }

	    spi->crc16 = SPI->RXCRCR;

	    spi_cr1 = SPI->CR1 & ~(SPI_CR1_CRCEN | SPI_CR1_CRCL | SPI_CR1_SPE);

	    SPI->CR1 = spi_cr1 | (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    SPI->CR1 = spi_cr1;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    stm32l4_spi_wr16(SPI, &tx_default);

	    spi->state = SPI_STATE_RECEIVE_16_CRC16;
	}
	else
	{
	    stm32l4_spi_finish(spi, SPI_EVENT_RECEIVE_DONE);
	}
	break;

    case SPI_STATE_RECEIVE_16_CRC16:
	stm32l4_spi_rd16(SPI, &spi->rx_crc16[0]);

	spi->crc16 ^= (spi->rx_crc16[0] << 8);
	spi->crc16 ^= (spi->rx_crc16[1] << 0);
	
	stm32l4_spi_finish(spi, SPI_EVENT_RECEIVE_DONE);
	break;
	
    case SPI_STATE_TRANSMIT_8_3:
	stm32l4_spi_rd8(SPI, &rx_null);

	spi->state = SPI_STATE_TRANSMIT_8_2;
	break;

    case SPI_STATE_TRANSMIT_8_2:
	stm32l4_spi_rd8(SPI, &rx_null);

	spi->state = SPI_STATE_TRANSMIT_8_1;
	break;

    case SPI_STATE_TRANSMIT_8_1:
	stm32l4_spi_rd8(SPI, &rx_null);

	if (SPI->CR1 & SPI_CR1_CRCEN)
	{
	    while (SPI->SR & SPI_SR_BSY) { }

	    tx_crc16[0] = SPI->TXCRCR >> 8;
	    tx_crc16[1] = SPI->TXCRCR >> 0;

	    spi_cr1 = SPI->CR1 & ~(SPI_CR1_CRCEN | SPI_CR1_CRCL | SPI_CR1_SPE);

	    SPI->CR1 = spi_cr1 | (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    SPI->CR1 = spi_cr1;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    stm32l4_spi_wr8(SPI, &tx_crc16[0]);
	    stm32l4_spi_wr8(SPI, &tx_crc16[1]);

	    spi->state = SPI_STATE_TRANSMIT_8_CRC16_H;
	}
	else
	{
	    stm32l4_spi_finish(spi, SPI_EVENT_TRANSMIT_DONE);
	}
	break;

    case SPI_STATE_TRANSMIT_8_CRC16_H:
	stm32l4_spi_rd8(SPI, &rx_null);

	spi->state = SPI_STATE_TRANSMIT_8_CRC16_L;
	break;

    case SPI_STATE_TRANSMIT_8_CRC16_L:
	stm32l4_spi_rd8(SPI, &rx_null);

	spi->crc16 = 0;
	
	stm32l4_spi_finish(spi, SPI_EVENT_TRANSMIT_DONE);
	break;
	
    case SPI_STATE_TRANSMIT_16_S:
	stm32l4_spi_rd8(SPI, &rx_null);

	spi_cr2 = SPI->CR2;

	spi_cr2 &= ~(SPI_CR2_DS | SPI_CR2_FRXTH | SPI_CR2_RXNEIE | SPI_CR2_TXEIE);
	spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXNEIE);
		
	SPI->CR2 = spi_cr2;

	stm32l4_spi_wr16(SPI, spi->tx_data);
	spi->tx_data += 2;

	stm32l4_spi_wr16(SPI, spi->tx_data);
	spi->tx_data += 2;

	if (spi->tx_data == spi->tx_data_e)
	{
	    spi->state = SPI_STATE_TRANSMIT_16_2;
	}
	else
	{
	    spi->state = SPI_STATE_TRANSMIT_16_M;
	}
	break;

    case SPI_STATE_TRANSMIT_16_M:
	stm32l4_spi_rd16(SPI, &rx_null);

	stm32l4_spi_wr16(SPI, spi->tx_data);
	spi->tx_data += 2;

	if (spi->tx_data == spi->tx_data_e)
	{
	    spi->state = SPI_STATE_TRANSMIT_16_2;
	}
	break;

    case SPI_STATE_TRANSMIT_16_2:
	stm32l4_spi_rd16(SPI, &rx_null);

	spi->state = SPI_STATE_TRANSMIT_16_1;
	break;

    case SPI_STATE_TRANSMIT_16_1:
	stm32l4_spi_rd16(SPI, &rx_null);

	if (SPI->CR1 & SPI_CR1_CRCEN)
	{
	    while (SPI->SR & SPI_SR_BSY) { }

	    tx_crc16[0] = SPI->TXCRCR >> 8;
	    tx_crc16[1] = SPI->TXCRCR >> 0;

	    spi_cr1 = SPI->CR1 & ~(SPI_CR1_CRCEN | SPI_CR1_CRCL | SPI_CR1_SPE);

	    SPI->CR1 = spi_cr1 | (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    SPI->CR1 = spi_cr1;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    stm32l4_spi_wr16(SPI, &tx_crc16[0]);

	    spi->state = SPI_STATE_TRANSMIT_16_CRC16;
	}
	else
	{
	    stm32l4_spi_finish(spi, SPI_EVENT_TRANSMIT_DONE);
	}
	break;

    case SPI_STATE_TRANSMIT_16_CRC16:
	stm32l4_spi_rd16(SPI, &rx_null);

	spi->crc16 = 0;
	
	stm32l4_spi_finish(spi, SPI_EVENT_TRANSMIT_DONE);
	break;

    case SPI_STATE_TRANSFER_8_3:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	spi->rx_data += 1;

	spi->state = SPI_STATE_TRANSFER_8_2;
	break;

    case SPI_STATE_TRANSFER_8_2:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	spi->rx_data += 1;

	spi->state = SPI_STATE_TRANSFER_8_1;
	break;

    case SPI_STATE_TRANSFER_8_1:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	
	if (SPI->CR1 & SPI_CR1_CRCEN)
	{
	    while (SPI->SR & SPI_SR_BSY) { }

	    spi->crc16 = SPI->RXCRCR;
	    
	    tx_crc16[0] = SPI->TXCRCR >> 8;
	    tx_crc16[1] = SPI->TXCRCR >> 0;

	    spi_cr1 = SPI->CR1 & ~(SPI_CR1_CRCEN | SPI_CR1_CRCL | SPI_CR1_SPE);

	    SPI->CR1 = spi_cr1 | (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    SPI->CR1 = spi_cr1;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    stm32l4_spi_wr8(SPI, &tx_crc16[0]);
	    stm32l4_spi_wr8(SPI, &tx_crc16[1]);

	    spi->state = SPI_STATE_TRANSFER_8_CRC16_H;
	}
	else
	{
	    stm32l4_spi_finish(spi, SPI_EVENT_TRANSFER_DONE);
	}
	break;

    case SPI_STATE_TRANSFER_8_CRC16_H:
	stm32l4_spi_rd8(SPI, &spi->rx_crc16[0]);

	spi->state = SPI_STATE_TRANSFER_8_CRC16_L;
	break;

    case SPI_STATE_TRANSFER_8_CRC16_L:
	stm32l4_spi_rd8(SPI, &spi->rx_crc16[1]);

	spi->crc16 ^= (spi->rx_crc16[0] << 8);
	spi->crc16 ^= (spi->rx_crc16[1] << 0);
	
	stm32l4_spi_finish(spi, SPI_EVENT_TRANSFER_DONE);
	break;

    case SPI_STATE_TRANSFER_16_S:
	stm32l4_spi_rd8(SPI, spi->rx_data);
	spi->rx_data += 1;

	spi_cr2 = SPI->CR2;

	spi_cr2 &= ~(SPI_CR2_DS | SPI_CR2_FRXTH | SPI_CR2_RXNEIE | SPI_CR2_TXEIE);
	spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXNEIE);
		
	SPI->CR2 = spi_cr2;

	stm32l4_spi_wr16(SPI, spi->tx_data);
	spi->tx_data += 2;

	stm32l4_spi_wr16(SPI, spi->tx_data);
	spi->tx_data += 2;

	if (spi->tx_data == spi->tx_data_e)
	{
	    spi->state = SPI_STATE_TRANSFER_16_2;
	}
	else
	{
	    spi->state = SPI_STATE_TRANSFER_16_M;
	}
	break;

    case SPI_STATE_TRANSFER_16_M:
	stm32l4_spi_rd16(SPI, spi->rx_data);
	spi->rx_data += 2;

	stm32l4_spi_wr16(SPI, spi->tx_data);

	spi->tx_data += 2;
	
	if (spi->tx_data == spi->tx_data_e)
	{
	    spi->state = SPI_STATE_TRANSFER_16_2;
	}
	break;

    case SPI_STATE_TRANSFER_16_2:
	stm32l4_spi_rd16(SPI, spi->rx_data);
	spi->rx_data += 2;

	spi->state = SPI_STATE_TRANSFER_16_1;
	break;

    case SPI_STATE_TRANSFER_16_1:
	stm32l4_spi_rd16(SPI, spi->rx_data);

	if (SPI->CR1 & SPI_CR1_CRCEN)
	{
	    while (SPI->SR & SPI_SR_BSY) { }

	    spi->crc16 = SPI->RXCRCR;
	    
	    tx_crc16[0] = SPI->TXCRCR >> 8;
	    tx_crc16[1] = SPI->TXCRCR >> 0;

	    spi_cr1 = SPI->CR1 & ~(SPI_CR1_CRCEN | SPI_CR1_CRCL | SPI_CR1_SPE);

	    SPI->CR1 = spi_cr1 | (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    SPI->CR1 = spi_cr1;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    stm32l4_spi_wr16(SPI, &tx_crc16[0]);

	    spi->state = SPI_STATE_TRANSFER_16_CRC16;
	}
	else
	{
	    stm32l4_spi_finish(spi, SPI_EVENT_TRANSFER_DONE);
	}
	break;

    case SPI_STATE_TRANSFER_16_CRC16:
	stm32l4_spi_rd16(SPI, &spi->rx_crc16[0]);

	spi->crc16 ^= (spi->rx_crc16[0] << 8);
	spi->crc16 ^= (spi->rx_crc16[1] << 0);
	
	stm32l4_spi_finish(spi, SPI_EVENT_TRANSFER_DONE);
	break;
    }
}

bool stm32l4_spi_create(stm32l4_spi_t *spi, unsigned int instance, const stm32l4_spi_pins_t *pins, unsigned int priority, unsigned int mode)
{
    spi->state = SPI_STATE_INIT;
    spi->instance = instance;
    spi->pins = *pins;

    switch (instance) {
    case SPI_INSTANCE_SPI1:
	spi->SPI = SPI1;
	spi->interrupt = SPI1_IRQn;
	break;

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    case SPI_INSTANCE_SPI2:
	spi->SPI = SPI2;
	spi->interrupt = SPI2_IRQn;
	break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */

    case SPI_INSTANCE_SPI3:
	spi->SPI = SPI3;
	spi->interrupt = SPI3_IRQn;
	break;

    default:
	spi->state = SPI_STATE_NONE;
	return false;
    }

    spi->priority = priority;
    spi->mode = mode & ~(SPI_MODE_RX_DMA | SPI_MODE_TX_DMA);
    
    spi->tx_default = 0xffff;

    if (mode & SPI_MODE_RX_DMA)
    {
	switch (spi->instance) {
	case SPI_INSTANCE_SPI1:
	    if ((!(mode & SPI_MODE_RX_DMA_SECONDARY) && stm32l4_dma_create(&spi->rx_dma, DMA_CHANNEL_DMA1_CH2_SPI1_RX, spi->priority)) ||
		stm32l4_dma_create(&spi->rx_dma, DMA_CHANNEL_DMA2_CH3_SPI1_RX, spi->priority))
	    {
		spi->mode |= SPI_MODE_RX_DMA;
	    }
	    break;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	case SPI_INSTANCE_SPI2:
	    if (stm32l4_dma_create(&spi->rx_dma, DMA_CHANNEL_DMA1_CH4_SPI2_RX, spi->priority)) { spi->mode |= SPI_MODE_RX_DMA; }
	    break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
	case SPI_INSTANCE_SPI3:
	    if (stm32l4_dma_create(&spi->rx_dma, DMA_CHANNEL_DMA2_CH1_SPI3_RX, spi->priority)) { spi->mode |= SPI_MODE_RX_DMA; }
	    break;
	}
    }
    
    if (mode & SPI_MODE_TX_DMA)
    {
	switch (spi->instance) {
	case SPI_INSTANCE_SPI1:
	    if ((!(mode & SPI_MODE_TX_DMA_SECONDARY) && stm32l4_dma_create(&spi->tx_dma, DMA_CHANNEL_DMA1_CH3_SPI1_TX, spi->priority)) ||
		stm32l4_dma_create(&spi->tx_dma, DMA_CHANNEL_DMA2_CH4_SPI1_TX, spi->priority))
	    {
		spi->mode |= SPI_MODE_TX_DMA;
	    }
	    break;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	case SPI_INSTANCE_SPI2:
	    if (stm32l4_dma_create(&spi->tx_dma, DMA_CHANNEL_DMA1_CH5_SPI2_TX, spi->priority)) { spi->mode |= SPI_MODE_TX_DMA; }
	    break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
	case SPI_INSTANCE_SPI3:
	    if (stm32l4_dma_create(&spi->tx_dma, DMA_CHANNEL_DMA2_CH2_SPI3_TX, spi->priority)) { spi->mode |= SPI_MODE_TX_DMA; }
	    break;
	}
    }

    if ((spi->mode ^ mode) & (SPI_MODE_RX_DMA | SPI_MODE_TX_DMA))
    {
	if (spi->mode & SPI_MODE_RX_DMA)
	{
	    stm32l4_dma_destroy(&spi->rx_dma);
			
	    spi->mode &= ~SPI_MODE_RX_DMA;
	}

	if (spi->mode & SPI_MODE_TX_DMA)
	{
	    stm32l4_dma_destroy(&spi->tx_dma);
			
	    spi->mode &= ~SPI_MODE_TX_DMA;
	}

	spi->state = SPI_STATE_NONE;
	return false;
    }

    stm32l4_spi_driver.instances[instance] = spi;

    return true;
}

bool stm32l4_spi_destroy(stm32l4_spi_t *spi)
{
    if (spi->state != SPI_STATE_INIT)
    {
	return false;
    }

    stm32l4_spi_driver.instances[spi->instance] = NULL;

    if (spi->mode & SPI_MODE_RX_DMA)
    {
	stm32l4_dma_destroy(&spi->rx_dma);
	
	spi->mode &= ~SPI_MODE_RX_DMA;
    }
    
    if (spi->mode & SPI_MODE_TX_DMA)
    {
	stm32l4_dma_destroy(&spi->tx_dma);
	
	spi->mode &= ~SPI_MODE_TX_DMA;
    }

    return true;
}

uint32_t stm32l4_spi_clock(stm32l4_spi_t *spi)
{
    if (spi->instance == SPI_INSTANCE_SPI1)
    {
	return stm32l4_system_pclk2();
    }
    else
    {
	return stm32l4_system_pclk1();
    }
}

bool stm32l4_spi_enable(stm32l4_spi_t *spi, stm32l4_spi_callback_t callback, void *context, uint32_t events)
{
    SPI_TypeDef *SPI = spi->SPI;

    if (spi->state != SPI_STATE_INIT)
    {
	return false;
    }

    spi->callback = NULL;
    spi->context = NULL;
    spi->events = 0;

    NVIC_SetPriority(spi->interrupt, spi->priority);

    spi->state = SPI_STATE_BUSY;

    stm32l4_spi_start(spi);

    SPI->CR1 = 0;
    SPI->CR2 = 0;
    
    spi->cr1 = SPI_CR1_MSTR;
    spi->cr2 = 0;

    if (spi->pins.ss == GPIO_PIN_NONE)
    {
	spi->cr1 |= (SPI_CR1_SSM | SPI_CR1_SSI);
    }
    else
    {
	spi->cr2 |= SPI_CR2_NSSP;
    }
    
    SPI->CRCPR = 0x1021;
    SPI->CR2 = spi->cr2;
    SPI->CR1 = spi->cr1;
    
    stm32l4_gpio_pin_configure(spi->pins.mosi, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_write(spi->pins.mosi, 1);

    if (spi->pins.miso != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(spi->pins.miso, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    }

    stm32l4_gpio_pin_configure(spi->pins.sck, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    
    if (spi->pins.ss != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(spi->pins.ss, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    }

    stm32l4_spi_stop(spi);

    stm32l4_spi_notify(spi, callback, context, events);

    spi->state = SPI_STATE_READY;

    return true;
}

bool stm32l4_spi_disable(stm32l4_spi_t *spi)
{
    if (spi->state != SPI_STATE_READY)
    {
	return false;
    }

    stm32l4_gpio_pin_configure(spi->pins.mosi, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));

    if (spi->pins.miso != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(spi->pins.miso, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    }

    stm32l4_gpio_pin_configure(spi->pins.sck, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    
    if (spi->pins.ss != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(spi->pins.ss, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    }

    spi->state = SPI_STATE_INIT;

    return true;
}

bool stm32l4_spi_notify(stm32l4_spi_t *spi, stm32l4_spi_callback_t callback, void *context, uint32_t events)
{
    if ((spi->state != SPI_STATE_READY) && (spi->state != SPI_STATE_BUSY))
    {
	return false;
    }

    spi->events = 0;
    spi->callback = callback;
    spi->context = context;
    spi->events = events;

    return true;
}

bool stm32l4_spi_select(stm32l4_spi_t *spi, uint32_t option) 
{
    SPI_TypeDef *SPI = spi->SPI;

    if ((spi->state != SPI_STATE_READY) && (spi->state != SPI_STATE_SELECTED))
    {
	return false;
    }

    spi->select++;
    spi->option = option;

    if (spi->select == 1)
    {
	stm32l4_spi_start(spi);

	spi->state = SPI_STATE_SELECTED;
    }

    SPI->CR2 = spi->cr2 | (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH);
    SPI->CR1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST)) | SPI_CR1_SPE;
    
    return true;
}

bool stm32l4_spi_unselect(stm32l4_spi_t *spi)
{
    SPI_TypeDef *SPI = spi->SPI;

    if (spi->state != SPI_STATE_SELECTED)
    {
	return false;
    }

    spi->select--;

    if (spi->select == 0)
    {
	SPI->CR1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST));

	while (SPI->SR & SPI_SR_BSY) { }

	stm32l4_spi_stop(spi);

	spi->state = SPI_STATE_READY;
    }

    return true;
}

bool stm32l4_spi_configure(stm32l4_spi_t *spi, uint32_t option)
{
    SPI_TypeDef *SPI = spi->SPI;

    if (spi->state != SPI_STATE_SELECTED)
    {
	return false;
    }
    
    spi->option = option;

    SPI->CR1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST)) | SPI_CR1_SPE;

    return true;
}

__attribute__((optimize("O3"))) void stm32l4_spi_exchange(stm32l4_spi_t *spi, const uint8_t *tx_data, uint8_t *rx_data, unsigned int count)
{
    SPI_TypeDef *SPI = spi->SPI;
    uint8_t *rx_data_e;
    const uint8_t *tx_data_e;
    uint8_t rx_null;
    const uint8_t tx_default = 0xff;

    if (rx_data)
    {
	if (tx_data)
	{
	    if (count < 3)
	    {
		if (count == 2)
		{
		    stm32l4_spi_wr8(SPI, tx_data);
		    tx_data += 1;
		    
		    stm32l4_spi_wr8(SPI, tx_data);
		    
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		    rx_data += 1;
		    
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		}
		else
		{
		    stm32l4_spi_wr8(SPI, tx_data);
		    
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		}
	    }
	    else
	    {
		tx_data_e = tx_data + count;
	
		stm32l4_spi_wr8(SPI, tx_data);
		tx_data += 1;
		
		stm32l4_spi_wr8(SPI, tx_data);
		tx_data += 1;
		
		stm32l4_spi_wr8(SPI, tx_data);
		tx_data += 1;
		
		while (tx_data != tx_data_e)
		{
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		    rx_data += 1;
		    
		    stm32l4_spi_wr8(SPI, tx_data);
		    tx_data += 1;
		}
		
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, rx_data);
		rx_data += 1;
		
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, rx_data);
		rx_data += 1;
		
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, rx_data);
	    }
	}
	else
	{
	    if (count < 3)
	    {
		if (count == 2)
		{
		    stm32l4_spi_wr8(SPI, &tx_default);
		    stm32l4_spi_wr8(SPI, &tx_default);
		    
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		    rx_data += 1;
		    
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		}
		else
		{
		    stm32l4_spi_wr8(SPI, &tx_default);
		    
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		}
	    }
	    else
	    {
		rx_data_e = rx_data + count - 3;
	
		stm32l4_spi_wr8(SPI, &tx_default);
		stm32l4_spi_wr8(SPI, &tx_default);
		stm32l4_spi_wr8(SPI, &tx_default);
		
		while (rx_data != rx_data_e)
		{
		    while (!(SPI->SR & SPI_SR_RXNE)) { }
		    stm32l4_spi_rd8(SPI, rx_data);
		    rx_data += 1;
		    
		    stm32l4_spi_wr8(SPI, &tx_default);
		}
		
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, rx_data);
		rx_data += 1;
		
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, rx_data);
		rx_data += 1;
		
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, rx_data);
	    }
	}
    }
    else
    {
	if (count < 3)
	{
	    if (count == 2)
	    {
		stm32l4_spi_wr8(SPI, tx_data);
		tx_data += 1;
		    
		stm32l4_spi_wr8(SPI, tx_data);
		    
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, &rx_null);
		    
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, &rx_null);
	    }
	    else
	    {
		stm32l4_spi_wr8(SPI, tx_data);
		    
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, &rx_null);
	    }
	}
	else
	{
	    tx_data_e = tx_data + count;
	
	    stm32l4_spi_wr8(SPI, tx_data);
	    tx_data += 1;
		
	    stm32l4_spi_wr8(SPI, tx_data);
	    tx_data += 1;
		
	    stm32l4_spi_wr8(SPI, tx_data);
	    tx_data += 1;
		
	    while (tx_data != tx_data_e)
	    {
		while (!(SPI->SR & SPI_SR_RXNE)) { }
		stm32l4_spi_rd8(SPI, &rx_null);
		    
		stm32l4_spi_wr8(SPI, tx_data);
		tx_data += 1;
	    }
		
	    while (!(SPI->SR & SPI_SR_RXNE)) { }
	    stm32l4_spi_rd8(SPI, &rx_null);
		
	    while (!(SPI->SR & SPI_SR_RXNE)) { }
	    stm32l4_spi_rd8(SPI, &rx_null);
		
	    while (!(SPI->SR & SPI_SR_RXNE)) { }
	    stm32l4_spi_rd8(SPI, &rx_null);
	}
    }
}

uint8_t stm32l4_spi_exchange8(stm32l4_spi_t *spi, uint8_t data)
{
    SPI_TypeDef *SPI = spi->SPI;

    stm32l4_spi_wr8(SPI, &data);
	
    while (!(SPI->SR & SPI_SR_RXNE)) { }
    stm32l4_spi_rd8(SPI, &data);
	
    return data;
}

uint16_t stm32l4_spi_exchange16(stm32l4_spi_t *spi, uint16_t data)
{
    SPI_TypeDef *SPI = spi->SPI;

    if (spi->cr1 & SPI_CR1_LSBFIRST)
    {
	stm32l4_spi_wr8(SPI, &((uint8_t*)&data)[0]);
	stm32l4_spi_wr8(SPI, &((uint8_t*)&data)[1]);
	
	while (!(SPI->SR & SPI_SR_RXNE)) { }
	stm32l4_spi_rd8(SPI, &((uint8_t*)&data)[0]);
	
	while (!(SPI->SR & SPI_SR_RXNE)) { }
	stm32l4_spi_rd8(SPI, &((uint8_t*)&data)[1]);
    }
    else
    {
	stm32l4_spi_wr8(SPI, &((uint8_t*)&data)[1]);
	stm32l4_spi_wr8(SPI, &((uint8_t*)&data)[0]);
	
	while (!(SPI->SR & SPI_SR_RXNE)) { }
	stm32l4_spi_rd8(SPI, &((uint8_t*)&data)[1]);
	
	while (!(SPI->SR & SPI_SR_RXNE)) { }
	stm32l4_spi_rd8(SPI, &((uint8_t*)&data)[0]);
    }

    return data;
}

bool stm32l4_spi_receive(stm32l4_spi_t *spi, uint8_t *rx_data, unsigned int rx_count, uint32_t control)
{
    SPI_TypeDef *SPI = spi->SPI;
    uint32_t spi_cr1, spi_cr2;
    uint16_t tx_default = 0xffff;
    unsigned int count;

    if (spi->state != SPI_STATE_SELECTED)
    {
	return false;
    }

    spi_cr1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST));
    spi_cr2 = spi->cr2;
    
    if (control & SPI_CONTROL_HALFDUPLEX)
    {
	spi->xf_count = 0;
	spi->xf_size = rx_count;
	spi->tx_data = NULL;
	spi->rx_data = rx_data;
	
	spi_cr1 |= SPI_CR1_BIDIMODE;

	SPI->CR1 = spi_cr1;

	if ((rx_count > 1) && (spi->mode & SPI_MODE_RX_DMA))
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXDMAEN);

	    SPI->CR2 = spi_cr2;

	    count = spi->xf_size - spi->xf_count - 1;
		
	    stm32l4_dma_start(&spi->rx_dma, (uint32_t)spi->rx_data, (uint32_t)&SPI->DR, count, SPI_RX_DMA_OPTION_RECEIVE_8);
		
	    spi->rx_data += count;
	    spi->xf_count += count;
		
	    spi->state = SPI_STATE_HALFDUPLEX_8_DMA;
	    
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	}
	else
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXNEIE);

	    SPI->CR2 = spi_cr2;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    if (spi->xf_size >= 2)
	    {
		spi->rx_data_e = spi->rx_data + spi->xf_size - 1;
		
		spi->state = SPI_STATE_HALFDUPLEX_8_M;
	    }
	    else
	    {
		spi->state = SPI_STATE_HALFDUPLEX_8_1;
		
		SPI->CR1 = spi_cr1 & ~SPI_CR1_SPE;
	    }
	    
	    NVIC_EnableIRQ(spi->interrupt);
	}

	return true;
    }
    else
    {
	spi->xf_count = 0;
	spi->xf_size = rx_count;
	spi->tx_data = NULL;
	spi->rx_data = rx_data;
	
	SPI->CR1 = spi_cr1;

	if (control & SPI_CONTROL_CRC16)
	{
	    spi_cr1 |= (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	    
	    SPI->CR1 = spi_cr1;
		
	    SPI->SR = 0;
	}

	if ((spi->mode & (SPI_MODE_RX_DMA | SPI_MODE_TX_DMA)) == (SPI_MODE_RX_DMA | SPI_MODE_TX_DMA))
	{
	    if (control & SPI_CONTROL_CRC16)
	    {
		/* With CRC16 generation, the last 2 bytes transfered are the CRC16. This is not
		 * what is needed, which would be 0xff value. Hence simply drive the MOSI line
		 * high ...
		 */
		stm32l4_gpio_pin_output(spi->pins.mosi);
	    }

	    if ((spi->xf_size & 1) || ((uint32_t)spi->rx_data & 1))
	    {
		spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
		
		SPI->CR2 = spi_cr2;
		
		count = spi->xf_size - spi->xf_count;
		
		stm32l4_dma_start(&spi->rx_dma, (uint32_t)spi->rx_data, (uint32_t)&SPI->DR, count, SPI_RX_DMA_OPTION_RECEIVE_8);
		stm32l4_dma_start(&spi->tx_dma, (uint32_t)&SPI->DR, (uint32_t)&spi->tx_default, count, SPI_TX_DMA_OPTION_RECEIVE_8);
		
		spi->state = SPI_STATE_RECEIVE_8_DMA;
	    }
	    else
	    {
		spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);

		SPI->CR2 = spi_cr2;

		count = spi->xf_size - spi->xf_count;
	    	    
		stm32l4_dma_start(&spi->rx_dma, (uint32_t)spi->rx_data, (uint32_t)&SPI->DR, count / 2, SPI_RX_DMA_OPTION_RECEIVE_16);
		stm32l4_dma_start(&spi->tx_dma, (uint32_t)&SPI->DR, (uint32_t)&spi->tx_default, count / 2, SPI_TX_DMA_OPTION_RECEIVE_16);

		spi->state = SPI_STATE_RECEIVE_16_DMA;
	    }

	    spi->rx_data += count;
	    spi->xf_count += count;
	
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	}
	else
	{
	    /* The idea here is to make use of the 32 bit FIFO, using 16 bit transfers. Thus if there is 
	     * less than 4 bytes to transfer, stuff them into the FIFO as 8 bit quantities and collect
	     * the read back data in the IRQ handler. Next, if the count is odd, send only the first
	     * byte, and start 16 bit transfers in the IRQ handler. Lastly if the count was even,
	     * stuff the first 4 bytes as 16 bit quantities into the FIFO and let the IRQ handler
	     * collect the read back data, along with sending new 16 bit quantities.
	     */
	    if (spi->xf_size < 4)
	    {
		spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXNEIE);

		SPI->CR2 = spi_cr2;
		SPI->CR1 = spi_cr1 | SPI_CR1_SPE;

		if (spi->xf_size == 3) 
		{
		    spi->state = SPI_STATE_RECEIVE_8_3;
		    
		    stm32l4_spi_wr8(SPI, &tx_default);
		    stm32l4_spi_wr8(SPI, &tx_default);
		    stm32l4_spi_wr8(SPI, &tx_default);
		    
		    spi->xf_count += 3;
		}
		else if (spi->xf_size == 2) 
		{
		    spi->state = SPI_STATE_RECEIVE_8_2;
		    
		    stm32l4_spi_wr8(SPI, &tx_default);
		    stm32l4_spi_wr8(SPI, &tx_default);
		    
		    spi->xf_count += 2;
		}
		else
		{
		    spi->state = SPI_STATE_RECEIVE_8_1;
		    
		    stm32l4_spi_wr8(SPI, &tx_default);
		    
		    spi->xf_count += 1;
		}
	    }
	    else
	    {
		spi->rx_data_e = spi->rx_data + spi->xf_size - 4;
		
		if (spi->xf_size & 1)
		{
		    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXNEIE);
		    
		    SPI->CR2 = spi_cr2;
		    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
		    
		    spi->state = SPI_STATE_RECEIVE_16_S;
		    
		    stm32l4_spi_wr8(SPI, &tx_default);
		}
		else
		{
		    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXNEIE);
		    
		    SPI->CR2 = spi_cr2;
		    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
		    
		    if (spi->xf_size == 4)
		    {
			spi->state = SPI_STATE_RECEIVE_16_2;
		    }
		    else
		    {
			spi->state = SPI_STATE_RECEIVE_16_M;
		    }
		    
		    stm32l4_spi_wr16(SPI, &tx_default);
		    stm32l4_spi_wr16(SPI, &tx_default);
		}
	    }
	    
	    NVIC_EnableIRQ(spi->interrupt);
	}
    }

    return true;
}

bool stm32l4_spi_transmit(stm32l4_spi_t *spi, const uint8_t *tx_data, unsigned int tx_count, uint32_t control)
{
    SPI_TypeDef *SPI = spi->SPI;
    uint32_t spi_cr1, spi_cr2;
    unsigned int count;

    if (spi->state != SPI_STATE_SELECTED)
    {
	return false;
    }
    
    spi_cr1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST));
    spi_cr2 = spi->cr2;

    spi->xf_count = 0;
    spi->xf_size = tx_count;
    spi->tx_data = tx_data;
    spi->rx_data = NULL;
    
    SPI->CR1 = spi_cr1;

    if (control & SPI_CONTROL_CRC16)
    {
	spi_cr1 |= (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	
	SPI->CR1 = spi_cr1;
	
	SPI->SR = 0;
    }
    
    if ((spi->mode & (SPI_MODE_RX_DMA | SPI_MODE_TX_DMA)) == (SPI_MODE_RX_DMA | SPI_MODE_TX_DMA))
    {
	if ((spi->xf_size & 1) || ((uint32_t)spi->tx_data & 1))
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);

	    SPI->CR2 = spi_cr2;
	    
	    count = spi->xf_size - spi->xf_count;
	    
	    stm32l4_dma_start(&spi->rx_dma, (uint32_t)&spi->rx_null, (uint32_t)&SPI->DR, count, SPI_RX_DMA_OPTION_TRANSMIT_8);
	    stm32l4_dma_start(&spi->tx_dma, (uint32_t)&SPI->DR, (uint32_t)spi->tx_data, count, SPI_TX_DMA_OPTION_TRANSMIT_8);
	    
	    spi->state = SPI_STATE_TRANSMIT_8_DMA;
	}
	else
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	    
	    SPI->CR2 = spi_cr2;
	    
	    count = spi->xf_size - spi->xf_count;
	    
	    stm32l4_dma_start(&spi->rx_dma, (uint32_t)&spi->rx_null, (uint32_t)&SPI->DR, count / 2, SPI_RX_DMA_OPTION_TRANSMIT_16);
	    stm32l4_dma_start(&spi->tx_dma, (uint32_t)&SPI->DR, (uint32_t)spi->tx_data, count / 2, SPI_TX_DMA_OPTION_TRANSMIT_16);
	    
	    spi->state = SPI_STATE_TRANSMIT_16_DMA;
	}
	
	spi->tx_data += count;
	spi->xf_count += count;
	
	SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
    }
    else
    {
	/* The idea here is to make use of the 32 bit FIFO, using 16 bit transfers. Thus if there is 
	 * less than 4 bytes to transfer, stuff them into the FIFO as 8 bit quantities and collect
	 * the read back data in the IRQ handler. Next, if the count is odd, send only the first
	 * byte, and start 16 bit transfers in the IRQ handler. Lastly if the count was even,
	 * stuff the first 4 bytes as 16 bit quantities into the FIFO and let the IRQ handler
	 * collect the read back data, along with sending new 16 bit quantities.
	 */
	if (spi->xf_size < 4)
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXNEIE);
	    
	    SPI->CR2 = spi_cr2;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    if (spi->xf_size == 3) 
	    {
		spi->state = SPI_STATE_TRANSMIT_8_3;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		spi->xf_count += 3;
	    }
	    else if (spi->xf_size == 2) 
	    {
		spi->state = SPI_STATE_TRANSMIT_8_2;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		spi->xf_count += 2;
	    }
	    else
	    {
		spi->state = SPI_STATE_TRANSMIT_8_1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		spi->xf_count += 1;
	    }
	}
	else
	{
	    spi->tx_data_e = spi->tx_data + spi->xf_size;
	    
	    if (spi->xf_size & 1)
	    {
		spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXNEIE);
		
		SPI->CR2 = spi_cr2;
		SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
		
		spi->state = SPI_STATE_TRANSMIT_16_S;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data++;
	    }
	    else
	    {
		spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXNEIE);
		
		SPI->CR2 = spi_cr2;
		SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
		
		if (spi->xf_size == 4)
		{
		    spi->state = SPI_STATE_TRANSMIT_16_2;
		}
		else
		{
		    spi->state = SPI_STATE_TRANSMIT_16_M;
		}
		
		stm32l4_spi_wr16(SPI, spi->tx_data);
		spi->tx_data += 2;
		
		stm32l4_spi_wr16(SPI, spi->tx_data);
		spi->tx_data += 2;
	    }
	}
	
	NVIC_EnableIRQ(spi->interrupt);
    }

    return true;
}

bool stm32l4_spi_transfer(stm32l4_spi_t *spi, const uint8_t *tx_data, uint8_t *rx_data, unsigned int xf_count, uint32_t control)
{
    SPI_TypeDef *SPI = spi->SPI;
    uint32_t spi_cr1, spi_cr2;
    unsigned int count;

    if (spi->state != SPI_STATE_SELECTED)
    {
	return false;
    }

    spi_cr1 = spi->cr1 | (spi->option & (SPI_OPTION_MODE_MASK | SPI_OPTION_DIV_MASK | SPI_OPTION_LSB_FIRST));
    spi_cr2 = spi->cr2;

    spi->xf_count = 0;
    spi->xf_size = xf_count;
    spi->tx_data = tx_data;
    spi->rx_data = rx_data;
    
    SPI->CR1 = spi_cr1;

    if (control & SPI_CONTROL_CRC16)
    {
	spi_cr1 |= (SPI_CR1_CRCEN | SPI_CR1_CRCL);
	
	SPI->CR1 = spi_cr1;
	
	SPI->SR = 0;
    }
    
    if ((spi->mode & (SPI_MODE_RX_DMA | SPI_MODE_TX_DMA)) == (SPI_MODE_RX_DMA | SPI_MODE_TX_DMA))
    {
	if ((spi->xf_size & 1) || ((uint32_t)spi->tx_data & 1) || ((uint32_t)spi->rx_data & 1))
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	    
	    SPI->CR2 = spi_cr2;
	    
	    count = spi->xf_size - spi->xf_count;
	    
	    stm32l4_dma_start(&spi->rx_dma, (uint32_t)spi->rx_data, (uint32_t)&SPI->DR, count, SPI_RX_DMA_OPTION_TRANSFER_8);
	    stm32l4_dma_start(&spi->tx_dma, (uint32_t)&SPI->DR, (uint32_t)spi->tx_data, count, SPI_TX_DMA_OPTION_TRANSFER_8);
	    
	    spi->state = SPI_STATE_TRANSFER_8_DMA;
	}
	else
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	    
	    SPI->CR2 = spi_cr2;
	    
	    count = spi->xf_size - spi->xf_count;
	    
	    stm32l4_dma_start(&spi->rx_dma, (uint32_t)spi->rx_data, (uint32_t)&SPI->DR, count / 2, SPI_RX_DMA_OPTION_TRANSFER_16);
	    stm32l4_dma_start(&spi->tx_dma, (uint32_t)&SPI->DR, (uint32_t)spi->tx_data, count / 2, SPI_TX_DMA_OPTION_TRANSFER_16);
	    
	    spi->state = SPI_STATE_TRANSFER_16_DMA;
	}
	
	spi->tx_data += count;
	spi->rx_data += count;
	spi->xf_count += count;
	
	SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
    }
    else
    {
	/* The idea here is to make use of the 32 bit FIFO, using 16 bit transfers. Thus if there is 
	 * less than 4 bytes to transfer, stuff them into the FIFO as 8 bit quantities and collect
	 * the read back data in the IRQ handler. Next, if the count is odd, send only the first
	 * byte, and start 16 bit transfers in the IRQ handler. Lastly if the count was even,
	 * stuff the first 4 bytes as 16 bit quantities into the FIFO and let the IRQ handler
	 * collect the read back data, along with sending new 16 bit quantities.
	 */
	if (spi->xf_size < 4)
	{
	    spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXNEIE);
	    
	    SPI->CR2 = spi_cr2;
	    SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
	    
	    if (spi->xf_size == 3) 
	    {
		spi->state = SPI_STATE_TRANSFER_8_3;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		spi->xf_count += 3;
	    }
	    else if (spi->xf_size == 2) 
	    {
		spi->state = SPI_STATE_TRANSFER_8_2;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		spi->xf_count += 2;
	    }
	    else
	    {
		spi->state = SPI_STATE_TRANSFER_8_1;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data += 1;
		
		spi->xf_count += 1;
	    }
	}
	else
	{
	    spi->tx_data_e = spi->tx_data + spi->xf_size;
	    
	    if (spi->xf_size & 1)
	    {
		spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_RXNEIE);
		
		SPI->CR2 = spi_cr2;
		SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
		
		spi->state = SPI_STATE_TRANSFER_16_S;
		
		stm32l4_spi_wr8(SPI, spi->tx_data);
		spi->tx_data++;
	    }
	    else
	    {
		spi_cr2 |= (SPI_CR2_DS_8BIT | SPI_CR2_RXNEIE);
		
		SPI->CR2 = spi_cr2;
		SPI->CR1 = spi_cr1 | SPI_CR1_SPE;
		
		if (spi->xf_size == 4)
		{
		    spi->state = SPI_STATE_TRANSFER_16_2;
		}
		else
		{
		    spi->state = SPI_STATE_TRANSFER_16_M;
		}
		
		stm32l4_spi_wr16(SPI, spi->tx_data);
		spi->tx_data += 2;
		
		stm32l4_spi_wr16(SPI, spi->tx_data);
		spi->tx_data += 2;
	    }
	}
	
	NVIC_EnableIRQ(spi->interrupt);
    }

    return true;
}

bool stm32l4_spi_done(stm32l4_spi_t *spi)
{
    return (spi->state <= SPI_STATE_SELECTED);
}

uint16_t stm32l4_spi_crc16(stm32l4_spi_t *spi)
{
    return spi->crc16;
}

void stm32l4_spi_poll(stm32l4_spi_t *spi)
{
    if (spi->state >= SPI_STATE_READY)
    {
	if (spi->mode & SPI_MODE_TX_DMA)
	{
	    stm32l4_dma_poll(&spi->tx_dma);
	}

	if (spi->mode & SPI_MODE_RX_DMA)
	{
	    stm32l4_dma_poll(&spi->rx_dma);
	}

	stm32l4_spi_interrupt(spi);
    }
}

void SPI1_IRQHandler(void)
{
    stm32l4_spi_interrupt(stm32l4_spi_driver.instances[SPI_INSTANCE_SPI1]);
}

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)

void SPI2_IRQHandler(void)
{
    stm32l4_spi_interrupt(stm32l4_spi_driver.instances[SPI_INSTANCE_SPI2]);
}

#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */

void SPI3_IRQHandler(void)
{
    stm32l4_spi_interrupt(stm32l4_spi_driver.instances[SPI_INSTANCE_SPI3]);
}
