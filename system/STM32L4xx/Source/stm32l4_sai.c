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
#include "stm32l4_sai.h"
#include "stm32l4_dma.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_sai_driver_t {
    stm32l4_sai_t     *instances[SAI_INSTANCE_COUNT];
} stm32l4_sai_driver_t;

static stm32l4_sai_driver_t stm32l4_sai_driver;

#define SAI_DMA_OPTION_RECEIVE_8	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_HIGH)

#define SAI_DMA_OPTION_RECEIVE_16	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_HIGH)

#define SAI_DMA_OPTION_RECEIVE_32	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_32 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_HIGH)

#define SAI_DMA_OPTION_TRANSMIT_8	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_HIGH)

#define SAI_DMA_OPTION_TRANSMIT_16	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_16 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_HIGH)

#define SAI_DMA_OPTION_TRANSMIT_32	  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |	  \
     DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_32 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_HIGH)

static void stm32l4_sai_dma_callback(stm32l4_sai_t *sai, uint32_t events);

static void stm32l4_sai_start(stm32l4_sai_t *sai)
{
    stm32l4_system_periph_enable(SYSTEM_PERIPH_SAI1 + (sai->instance >> 1));

    if (sai->state != SAI_STATE_BUSY)
    {
	if (sai->mode & SAI_MODE_DMA)
	{
	    stm32l4_dma_enable(&sai->dma, (stm32l4_dma_callback_t)stm32l4_sai_dma_callback, sai);
	}
    }
}

static void stm32l4_sai_stop(stm32l4_sai_t *sai)
{
    if (sai->state != SAI_STATE_BUSY)
    {
	if (sai->mode & SAI_MODE_DMA)
	{
	    stm32l4_dma_disable(&sai->dma);
	}
    }

    stm32l4_system_periph_disable(SYSTEM_PERIPH_SAI1 + (sai->instance >> 1));
}

static __attribute__((optimize("O3"))) void stm32l4_sai_dma_callback(stm32l4_sai_t *sai, uint32_t events)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;

    SAIx->CR1 &= ~SAI_xCR1_DMAEN;

    if (sai->state == SAI_STATE_RECEIVE_DMA)
    {
	sai->state = SAI_STATE_RECEIVE_REQUEST;

	if (sai->events & SAI_EVENT_RECEIVE_REQUEST)
	{
	    (*sai->callback)(sai->context, SAI_EVENT_RECEIVE_REQUEST);
	}

	if (sai->state == SAI_STATE_RECEIVE_REQUEST)
	{
	    SAIx->CR1 &= ~(SAI_xCR1_SAIEN | SAI_xCR1_MODE_0);
	    SAIx->CR2 = SAI_xCR2_FFLUSH;
	
	    SAIx->CLRFR = ~0;

	    stm32l4_sai_stop(sai);

	    sai->state = SAI_STATE_READY;

	    if (sai->events & SAI_EVENT_RECEIVE_DONE)
	    {
		(*sai->callback)(sai->context, SAI_EVENT_RECEIVE_DONE);
	    }
	}
    }
    else
    {
	sai->state = SAI_STATE_TRANSMIT_REQUEST;

	if (sai->events & SAI_EVENT_TRANSMIT_REQUEST)
	{
	    (*sai->callback)(sai->context, SAI_EVENT_TRANSMIT_REQUEST);
	}

	if (sai->state == SAI_STATE_TRANSMIT_REQUEST)
	{
	    sai->state = SAI_STATE_TRANSMIT_DONE;

	    SAIx->IMR |= SAI_xIMR_OVRUDRIE;
	}
    }
}

static __attribute__((optimize("O3"))) void stm32l4_sai_receive_8_callback(stm32l4_sai_t *sai)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    uint8_t *rx_data, *rx_data_e;

    SAIx->CLRFR = ~0;

    while (1)
    {
	rx_data = (uint8_t*)sai->xf_data;
	rx_data_e = (uint8_t*)sai->xf_data_e;
		
	while (rx_data != rx_data_e)
	{
	    if ((SAIx->SR & SAI_xSR_FLVL) == 0)
	    {
		break;
	    }
	    
	    *rx_data++ = SAIx->DR;
	}
		
	if (rx_data == rx_data_e)
	{
	    sai->state = SAI_STATE_RECEIVE_REQUEST;
		    
	    if (sai->events & SAI_EVENT_RECEIVE_REQUEST)
	    {
		(*sai->callback)(sai->context, SAI_EVENT_RECEIVE_REQUEST);
	    }
	    
	    if (sai->state == SAI_STATE_RECEIVE_REQUEST)
	    {
		sai->xf_callback = NULL;

		SAIx->CR1 &= ~(SAI_xCR1_SAIEN | SAI_xCR1_MODE_0);
		SAIx->CR2 = SAI_xCR2_FFLUSH;

		SAIx->IMR &= ~SAI_xIMR_FREQIE;
	
		SAIx->CLRFR = ~0;

		stm32l4_sai_stop(sai);
		
		sai->state = SAI_STATE_READY;
		
		if (sai->events & SAI_EVENT_RECEIVE_DONE)
		{
		    (*sai->callback)(sai->context, SAI_EVENT_RECEIVE_DONE);
		}
			
		break;
	    }
	}
	else
	{
	    sai->xf_data = (void*)rx_data;

	    break;
	}
    }
}

