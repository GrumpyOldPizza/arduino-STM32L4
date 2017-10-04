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
#include <string.h>

#include "stm32l4xx.h"

#include "armv7m.h"

#include "stm32l4_gpio.h"
#include "stm32l4_dma.h"
#include "stm32l4_uart.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_uart_driver_t {
    stm32l4_uart_t     *instances[UART_INSTANCE_COUNT];
} stm32l4_uart_driver_t;

static stm32l4_uart_driver_t stm32l4_uart_driver;

#define UART_RX_DATA_NONE 0x8000

#define UART_RX_DMA_OPTION		  \
    (DMA_OPTION_EVENT_TRANSFER_DONE |     \
     DMA_OPTION_EVENT_TRANSFER_HALF |     \
     DMA_OPTION_CIRCULAR |		  \
     DMA_OPTION_PERIPHERAL_TO_MEMORY |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_32 | \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

#define UART_TX_DMA_OPTION		  \
    (DMA_OPTION_MEMORY_TO_PERIPHERAL |	  \
     DMA_OPTION_PERIPHERAL_DATA_SIZE_16 | \
     DMA_OPTION_MEMORY_DATA_SIZE_8 |	  \
     DMA_OPTION_MEMORY_DATA_INCREMENT |	  \
     DMA_OPTION_PRIORITY_MEDIUM)

static USART_TypeDef * const stm32l4_uart_xlate_USART[UART_INSTANCE_COUNT] = {
    USART1,
    USART2,
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    USART3,
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    UART4,
    UART5,
#endif
    LPUART1,
};

static const IRQn_Type stm32l4_uart_xlate_IRQn[UART_INSTANCE_COUNT] = {
    USART1_IRQn,
    USART2_IRQn,
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    USART3_IRQn,
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    UART4_IRQn,
    UART5_IRQn,
#endif
    LPUART1_IRQn,
};

static void stm32l4_uart_dma_callback(stm32l4_uart_t *uart, uint32_t events)
{
    uint32_t rx_index, rx_count, rx_total, rx_size, rx_write;
    bool overrun = false;

    rx_index = uart->rx_index;
    rx_count = stm32l4_dma_count(&uart->rx_dma);

    if (rx_index != rx_count)
    {
	rx_count = (rx_count - rx_index) & 15;

	uart->rx_index = (rx_index + rx_count) & 15;

	if (rx_count > (uart->rx_size - uart->rx_count))
	{
	    rx_count = (uart->rx_size - uart->rx_count);

	    overrun = true;
	}
	
	rx_total = rx_count;
	rx_write = uart->rx_write;
	rx_size  = rx_total;
	
	if (rx_size > (uart->rx_size - rx_write))
	{
	    rx_size = (uart->rx_size - rx_write);
	}
	
	memcpy(&uart->rx_data[rx_write], &uart->rx_fifo[rx_index], rx_size);
	
	rx_write += rx_size;
	rx_index += rx_size;
	rx_total -= rx_size;
	
	if (rx_write == uart->rx_size)
	{
	    rx_write = 0;
	}
	
	if (rx_total)
	{
	    memcpy(&uart->rx_data[rx_write], &uart->rx_fifo[rx_index], rx_total);
	    
	    rx_write += rx_total;
	}
	
	uart->rx_write = rx_write;
	
	armv7m_atomic_add(&uart->rx_count, rx_count);
	
	uart->rx_event += rx_count;

	if (uart->rx_event >= 16)
	{
	    uart->rx_event = 0;
	    
	    if (uart->events & UART_EVENT_RECEIVE)
	    {
		(*uart->callback)(uart->context, UART_EVENT_RECEIVE);
	    }
	}
	
	if (overrun)
	{
	    if (uart->events & UART_EVENT_OVERRUN)
	    {
		(*uart->callback)(uart->context, UART_EVENT_OVERRUN);
	    }
	}
    }
}

static void stm32l4_uart_interrupt(stm32l4_uart_t *uart)
{
    USART_TypeDef *USART = uart->USART;
    uint32_t events, rx_index, rx_count, rx_total, rx_size, rx_write;
    uint8_t rx_data;
    
    events = 0;

    if (USART->ISR & USART_ISR_RXNE)
    {
	if (USART->CR1 & USART_CR1_RXNEIE)
	{
	    rx_data = USART->RDR;

	    if (uart->rx_count != uart->rx_size)
	    {
		rx_write = uart->rx_write;

		uart->rx_data[rx_write] = rx_data;

		rx_write++;

		if (rx_write == uart->rx_size)
		{
		    rx_write = 0;
		}

		uart->rx_write = rx_write;

		armv7m_atomic_add(&uart->rx_count, 1);

		uart->rx_event++;

		if (uart->rx_event >= 16)
		{
		    uart->rx_event = 0;

		    events |= UART_EVENT_RECEIVE;
		}
	    }
	    else
	    {
		events |= UART_EVENT_OVERRUN;
	    }
	}
    }

    if (USART->ISR & USART_ISR_RTOF)
    {
	if (USART->CR1 & USART_CR1_RTOIE)
	{
	    if (uart->mode & UART_MODE_RX_DMA)
	    {
		rx_index = uart->rx_index;
		rx_count = stm32l4_dma_count(&uart->rx_dma);
		
		if (rx_index != rx_count)
		{
		    rx_count = (rx_count - rx_index) & 15;
		    
		    uart->rx_index = (rx_index + rx_count) & 15;
		    
		    if (rx_count > (uart->rx_size - uart->rx_count))
		    {
			rx_count = (uart->rx_size - uart->rx_count);
			
			events |= UART_EVENT_OVERRUN;
		    }
		    
		    rx_total = rx_count;
		    rx_write = uart->rx_write;
		    rx_size  = rx_total;
		    
		    if (rx_size > (uart->rx_size - rx_write))
		    {
			rx_size = (uart->rx_size - rx_write);
		    }
		    
		    memcpy(&uart->rx_data[rx_write], &uart->rx_fifo[rx_index], rx_size);
		    
		    rx_write += rx_size;
		    rx_index += rx_size;
		    rx_total -= rx_size;
		    
		    if (rx_write == uart->rx_size)
		    {
			rx_write = 0;
		    }
		    
		    if (rx_total)
		    {
			memcpy(&uart->rx_data[rx_write], &uart->rx_fifo[rx_index], rx_total);
			
			rx_write += rx_total;
		    }
		    
		    uart->rx_write = rx_write;
		    
		    armv7m_atomic_add(&uart->rx_count, rx_count);
		    
		    uart->rx_event += rx_count;
		}
	    }

	    if (uart->rx_event)
	    {
		uart->rx_event = 0;
		
		events |= UART_EVENT_RECEIVE;
	    }
	}

	USART->ICR = USART_ICR_RTOCF;
    }

    if (USART->ISR & USART_ISR_TXE)
    {
	if (USART->CR1 & USART_CR1_TXEIE)
	{
	    uart->tx_size++;

	    if (uart->tx_size == uart->tx_count)
	    {
		armv7m_atomic_and(&USART->CR1, ~USART_CR1_TXEIE);
		armv7m_atomic_or(&USART->CR1, USART_CR1_TCIE);
	    }

	    USART->TDR = *uart->tx_data++;
	}
    }

    if (USART->ISR & USART_ISR_TC)
    {
	USART->ICR = USART_ICR_TCCF;

	if (USART->CR1 & USART_CR1_TCIE)
	{
	    if (uart->mode & UART_MODE_TX_DMA)
	    {
		stm32l4_dma_stop(&uart->tx_dma);
		
		armv7m_atomic_and(&USART->CR3, ~USART_CR3_DMAT);
	    }

	    armv7m_atomic_and(&USART->CR1, ~USART_CR1_TCIE);

	    uart->state = UART_STATE_READY;
	    
	    events |= UART_EVENT_TRANSMIT;
	}
    }

    if (USART->ISR & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE | USART_ISR_IDLE | USART_ISR_LBDF))
    {
	if (uart->events & (UART_EVENT_IDLE | UART_EVENT_BREAK | UART_EVENT_NOISE | UART_EVENT_PARITY | UART_EVENT_FRAMING | UART_EVENT_OVERRUN))
	{
	    if (USART->ISR & USART_ISR_PE)
	    {
		events |= UART_EVENT_PARITY;
	    }
	    
	    if (USART->ISR & USART_ISR_FE)
	    {
		events |= UART_EVENT_FRAMING;
	    }
	    
	    if (USART->ISR & USART_ISR_NE)
	    {
		events |= UART_EVENT_NOISE;
	    }
	    
	    if (USART->ISR & USART_ISR_IDLE)
	    {
		events |= UART_EVENT_IDLE;
	    }
	    
	    if (USART->ISR & USART_ISR_ORE)
	    {
		events |= UART_EVENT_OVERRUN;
	    }
	    
	    if (USART->ISR & USART_ISR_LBDF)
	    {
		events |= UART_EVENT_BREAK;
	    }
	}

	USART->ICR = (USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NCF | USART_ICR_ORECF | USART_ICR_IDLECF | USART_ICR_LBDCF);
    }

    events &= uart->events;

    if (events)
    {
	(*uart->callback)(uart->context, events);
    }
}

