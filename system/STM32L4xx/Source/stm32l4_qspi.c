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

#include <stdio.h>

#include "stm32l4xx.h"

#include "armv7m.h"

#include "stm32l4_gpio.h"
#include "stm32l4_qspi.h"
#include "stm32l4_dma.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_qspi_driver_t {
    stm32l4_qspi_t     *instances[QSPI_INSTANCE_COUNT];
} stm32l4_qspi_driver_t;

static stm32l4_qspi_driver_t stm32l4_qspi_driver;

#define QSPI_RX_DMA_OPTION_RECEIVE_8	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define QSPI_RX_DMA_OPTION_RECEIVE_16	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define QSPI_RX_DMA_OPTION_RECEIVE_32	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_32 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define QSPI_TX_DMA_OPTION_TRANSMIT_8	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_8 |  \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define QSPI_TX_DMA_OPTION_TRANSMIT_16	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define QSPI_TX_DMA_OPTION_TRANSMIT_32	  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_32 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)



#define QUADSPI_CCR_FMODE_INDIRECT_WRITE    0
#define QUADSPI_CCR_FMODE_INDIRECT_READ     QUADSPI_CCR_FMODE_0
#define QUADSPI_CCR_FMODE_AUTOMATIC_POLLING QUADSPI_CCR_FMODE_1
#define QUADSPI_CCR_FMODE_MEMORY_MAPPED     (QUADSPI_CCR_FMODE_0 | QUADSPI_CCR_FMODE_1)

#define QUADSPI_CR_FTHRES_1                 0x00000000
#define QUADSPI_CR_FTHRES_2                 0x00000100
#define QUADSPI_CR_FTHRES_3                 0x00000200
#define QUADSPI_CR_FTHRES_4                 0x00000300
#define QUADSPI_CR_FTHRES_8                 0x00000700
#define QUADSPI_CR_FTHRES_16                0x00000F00

static void stm32l4_qspi_dma_callback(stm32l4_qspi_t *qspi, uint32_t events);

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_qspi_rd8(void *rx_data)
{
    *((uint8_t*)rx_data) = *((volatile uint8_t*)(&QUADSPI->DR));
}

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_qspi_rd32(void *rx_data)
{
    *((uint32_t*)rx_data) = QUADSPI->DR;
}

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_qspi_wr8(const void *tx_data)
{
    *((volatile uint8_t*)(&QUADSPI->DR)) = *((const uint8_t*)tx_data);
}

static inline __attribute__((optimize("O3"),always_inline)) void stm32l4_qspi_wr32(const void *tx_data)
{
    QUADSPI->DR = *((const uint32_t*)tx_data);
}

static void stm32l4_qspi_start(stm32l4_qspi_t *qspi)
{
    stm32l4_system_periph_enable(SYSTEM_PERIPH_QSPI);

    if (qspi->state != QSPI_STATE_BUSY)
    {
	if (qspi->mode & QSPI_MODE_DMA)
	{
	    stm32l4_dma_enable(&qspi->dma, (stm32l4_dma_callback_t)stm32l4_qspi_dma_callback, qspi);
	}
    }
}

static void stm32l4_qspi_stop(stm32l4_qspi_t *qspi)
{
    if (qspi->state != QSPI_STATE_BUSY)
    {
	if (qspi->mode & QSPI_MODE_DMA)
	{
	    stm32l4_dma_disable(&qspi->dma);
	}
    }

    stm32l4_system_periph_disable(SYSTEM_PERIPH_QSPI);
}

static void stm32l4_qspi_dma_callback(stm32l4_qspi_t *qspi, uint32_t events)
{
    QUADSPI->FCR = QUADSPI_FCR_CTCF;

    QUADSPI->CR |= QUADSPI_CR_ABORT;

    qspi->state = QSPI_STATE_SELECTED;

    if (qspi->events & QSPI_EVENT_RECEIVE_DONE)
    {
	(*qspi->callback)(qspi->context, QSPI_EVENT_RECEIVE_DONE);
    }
}