static __attribute__((optimize("O3"))) void stm32l4_sai_receive_16_callback(stm32l4_sai_t *sai)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    uint16_t *rx_data, *rx_data_e;

    SAIx->CLRFR = ~0;

    while (1)
    {
	rx_data = (uint16_t*)sai->xf_data;
	rx_data_e = (uint16_t*)sai->xf_data_e;
		
	while (rx_data != rx_data_e)
	{
	    if ((SAIx->SR & SAI_xSR_FLVL) == 0)
	    {
		break;
	    }
	    
	    *rx_data++ = SAIx->DR;
	}
		
	if (rx_data == rx_data_e)
	{
	    sai->state = SAI_STATE_RECEIVE_REQUEST;
		    
	    if (sai->events & SAI_EVENT_RECEIVE_REQUEST)
	    {
		(*sai->callback)(sai->context, SAI_EVENT_RECEIVE_REQUEST);
	    }
	    
	    if (sai->state == SAI_STATE_RECEIVE_REQUEST)
	    {
		sai->xf_callback = NULL;

		SAIx->CR1 &= ~(SAI_xCR1_SAIEN | SAI_xCR1_MODE_0);
		SAIx->CR2 = SAI_xCR2_FFLUSH;

		SAIx->IMR &= ~SAI_xIMR_FREQIE;
	
		SAIx->CLRFR = ~0;

		stm32l4_sai_stop(sai);
		
		sai->state = SAI_STATE_READY;
		
		if (sai->events & SAI_EVENT_RECEIVE_DONE)
		{
		    (*sai->callback)(sai->context, SAI_EVENT_RECEIVE_DONE);
		}
			
		break;
	    }
	}
	else
	{
	    sai->xf_data = (void*)rx_data;

	    break;
	}
    }
}

static __attribute__((optimize("O3"))) void stm32l4_sai_receive_32_callback(stm32l4_sai_t *sai)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    uint32_t *rx_data, *rx_data_e;

    SAIx->CLRFR = ~0;

    while (1)
    {
	rx_data = (uint32_t*)sai->xf_data;
	rx_data_e = (uint32_t*)sai->xf_data_e;
		
	while (rx_data != rx_data_e)
	{
	    if ((SAIx->SR & SAI_xSR_FLVL) == 0)
	    {
		break;
	    }
	    
	    *rx_data++ = SAIx->DR;
	}
		
	if (rx_data == rx_data_e)
	{
	    sai->state = SAI_STATE_RECEIVE_REQUEST;
		    
	    if (sai->events & SAI_EVENT_RECEIVE_REQUEST)
	    {
		(*sai->callback)(sai->context, SAI_EVENT_RECEIVE_REQUEST);
	    }
	    
	    if (sai->state == SAI_STATE_RECEIVE_REQUEST)
	    {
		sai->xf_callback = NULL;

		SAIx->CR1 &= ~(SAI_xCR1_SAIEN | SAI_xCR1_MODE_0);
		SAIx->CR2 = SAI_xCR2_FFLUSH;

		SAIx->IMR &= ~SAI_xIMR_FREQIE;
	
		SAIx->CLRFR = ~0;

		stm32l4_sai_stop(sai);
		
		sai->state = SAI_STATE_READY;
		
		if (sai->events & SAI_EVENT_RECEIVE_DONE)
		{
		    (*sai->callback)(sai->context, SAI_EVENT_RECEIVE_DONE);
		}
			
		break;
	    }
	}
	else
	{
	    sai->xf_data = (void*)rx_data;

	    break;
	}
    }
}

static __attribute__((optimize("O3"))) void stm32l4_sai_transmit_8_callback(stm32l4_sai_t *sai)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    const uint8_t *tx_data, *tx_data_e;

    SAIx->CLRFR = ~0;

    while (1)
    {
	tx_data = (const uint8_t*)sai->xf_data;
	tx_data_e = (const uint8_t*)sai->xf_data_e;
		
	while (tx_data != tx_data_e)
	{
	    if ((SAIx->SR & SAI_xSR_FLVL) == (SAI_xSR_FLVL_2 | SAI_xSR_FLVL_0))
	    {
		break;
	    }
	    
	    SAIx->DR = *tx_data++;
	}
		
	if (tx_data == tx_data_e)
	{
	    sai->state = SAI_STATE_TRANSMIT_REQUEST;
		    
	    if (sai->events & SAI_EVENT_TRANSMIT_REQUEST)
	    {
		(*sai->callback)(sai->context, SAI_EVENT_TRANSMIT_REQUEST);
	    }
	    
	    if (sai->state == SAI_STATE_TRANSMIT_REQUEST)
	    {
		sai->state = SAI_STATE_TRANSMIT_DONE;

		sai->xf_callback = NULL;

		SAIx->IMR &= ~SAI_xIMR_FREQIE;
		SAIx->IMR |= SAI_xIMR_OVRUDRIE;
			
		break;
	    }
	}
	else
	{
	    sai->xf_data = (void*)tx_data;

	    break;
	}
    }
}

