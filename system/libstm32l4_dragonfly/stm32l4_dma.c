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

#include "stm32l4_dma.h"

#include "armv7m.h"

static DMA_Channel_TypeDef * const  stm32l4_dma_channel_table[16] = {
    NULL,
    DMA1_Channel1,
    DMA1_Channel2,
    DMA1_Channel3,
    DMA1_Channel4,
    DMA1_Channel5,
    DMA1_Channel6,
    DMA1_Channel7,
    NULL,
    DMA2_Channel1,
    DMA2_Channel2,
    DMA2_Channel3,
    DMA2_Channel4,
    DMA2_Channel5,
    DMA2_Channel6,
    DMA2_Channel7,
};

static const IRQn_Type stm32l4_dma_interrupt_table[16] = {
    0,
    DMA1_Channel1_IRQn,
    DMA1_Channel2_IRQn,
    DMA1_Channel3_IRQn,
    DMA1_Channel4_IRQn,
    DMA1_Channel5_IRQn,
    DMA1_Channel6_IRQn,
    DMA1_Channel7_IRQn,
    0,
    DMA2_Channel1_IRQn,
    DMA2_Channel2_IRQn,
    DMA2_Channel3_IRQn,
    DMA2_Channel4_IRQn,
    DMA2_Channel5_IRQn,
    DMA2_Channel6_IRQn,
    DMA2_Channel7_IRQn,
};

typedef struct _stm32l4_dma_driver_t {
    volatile uint32_t      mask;
#ifdef notyet
    volatile uint32_t      flash;
    volatile uint32_t      sram1;
    volatile uint32_t      sram2;
#endif
    stm32l4_dma_t          *instances[16];
} stm32l4_dma_driver_t;

static stm32l4_dma_driver_t stm32l4_dma_driver;

static void stm32l4_dma_track(uint8_t channel, uint32_t address)
{
#ifdef notyet
    if (address < 0x40000000)
    {
	if (address < 0x10000000)
	{
	    armv7m_atomic_add(&stm32l4_dma_driver.flash, 1);

	    armv7m_atomic_or(&RCC->AHB1SMENR, RCC_AHB1SMENR_FLASHSMEN);
	}
	else if (address < 0x20000000)
	{
	    armv7m_atomic_add(&stm32l4_dma_driver.sram2, 1);

	    armv7m_atomic_or(&RCC->AHB2SMENR, RCC_AHB2SMENR_SRAM2SMEN);
	}
	else
	{
	    armv7m_atomic_add(&stm32l4_dma_driver.sram1, 1);

	    armv7m_atomic_or(&RCC->AHB1SMENR, RCC_AHB1SMENR_SRAM1SMEN);
	}
    }
#endif
}

static void stm32l4_dma_untrack(uint8_t channel, uint32_t address)
{
#ifdef notyet
    uint32_t o_flash, o_sram1, o_sram2, n_flash, n_sram1, n_sram2;

    if (address < 0x40000000)
    {
	if (address < 0x10000000)
	{
	    o_flash = stm32l4_dma_driver.flash;

	    do
	    {
		n_flash = o_flash - 1;

		if (n_flash == 0)
		{
		    armv7m_atomic_and(&RCC->AHB1SMENR, ~RCC_AHB1SMENR_FLASHSMEN);
		}
		else
		{
		    armv7m_atomic_or(&RCC->AHB1SMENR, RCC_AHB1SMENR_FLASHSMEN);
		}
	    }
	    while (!armv7m_atomic_compare_exchange(&stm32l4_dma_driver.flash, &o_flash, n_flash));
	}
	else if (address < 0x20000000)
	{
	    o_sram2 = stm32l4_dma_driver.sram2;

	    do
	    {
		n_sram2 = o_sram2 - 1;

		if (n_sram2 == 0)
		{
		    armv7m_atomic_and(&RCC->AHB2SMENR, ~RCC_AHB2SMENR_SRAM2SMEN);
		}
		else
		{
		    armv7m_atomic_or(&RCC->AHB2SMENR, RCC_AHB2SMENR_SRAM2SMEN);
		}
	    }
	    while (!armv7m_atomic_compare_exchange(&stm32l4_dma_driver.sram2, &o_sram2, n_sram2));
	}
	else
	{
	    o_sram1 = stm32l4_dma_driver.sram1;

	    do
	    {
		n_sram1 = o_sram1 - 1;

		if (n_sram1 == 0)
		{
		    armv7m_atomic_and(&RCC->AHB1SMENR, ~RCC_AHB1SMENR_SRAM1SMEN);
		}
		else
		{
		    armv7m_atomic_or(&RCC->AHB1SMENR, RCC_AHB1SMENR_SRAM1SMEN);
		}
	    }
	    while (!armv7m_atomic_compare_exchange(&stm32l4_dma_driver.sram1, &o_sram1, n_sram1));
	}
    }
#endif
}

static void stm32l4_dma_interrupt(stm32l4_dma_t *dma)
{
    unsigned int shift;
    uint32_t events;

    shift = ((dma->channel & 7) -1) << 2;

    if (!(dma->channel & 8))
    {
	events = (DMA1->ISR >> shift) & 0x0000000e;

	DMA1->IFCR = (15 << shift);
    }
    else
    {
	events = (DMA2->ISR >> shift) & 0x0000000e;

	DMA2->IFCR = (15 << shift);
    }

    if (events)
    {
	(*dma->callback)(dma->context, events);
    }
}