bool stm32l4_uart_create(stm32l4_uart_t *uart, unsigned int instance, const stm32l4_uart_pins_t *pins, unsigned int priority, unsigned int mode)
{
    if (instance >= UART_INSTANCE_COUNT)
    {
	return false;
    }

    uart->USART = stm32l4_uart_xlate_USART[instance];
    uart->state = UART_STATE_INIT;
    uart->instance = instance;
    uart->interrupt = stm32l4_uart_xlate_IRQn[instance];
    uart->priority = priority;
    uart->pins = *pins;
    uart->callback = NULL;
    uart->context = NULL;
    uart->events = 0;

    uart->mode = mode & ~(UART_MODE_RX_DMA | UART_MODE_TX_DMA);

    if (mode & UART_MODE_RX_DMA)
    {
	switch (instance) {
	case UART_INSTANCE_USART1:
	    if ((!(mode & UART_MODE_RX_DMA_SECONDARY) && stm32l4_dma_create(&uart->rx_dma, DMA_CHANNEL_DMA1_CH5_USART1_RX, uart->priority)) ||
		stm32l4_dma_create(&uart->rx_dma, DMA_CHANNEL_DMA2_CH7_USART1_RX, uart->priority))
	    {
		uart->mode |= UART_MODE_RX_DMA;
	    }
	    break;

	case UART_INSTANCE_USART2:
	    if (stm32l4_dma_create(&uart->rx_dma, DMA_CHANNEL_DMA1_CH6_USART2_RX, uart->priority)) { uart->mode |= UART_MODE_RX_DMA; }
	    break;

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	case UART_INSTANCE_USART3:
	    if (stm32l4_dma_create(&uart->rx_dma, DMA_CHANNEL_DMA1_CH3_USART3_RX, uart->priority)) { uart->mode |= UART_MODE_RX_DMA; }
	    break;
#endif

#if defined(STM32L476xx) || defined(STM32L496xx)
	case UART_INSTANCE_UART4:
	    if (stm32l4_dma_create(&uart->rx_dma, DMA_CHANNEL_DMA2_CH5_UART4_RX, uart->priority)) { uart->mode |= UART_MODE_RX_DMA; }
	    break;
	case UART_INSTANCE_UART5:
	    if (stm32l4_dma_create(&uart->rx_dma, DMA_CHANNEL_DMA2_CH2_UART5_RX, uart->priority)) { uart->mode |= UART_MODE_RX_DMA; }
	    break;
#endif
	case UART_INSTANCE_LPUART1:
	    break;
      
	}
    }
	
    if (mode & UART_MODE_TX_DMA)
    {
	switch (instance) {
	case UART_INSTANCE_USART1:
	    if ((!(mode & UART_MODE_TX_DMA_SECONDARY) && stm32l4_dma_create(&uart->tx_dma, DMA_CHANNEL_DMA1_CH4_USART1_TX, uart->priority)) ||
		stm32l4_dma_create(&uart->tx_dma, DMA_CHANNEL_DMA2_CH6_USART1_TX, uart->priority))
	    {
		uart->mode |= UART_MODE_TX_DMA;
	    }
	    break;

	case UART_INSTANCE_USART2:
	    if (stm32l4_dma_create(&uart->tx_dma, DMA_CHANNEL_DMA1_CH7_USART2_TX, uart->priority)) { uart->mode |= UART_MODE_TX_DMA; }
	    break;

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
	case UART_INSTANCE_USART3:
	    if (stm32l4_dma_create(&uart->tx_dma, DMA_CHANNEL_DMA1_CH2_USART3_TX, uart->priority)) { uart->mode |= UART_MODE_TX_DMA; }
	    break;
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
	case UART_INSTANCE_UART4:
	    if (stm32l4_dma_create(&uart->tx_dma, DMA_CHANNEL_DMA2_CH3_UART4_TX, uart->priority)) { uart->mode |= UART_MODE_TX_DMA; }
	    break;
	case UART_INSTANCE_UART5:
	    if (stm32l4_dma_create(&uart->tx_dma, DMA_CHANNEL_DMA2_CH1_UART5_TX, uart->priority)) { uart->mode |= UART_MODE_TX_DMA; }
	    break;
#endif

	case UART_INSTANCE_LPUART1:
	    break;
      
	}
    }

    if ((uart->mode ^ mode) & (UART_MODE_RX_DMA | UART_MODE_TX_DMA))
    {
	if (uart->mode & UART_MODE_RX_DMA)
	{
	    stm32l4_dma_destroy(&uart->rx_dma);
			
	    uart->mode &= ~UART_MODE_RX_DMA;
	}

	if (uart->mode & UART_MODE_TX_DMA)
	{
	    stm32l4_dma_destroy(&uart->tx_dma);
			
	    uart->mode &= ~UART_MODE_TX_DMA;
	}

	uart->state = UART_STATE_NONE;

	return false;
    }

    stm32l4_uart_driver.instances[instance] = uart;

    return true;
}

