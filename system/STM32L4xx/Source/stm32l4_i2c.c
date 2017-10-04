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
#include "stm32l4_i2c.h"
#include "stm32l4_dma.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_i2c_driver_t {
    stm32l4_i2c_t     *instances[I2C_INSTANCE_COUNT];
} stm32l4_i2c_driver_t;

static stm32l4_i2c_driver_t stm32l4_i2c_driver;

#define I2C_RX_DMA_OPTION \
    (DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define I2C_TX_DMA_OPTION \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

static void stm32l4_i2c_start(stm32l4_i2c_t *i2c)
{
    stm32l4_system_periph_enable(SYSTEM_PERIPH_I2C1 + i2c->instance);

    if (i2c->state != I2C_STATE_BUSY)
    {
	if (i2c->mode & I2C_MODE_RX_DMA)
	{
	    stm32l4_dma_enable(&i2c->rx_dma, NULL, NULL);
	}
	
	if (i2c->mode & I2C_MODE_TX_DMA)
	{
	    stm32l4_dma_enable(&i2c->tx_dma, NULL, NULL);
	}
	
	NVIC_EnableIRQ(i2c->interrupt+1); /* ERR */
	NVIC_EnableIRQ(i2c->interrupt+0); /* EV  */
    }
}

static void stm32l4_i2c_stop(stm32l4_i2c_t *i2c)
{
    if (i2c->state != I2C_STATE_BUSY)
    {
	NVIC_DisableIRQ(i2c->interrupt+0); /* EV  */
	NVIC_DisableIRQ(i2c->interrupt+1); /* ERR */

	if (i2c->mode & I2C_MODE_RX_DMA)
	{
	    stm32l4_dma_disable(&i2c->rx_dma);
	}
	
	if (i2c->mode & I2C_MODE_TX_DMA)
	{
	    stm32l4_dma_disable(&i2c->tx_dma);
	}
    }

    stm32l4_system_periph_disable(SYSTEM_PERIPH_I2C1 + i2c->instance);
}

static void stm32l4_i2c_master_receive(stm32l4_i2c_t *i2c)
{
    I2C_TypeDef *I2C = i2c->I2C;
    uint32_t i2c_cr2, count;

    i2c->state = I2C_STATE_MASTER_RECEIVE;

    i2c->xf_count = 0;

    count = i2c->rx_count;

    i2c_cr2 = (i2c->xf_address << 1) | I2C_CR2_RD_WRN;

    if (count > 255)
    {
	count = 255;

	i2c_cr2 |= I2C_CR2_RELOAD;
    }
    else
    {
	if (!(i2c->xf_control & I2C_CONTROL_RESTART))
	{
	    i2c_cr2 |= I2C_CR2_AUTOEND;
	}
    }

    if (i2c->mode & I2C_MODE_RX_DMA)
    {
	I2C->CR1 |= I2C_CR1_RXDMAEN;

	stm32l4_dma_start(&i2c->rx_dma, (uint32_t)&i2c->rx_data[0], (uint32_t)&I2C->RXDR, count, I2C_RX_DMA_OPTION);

	I2C->CR2 = (i2c_cr2 | I2C_CR2_START | (count << 16));

	I2C->CR1 |= (I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);
    }
    else
    {
	I2C->CR2 = (i2c_cr2 | I2C_CR2_START | (count << 16));

	I2C->CR1 |= (I2C_CR1_RXIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);
    }
}

static void stm32l4_i2c_master_transmit(stm32l4_i2c_t *i2c)
{
    I2C_TypeDef *I2C = i2c->I2C;
    uint32_t i2c_cr2, count;

    i2c->state = I2C_STATE_MASTER_TRANSMIT;

    i2c->xf_count = 0;

    count = i2c->tx_count;

    i2c_cr2 = (i2c->xf_address << 1);

    if (count > 255)
    {
	count = 255;

	i2c_cr2 |= I2C_CR2_RELOAD;
    }
    else
    {
	if (!i2c->rx_data && !(i2c->xf_control & I2C_CONTROL_RESTART))
	{
	    i2c_cr2 |= I2C_CR2_AUTOEND;
	}
    }

    if (i2c->mode & I2C_MODE_TX_DMA)
    {
	stm32l4_dma_start(&i2c->tx_dma, (uint32_t)&I2C->TXDR, (uint32_t)i2c->tx_data, i2c->tx_count, I2C_TX_DMA_OPTION);

	I2C->CR1 |= I2C_CR1_TXDMAEN;

	I2C->ISR |= I2C_ISR_TXE;

	I2C->CR2 = (i2c_cr2 | I2C_CR2_START | (count << 16));

	I2C->CR1 |= (I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);
    }
    else
    {
        I2C->ISR |= I2C_ISR_TXE;


	/* Load TXDR early to avoid the first TXE event interrupt.
	 */

	I2C->TXDR = *(i2c->tx_data)++;
		
	i2c->xf_count++;

	I2C->CR2 = (i2c_cr2 | I2C_CR2_START | (count << 16));

	I2C->CR1 |= (I2C_CR1_TXIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);
    }
}

static uint32_t stm32l4_i2c_slave_address_match(stm32l4_i2c_t *i2c)
{
    I2C_TypeDef *I2C = i2c->I2C;
    uint32_t events;

    events = 0;

    i2c->xf_address = (I2C->ISR >> 17) & 0x7f;

    events |= I2C_EVENT_ADDRESS_MATCH;
    
    if (I2C->ISR & I2C_ISR_DIR)
    {
	I2C->CR1 |= (I2C_CR1_TXIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE);
	
	i2c->state = I2C_STATE_SLAVE_TRANSMIT;
	
	events |= I2C_EVENT_TRANSMIT_REQUEST;
    }
    else
    {
	I2C->CR1 |= (I2C_CR1_RXIE | I2C_CR1_STOPIE);
	
	i2c->state = I2C_STATE_SLAVE_RECEIVE;
	
	events |= I2C_EVENT_RECEIVE_REQUEST;
    }
    
    events &= i2c->events;
    
    if (events)
    {
	(*i2c->callback)(i2c->context, events);
    }

    events = 0;

    I2C->ICR = I2C_ICR_ADDRCF;
    
    if (i2c->state == I2C_STATE_SLAVE_TRANSMIT)
    {
	I2C->ISR |= I2C_ISR_TXE;

	I2C->TXDR = *(i2c->tx_data)++;
    }
    
    return events;
}

static void stm32l4_i2c_event_interrupt(stm32l4_i2c_t *i2c)
{
    I2C_TypeDef *I2C = i2c->I2C;
    uint32_t i2c_cr2, i2c_isr, count, events;

    events = 0;

    i2c_isr = I2C->ISR;

    switch (i2c->state) {

    case I2C_STATE_NONE:
    case I2C_STATE_BUSY:
	break;

    case I2C_STATE_READY:
	if (i2c_isr & I2C_ISR_ADDR)
	{
	    stm32l4_i2c_slave_address_match(i2c);
	}
	break;

    case I2C_STATE_SLAVE_RECEIVE:
	if (i2c_isr & I2C_ISR_RXNE)
	{
	    *(i2c->rx_data)++ = I2C->RXDR;
	    
	    i2c->xf_count++;
	    
	    if (i2c->xf_count == i2c->rx_count)
	    {
		(*i2c->callback)(i2c->context, I2C_EVENT_RECEIVE_REQUEST);
	    }
	}
	else
	{
	    /* A slave transfer is terminated either by STOP, or by a repeated
	     * start, i.e. an ADDR match.
	     */
	    if (i2c_isr & (I2C_ISR_ADDR | I2C_ISR_STOPF))
	    {
		I2C->CR1 &= ~(I2C_CR1_RXIE | I2C_CR1_STOPIE);

		if (i2c_isr & I2C_ISR_STOPF)
		{
		    I2C->ICR = I2C_ICR_STOPCF;
		
		    i2c->state = I2C_STATE_READY;
		
		    events |= I2C_EVENT_RECEIVE_DONE;
		}
		else
		{
		    if (i2c->events & I2C_EVENT_RECEIVE_DONE)
		    {
			(*i2c->callback)(i2c->context, I2C_EVENT_RECEIVE_DONE);
		    }
		    
		    events = stm32l4_i2c_slave_address_match(i2c);
		}
	    }
	}
	break;

    case I2C_STATE_SLAVE_TRANSMIT:
	/* A slave transfer is terminated by a NACK of the master receive. If a
	 * STOP or a ADDR zips by, it's due to a bus reset ...
	 */
	if (i2c_isr & (I2C_ISR_ADDR | I2C_ISR_NACKF | I2C_ISR_STOPF))
	{
	    I2C->CR1 &= ~(I2C_CR1_TXIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE);

	    if (i2c_isr & I2C_ISR_ADDR)
	    {
		if (i2c->events & I2C_EVENT_TRANSMIT_DONE)
		{
		    (*i2c->callback)(i2c->context, I2C_EVENT_TRANSMIT_DONE);
		}

		events = stm32l4_i2c_slave_address_match(i2c);
	    }
	    else
	    {
		I2C->ICR = (I2C_ICR_NACKCF | I2C_ICR_STOPCF);

		i2c->state = I2C_STATE_READY;
		
		events |= I2C_EVENT_TRANSMIT_DONE;
	    }
	}
	else 
	{
	    if (i2c_isr & I2C_ISR_TXE)
	    {
		i2c->xf_count++;
	
		if (i2c->xf_count == i2c->tx_count)
		{
		    (*i2c->callback)(i2c->context, I2C_EVENT_TRANSMIT_REQUEST);
		}

		I2C->TXDR = *(i2c->tx_data)++;
	    }
	}
	break;

    case I2C_STATE_MASTER_RESTART:
	break;

    case I2C_STATE_MASTER_RECEIVE:
	if (i2c_isr & I2C_ISR_RXNE)
	{
	    *(i2c->rx_data)++ = I2C->RXDR;
	    
	    i2c->xf_count++;
	}
	else if (i2c_isr & I2C_ISR_TCR)
	{
	    i2c_cr2 = (i2c->xf_address << 1);
	    
	    count = (i2c->rx_count - i2c->xf_count);
	    
	    if (count > 255)
	    {
		count = 255;
		
		i2c_cr2 |= I2C_CR2_RELOAD;
	    }
	    else
	    {
		if (!(i2c->xf_control & I2C_CONTROL_RESTART))
		{
		    i2c_cr2 |= I2C_CR2_AUTOEND;
		}
	    }
	    
	    I2C->CR2 = (i2c_cr2 | I2C_CR2_START | I2C_CR2_RD_WRN | (count << 16));
	}
	else if (i2c_isr & (I2C_ISR_TC | I2C_ISR_STOPF))
	{
	    I2C->ICR = I2C_ICR_STOPCF;

	    if (i2c->mode & I2C_MODE_RX_DMA)
	    {
		I2C->CR1 &= ~(I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE | I2C_CR1_RXDMAEN);

		i2c->xf_count = stm32l4_dma_stop(&i2c->rx_dma);
	    }
	    else
	    {
		I2C->CR1 &= ~(I2C_CR1_RXIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);
	    }

	    if (i2c->xf_status)
	    {
		events |= i2c->xf_status;

		events |= I2C_EVENT_RECEIVE_ERROR;

		if (!(i2c->option & I2C_OPTION_ADDRESS_MASK))
		{
		    stm32l4_i2c_stop(i2c);
		}
		
		i2c->state = I2C_STATE_READY;
	    }
	    else
	    {
		if (i2c->tx_data)
		{
		    events |= I2C_EVENT_TRANSFER_DONE;
		}
		else
		{
		    events |= I2C_EVENT_RECEIVE_DONE;
		}

		if (i2c_isr & I2C_ISR_TC)
		{
		    i2c->state = I2C_STATE_MASTER_RESTART;
		}
		else
		{
		    if (!(i2c->option & I2C_OPTION_ADDRESS_MASK))
		    {
			stm32l4_i2c_stop(i2c);
		    }
		    
		    i2c->state = I2C_STATE_READY;
		}
	    }
	}
	else if (i2c_isr & I2C_ISR_NACKF)
	{
	    I2C->ICR = I2C_ICR_NACKCF;

	    i2c->xf_status |= I2C_STATUS_ADDRESS_NACK;

	    if (i2c->xf_control & I2C_CONTROL_RESTART)
	    {
		/* If a NACK occured during a restart operation,
		 * send a STOP to properly terminate the sequence.
		 */
		I2C->CR2 |= I2C_CR2_STOP;
	    }
	}
	break;

    case I2C_STATE_MASTER_TRANSMIT:
	if (i2c_isr & (I2C_ISR_TC | I2C_ISR_NACKF | I2C_ISR_STOPF))
	{
	    if (i2c_isr & I2C_ISR_NACKF)
	    {
		I2C->ICR = I2C_ICR_NACKCF;

		if (i2c->xf_count == 0)
		{
		    i2c->xf_status |= I2C_STATUS_ADDRESS_NACK;
		}
		else
		{
		    i2c->xf_status |= I2C_STATUS_DATA_NACK;
		}

		if (i2c->rx_data || (i2c->xf_control & I2C_CONTROL_RESTART))
		{
		    /* If a NACK occured during a restart operation,
		     * send a STOP to properly terminate the sequence.
		     */
		    I2C->CR2 |= I2C_CR2_STOP;
		}
	    }
	    else
	    {
		I2C->ICR = I2C_ICR_STOPCF;

		if (i2c->mode & I2C_MODE_TX_DMA)
		{
		    I2C->CR1 &= ~(I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE | I2C_CR1_TXDMAEN);

		    i2c->xf_count = stm32l4_dma_stop(&i2c->tx_dma);
		}
		else
		{
		    I2C->CR1 &= ~(I2C_CR1_TXIE | I2C_CR1_NACKIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);
		}

		if (i2c->xf_status)
		{
		    events |= i2c->xf_status;

		    events |= I2C_EVENT_TRANSMIT_ERROR;

		    if (!(i2c->option & I2C_OPTION_ADDRESS_MASK))
		    {
			stm32l4_i2c_stop(i2c);
		    }
		    
		    i2c->state = I2C_STATE_READY;
		}
		else
		{
		    if (i2c->rx_data)
		    {
			stm32l4_i2c_master_receive(i2c);
		    }
		    else
		    {
			events |= I2C_EVENT_TRANSMIT_DONE;

			if (i2c_isr & I2C_ISR_TC)
			{
			    i2c->state = I2C_STATE_MASTER_RESTART;
			}
			else
			{
			    if (!(i2c->option & I2C_OPTION_ADDRESS_MASK))
			    {
				stm32l4_i2c_stop(i2c);
			    }
			    
			    i2c->state = I2C_STATE_READY;
			}
		    }
		}
	    }
	}
	else
	{
	    if (i2c_isr & I2C_ISR_TXE)
	    {
		I2C->TXDR = *(i2c->tx_data)++;
		
		i2c->xf_count++;
	    }
	    else if (i2c_isr & I2C_ISR_TCR)
	    {
		i2c_cr2 = (i2c->xf_address << 1);
		
		count = i2c->tx_count - i2c->xf_count;
		
		if (count > 255)
		{
		    count = 255;
		    
		    i2c_cr2 |= I2C_CR2_RELOAD;
		}
		else
		{
		    if (!i2c->rx_data && !(i2c->xf_control & I2C_CONTROL_RESTART))
		    {
			i2c_cr2 |= I2C_CR2_AUTOEND;
		    }
		}
		
		I2C->CR2 = (i2c_cr2 | I2C_CR2_START | (count << 16));
	    }
	}
	break;
    }

    events &= i2c->events;

    if (events)
    {
	(*i2c->callback)(i2c->context, events);
    }
}

static void stm32l4_i2c_error_interrupt(stm32l4_i2c_t *i2c)
{
    I2C_TypeDef *I2C = i2c->I2C;
    uint32_t i2c_isr, events;

    events = 0;

    i2c_isr = I2C->ISR;

    if (i2c_isr & I2C_ISR_BERR)
    {
	I2C->ICR = I2C_ICR_BERRCF;
    }

    if (i2c_isr & I2C_ISR_ARLO)
    {
	I2C->ICR = I2C_ICR_ARLOCF;
    }

    if (i2c_isr & I2C_ISR_OVR)
    {
	I2C->ICR = I2C_ICR_OVRCF;
    }

    switch (i2c->state) {
	
    case I2C_STATE_NONE:
    case I2C_STATE_BUSY:
    case I2C_STATE_READY:
	break;

    case I2C_STATE_SLAVE_RECEIVE:
    case I2C_STATE_SLAVE_TRANSMIT:
	if (i2c_isr & (I2C_ISR_BERR | I2C_ISR_OVR))
	{
	    if (i2c_isr & I2C_ISR_BERR)
	    {
		i2c->xf_status |= I2C_STATUS_BUS_ERROR;
	    }

	    if (i2c_isr & I2C_ISR_OVR)
	    {
		i2c->xf_status |= I2C_STATUS_OVERRUN;
	    }
	}
	break;

    case I2C_STATE_MASTER_RESTART:
	break;

    case I2C_STATE_MASTER_RECEIVE:
    case I2C_STATE_MASTER_TRANSMIT:
	/* Silicon ERRATA 2.6.4. There is a possible random BUSERR in master mode.
	 * WAR is to ignore it.
	 */
	if (i2c_isr & I2C_ISR_ARLO)
	{
	    i2c->xf_status |= I2C_STATUS_ARBITRATION_LOST;

	    events |= I2C_EVENT_ARBITRATION_LOST;

	    /* Abort the current transaction.
	     */

	    if (i2c->state == I2C_STATE_MASTER_RECEIVE)
	    {
		events |= I2C_EVENT_RECEIVE_ERROR;
	    }
	    else
	    {
		events |= I2C_EVENT_TRANSMIT_ERROR;
	    }

	    if (!(i2c->option & I2C_OPTION_ADDRESS_MASK))
	    {
		stm32l4_i2c_stop(i2c);
	    }

	    i2c->state = I2C_STATE_READY;
	}
	break;
    }

    events &= i2c->events;

    if (events)
    {
	(*i2c->callback)(i2c->context, events);
    }
}

bool stm32l4_i2c_create(stm32l4_i2c_t *i2c, unsigned int instance, const stm32l4_i2c_pins_t *pins, unsigned int priority, unsigned int mode)
{
    i2c->state = I2C_STATE_INIT;
    i2c->instance = instance;

    switch (instance) {
    case I2C_INSTANCE_I2C1:
	i2c->I2C = I2C1;
	i2c->interrupt = I2C1_EV_IRQn;
	break;

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    case I2C_INSTANCE_I2C2:
	i2c->I2C = I2C2;
	i2c->interrupt = I2C2_EV_IRQn;
	break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */

    case I2C_INSTANCE_I2C3:
	i2c->I2C = I2C3;
	i2c->interrupt = I2C3_EV_IRQn;
	break;

#if defined(STM32L496xx)
    case I2C_INSTANCE_I2C4:
	i2c->I2C = I2C4;
	i2c->interrupt = I2C4_EV_IRQn;
	break;
#endif /* defined(STM32L496xx) */

    default:
	i2c->state = I2C_STATE_NONE;
	return false;
    }

    i2c->priority = priority;
    i2c->pins = *pins;

    i2c->mode = mode & ~(I2C_MODE_RX_DMA | I2C_MODE_TX_DMA);

    if (mode & I2C_MODE_RX_DMA)
    {
	switch (i2c->instance) {
	case I2C_INSTANCE_I2C1:
	    if ((!(mode & I2C_MODE_RX_DMA_SECONDARY) && stm32l4_dma_create(&i2c->rx_dma, DMA_CHANNEL_DMA1_CH7_I2C1_RX, i2c->priority)) ||
		stm32l4_dma_create(&i2c->rx_dma, DMA_CHANNEL_DMA2_CH6_I2C1_RX, i2c->priority))
	    {
		i2c->mode |= I2C_MODE_RX_DMA;
	    }
	    break;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	case I2C_INSTANCE_I2C2:
	    if (stm32l4_dma_create(&i2c->rx_dma, DMA_CHANNEL_DMA1_CH5_I2C2_RX, i2c->priority)) { i2c->mode |= I2C_MODE_RX_DMA; }
	    break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
	case I2C_INSTANCE_I2C3:
	    if (stm32l4_dma_create(&i2c->rx_dma, DMA_CHANNEL_DMA1_CH3_I2C3_RX, i2c->priority)) { i2c->mode |= I2C_MODE_RX_DMA; }
	    break;
#if defined(STM32L496xx)
	case I2C_INSTANCE_I2C4:
	    if (stm32l4_dma_create(&i2c->rx_dma, DMA_CHANNEL_DMA2_CH1_I2C4_RX, i2c->priority)) { i2c->mode |= I2C_MODE_RX_DMA; }
	    break;
#endif /* defined(STM32L496xx) */
	}
    }

    if (mode & I2C_MODE_TX_DMA)
    {
	switch (i2c->instance) {
	case I2C_INSTANCE_I2C1:
	    if ((!(mode & I2C_MODE_TX_DMA_SECONDARY) && stm32l4_dma_create(&i2c->tx_dma, DMA_CHANNEL_DMA1_CH6_I2C1_TX, i2c->priority)) ||
		stm32l4_dma_create(&i2c->tx_dma, DMA_CHANNEL_DMA2_CH7_I2C1_TX, i2c->priority))
	    {
		i2c->mode |= I2C_MODE_TX_DMA;
	    }
	    break;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	case I2C_INSTANCE_I2C2:
	    if (stm32l4_dma_create(&i2c->tx_dma, DMA_CHANNEL_DMA1_CH4_I2C2_TX, i2c->priority)) { i2c->mode |= I2C_MODE_TX_DMA; }
	    break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
	case I2C_INSTANCE_I2C3:
	    if (stm32l4_dma_create(&i2c->tx_dma, DMA_CHANNEL_DMA1_CH2_I2C3_TX, i2c->priority)) { i2c->mode |= I2C_MODE_TX_DMA; }
	    break;
#if defined(STM32L496xx)
	case I2C_INSTANCE_I2C4:
	    if (stm32l4_dma_create(&i2c->tx_dma, DMA_CHANNEL_DMA2_CH2_I2C4_TX, i2c->priority)) { i2c->mode |= I2C_MODE_TX_DMA; }
	    break;
#endif /* defined(STM32L496xx) */
	}
    }

    if ((i2c->mode ^ mode) & (I2C_MODE_RX_DMA | I2C_MODE_TX_DMA))
    {
	if (i2c->mode & I2C_MODE_RX_DMA)
	{
	    stm32l4_dma_destroy(&i2c->rx_dma);
			
	    i2c->mode &= ~I2C_MODE_RX_DMA;
	}

	if (i2c->mode & I2C_MODE_TX_DMA)
	{
	    stm32l4_dma_destroy(&i2c->tx_dma);
			
	    i2c->mode &= ~I2C_MODE_TX_DMA;
	}

	i2c->state = I2C_STATE_NONE;
	return false;
    }

    stm32l4_i2c_driver.instances[instance] = i2c;

    return true;
}

bool stm32l4_i2c_destroy(stm32l4_i2c_t *i2c)
{
    if (i2c->state != I2C_STATE_INIT)
    {
	return false;
    }

    stm32l4_i2c_driver.instances[i2c->instance] = NULL;

    if (i2c->mode & I2C_MODE_RX_DMA)
    {
	stm32l4_dma_destroy(&i2c->rx_dma);
	
	i2c->mode &= ~I2C_MODE_RX_DMA;
    }
    
    if (i2c->mode & I2C_MODE_TX_DMA)
    {
	stm32l4_dma_destroy(&i2c->tx_dma);
	
	i2c->mode &= ~I2C_MODE_TX_DMA;
    }

    i2c->state = I2C_STATE_NONE;

    return true;
}

bool stm32l4_i2c_enable(stm32l4_i2c_t *i2c, uint32_t clock, uint32_t option, stm32l4_i2c_callback_t callback, void *context, uint32_t events)
{
    if (i2c->state != I2C_STATE_INIT)
    {
	return false;
    }

    i2c->callback = NULL;
    i2c->context = NULL;
    i2c->events = 0x00000000;

    stm32l4_system_hsi16_enable();

    switch (i2c->instance) {
    case I2C_INSTANCE_I2C1:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_I2C1SEL, RCC_CCIPR_I2C1SEL_1); /* HSI */
	break;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    case I2C_INSTANCE_I2C2:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_I2C2SEL, RCC_CCIPR_I2C2SEL_1); /* HSI */
	break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
    case I2C_INSTANCE_I2C3:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_I2C3SEL, RCC_CCIPR_I2C3SEL_1); /* HSI */
	break;