bool stm32l4_dma_create(stm32l4_dma_t *dma, uint8_t channel, unsigned int priority)
{
    uint32_t o_mask, n_mask;

    n_mask = (1ul << (channel & 15));

    o_mask = armv7m_atomic_or(&stm32l4_dma_driver.mask, n_mask);

    if (o_mask & n_mask)
    {
	return false;
    }

    
    dma->DMA = stm32l4_dma_channel_table[channel & 15];
    dma->interrupt = stm32l4_dma_interrupt_table[channel & 15];
    dma->channel = channel;

    dma->callback = NULL;
    dma->context = NULL;

    stm32l4_dma_driver.instances[channel & 15] = dma;

    NVIC_SetPriority(dma->interrupt, priority);
    
    return true;
}

void stm32l4_dma_destroy(stm32l4_dma_t *dma)
{
    uint32_t n_mask;

    stm32l4_dma_driver.instances[dma->channel & 15] = NULL;
    
    n_mask = (1ul << (dma->channel & 15));

    armv7m_atomic_and(&stm32l4_dma_driver.mask, ~n_mask);
}

void stm32l4_dma_enable(stm32l4_dma_t *dma, stm32l4_dma_callback_t callback, void *context)
{
    unsigned int shift;

    dma->callback = callback;
    dma->context = context;

    shift = ((dma->channel & 7) -1) << 2;

    if (!(dma->channel & 8))
    {
	armv7m_atomic_or(&RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);
	armv7m_atomic_modify(&DMA1_CSELR->CSELR, (15 << shift), (dma->channel >> 4) << shift);
    }
    else
    {
	armv7m_atomic_or(&RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);
	armv7m_atomic_modify(&DMA2_CSELR->CSELR, (15 << shift), (dma->channel >> 4) << shift);
    }

    if (callback)
    {
	NVIC_EnableIRQ(dma->interrupt);
    }
}

void stm32l4_dma_disable(stm32l4_dma_t *dma)
{
    NVIC_DisableIRQ(dma->interrupt);
}

void stm32l4_dma_start(stm32l4_dma_t *dma, uint32_t tx_data, uint32_t rx_data, uint16_t xf_count, uint32_t option)
{
    DMA_Channel_TypeDef *DMA = dma->DMA;
    unsigned int shift;

    DMA->CCR &= ~DMA_CCR_EN;

    shift = ((dma->channel & 7) -1) << 2;

    if (!(dma->channel & 8))
    {
	DMA1->IFCR = (15 << shift);
    }
    else
    {
	DMA2->IFCR = (15 << shift);
    }

    dma->size = xf_count;

    if (option & DMA_OPTION_MEMORY_TO_PERIPHERAL)
    {
	stm32l4_dma_track(dma->channel, rx_data);

	DMA->CMAR = rx_data;
	DMA->CPAR = tx_data;
    }
    else
    {
	stm32l4_dma_track(dma->channel, (uint32_t)tx_data);

	DMA->CMAR = tx_data;
	DMA->CPAR = rx_data;
    }

    DMA->CNDTR = xf_count;
    DMA->CCR = option | DMA_CCR_EN;
}

uint16_t stm32l4_dma_stop(stm32l4_dma_t *dma)
{
    DMA_Channel_TypeDef *DMA = dma->DMA;

    DMA->CCR &= ~(DMA_CCR_EN | DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_TEIE);

    stm32l4_dma_untrack(dma->channel, DMA->CMAR);

    return dma->size - (DMA->CNDTR & 0xffff);
}

uint16_t stm32l4_dma_count(stm32l4_dma_t *dma)
{
    DMA_Channel_TypeDef *DMA = dma->DMA;

    return dma->size - (DMA->CNDTR & 0xffff);
}

bool stm32l4_dma_done(stm32l4_dma_t *dma)
{
    unsigned int shift;

    shift = ((dma->channel & 7) -1) << 2;

    if (!(dma->channel & 8))
    {
	return !!(DMA1->ISR & (DMA_ISR_TCIF1 << shift));
    }
    else
    {
	return !!(DMA2->ISR & (DMA_ISR_TCIF1 << shift));
    }
}

void stm32l4_dma_poll(stm32l4_dma_t *dma)
{
    stm32l4_dma_interrupt(dma);
}

void DMA1_Channel1_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA1_CH1_INDEX]);
}

void DMA1_Channel2_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA1_CH2_INDEX]);
}

void DMA1_Channel3_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA1_CH3_INDEX]);
}

void DMA1_Channel4_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA1_CH4_INDEX]);
}

void DMA1_Channel5_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA1_CH5_INDEX]);
}

void DMA1_Channel6_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA1_CH6_INDEX]);
}

void DMA1_Channel7_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA1_CH7_INDEX]);
}

void DMA2_Channel1_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA2_CH1_INDEX]);
}

void DMA2_Channel2_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA2_CH2_INDEX]);
}

void DMA2_Channel3_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA2_CH3_INDEX]);
}

void DMA2_Channel4_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA2_CH4_INDEX]);
}

void DMA2_Channel5_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA2_CH5_INDEX]);
}

void DMA2_Channel6_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA2_CH6_INDEX]);
}

void DMA2_Channel7_IRQHandler(void)
{
    stm32l4_dma_interrupt(stm32l4_dma_driver.instances[DMA_CHANNEL_DMA2_CH7_INDEX]);
}