static void stm32l4_qspi_interrupt(stm32l4_qspi_t *qspi)
{
    uint8_t *rx_data, *rx_data_e;

      
    switch (qspi->state) {
    case QSPI_STATE_NONE:
    case QSPI_STATE_BUSY:
    case QSPI_STATE_READY:
    case QSPI_STATE_SELECTED:
    case QSPI_STATE_RECEIVE:
	break;

    case QSPI_STATE_TRANSMIT:
	if (QUADSPI->SR & QUADSPI_SR_TCF)
	{
	    NVIC_DisableIRQ(qspi->interrupt);

	    QUADSPI->FCR = QUADSPI_FCR_CTCF;
	    QUADSPI->CR = (QUADSPI->CR & ~QUADSPI_CR_TCIE);

	    qspi->state = QSPI_STATE_SELECTED;

	    if (qspi->events & QSPI_EVENT_TRANSMIT_DONE)
	    {
		(*qspi->callback)(qspi->context, QSPI_EVENT_TRANSMIT_DONE);
	    }
	}
	break;

    case QSPI_STATE_WAIT:
	if (QUADSPI->SR & QUADSPI_SR_SMF)
	{
	    NVIC_DisableIRQ(qspi->interrupt);

	    QUADSPI->FCR = QUADSPI_FCR_CSMF | QUADSPI_FCR_CTCF;
	    QUADSPI->CR = (QUADSPI->CR & ~QUADSPI_CR_SMIE);

	    rx_data = qspi->rx_data;
	    rx_data_e = rx_data + qspi->rx_count;
	    
	    while (rx_data != rx_data_e)
	    {
		stm32l4_qspi_rd8(rx_data);
		rx_data++;
	    }

	    qspi->state = QSPI_STATE_SELECTED;

	    if (qspi->events & QSPI_EVENT_WAIT_DONE)
	    {
		(*qspi->callback)(qspi->context, QSPI_EVENT_WAIT_DONE);
	    }
	}
	break;
    }
}

bool stm32l4_qspi_create(stm32l4_qspi_t *qspi, unsigned int instance, const stm32l4_qspi_pins_t *pins, unsigned int priority, unsigned int mode)
{
    qspi->state = QSPI_STATE_NONE;
    qspi->instance = instance;
    qspi->pins = *pins;

    if (instance == QSPI_INSTANCE_QUADSPI)
    {
	qspi->interrupt = QUADSPI_IRQn;
    }
    else
    {
	return false;
    }

    qspi->priority = priority;
    qspi->mode = mode & ~QSPI_MODE_DMA;
    
    if (mode & QSPI_MODE_DMA)
    {
	if ((!(mode & QSPI_MODE_DMA_SECONDARY) && stm32l4_dma_create(&qspi->dma, DMA_CHANNEL_DMA1_CH5_QUADSPI, qspi->priority)) ||
	    stm32l4_dma_create(&qspi->dma, DMA_CHANNEL_DMA2_CH7_QUADSPI, qspi->priority))
	{
	    qspi->mode |= QSPI_MODE_DMA;
	}
	else
	{
	    return false;
	}
    }

    stm32l4_qspi_driver.instances[instance] = qspi;

    return true;
}

bool stm32l4_qspi_destroy(stm32l4_qspi_t *qspi)
{
    if ((qspi->state != QSPI_STATE_NONE) && (qspi->state != QSPI_STATE_BUSY))
    {
	return false;
    }

    stm32l4_qspi_driver.instances[qspi->instance] = NULL;

    if (qspi->mode & QSPI_MODE_DMA)
    {
	stm32l4_dma_destroy(&qspi->dma);
	
	qspi->mode &= ~QSPI_MODE_DMA;
    }

    return true;
}

bool stm32l4_qspi_enable(stm32l4_qspi_t *qspi, uint32_t clock, uint32_t option, stm32l4_qspi_callback_t callback, void *context, uint32_t events)
{
    if (qspi->state != QSPI_STATE_NONE)
    {
	return false;
    }

    qspi->callback = NULL;
    qspi->context = NULL;
    qspi->events = 0;

    NVIC_SetPriority(qspi->interrupt, qspi->priority);

    qspi->state = QSPI_STATE_BUSY;

    if (!stm32l4_qspi_configure(qspi, clock, option))
    {
	qspi->state = QSPI_STATE_NONE;

	return false;
    }

    stm32l4_qspi_notify(qspi, callback, context, events);

    qspi->state = QSPI_STATE_READY;

    return true;
}