#if defined(STM32L496xx)
    case I2C_INSTANCE_I2C4:
	armv7m_atomic_modify(&RCC->CCIPR2, RCC_CCIPR2_I2C4SEL, RCC_CCIPR2_I2C4SEL_1); /* HSI */
	break;
#endif /* defined(STM32L496xx) */
    }

    NVIC_SetPriority(i2c->interrupt+0, i2c->priority); /* EV  */
    NVIC_SetPriority(i2c->interrupt+1, i2c->priority); /* ERR */

    i2c->state = I2C_STATE_BUSY;

    if (!stm32l4_i2c_configure(i2c, clock, option))
    {
	i2c->state = I2C_STATE_INIT;

	return false;
    }

    stm32l4_i2c_notify(i2c, callback, context, events);

    i2c->state = I2C_STATE_READY;

    return true;
}

bool stm32l4_i2c_disable(stm32l4_i2c_t *i2c)
{
    uint32_t pin_scl, pin_sda;

    if (i2c->state != I2C_STATE_READY)
    {
	return false;
    }

    if (i2c->option & I2C_OPTION_ADDRESS_MASK)
    {
	stm32l4_i2c_stop(i2c);
    }

    stm32l4_system_hsi16_disable();

    pin_scl = i2c->pins.scl;
    pin_sda = i2c->pins.sda;

#if defined(STM32L433xx)
    if ((i2c->option & I2C_OPTION_ALTERNATE) && (pin_scl == GPIO_PIN_PB6_I2C1_SCL) && (pin_sda == GPIO_PIN_PB7_I2C1_SDA))
    {
	pin_scl = GPIO_PIN_PB8_I2C1_SCL;
	pin_sda = GPIO_PIN_PB9_I2C1_SDA;
    }
#endif /* defined(STM32L433xx) */

#if defined(STM32L476xx) ||  defined(STM32L496xx)
    if ((i2c->option & I2C_OPTION_ALTERNATE) && (pin_scl == GPIO_PIN_PB8_I2C1_SCL) && (pin_sda == GPIO_PIN_PB9_I2C1_SDA))
    {
	pin_scl = GPIO_PIN_PB6_I2C1_SCL;
	pin_sda = GPIO_PIN_PB7_I2C1_SDA;
    }
#endif /* defined(STM32L476xx) ||  defined(STM32L496xx) */

    stm32l4_gpio_pin_configure(pin_scl, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    stm32l4_gpio_pin_configure(pin_sda, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));

    i2c->state = I2C_STATE_INIT;

    return true;
}