bool stm32l4_uart_destroy(stm32l4_uart_t *uart)
{
    if (uart->state != UART_STATE_INIT)
    {
	return false;
    }

    stm32l4_uart_driver.instances[uart->instance] = NULL;

    if (uart->mode & UART_MODE_RX_DMA)
    {
	stm32l4_dma_destroy(&uart->rx_dma);
	
	uart->mode &= ~UART_MODE_RX_DMA;
    }
    
    if (uart->mode & UART_MODE_TX_DMA)
    {
	stm32l4_dma_destroy(&uart->tx_dma);
	
	uart->mode &= ~UART_MODE_TX_DMA;
    }

    uart->state = UART_STATE_NONE;

    return true;
}

bool stm32l4_uart_enable(stm32l4_uart_t *uart, uint8_t *rx_data, uint16_t rx_size, uint32_t bitrate, uint32_t option, stm32l4_uart_callback_t callback, void *context, uint32_t events)
{
    if (uart->state != UART_STATE_INIT)
    {
	return false;
    }
    
    if ((rx_data == NULL) || (rx_size < 16))
    {
	return false;
    }

    uart->rx_data  = rx_data;
    uart->rx_size  = rx_size;
    uart->rx_read  = 0;
    uart->rx_write = 0;
    uart->rx_index = 0;
    uart->rx_event = 0;
    uart->rx_count = 0;

    if (uart->instance != UART_INSTANCE_LPUART1)
    {
	stm32l4_system_hsi16_enable();
    }

    switch (uart->instance) {
    case UART_INSTANCE_USART1:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_USART1SEL, RCC_CCIPR_USART1SEL_1); /* HSI */
	break;
    case UART_INSTANCE_USART2:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_USART2SEL, RCC_CCIPR_USART2SEL_1); /* HSI */
	break;