static __attribute__((optimize("O3"))) void stm32l4_sai_transmit_16_callback(stm32l4_sai_t *sai)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    const uint16_t *tx_data, *tx_data_e;

    SAIx->CLRFR = ~0;

    while (1)
    {
	tx_data = (const uint16_t*)sai->xf_data;
	tx_data_e = (const uint16_t*)sai->xf_data_e;
		
	while (tx_data != tx_data_e)
	{
	    if ((SAIx->SR & SAI_xSR_FLVL) == (SAI_xSR_FLVL_2 | SAI_xSR_FLVL_0))
	    {
		break;
	    }
	    
	    SAIx->DR = *tx_data++;
	}
		
	if (tx_data == tx_data_e)
	{
	    sai->state = SAI_STATE_TRANSMIT_REQUEST;
		    
	    if (sai->events & SAI_EVENT_TRANSMIT_REQUEST)
	    {
		(*sai->callback)(sai->context, SAI_EVENT_TRANSMIT_REQUEST);
	    }
	    
	    if (sai->state == SAI_STATE_TRANSMIT_REQUEST)
	    {
		sai->state = SAI_STATE_TRANSMIT_DONE;

		sai->xf_callback = NULL;

		SAIx->IMR &= ~SAI_xIMR_FREQIE;
		SAIx->IMR |= SAI_xIMR_OVRUDRIE;
			
		break;
	    }
	}
	else
	{
	    sai->xf_data = (void*)tx_data;

	    break;
	}
    }
}

static __attribute__((optimize("O3"))) void stm32l4_sai_transmit_32_callback(stm32l4_sai_t *sai)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    const uint32_t *tx_data, *tx_data_e;

    SAIx->CLRFR = ~0;

    while (1)
    {
	tx_data = (const uint32_t*)sai->xf_data;
	tx_data_e = (const uint32_t*)sai->xf_data_e;
		
	while (tx_data != tx_data_e)
	{
	    if ((SAIx->SR & SAI_xSR_FLVL) == (SAI_xSR_FLVL_2 | SAI_xSR_FLVL_0))
	    {
		break;
	    }
	    
	    SAIx->DR = *tx_data++;
	}
		
	if (tx_data == tx_data_e)
	{
	    sai->state = SAI_STATE_TRANSMIT_REQUEST;
		    
	    if (sai->events & SAI_EVENT_TRANSMIT_REQUEST)
	    {
		(*sai->callback)(sai->context, SAI_EVENT_TRANSMIT_REQUEST);
	    }
	    
	    if (sai->state == SAI_STATE_TRANSMIT_REQUEST)
	    {
		sai->state = SAI_STATE_TRANSMIT_DONE;

		sai->xf_callback = NULL;

		SAIx->IMR &= ~SAI_xIMR_FREQIE;
		SAIx->IMR |= SAI_xIMR_OVRUDRIE;
			
		break;
	    }
	}
	else
	{
	    sai->xf_data = (void*)tx_data;

	    break;
	}
    }
}

static __attribute__((optimize("O3"))) void stm32l4_sai_interrupt(stm32l4_sai_t *sai)
{
    if (sai->xf_callback)
    {
	(*sai->xf_callback)(sai);
    }
    else
    {
	SAI_Block_TypeDef *SAIx = sai->SAIx;

	SAIx->IMR &= ~SAI_xIMR_OVRUDRIE;

	SAIx->CR1 &= ~(SAI_xCR1_SAIEN | SAI_xCR1_MODE_0);
	SAIx->CR2 = SAI_xCR2_FFLUSH;
	
	SAIx->CLRFR = ~0;
    
	stm32l4_sai_stop(sai);

	sai->state = SAI_STATE_READY;

	if (sai->events & SAI_EVENT_TRANSMIT_DONE)
	{
	    (*sai->callback)(sai->context, SAI_EVENT_TRANSMIT_DONE);
	}
    }
}