bool stm32l4_i2c_configure(stm32l4_i2c_t *i2c, uint32_t clock, uint32_t option)
{
    I2C_TypeDef *I2C = i2c->I2C;
    uint32_t pin_scl, pin_sda, i2c_cr1, i2c_cr2, i2c_oar1, i2c_oar2, i2c_timingr, syscfg_cfgr1;

    if ((i2c->state != I2C_STATE_READY) && (i2c->state != I2C_STATE_BUSY))
    {
	return false;
    }

    if      (clock >= 1000000) { clock = 1000000; }
    else if (clock >=  400000) { clock =  400000; }
    else if (clock >=  100000) { clock =  100000; }
    else                       { clock =   10000; }

    pin_scl = i2c->pins.scl;
    pin_sda = i2c->pins.sda;

#if defined(STM32L476xx)
    /* Silicon ERRATA 2.6.1. FM+ is not working on all pins.
     */
    if ((clock == 1000000) &&
	((pin_scl == GPIO_PIN_PB10_I2C2_SCL) || (pin_scl == GPIO_PIN_PF1_I2C2_SCL) || (pin_scl == GPIO_PIN_PG14_I2C1_SCL) ||
	 (pin_sda == GPIO_PIN_PB11_I2C2_SDA) || (pin_sda == GPIO_PIN_PF0_I2C2_SDA) || (pin_sda == GPIO_PIN_PG13_I2C1_SDA)))
    {
	return false;
    }
#endif /* defined(STM32L476xx) */

#if defined(STM32L433xx)
    if ((i2c->option & I2C_OPTION_ALTERNATE) && (pin_scl == GPIO_PIN_PB6_I2C1_SCL) && (pin_sda == GPIO_PIN_PB7_I2C1_SDA))
    {
	pin_scl = GPIO_PIN_PB8_I2C1_SCL;
	pin_sda = GPIO_PIN_PB9_I2C1_SDA;
    }
#endif /* defined(STM32L433xx) */

#if defined(STM32L476xx) ||  defined(STM32L496xx)
    if ((i2c->option & I2C_OPTION_ALTERNATE) && (pin_scl == GPIO_PIN_PB8_I2C1_SCL) && (pin_sda == GPIO_PIN_PB9_I2C1_SDA))
    {
	pin_scl = GPIO_PIN_PB6_I2C1_SCL;
	pin_sda = GPIO_PIN_PB7_I2C1_SDA;
    }
#endif /* defined(STM32L476xx) ||  defined(STM32L496xx) */

    i2c->clock = clock;
    i2c->option = option;

    stm32l4_i2c_start(i2c);
	
    I2C->CR1 = 0;
    
    i2c_cr1 = 0;
    i2c_cr2 = 0;
    i2c_oar1 = 0;
    i2c_oar2 = 0;
    i2c_timingr = 0;

    /*
     * This is from RM0351 for 16MHz, i.e. HSI.
     */
    
    switch (i2c->clock) {
    case   10000: i2c_timingr = 0x3042c3c7; break;
    case  100000: i2c_timingr = 0x30420f13; break;
    case  400000: i2c_timingr = 0x10320309; break;
    case 1000000: i2c_timingr = 0x00200106; break;
    }
    
    if (i2c->option & I2C_OPTION_ADDRESS_MASK)
    {
	i2c_oar1 = I2C_OAR1_OA1EN | (((i2c->option & I2C_OPTION_ADDRESS_MASK) >> I2C_OPTION_ADDRESS_SHIFT) << 1);
	
	if (i2c->option & I2C_OPTION_GENERAL_CALL)
	{
	    i2c_cr1 |= I2C_CR1_GCEN;
	}
	
	i2c_cr1 |=  I2C_CR1_ADDRIE;
    }

    if (i2c->option & I2C_OPTION_WAKEUP)
    {
	i2c_cr1 |= I2C_CR1_WUPEN;
    }
    
    i2c_cr1 |=  I2C_CR1_ERRIE;
    
    syscfg_cfgr1 = 0;
    
    switch (i2c->instance) {
    case I2C_INSTANCE_I2C1:
	syscfg_cfgr1 = SYSCFG_CFGR1_I2C1_FMP;
	
	if (pin_scl == GPIO_PIN_PB6_I2C1_SCL)
	{
	    syscfg_cfgr1 |= SYSCFG_CFGR1_I2C_PB6_FMP;
	}
	
	if (pin_sda == GPIO_PIN_PB7_I2C1_SDA)
	{
	    syscfg_cfgr1 |= SYSCFG_CFGR1_I2C_PB7_FMP;
	}
	
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	if (pin_scl == GPIO_PIN_PB8_I2C1_SCL)
	{
	    syscfg_cfgr1 |= SYSCFG_CFGR1_I2C_PB8_FMP;
	}
	
	if (pin_sda == GPIO_PIN_PB9_I2C1_SDA)
	{
	    syscfg_cfgr1 |= SYSCFG_CFGR1_I2C_PB9_FMP;
	}
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
	break;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    case I2C_INSTANCE_I2C2:
	syscfg_cfgr1 = SYSCFG_CFGR1_I2C2_FMP;
	break;
#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */
    case I2C_INSTANCE_I2C3:
	syscfg_cfgr1 = SYSCFG_CFGR1_I2C3_FMP;
	break;
#if defined(STM32L496xx)
    case I2C_INSTANCE_I2C4:
	syscfg_cfgr1 = SYSCFG_CFGR1_I2C4_FMP;
	break;
#endif /* defined(STM32L496xx) */
    }

    if (i2c->clock == 1000000)
    {
	armv7m_atomic_or(&SYSCFG->CFGR1, syscfg_cfgr1);
    }
    else
    {
	armv7m_atomic_and(&SYSCFG->CFGR1, ~syscfg_cfgr1);
    }

    I2C->TIMINGR = i2c_timingr;
    I2C->OAR2 = i2c_oar2;
    I2C->OAR1 = i2c_oar1;
    I2C->CR2 = i2c_cr2;
    I2C->CR1 = i2c_cr1 | I2C_CR1_PE;

    if ((i2c->state == I2C_STATE_BUSY) && (i2c->option & I2C_OPTION_RESET))
    {
	stm32l4_i2c_reset(i2c);
    }
    else
    {
	stm32l4_gpio_pin_configure(pin_scl, (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_OPENDRAIN | GPIO_MODE_ALTERNATE));
	stm32l4_gpio_pin_configure(pin_sda, (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_OPENDRAIN | GPIO_MODE_ALTERNATE));
    }

    if (!(i2c->option & I2C_OPTION_ADDRESS_MASK))
    {
	stm32l4_i2c_stop(i2c);
    }

    return true;
}