bool stm32l4_qspi_disable(stm32l4_qspi_t *qspi)
{
    if (qspi->state != QSPI_STATE_READY)
    {
	return false;
    }

    stm32l4_gpio_pin_configure(qspi->pins.clk, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(qspi->pins.ncs, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(qspi->pins.io0, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(qspi->pins.io1, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(qspi->pins.io2, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(qspi->pins.io3, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));

    qspi->state = QSPI_STATE_NONE;

    return true;
}

bool stm32l4_qspi_configure(stm32l4_qspi_t *qspi, uint32_t clock, uint32_t option)
{
    uint32_t hclk;

    if ((qspi->state != QSPI_STATE_READY) && (qspi->state != QSPI_STATE_BUSY))
    {
	return false;
    }

    if (clock > 48000000)
    {
	return false;
    }

    hclk = stm32l4_system_hclk();

    if (clock > hclk)
    {
	clock = hclk;
    }

    qspi->clock = clock;
    qspi->option = option;

    stm32l4_qspi_start(qspi);

    QUADSPI->CR &= ~QUADSPI_CR_EN;
    
    QUADSPI->CR = (((hclk / clock) -1) << 24) | QUADSPI_CR_APMS | QUADSPI_CR_FTHRES_1;
    // QUADSPI->CR = QUADSPI_CR_APMS | QUADSPI_CR_FTHRES_1;

    QUADSPI->DCR = QUADSPI_DCR_FSIZE | ((qspi->option & QSPI_OPTION_MODE_MASK) >> QSPI_OPTION_MODE_SHIFT);
    
    stm32l4_gpio_pin_configure(qspi->pins.clk, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(qspi->pins.ncs, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(qspi->pins.io0, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(qspi->pins.io1, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(qspi->pins.io2, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(qspi->pins.io3, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
	
    stm32l4_qspi_stop(qspi);

    return true;
}

bool stm32l4_qspi_notify(stm32l4_qspi_t *qspi, stm32l4_qspi_callback_t callback, void *context, uint32_t events)
{
    if ((qspi->state != QSPI_STATE_READY) && (qspi->state != QSPI_STATE_BUSY))
    {
	return false;
    }

    qspi->callback = callback;
    qspi->context = context;
    qspi->events = callback ? events : 0x00000000;

    return true;
}

bool stm32l4_qspi_select(stm32l4_qspi_t *qspi) 
{
    if ((qspi->state != QSPI_STATE_READY) && (qspi->state != QSPI_STATE_SELECTED))
    {
	return false;
    }

    qspi->select++;

    if (qspi->select == 1)
    {
	stm32l4_qspi_start(qspi);

	QUADSPI->CR |= QUADSPI_CR_EN;

	qspi->state = QSPI_STATE_SELECTED;
    }

    return true;
}

bool stm32l4_qspi_unselect(stm32l4_qspi_t *qspi)
{
    if (qspi->state != QSPI_STATE_SELECTED)
    {
	return false;
    }

    qspi->select--;

    if (qspi->select == 0)
    {
	QUADSPI->CR &= ~QUADSPI_CR_EN;

	stm32l4_qspi_stop(qspi);

	qspi->state = QSPI_STATE_READY;
    }

    return true;
}

bool stm32l4_qspi_mode(stm32l4_qspi_t *qspi, uint32_t mode)
{
    if (qspi->state != QSPI_STATE_SELECTED)
    {
	return false;
    }

    while (QUADSPI->SR & QUADSPI_SR_BUSY) { }

    QUADSPI->ABR = mode;

    return true;
}

bool stm32l4_qspi_command(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address)
{
    if (qspi->state != QSPI_STATE_SELECTED)
    {
	return false;
    }
    
    while (QUADSPI->SR & QUADSPI_SR_BUSY) { }

    QUADSPI->CCR = command | QUADSPI_CCR_FMODE_INDIRECT_READ;

    if ((command & QSPI_COMMAND_ADDRESS_MASK) != QSPI_COMMAND_ADDRESS_NONE)
    {
	QUADSPI->AR = address;
    }

    return true;
}

bool stm32l4_qspi_receive(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address, uint8_t *rx_data, unsigned int rx_count, uint32_t control)
{
    uint32_t quadspi_cr, quadspi_sr, dma_count, dma_option;
    uint8_t *rx_data_e;

    if (qspi->state != QSPI_STATE_SELECTED)
    {
	return false;
    }

    if (!(control & QSPI_CONTROL_ASYNC))
    {
	while (QUADSPI->SR & QUADSPI_SR_BUSY) { }

	if (!(rx_count & 3))
	{
	    QUADSPI->CR = (QUADSPI->CR & ~QUADSPI_CR_FTHRES) | QUADSPI_CR_FTHRES_4;
	}
	else
	{
	    QUADSPI->CR = (QUADSPI->CR & ~QUADSPI_CR_FTHRES) | QUADSPI_CR_FTHRES_1;
	}

	QUADSPI->DLR = rx_count -1;
	QUADSPI->CCR = command | QUADSPI_CCR_FMODE_INDIRECT_READ;

	if ((command & QSPI_COMMAND_ADDRESS_MASK) != QSPI_COMMAND_ADDRESS_NONE)
	{
	    QUADSPI->AR = address;
	}

	rx_data_e = rx_data + rx_count;

	if (!(rx_count & 3))
	{
	    while (rx_data != rx_data_e)
	    {
		quadspi_sr = QUADSPI->SR;

		if (quadspi_sr & QUADSPI_SR_TCF)
		{
		    while (rx_data != rx_data_e)
		    {
			stm32l4_qspi_rd32(rx_data);
			rx_data += 4;
		    }

		    break;
		}

		if (quadspi_sr & QUADSPI_SR_FTF)
		{
		    stm32l4_qspi_rd32(rx_data);
		    rx_data += 4;
		}
	    }
	}
	else
	{
	    while (rx_data != rx_data_e)
	    {
		quadspi_sr = QUADSPI->SR;

		if (quadspi_sr & QUADSPI_SR_TCF)
		{
		    while (rx_data != rx_data_e)
		    {
			stm32l4_qspi_rd8(rx_data);
			rx_data += 1;
		    }

		    break;
		}

		if (quadspi_sr & QUADSPI_SR_FTF)
		{
		    stm32l4_qspi_rd8(rx_data);
		    rx_data += 1;
		}
	    }
	}

	QUADSPI->FCR = QUADSPI_FCR_CTCF;

	QUADSPI->CR |= QUADSPI_CR_ABORT;
    }
    else
    {
	if (!(qspi->mode & QSPI_MODE_DMA))
	{
	    return false;
	}

	while (QUADSPI->SR & QUADSPI_SR_BUSY) { }

	quadspi_cr = QUADSPI->CR & ~QUADSPI_CR_FTHRES;

	if (!(((uint32_t)rx_data | rx_count) & 3))
	{
	    quadspi_cr |= QUADSPI_CR_FTHRES_4;

	    dma_option = QSPI_RX_DMA_OPTION_RECEIVE_32;
	    dma_count = rx_count / 4;
	}
	else if (!(((uint32_t)rx_data | rx_count) & 1))
	{
	    quadspi_cr |= QUADSPI_CR_FTHRES_2;

	    dma_option = QSPI_RX_DMA_OPTION_RECEIVE_16;
	    dma_count = rx_count / 2;
	}
	else
	{
	    quadspi_cr |= QUADSPI_CR_FTHRES_1;

	    dma_option = QSPI_RX_DMA_OPTION_RECEIVE_8;
	    dma_count = rx_count;
	}

	qspi->state = QSPI_STATE_RECEIVE;

	QUADSPI->CR = quadspi_cr | QUADSPI_CR_DMAEN;

	QUADSPI->DLR = rx_count -1;
	QUADSPI->CCR = command | QUADSPI_CCR_FMODE_INDIRECT_READ;
	
	if ((command & QSPI_COMMAND_ADDRESS_MASK) != QSPI_COMMAND_ADDRESS_NONE)
	{
	    QUADSPI->AR = address;
	}
	
	stm32l4_dma_start(&qspi->dma, (uint32_t)rx_data, (uint32_t)&QUADSPI->DR, dma_count, dma_option);
    }

    return true;
}

bool stm32l4_qspi_transmit(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address, const uint8_t *tx_data, unsigned int tx_count, uint32_t control)
{
    const uint8_t *tx_data_e;
    uint32_t quadspi_cr, dma_count, dma_option;

    if (qspi->state != QSPI_STATE_SELECTED)
    {
	return false;
    }
    
    if (!(control & QSPI_CONTROL_ASYNC))
    {
	while (QUADSPI->SR & QUADSPI_SR_BUSY) { }

	if (!(tx_count & 3))
	{
	    QUADSPI->CR = (QUADSPI->CR & ~QUADSPI_CR_FTHRES) | QUADSPI_CR_FTHRES_4;
	}
	else
	{
	    QUADSPI->CR = (QUADSPI->CR & ~QUADSPI_CR_FTHRES) | QUADSPI_CR_FTHRES_1;
	}

	QUADSPI->DLR = tx_count -1;
	QUADSPI->CCR = command | QUADSPI_CCR_FMODE_INDIRECT_WRITE;

	if ((command & QSPI_COMMAND_ADDRESS_MASK) != QSPI_COMMAND_ADDRESS_NONE)
	{
	    QUADSPI->AR = address;
	}

	tx_data_e = tx_data + tx_count;

	if (!(tx_count & 3))
	{
	    while (tx_data != tx_data_e)
	    {
		while (!(QUADSPI->SR & QUADSPI_SR_FTF)) { };
		
		stm32l4_qspi_wr32(tx_data);
		tx_data += 4;
	    }
	}
	else
	{
	    while (tx_data != tx_data_e)
	    {
		while (!(QUADSPI->SR & QUADSPI_SR_FTF)) { };
		
		stm32l4_qspi_wr8(tx_data);
		tx_data += 1;
	    }
	}

	while (!(QUADSPI->SR & QUADSPI_SR_TCF)) { };
	
	QUADSPI->FCR = QUADSPI_FCR_CTCF;
    }
    else
    {
	if (!(qspi->mode & QSPI_MODE_DMA))
	{
	    return false;
	}

	while (QUADSPI->SR & QUADSPI_SR_BUSY) { }

	quadspi_cr = QUADSPI->CR & ~QUADSPI_CR_FTHRES;
	
	if (!(((uint32_t)tx_data | tx_count) & 3))
	{
	    quadspi_cr |= QUADSPI_CR_FTHRES_4;

	    dma_option = QSPI_TX_DMA_OPTION_TRANSMIT_32;
	    dma_count = tx_count / 4;
	}
	else if (!(((uint32_t)tx_data | tx_count) & 1))
	{
	    quadspi_cr |= QUADSPI_CR_FTHRES_2;

	    dma_option = QSPI_TX_DMA_OPTION_TRANSMIT_16;
	    dma_count = tx_count / 2;
	}
	else
	{
	    quadspi_cr |= QUADSPI_CR_FTHRES_1;
	    
	    dma_option = QSPI_TX_DMA_OPTION_TRANSMIT_8;
	    dma_count = tx_count;
	}

	qspi->state = QSPI_STATE_TRANSMIT;

	NVIC_EnableIRQ(qspi->interrupt);
	    
	QUADSPI->CR = quadspi_cr | QUADSPI_CR_TCIE | QUADSPI_CR_DMAEN;
	    
	QUADSPI->DLR = tx_count -1;
	QUADSPI->CCR = command | QUADSPI_CCR_FMODE_INDIRECT_WRITE;
	
	if ((command & QSPI_COMMAND_ADDRESS_MASK) != QSPI_COMMAND_ADDRESS_NONE)
	{
	    QUADSPI->AR = address;
	}
	
	stm32l4_dma_start(&qspi->dma, (uint32_t)&QUADSPI->DR, (uint32_t)tx_data, dma_count, dma_option);
    }

    return true;
}

bool stm32l4_qspi_wait(stm32l4_qspi_t *qspi, uint32_t command, uint32_t address, uint8_t *rx_data, unsigned int rx_count, uint32_t mask, uint32_t match, uint32_t control)
{
    uint8_t *rx_data_e;

    if (qspi->state != QSPI_STATE_SELECTED)
    {
	return false;
    }
    
    while (QUADSPI->SR & QUADSPI_SR_BUSY) { }

    QUADSPI->PSMKR = mask;
    QUADSPI->PSMAR = match;
    QUADSPI->PIR   = 0;
    
    QUADSPI->DLR = rx_count -1;
    
    if (!(control & QSPI_CONTROL_ASYNC))
    {
	QUADSPI->CCR = command | QUADSPI_CCR_FMODE_AUTOMATIC_POLLING;

	if ((command & QSPI_COMMAND_ADDRESS_MASK) != QSPI_COMMAND_ADDRESS_NONE)
	{
	    QUADSPI->AR = address;
	}

	while (!(QUADSPI->SR & QUADSPI_SR_SMF)) { }

	QUADSPI->FCR = QUADSPI_FCR_CSMF | QUADSPI_FCR_CTCF;

	rx_data_e = rx_data + rx_count;

	while (rx_data != rx_data_e)
	{
	    stm32l4_qspi_rd8(rx_data);
	    rx_data++;
	}
    }
    else
    {
	qspi->rx_data = rx_data;
	qspi->rx_count = rx_count;

	qspi->state = QSPI_STATE_WAIT;

	QUADSPI->CR |= QUADSPI_CR_SMIE;

	NVIC_EnableIRQ(qspi->interrupt);

	QUADSPI->CCR = command | QUADSPI_CCR_FMODE_AUTOMATIC_POLLING;

	if ((command & QSPI_COMMAND_ADDRESS_MASK) != QSPI_COMMAND_ADDRESS_NONE)
	{
	    QUADSPI->AR = address;
	}
    }

    return true;
}

bool stm32l4_qspi_done(stm32l4_qspi_t *qspi)
{
    return ((qspi->state == QSPI_STATE_READY) || (qspi->state == QSPI_STATE_SELECTED));
}

void QUADSPI_IRQHandler(void)
{
    stm32l4_qspi_interrupt(stm32l4_qspi_driver.instances[QSPI_INSTANCE_QUADSPI]);
}