bool stm32l4_sai_create(stm32l4_sai_t *sai, unsigned int instance, const stm32l4_sai_pins_t *pins, unsigned int priority, unsigned int mode)
{
    sai->state = SAI_STATE_INIT;
    sai->instance = instance;

    switch (instance) {
    case SAI_INSTANCE_SAI1A:
	sai->SAIx = SAI1_Block_A;
	sai->interrupt = SAI1_IRQn;
	break;

    case SAI_INSTANCE_SAI1B:
	sai->SAIx = SAI1_Block_B;
	sai->interrupt = SAI1_IRQn;
	break;

#if defined(STM32L476xx) || defined(STM32L496xx)
    case SAI_INSTANCE_SAI2A:
	sai->SAIx = SAI2_Block_A;
	sai->interrupt = SAI2_IRQn;
	break;

    case SAI_INSTANCE_SAI2B:
	sai->SAIx = SAI2_Block_B;
	sai->interrupt = SAI2_IRQn;
	break;
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */

    default:
	sai->state = SAI_STATE_NONE;
	return false;
    }

    sai->priority = priority;
    sai->pins = *pins;

    sai->mode = mode & ~SAI_MODE_DMA;

    if (mode & SAI_MODE_DMA)
    {
	switch (sai->instance) {
	case SAI_INSTANCE_SAI1A:
	    if ((!(mode & SAI_MODE_DMA_SECONDARY) && stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA2_CH6_SAI1_A, sai->priority)) ||
		stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA2_CH1_SAI1_A, sai->priority))
	    {
		sai->mode |= SAI_MODE_DMA;
	    }
	    break;
	case SAI_INSTANCE_SAI1B:
	    if ((!(mode & SAI_MODE_DMA_SECONDARY) && stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA2_CH7_SAI1_B, sai->priority)) ||
		stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA2_CH2_SAI1_B, sai->priority))
	    {
		sai->mode |= SAI_MODE_DMA;
	    }
	    break;
#if defined(STM32L476xx) || defined(STM32L496xx)
	case SAI_INSTANCE_SAI2A:
	    if ((!(mode & SAI_MODE_DMA_SECONDARY) && stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA2_CH3_SAI2_A, sai->priority)) ||
		stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA1_CH6_SAI2_A, sai->priority))
	    {
		sai->mode |= SAI_MODE_DMA;
	    }
	    break;
	case SAI_INSTANCE_SAI2B:
	    if ((!(mode & SAI_MODE_DMA_SECONDARY) && stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA2_CH4_SAI2_B, sai->priority)) ||
		stm32l4_dma_create(&sai->dma, DMA_CHANNEL_DMA1_CH7_SAI2_B, sai->priority))
	    {
		sai->mode |= SAI_MODE_DMA;
	    }
	    break;
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
	}
    }

    if ((sai->mode ^ mode) & SAI_MODE_DMA)
    {
	if (sai->mode & SAI_MODE_DMA)
	{
	    stm32l4_dma_destroy(&sai->dma);
			
	    sai->mode &= ~SAI_MODE_DMA;
	}

	sai->state = SAI_STATE_NONE;

	return false;
    }

    stm32l4_sai_driver.instances[instance] = sai;

    return true;
}

bool stm32l4_sai_destroy(stm32l4_sai_t *sai)
{
    if ((sai->state != SAI_STATE_NONE) && (sai->state != SAI_STATE_BUSY))
    {
	return false;
    }

    stm32l4_sai_driver.instances[sai->instance] = NULL;

    if (sai->mode & SAI_MODE_DMA)
    {
	stm32l4_dma_destroy(&sai->dma);
	
	sai->mode &= ~SAI_MODE_DMA;
    }

    return true;
}

bool stm32l4_sai_enable(stm32l4_sai_t *sai, uint32_t width, uint32_t clock, uint32_t option, stm32l4_sai_callback_t callback, void *context, uint32_t events)
{
    if (sai->state != SAI_STATE_INIT)
    {
	return false;
    }

    sai->callback = NULL;
    sai->context = NULL;
    sai->events = 0;

    NVIC_SetPriority(sai->interrupt, sai->priority);
    NVIC_EnableIRQ(sai->interrupt);

    sai->state = SAI_STATE_BUSY;

    if (!stm32l4_sai_configure(sai, width, clock, option))
    {
	sai->state = SAI_STATE_NONE;

	return false;
    }

    stm32l4_sai_notify(sai, callback, context, events);

    sai->state = SAI_STATE_READY;

    return true;
}