bool stm32l4_i2c_notify(stm32l4_i2c_t *i2c, stm32l4_i2c_callback_t callback, void *context, uint32_t events)
{
    if ((i2c->state != I2C_STATE_READY) && (i2c->state != I2C_STATE_BUSY))
    {
	return false;
    }

    NVIC_DisableIRQ(i2c->interrupt+0); /* EV  */
    NVIC_DisableIRQ(i2c->interrupt+1); /* ERR */

    i2c->callback = callback;
    i2c->context = context;
    i2c->events = callback ? events : 0x00000000;

    NVIC_EnableIRQ(i2c->interrupt+1); /* ERR */
    NVIC_EnableIRQ(i2c->interrupt+0); /* EV  */

    return true;
}

bool stm32l4_i2c_reset(stm32l4_i2c_t *i2c)
{
    uint32_t pin_scl, pin_sda, count;

    if ((i2c->state != I2C_STATE_READY) && (i2c->state != I2C_STATE_BUSY))
    {
	return false;
    }

    pin_scl = i2c->pins.scl;
    pin_sda = i2c->pins.sda;

#if defined(STM32L433xx)
    if ((i2c->option & I2C_OPTION_ALTERNATE) && (pin_scl == GPIO_PIN_PB6_I2C1_SCL) && (pin_sda == GPIO_PIN_PB7_I2C1_SDA))
    {
	pin_scl = GPIO_PIN_PB8_I2C1_SCL;
	pin_sda = GPIO_PIN_PB9_I2C1_SDA;
    }
#endif /* defined(STM32L433xx) */

#if defined(STM32L476xx) ||  defined(STM32L496xx)
    if ((i2c->option & I2C_OPTION_ALTERNATE) && (pin_scl == GPIO_PIN_PB8_I2C1_SCL) && (pin_sda == GPIO_PIN_PB9_I2C1_SDA))
    {
	pin_scl = GPIO_PIN_PB6_I2C1_SCL;
	pin_sda = GPIO_PIN_PB7_I2C1_SDA;
    }
#endif /* defined(STM32L476xx) ||  defined(STM32L496xx) */

    stm32l4_gpio_pin_configure(pin_scl, (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_OPENDRAIN | GPIO_MODE_OUTPUT));
    stm32l4_gpio_pin_configure(pin_sda, (GPIO_PUPD_PULLUP | GPIO_MODE_INPUT));
    
    /* Clock 9 SCL cycles to force a slave to release SDA and then issue a manul STOP condition.
     */
    
    /* Set SCL to H */ 
    stm32l4_gpio_pin_write(pin_scl, 1);
    armv7m_core_udelay(5);    
    
    for (count = 0; count < 9; count++)
    {
	/* Set SCL to L */ 
	stm32l4_gpio_pin_write(pin_scl, 0);
	armv7m_core_udelay(5);    
	
	/* Set SCL to H */ 
	stm32l4_gpio_pin_write(pin_scl, 1);
	armv7m_core_udelay(5);    
    }
    
    stm32l4_gpio_pin_configure(pin_sda, (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_OPENDRAIN | GPIO_MODE_OUTPUT));
    
    stm32l4_gpio_pin_write(pin_sda, 1);
    armv7m_core_udelay(5);    

    
    /* Now SCL is H and SDA is H, so generate a STOP condition.
     */
    
    /* Set SCL to L */ 
    stm32l4_gpio_pin_write(pin_scl, 0);
    armv7m_core_udelay(5);    
    
    /* Set SDA to L */ 
    stm32l4_gpio_pin_write(pin_sda, 0);
    armv7m_core_udelay(5);    
    
    /* Set SCL to H */ 
    stm32l4_gpio_pin_write(pin_scl, 1);
    armv7m_core_udelay(5);    
    
    /* Set SDA to H */ 
    stm32l4_gpio_pin_write(pin_sda, 1);
    armv7m_core_udelay(5);    

    stm32l4_gpio_pin_configure(pin_scl, (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_OPENDRAIN | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(pin_sda, (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_OPENDRAIN | GPIO_MODE_ALTERNATE));

    return true;
}

bool stm32l4_i2c_receive(stm32l4_i2c_t *i2c, uint16_t address, uint8_t *rx_data, uint16_t rx_count, uint32_t control)
{
    if ((i2c->state != I2C_STATE_READY) && (i2c->state != I2C_STATE_MASTER_RESTART))
    {
	return false;
    }

    i2c->xf_address = address;
    i2c->xf_control = control;
    i2c->xf_status = 0;
    i2c->tx_data = NULL;
    i2c->tx_count = 0;
    i2c->rx_data = rx_data;
    i2c->rx_count = rx_count;

    if ((i2c->state == I2C_STATE_READY) && !(i2c->option & I2C_OPTION_ADDRESS_MASK))
    {
	stm32l4_i2c_start(i2c);
    }

    stm32l4_i2c_master_receive(i2c);

    return true;
}

bool stm32l4_i2c_transmit(stm32l4_i2c_t *i2c, uint16_t address, const uint8_t *tx_data, uint16_t tx_count, uint32_t control)
{
    if ((i2c->state != I2C_STATE_READY) && (i2c->state != I2C_STATE_MASTER_RESTART))
    {
	return false;
    }

    i2c->xf_address = address;
    i2c->xf_control = control;
    i2c->xf_status = 0;
    i2c->tx_data = tx_data;
    i2c->tx_count = tx_count;
    i2c->rx_data = NULL;
    i2c->rx_count = 0;

    if ((i2c->state == I2C_STATE_READY) && !(i2c->option & I2C_OPTION_ADDRESS_MASK))
    {
	stm32l4_i2c_start(i2c);
    }

    stm32l4_i2c_master_transmit(i2c);

    return true;
}

bool stm32l4_i2c_transfer(stm32l4_i2c_t *i2c, uint16_t address, const uint8_t *tx_data, uint16_t tx_count, uint8_t *rx_data, uint16_t rx_count, uint32_t control)
{
    if ((i2c->state != I2C_STATE_READY) && (i2c->state != I2C_STATE_MASTER_RESTART))
    {
	return false;
    }

    i2c->xf_address = address;
    i2c->xf_control = control;
    i2c->xf_status = 0;
    i2c->tx_data = tx_data;
    i2c->tx_count = tx_count;
    i2c->rx_data = rx_data;
    i2c->rx_count = rx_count;

    if ((i2c->state == I2C_STATE_READY) && !(i2c->option & I2C_OPTION_ADDRESS_MASK))
    {
	stm32l4_i2c_start(i2c);
    }

    stm32l4_i2c_master_transmit(i2c);

    return true;
}

bool stm32l4_i2c_service(stm32l4_i2c_t *i2c, uint8_t *xf_data, uint16_t xf_count)
{
    if (!((i2c->state == I2C_STATE_SLAVE_RECEIVE) || (i2c->state == I2C_STATE_SLAVE_TRANSMIT)))
    {
	return false;
    }

    i2c->xf_count = 0;

    if (i2c->state == I2C_STATE_SLAVE_RECEIVE)
    {
	i2c->rx_data = xf_data;
	i2c->rx_count = xf_count;
    }
    else
    {
	i2c->tx_data = xf_data;
	i2c->tx_count = xf_count;
    }

    return true;
}

bool stm32l4_i2c_done(stm32l4_i2c_t *i2c)
{
    return (i2c->state <= I2C_STATE_MASTER_RESTART);
}

unsigned int stm32l4_i2c_address(stm32l4_i2c_t *i2c)
{
    return i2c->xf_address;
}

unsigned int stm32l4_i2c_count(stm32l4_i2c_t *i2c)
{
    return i2c->xf_count;
}

unsigned int stm32l4_i2c_status(stm32l4_i2c_t *i2c)
{
    unsigned int status;

    status = i2c->xf_status;

    return status;
}

void stm32l4_i2c_poll(stm32l4_i2c_t *i2c)
{
    if (i2c->state >= I2C_STATE_READY)
    {
	if (i2c->mode & I2C_MODE_TX_DMA)
	{
	    stm32l4_dma_poll(&i2c->tx_dma);
	}

	if (i2c->mode & I2C_MODE_RX_DMA)
	{
	    stm32l4_dma_poll(&i2c->rx_dma);
	}

	stm32l4_i2c_error_interrupt(i2c);
	stm32l4_i2c_event_interrupt(i2c);
    }
}

void I2C1_EV_IRQHandler(void)
{
    stm32l4_i2c_event_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C1]);
}

void I2C1_ER_IRQHandler(void)
{
    stm32l4_i2c_error_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C1]);
}

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)

void I2C2_EV_IRQHandler(void)
{
    stm32l4_i2c_event_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C2]);
}

void I2C2_ER_IRQHandler(void)
{
    stm32l4_i2c_error_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C2]);
}

#endif /* defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx) */

void I2C3_EV_IRQHandler(void)
{
    stm32l4_i2c_event_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C3]);
}

void I2C3_ER_IRQHandler(void)
{
    stm32l4_i2c_error_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C3]);
}

#if defined(STM32L496xx)

void I2C4_EV_IRQHandler(void)
{
    stm32l4_i2c_event_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C4]);
}

void I2C4_ER_IRQHandler(void)
{
    stm32l4_i2c_error_interrupt(stm32l4_i2c_driver.instances[I2C_INSTANCE_I2C4]);
}

#endif /* defined(STM32L496xx) */