#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)
    case UART_INSTANCE_USART3:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_USART3SEL, RCC_CCIPR_USART3SEL_1); /* HSI */
	break;
#endif
#if defined(STM32L476xx) || defined(STM32L496xx)
    case UART_INSTANCE_UART4:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_UART4SEL, RCC_CCIPR_UART4SEL_1);   /* HSI */
	break;
    case UART_INSTANCE_UART5:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_UART5SEL, RCC_CCIPR_UART5SEL_1);   /* HSI */
	break;
#endif
    case UART_INSTANCE_LPUART1:
	armv7m_atomic_modify(&RCC->CCIPR, RCC_CCIPR_LPUART1SEL, (RCC_CCIPR_LPUART1SEL_0 | RCC_CCIPR_LPUART1SEL_1)); /* LSE */
	break;
    }

    uart->state = UART_STATE_BUSY;

    if (!stm32l4_uart_configure(uart, bitrate, option))
    {
	uart->state = UART_STATE_INIT;

	return false;
    }

    uart->state = UART_STATE_READY;

    stm32l4_uart_notify(uart, callback, context, events);

    NVIC_SetPriority(uart->interrupt, uart->priority);

    NVIC_EnableIRQ(uart->interrupt);

    return true;
}

bool stm32l4_uart_disable(stm32l4_uart_t *uart)
{
    USART_TypeDef *USART = uart->USART;

    if (uart->state != UART_STATE_READY)
    {
	return false;
    }

    uart->events = 0;
    uart->callback = NULL;
    uart->context = NULL;

    USART->CR1 = 0;

    NVIC_DisableIRQ(uart->interrupt);

    if (uart->mode & UART_MODE_RX_DMA)
    {
	stm32l4_dma_disable(&uart->rx_dma);
    }

    if (uart->mode & UART_MODE_TX_DMA)
    {
	stm32l4_dma_disable(&uart->tx_dma);
    }

    stm32l4_system_periph_disable(SYSTEM_PERIPH_USART1 + uart->instance);

    if (uart->instance != UART_INSTANCE_LPUART1)
    {
	stm32l4_system_hsi16_disable();
    }

    if (uart->pins.rx != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(uart->pins.rx, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    }

    if (uart->pins.tx != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(uart->pins.tx, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    }

    if (uart->pins.cts != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(uart->pins.cts, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    }

    if (uart->pins.rts_de != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(uart->pins.rts_de, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    }

    uart->state = UART_STATE_INIT;

    return true;
}

bool stm32l4_uart_configure(stm32l4_uart_t *uart, uint32_t bitrate, uint32_t option)
{
    USART_TypeDef *USART = uart->USART;
    uint32_t usart_cr1, usart_cr2, usart_cr3;
    uint16_t mode;

    if ((uart->state != UART_STATE_READY) && (uart->state != UART_STATE_BUSY))
    {
	return false;
    }

    if (bitrate == 0)
    {
	return false;
    }

    if ((bitrate > 9600) && (uart->instance == UART_INSTANCE_LPUART1))
    {
	return false;
    }

    if ((bitrate > 921600) && (uart->instance != UART_INSTANCE_LPUART1))
    {
	return false;
    }

    if (((option & UART_OPTION_DATA_SIZE_MASK) == UART_OPTION_DATA_SIZE_9) && ((option & UART_OPTION_PARITY_MASK) != UART_OPTION_PARITY_NONE))
    {
	return false;
    }

    if ((option & (UART_OPTION_CTS | UART_OPTION_RTS)) && (option & UART_OPTION_DE_MODE))
    {
	return false;
    }

    if ((option & UART_OPTION_LIN_MODE) && (option & UART_OPTION_HALF_DUPLEX_MODE))
    {
	return false;
    }

    if ((option & UART_OPTION_LIN_MODE) && (uart->instance == UART_INSTANCE_LPUART1))
    {
	return false;
    }

    if (((option & UART_OPTION_STOP_MASK) == UART_OPTION_STOP_1_5) && (uart->instance == UART_INSTANCE_LPUART1))
    {
	return false;
    }

    if (uart->state == UART_STATE_BUSY)
    {
	stm32l4_system_periph_enable(SYSTEM_PERIPH_USART1 + uart->instance);
    }
    else
    {
	NVIC_DisableIRQ(uart->interrupt);
    }

    USART->CR1 = 0;

    usart_cr1 = 0;
    usart_cr2 = 0;
    usart_cr3 = 0;

    if ((uart->pins.rx != GPIO_PIN_NONE) || ((uart->pins.tx != GPIO_PIN_NONE) && (option & UART_OPTION_HALF_DUPLEX_MODE)))
    {
	usart_cr1 |= USART_CR1_RE;
	usart_cr1 |= USART_CR1_RTOIE;
	usart_cr2 |= USART_CR2_RTOEN;

	if (uart->mode & UART_MODE_RX_DMA)
	{
	    usart_cr3 |= USART_CR3_DMAR;
	}
	else
	{
	    usart_cr1 |= USART_CR1_RXNEIE;
	}

	USART->RTOR = 32;
    }

    if (uart->pins.tx != GPIO_PIN_NONE)
    {
	/* HALF_DUPLEX_MODE mode requires OPENDRAIN on the TX pin. Hence always set
	 * the GPIO mode here to switch between PUSHPULL and OPENDRAIN.
	 */
	
	if (option & UART_OPTION_HALF_DUPLEX_MODE)
	{
	    usart_cr3 |= USART_CR3_HDSEL;
	    
	    mode = (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_OPENDRAIN | GPIO_MODE_ALTERNATE);
	}
	else
	{
	    mode = (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE);
	}
	
	stm32l4_gpio_pin_configure(uart->pins.tx, mode);
	
	usart_cr1 |= USART_CR1_TE;
    }

    switch (option & UART_OPTION_STOP_MASK) {
    case UART_OPTION_STOP_1:
	break;
    case UART_OPTION_STOP_1_5:
	usart_cr2 |= (USART_CR2_STOP_1 | USART_CR2_STOP_0);
	break;
    case UART_OPTION_STOP_2:
	usart_cr2 |= (USART_CR2_STOP_1);
	break;
    default:
	break;
    }

    if ((option & UART_OPTION_PARITY_MASK) != UART_OPTION_PARITY_NONE)
    {
	usart_cr1 |= USART_CR1_PCE;
	
	if ((option & UART_OPTION_PARITY_MASK) == UART_OPTION_PARITY_ODD)
	{
	    usart_cr1 |= USART_CR1_PS;
	}
	
	option += (1 << UART_OPTION_DATA_SIZE_SHIFT);
    }

    switch (option & UART_OPTION_DATA_SIZE_MASK) {
    case UART_OPTION_DATA_SIZE_7:
	usart_cr1 |= USART_CR1_M1;
	break;
    case UART_OPTION_DATA_SIZE_8:
	break;
    case UART_OPTION_DATA_SIZE_9:
	usart_cr1 |= USART_CR1_M0;
	break;
    default:
	break;
    }
    
    if (option & UART_OPTION_CTS)
    {
	usart_cr3 |= USART_CR3_CTSE;
    }
    
    if (option & UART_OPTION_RTS)
    {
	usart_cr3 |= USART_CR3_RTSE;
    }
    
    if (option & UART_OPTION_DE_MODE)
    {
	usart_cr3 |= USART_CR3_DEM;
	
	if (option & UART_OPTION_DE_ACTIVE_HIGH)
	{
	    usart_cr3 |= USART_CR3_DEP;
	}
    }
    
    if (option & UART_OPTION_LIN_MODE)
    {
	usart_cr2 |= USART_CR2_LINEN;
	
	if (usart_cr1 & USART_CR1_RE)
	{
	    if (option & UART_OPTION_LIN_BREAK_LONG)
	    {
		usart_cr2 |= USART_CR2_LBDL;
	    }
	}
    }
    
    if (option & UART_OPTION_RX_INVERT)
    {
	usart_cr2 |= USART_CR2_RXINV;
    }
    
    if (option & UART_OPTION_TX_INVERT)
    {
	usart_cr2 |= USART_CR2_TXINV;
    }
    
    if (option & UART_OPTION_DATA_INVERT)
    {
	usart_cr2 |= USART_CR2_DATAINV;
    }
    
    if (uart->instance == UART_INSTANCE_LPUART1)
    {
	USART->BRR = (256 * 32768 + (bitrate >> 1)) / bitrate;  /* LSE */
    }
    else
    {
	USART->BRR = ((16000000 + (bitrate >> 1)) / bitrate);   /* HSI */
    }

    if (uart->mode & UART_MODE_RX_DMA)
    {
	if (uart->state == UART_STATE_BUSY)
	{
	    stm32l4_dma_enable(&uart->rx_dma, (stm32l4_dma_callback_t)stm32l4_uart_dma_callback, uart);
	    stm32l4_dma_start(&uart->rx_dma, (uint32_t)uart->rx_fifo, (uint32_t)&USART->RDR, 16, UART_RX_DMA_OPTION);
	}
    }

    if (uart->mode & UART_MODE_TX_DMA)
    {
	if (uart->state == UART_STATE_BUSY)
	{
	    stm32l4_dma_enable(&uart->tx_dma, NULL, NULL);
	}
    }

    if (uart->pins.rx != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(uart->pins.rx, (GPIO_PUPD_PULLUP | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    }

    if (uart->pins.cts != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(uart->pins.cts, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    }

    if (uart->pins.rts_de != GPIO_PIN_NONE)
    {
	stm32l4_gpio_pin_configure(uart->pins.rts_de, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    }

    if (usart_cr1 & USART_CR1_RE)
    {
	if (uart->events & UART_EVENT_PARITY)
	{
	    usart_cr1 |= USART_CR1_PEIE;
	}

	if (uart->events & UART_EVENT_IDLE)
	{
	    usart_cr1 |= USART_CR1_IDLEIE;
	}

	if (uart->events & (UART_EVENT_OVERRUN | UART_EVENT_FRAMING | UART_EVENT_NOISE))
	{
	    usart_cr3 |= USART_CR3_EIE;
	}

	if (usart_cr2 & USART_CR2_LINEN)
	{
	    if (uart->events & UART_EVENT_BREAK)
	    {
		usart_cr2 |= USART_CR2_LBDIE;
	    }
	}
    }

    USART->CR3 = usart_cr3;
    USART->CR2 = usart_cr2;
    USART->CR1 = usart_cr1 | USART_CR1_UE;
    
    /* Flush partial corrupt input and reset error flags.
     */
    USART->RQR = USART_RQR_RXFRQ;
    
    USART->ICR = (USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NCF | USART_ICR_ORECF | USART_ICR_IDLECF | USART_ICR_LBDCF);

    if (uart->state == UART_STATE_READY)
    {
	NVIC_EnableIRQ(uart->interrupt);
    }

    return true;
}

bool stm32l4_uart_notify(stm32l4_uart_t *uart, stm32l4_uart_callback_t callback, void *context, uint32_t events)
{
    USART_TypeDef *USART = uart->USART;

    if (uart->state != UART_STATE_READY)
    {
	return false;
    }

    uart->events = 0;

    uart->callback = callback;
    uart->context = context;
    uart->events = events;

    if (USART->CR1 & USART_CR1_RE)
    {
	if (uart->events & UART_EVENT_PARITY)
	{
	    armv7m_atomic_or(&USART->CR1, USART_CR1_PEIE);
	}
	else
	{
	    armv7m_atomic_and(&USART->CR1, ~USART_CR1_PEIE);
	}

	if (uart->events & UART_EVENT_IDLE)
	{
	    armv7m_atomic_or(&USART->CR1, USART_CR1_IDLEIE);
	}
	else
	{
	    armv7m_atomic_and(&USART->CR1, ~USART_CR1_IDLEIE);
	}

	if (uart->events & (UART_EVENT_OVERRUN | UART_EVENT_FRAMING | UART_EVENT_NOISE))
	{
	    armv7m_atomic_or(&USART->CR3, USART_CR3_EIE);
	}
	else
	{
	    armv7m_atomic_and(&USART->CR3, ~USART_CR3_EIE);
	}

	if (USART->CR2 & USART_CR2_LINEN)
	{
	    if (uart->events & UART_EVENT_BREAK)
	    {
		armv7m_atomic_or(&USART->CR2, USART_CR2_LBDIE);
	    }
	    else
	    {
		armv7m_atomic_and(&USART->CR2, USART_CR2_LBDIE);
	    }
	}
    }

    return true;
}

unsigned int stm32l4_uart_receive(stm32l4_uart_t *uart, uint8_t *rx_data, uint16_t rx_count)
{
    uint32_t rx_total, rx_size, rx_read;

    if (uart->state < UART_STATE_READY)
    {
	return false;
    }

    rx_size = uart->rx_count;

    if (rx_count > rx_size)
    {
	rx_count = rx_size;
    }

    rx_total = rx_count;

    rx_read = uart->rx_read;
    rx_size = rx_total;

    if ((rx_read + rx_size) > uart->rx_size)
    {
	rx_size = uart->rx_size - rx_read;
    }

    memcpy(rx_data, &uart->rx_data[rx_read], rx_size);

    rx_read += rx_size;
    rx_total -= rx_size;

    if (rx_read == uart->rx_size)
    {
	rx_read = 0;
    }

    uart->rx_read = rx_read;

    armv7m_atomic_sub(&uart->rx_count, rx_size);

    if (rx_total)
    {
	memcpy(rx_data + rx_size, &uart->rx_data[rx_read], rx_total);

	rx_read += rx_total;

	uart->rx_read = rx_read;

	armv7m_atomic_sub(&uart->rx_count, rx_total);
    }

    return rx_count;
}

unsigned int stm32l4_uart_count(stm32l4_uart_t *uart)
{
    if (uart->state < UART_STATE_READY)
    {
	return 0;
    }

    return uart->rx_count;
}

int stm32l4_uart_peek(stm32l4_uart_t *uart)
{
    if (uart->state < UART_STATE_READY)
    {
	return -1;
    }

    if (!uart->rx_count)
    {
	return -1;
    }

    return uart->rx_data[uart->rx_read];
}

bool stm32l4_uart_transmit(stm32l4_uart_t *uart, const uint8_t *tx_data, uint16_t tx_count)
{
    USART_TypeDef *USART = uart->USART;

    if (uart->state != UART_STATE_READY)
    {
	return false;
    }

    uart->state = UART_STATE_TRANSMIT;

    if (uart->mode & UART_MODE_TX_DMA)
    {
	armv7m_atomic_or(&USART->CR3, USART_CR3_DMAT);

	USART->ICR = USART_ICR_TCCF;

	uart->tx_data  = tx_data;
	uart->tx_count = tx_count;
	uart->tx_size =  0;

	armv7m_atomic_or(&USART->CR1, USART_CR1_TCIE);

	stm32l4_dma_start(&uart->tx_dma, (uint32_t)&USART->TDR, (uint32_t)uart->tx_data, tx_count, UART_TX_DMA_OPTION);
    }
    else
    {
	USART->TDR = *tx_data++;

	uart->tx_data  = tx_data;
	uart->tx_count = tx_count;
	uart->tx_size  = 1;

	if (uart->tx_size == uart->tx_count)
	{
	    armv7m_atomic_or(&USART->CR1, USART_CR1_TCIE);
	}
	else
	{
	    armv7m_atomic_or(&USART->CR1, USART_CR1_TXEIE);
	}
    }

    return true;
}

bool stm32l4_uart_send_break(stm32l4_uart_t *uart)
{
    USART_TypeDef *USART = uart->USART;

    if (uart->state != UART_STATE_READY)
    {
	return false;
    }

    uart->state = UART_STATE_BREAK;

    USART->RQR = USART_RQR_SBKRQ;

    armv7m_atomic_or(&USART->CR1, USART_CR1_TCIE);
    
    return true;
}

bool stm32l4_uart_done(stm32l4_uart_t *uart)
{
    return (uart->state == UART_STATE_READY);
}

void stm32l4_uart_poll(stm32l4_uart_t *uart)
{
    if (uart->state >= UART_STATE_READY)
    {
	if (uart->mode & UART_MODE_TX_DMA)
	{
	    stm32l4_dma_poll(&uart->tx_dma);
	}

	if (uart->mode & UART_MODE_RX_DMA)
	{
	    stm32l4_dma_poll(&uart->rx_dma);
	}

	stm32l4_uart_interrupt(uart);
    }
}

void USART1_IRQHandler(void)
{
    stm32l4_uart_interrupt(stm32l4_uart_driver.instances[UART_INSTANCE_USART1]);
}

void USART2_IRQHandler(void)
{
    stm32l4_uart_interrupt(stm32l4_uart_driver.instances[UART_INSTANCE_USART2]);
}

#if defined(STM32L433xx) || defined(STM32L476xx) || defined(STM32L496xx)

void USART3_IRQHandler(void)
{
    stm32l4_uart_interrupt(stm32l4_uart_driver.instances[UART_INSTANCE_USART3]);
}

#endif

#if defined(STM32L476xx) || defined(STM32L496xx)

void UART4_IRQHandler(void)
{
    stm32l4_uart_interrupt(stm32l4_uart_driver.instances[UART_INSTANCE_UART4]);
}

void UART5_IRQHandler(void)
{
    stm32l4_uart_interrupt(stm32l4_uart_driver.instances[UART_INSTANCE_UART5]);
}

#endif

void LPUART1_IRQHandler(void)
{
    stm32l4_uart_interrupt(stm32l4_uart_driver.instances[UART_INSTANCE_LPUART1]);
}