bool stm32l4_sai_disable(stm32l4_sai_t *sai)
{
    if (sai->state != SAI_STATE_READY)
    {
	return false;
    }

    stm32l4_system_saiclk_configure(SYSTEM_SAICLK_NONE);

    stm32l4_gpio_pin_configure(sai->pins.sck, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(sai->pins.fs, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(sai->pins.sd, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));

    if (sai->option & SAI_OPTION_MCK)
    {
	stm32l4_gpio_pin_configure(sai->pins.mck, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    }

    sai->state = SAI_STATE_INIT;

    return true;
}

bool stm32l4_sai_configure(stm32l4_sai_t *sai, uint32_t width, uint32_t clock, uint32_t option)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    uint32_t sai_cr1, sai_frcr, sai_slotr, saiclk;

    if ((sai->state != SAI_STATE_READY) && (sai->state != SAI_STATE_BUSY))
    {
	return false;
    }

    switch (width) {
    case 8:  sai_cr1 = (SAI_xCR1_DS_1);                                 break;
    case 10: sai_cr1 = (SAI_xCR1_DS_1 | SAI_xCR1_DS_0);                 break;
    case 16: sai_cr1 = (SAI_xCR1_DS_2);         	                break;
    case 20: sai_cr1 = (SAI_xCR1_DS_2 | SAI_xCR1_DS_0);                 break;
    case 24: sai_cr1 = (SAI_xCR1_DS_2 | SAI_xCR1_DS_1);                 break;
    case 32: sai_cr1 = (SAI_xCR1_DS_2 | SAI_xCR1_DS_1 | SAI_xCR1_DS_0); break;
    default:
	return false;
    }

    if (clock)
    {
#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
	switch (clock) {
	case 8000:  saiclk = SYSTEM_SAICLK_8192000;  sai_cr1 |= (2 << SAI_xCR1_MCKDIV_Pos); break;
	case 16000: saiclk = SYSTEM_SAICLK_8192000;  sai_cr1 |= (1 << SAI_xCR1_MCKDIV_Pos); break;
	case 32000: saiclk = SYSTEM_SAICLK_8192000;  sai_cr1 |= (0 << SAI_xCR1_MCKDIV_Pos); break;
	case 12000: saiclk = SYSTEM_SAICLK_24576000; sai_cr1 |= (4 << SAI_xCR1_MCKDIV_Pos); break;
	case 24000: saiclk = SYSTEM_SAICLK_24576000; sai_cr1 |= (2 << SAI_xCR1_MCKDIV_Pos); break;
	case 48000: saiclk = SYSTEM_SAICLK_24576000; sai_cr1 |= (1 << SAI_xCR1_MCKDIV_Pos); break;
	case 96000: saiclk = SYSTEM_SAICLK_24576000; sai_cr1 |= (0 << SAI_xCR1_MCKDIV_Pos); break;
	case 11025: saiclk = SYSTEM_SAICLK_11289600; sai_cr1 |= (2 << SAI_xCR1_MCKDIV_Pos); break;
	case 22050: saiclk = SYSTEM_SAICLK_11289600; sai_cr1 |= (1 << SAI_xCR1_MCKDIV_Pos); break;
	case 44100: saiclk = SYSTEM_SAICLK_11289600; sai_cr1 |= (0 << SAI_xCR1_MCKDIV_Pos); break;
	default:
	    return false;
	}
#else /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
	switch (clock) {
	case 8000:  saiclk = SYSTEM_SAICLK_49152000; sai_cr1 |= (12 << SAI_xCR1_MCKDIV_Pos); break;
	case 16000: saiclk = SYSTEM_SAICLK_49152000; sai_cr1 |= (6 << SAI_xCR1_MCKDIV_Pos);  break;
	case 32000: saiclk = SYSTEM_SAICLK_49152000; sai_cr1 |= (3 << SAI_xCR1_MCKDIV_Pos);  break;
	case 12000: saiclk = SYSTEM_SAICLK_49152000; sai_cr1 |= (8 << SAI_xCR1_MCKDIV_Pos);  break;
	case 24000: saiclk = SYSTEM_SAICLK_49152000; sai_cr1 |= (4 << SAI_xCR1_MCKDIV_Pos);  break;
	case 48000: saiclk = SYSTEM_SAICLK_49152000; sai_cr1 |= (2 << SAI_xCR1_MCKDIV_Pos);  break;
	case 96000: saiclk = SYSTEM_SAICLK_49152000; sai_cr1 |= (1 << SAI_xCR1_MCKDIV_Pos);  break;
	case 11025: saiclk = SYSTEM_SAICLK_11289600; sai_cr1 |= (2 << SAI_xCR1_MCKDIV_Pos);  break;
	case 22050: saiclk = SYSTEM_SAICLK_11289600; sai_cr1 |= (1 << SAI_xCR1_MCKDIV_Pos);  break;
	case 44100: saiclk = SYSTEM_SAICLK_11289600; sai_cr1 |= (0 << SAI_xCR1_MCKDIV_Pos);  break;
	default:
	    return false;
	}
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
    }
    else
    {
	saiclk = SYSTEM_SAICLK_NONE;

	sai_cr1 |= SAI_xCR1_MODE_1;
    }

    switch ((option & SAI_OPTION_FORMAT_MASK) >> SAI_OPTION_FORMAT_SHIFT) {
    case SAI_OPTION_FORMAT_I2S:
	sai_cr1 |= SAI_xCR1_CKSTR;

	if (option & SAI_OPTION_MONO)
	{
	    sai_cr1 |= SAI_xCR1_MONO;
	}

	if (width <= 16)
	{
	    sai_frcr = (31 << SAI_xFRCR_FRL_Pos) | (15 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSDEF | SAI_xFRCR_FSOFF;
	    sai_slotr = SAI_xSLOTR_NBSLOT_0 | (0x0003 << SAI_xSLOTR_SLOTEN_Pos) | SAI_xSLOTR_SLOTSZ_0;
	}
	else
	{
	    sai_frcr = (63 << SAI_xFRCR_FRL_Pos) | (31 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSDEF | SAI_xFRCR_FSOFF;
	    sai_slotr = SAI_xSLOTR_NBSLOT_0 | (0x0003 << SAI_xSLOTR_SLOTEN_Pos) | SAI_xSLOTR_SLOTSZ_1;
	}
	break;

    case SAI_OPTION_FORMAT_LEFT_JUSTIFIED:
	sai_cr1 |= SAI_xCR1_CKSTR;

	if (option & SAI_OPTION_MONO)
	{
	    sai_cr1 |= SAI_xCR1_MONO;
	}

	if (width <= 16)
	{
	    sai_frcr = (31 << SAI_xFRCR_FRL_Pos) | (15 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSDEF | SAI_xFRCR_FSPOL;
	    sai_slotr = SAI_xSLOTR_NBSLOT_0 | (0x0003 << SAI_xSLOTR_SLOTEN_Pos) | SAI_xSLOTR_SLOTSZ_0;
	}
	else
	{
	    sai_frcr = (63 << SAI_xFRCR_FRL_Pos) | (31 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSDEF | SAI_xFRCR_FSPOL;
	    sai_slotr = SAI_xSLOTR_NBSLOT_0 | (0x0003 << SAI_xSLOTR_SLOTEN_Pos) | SAI_xSLOTR_SLOTSZ_1;
	}
	break;

    case SAI_OPTION_FORMAT_RIGHT_JUSTIFIED:
	sai_cr1 |= SAI_xCR1_CKSTR;

	if (option & SAI_OPTION_MONO)
	{
	    sai_cr1 |= SAI_xCR1_MONO;
	}

	if (width <= 16)
	{
	    sai_frcr = (31 << SAI_xFRCR_FRL_Pos) | (15 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSDEF | SAI_xFRCR_FSPOL;
	    sai_slotr = SAI_xSLOTR_NBSLOT_0 | (0x0003 << SAI_xSLOTR_SLOTEN_Pos) | SAI_xSLOTR_SLOTSZ_0 | ((16 - width) << SAI_xSLOTR_FBOFF_Pos);
	}
	else
	{
	    sai_frcr = (63 << SAI_xFRCR_FRL_Pos) | (31 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSDEF | SAI_xFRCR_FSPOL;
	    sai_slotr = SAI_xSLOTR_NBSLOT_0 | (0x0003 << SAI_xSLOTR_SLOTEN_Pos) | SAI_xSLOTR_SLOTSZ_1 | ((32 - width) << SAI_xSLOTR_FBOFF_Pos);
	}
	break;

    case SAI_OPTION_FORMAT_PCM_SHORT:
	sai_cr1 |= SAI_xCR1_CKSTR;

	if (width <= 8)
	{
	    sai_frcr = (7 << SAI_xFRCR_FRL_Pos) | (0 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF;
	}
	else if (width <= 16)
	{
	    sai_frcr = (15 << SAI_xFRCR_FRL_Pos) | (0 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF;
	}
	else
	{
	    sai_frcr = (31 << SAI_xFRCR_FRL_Pos) | (0 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF;
	}

	sai_slotr = (0x0001 << SAI_xSLOTR_SLOTEN_Pos);
	break;

    case SAI_OPTION_FORMAT_PCM_LONG:
	sai_cr1 |= SAI_xCR1_CKSTR;

	if (width <= 16)
	{
	    sai_frcr = (15 << SAI_xFRCR_FRL_Pos) | (12 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL;
	}
	else
	{
	    sai_frcr = (31 << SAI_xFRCR_FRL_Pos) | (12 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL;
	}

	sai_slotr = (0x0001 << SAI_xSLOTR_SLOTEN_Pos);
	break;

    case SAI_OPTION_FORMAT_DSP:
	sai_cr1 |= SAI_xCR1_CKSTR;

	if (option & SAI_OPTION_MONO)
	{
	    sai_cr1 |= SAI_xCR1_MONO;
	}

	if (width <= 8)
	{
	    sai_frcr = (15 << SAI_xFRCR_FRL_Pos) | (0 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF;
	}
	else if (width <= 16)
	{
	    sai_frcr = (31 << SAI_xFRCR_FRL_Pos) | (0 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF;
	}
	else
	{
	    sai_frcr = (63 << SAI_xFRCR_FRL_Pos) | (0 << SAI_xFRCR_FSALL_Pos) | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF;
	}

	sai_slotr = SAI_xSLOTR_NBSLOT_0 | (0x0003 << SAI_xSLOTR_SLOTEN_Pos);
	break;

    default:
	return false;
    }

    sai->width = width;
    sai->option = option;

    stm32l4_sai_start(sai);

    SAIx->CR1 = sai_cr1;
    SAIx->FRCR = sai_frcr;
    SAIx->SLOTR = sai_slotr;

    stm32l4_system_saiclk_configure(saiclk);

    stm32l4_gpio_pin_configure(sai->pins.sck, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(sai->pins.fs, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(sai->pins.sd, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));

    if (option & SAI_OPTION_MCK)
    {
	stm32l4_gpio_pin_configure(sai->pins.mck, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    }

    stm32l4_sai_stop(sai);

    return true;
}

bool stm32l4_sai_notify(stm32l4_sai_t *sai, stm32l4_sai_callback_t callback, void *context, uint32_t events)
{
    if ((sai->state != SAI_STATE_READY) && (sai->state != SAI_STATE_BUSY))
    {
	return false;
    }

    sai->callback = callback;
    sai->context = context;
    sai->events = callback ? events : 0x00000000;

    return true;
}

bool stm32l4_sai_receive(stm32l4_sai_t *sai, uint8_t *rx_data, uint16_t rx_count)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    uint32_t dma_option, dma_count;

    if ((sai->state != SAI_STATE_READY) && (sai->state != SAI_STATE_RECEIVE_REQUEST))
    {
	return false;
    }

    if (sai->state == SAI_STATE_READY)
    {
	stm32l4_sai_start(sai);
    }

    if (sai->mode & SAI_MODE_DMA)
    {
	if (sai->width <= 8)
	{
	    dma_option = SAI_DMA_OPTION_RECEIVE_8;
	    dma_count = rx_count / 1;
	}
	else if (sai->width <= 16)
	{
	    dma_option = SAI_DMA_OPTION_RECEIVE_16;
	    dma_count = rx_count / 2;
	}
	else
	{
	    dma_option = SAI_DMA_OPTION_RECEIVE_32;
	    dma_count = rx_count / 4;
	}
	
	sai->state = SAI_STATE_RECEIVE_DMA;
	
	stm32l4_dma_start(&sai->dma, (uint32_t)rx_data, (uint32_t)&SAIx->DR, dma_count, dma_option);
	
	if (!(SAIx->CR1 & SAI_xCR1_SAIEN))
	{
	    SAIx->CR2 = 0;
	    SAIx->CR1 |= (SAI_xCR1_SAIEN | SAI_xCR1_MODE_0);
	}
	
	SAIx->CR1 |= SAI_xCR1_DMAEN;
    }
    else
    {
	if (sai->width <= 8)
	{
	    sai->state = SAI_STATE_RECEIVE_8;

	    sai->xf_callback = stm32l4_sai_receive_8_callback;
	    sai->xf_data = (void*)rx_data;
	    sai->xf_data_e = (void*)((uint8_t*)rx_data + rx_count);
	}
	else if (sai->width <= 16)
	{
	    sai->state = SAI_STATE_RECEIVE_16;

	    sai->xf_callback = stm32l4_sai_receive_16_callback;
	    sai->xf_data = (void*)rx_data;
	    sai->xf_data_e = (void*)((uint8_t*)rx_data + (rx_count & ~1));
	}
	else
	{
	    sai->state = SAI_STATE_RECEIVE_32;

	    sai->xf_callback = stm32l4_sai_receive_32_callback;
	    sai->xf_data = (void*)rx_data;
	    sai->xf_data_e = (void*)((uint8_t*)rx_data + (rx_count & ~3));
	}

	if (!(SAIx->CR1 & SAI_xCR1_SAIEN))
	{
	    SAIx->CR2 = SAI_xCR2_FTH_1;
	    SAIx->CR1 |= (SAI_xCR1_SAIEN | SAI_xCR1_MODE_0);

	    SAIx->IMR |= SAI_xIMR_FREQIE;
	}
    }

    return true;
}

bool stm32l4_sai_transmit(stm32l4_sai_t *sai, const uint8_t *tx_data, uint16_t tx_count)
{
    SAI_Block_TypeDef *SAIx = sai->SAIx;
    uint32_t dma_option, dma_count;

    if ((sai->state != SAI_STATE_READY) && (sai->state != SAI_STATE_TRANSMIT_REQUEST))
    {
	return false;
    }

    if (sai->state == SAI_STATE_READY)
    {
	stm32l4_sai_start(sai);
    }

    if (sai->mode & SAI_MODE_DMA)
    {
	if (sai->width <= 8)
	{
	    dma_option = SAI_DMA_OPTION_TRANSMIT_8;
	    dma_count = tx_count / 1;
	}
	else if (sai->width <= 16)
	{
	    dma_option = SAI_DMA_OPTION_TRANSMIT_16;
	    dma_count = tx_count / 2;
	}
	else
	{
	    dma_option = SAI_DMA_OPTION_TRANSMIT_32;
	    dma_count = tx_count / 4;
	}

	sai->state = SAI_STATE_TRANSMIT_DMA;
	
	stm32l4_dma_start(&sai->dma, (uint32_t)&SAIx->DR, (uint32_t)tx_data, dma_count, dma_option);
	
	if (!(SAIx->CR1 & SAI_xCR1_SAIEN))
	{
	    SAIx->CR2 = SAI_xCR2_FTH_1;
	    SAIx->CR1 |= SAI_xCR1_SAIEN;
	}

	SAIx->CR1 |= SAI_xCR1_DMAEN;
    }
    else
    {
	if (sai->state == SAI_STATE_READY)
	{
	    stm32l4_sai_start(sai);

	    SAIx->CR2 = SAI_xCR2_FTH_1;
	}

	if (sai->width <= 8)
	{
	    const uint8_t *tx_data_e = (const uint8_t*)((const uint8_t*)tx_data + tx_count);

	    while ((const uint8_t*)tx_data != tx_data_e)
	    {
		if ((SAIx->SR & SAI_xSR_FLVL) == (SAI_xSR_FLVL_2 | SAI_xSR_FLVL_0))
		{
		    break;
		}
		
		SAIx->DR = *((const uint8_t*)tx_data);

		tx_data += 1;
	    }

	    sai->state = SAI_STATE_TRANSMIT_8;

	    sai->xf_callback = stm32l4_sai_transmit_8_callback;
	    sai->xf_data = (void*)tx_data;
	    sai->xf_data_e = (void*)tx_data_e;
	}
	else if (sai->width <= 16)
	{
	    const uint16_t *tx_data_e = (const uint16_t*)((const uint8_t*)tx_data + (tx_count & ~1));

	    while ((const uint16_t*)tx_data != tx_data_e)
	    {
		if ((SAIx->SR & SAI_xSR_FLVL) == (SAI_xSR_FLVL_2 | SAI_xSR_FLVL_0))
		{
		    break;
		}
		
		SAIx->DR = *((const uint16_t*)tx_data);

		tx_data += 2;
	    }

	    sai->state = SAI_STATE_TRANSMIT_16;

	    sai->xf_callback = stm32l4_sai_transmit_16_callback;
	    sai->xf_data = (void*)tx_data;
	    sai->xf_data_e = (void*)tx_data_e;
	}
	else
	{
	    const uint32_t *tx_data_e = (const uint32_t*)((const uint8_t*)tx_data + (tx_count & ~3));

	    while ((const uint32_t*)tx_data != tx_data_e)
	    {
		if ((SAIx->SR & SAI_xSR_FLVL) == (SAI_xSR_FLVL_2 | SAI_xSR_FLVL_0))
		{
		    break;
		}
		
		SAIx->DR = *((const uint32_t*)tx_data);

		tx_data += 4;
	    }

	    sai->state = SAI_STATE_TRANSMIT_32;

	    sai->xf_callback = stm32l4_sai_transmit_32_callback;
	    sai->xf_data = (void*)tx_data;
	    sai->xf_data_e = (void*)tx_data_e;
	}

	if (!(SAIx->CR1 & SAI_xCR1_SAIEN))
	{
	    SAIx->CR2 = SAI_xCR2_FTH_1;
	    SAIx->CR1 |= SAI_xCR1_SAIEN;

	    SAIx->IMR |= SAI_xIMR_FREQIE;
	}
    }

    return true;
}

bool stm32l4_sai_done(stm32l4_sai_t *sai)
{
    return (sai->state == SAI_STATE_READY);
}

void SAI1_IRQHandler(void)
{
    if (SAI1_Block_A->SR & SAI1_Block_A->IMR)
    {
	stm32l4_sai_interrupt(stm32l4_sai_driver.instances[SAI_INSTANCE_SAI1A]);
    }

    if (SAI1_Block_B->SR & SAI1_Block_B->IMR)
    {
	stm32l4_sai_interrupt(stm32l4_sai_driver.instances[SAI_INSTANCE_SAI1B]);
    }
}

#if defined(STM32L476xx) || defined(STM32L496xx)

void SAI2_IRQHandler(void)
{
    if (SAI2_Block_A->SR & SAI2_Block_A->IMR)
    {
	stm32l4_sai_interrupt(stm32l4_sai_driver.instances[SAI_INSTANCE_SAI2A]);
    }

    if (SAI2_Block_B->SR & SAI2_Block_B->IMR)
    {
	stm32l4_sai_interrupt(stm32l4_sai_driver.instances[SAI_INSTANCE_SAI2B]);
    }
}

#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
