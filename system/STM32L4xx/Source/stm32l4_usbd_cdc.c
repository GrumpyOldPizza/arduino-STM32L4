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

#include "stm32l4_system.h"
#include "stm32l4_usbd_cdc.h"

#include "usbd_cdc.h"

extern void USBD_Detach(void);
extern void USBD_RegisterCallbacks(void(*sof_callback)(void), void(*suspend_callback)(void), void(*resume_callback)(void));

volatile stm32l4_usbd_cdc_info_t stm32l4_usbd_cdc_info;

typedef struct _stm32l4_usbd_cdc_driver_t {
    stm32l4_usbd_cdc_t             *instances[USBD_CDC_INSTANCE_COUNT];
} stm32l4_usbd_cdc_driver_t;

static stm32l4_usbd_cdc_driver_t stm32l4_usbd_cdc_driver;

typedef struct _stm32l4_usbd_cdc_device_t {
    struct _USBD_HandleTypeDef     *USBD;
    volatile uint8_t               rx_busy;
    volatile uint8_t               tx_busy;
    volatile uint8_t               tx_flush;
    volatile uint8_t               control;
    volatile uint8_t               suspended;
    uint32_t                       timeout;
} stm32l4_usbd_cdc_device_t;

static stm32l4_usbd_cdc_device_t stm32l4_usbd_cdc_device;

static void stm32l4_usbd_cdc_sof_callback(void)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    if (stm32l4_usbd_cdc_device.timeout) 
    {
	if (stm32l4_usbd_cdc_device.timeout == 1)
	{
	    stm32l4_system_dfu();
	}
	else
	{
	    stm32l4_usbd_cdc_device.timeout--;
	}
    }

    if (usbd_cdc && (usbd_cdc->state > USBD_CDC_STATE_INIT))
    {
	if (usbd_cdc->events & USBD_CDC_EVENT_SOF)
	{
	    (*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_SOF);
	}
    }
}

static void stm32l4_usbd_cdc_suspend_callback(void)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    if (usbd_cdc && (usbd_cdc->state > USBD_CDC_STATE_INIT))
    {
	usbd_cdc->state = USBD_CDC_STATE_SUSPENDED;
    }

    stm32l4_usbd_cdc_device.suspended = 1;

    if (stm32l4_usbd_cdc_device.tx_busy && !stm32l4_usbd_cdc_device.tx_flush)
    {
	stm32l4_usbd_cdc_device.tx_flush = 1;

	if (usbd_cdc && (usbd_cdc->events & USBD_CDC_EVENT_TRANSMIT))
	{
	    (*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_TRANSMIT);
	}
    }
}

static void stm32l4_usbd_cdc_resume_callback(void)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    stm32l4_usbd_cdc_device.suspended = 0;

    if (usbd_cdc && (usbd_cdc->state == USBD_CDC_STATE_SUSPENDED))
    {
	usbd_cdc->state = stm32l4_usbd_cdc_device.control ? USBD_CDC_STATE_READY : USBD_CDC_STATE_RESET;
    }
}

static void stm32l4_usbd_cdc_init(USBD_HandleTypeDef *USBD)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    stm32l4_usbd_cdc_device.USBD = USBD;
    stm32l4_usbd_cdc_device.rx_busy = 0;
    stm32l4_usbd_cdc_device.tx_busy = 0;
    stm32l4_usbd_cdc_device.tx_flush = 0;
    stm32l4_usbd_cdc_device.control = 0;
    stm32l4_usbd_cdc_device.suspended = 0;
    stm32l4_usbd_cdc_device.timeout = 0;

    stm32l4_usbd_cdc_info.dwDTERate = 115200;
    stm32l4_usbd_cdc_info.bCharFormat = 0;
    stm32l4_usbd_cdc_info.bParityType = 0;
    stm32l4_usbd_cdc_info.bDataBits = 8;
    stm32l4_usbd_cdc_info.lineState = 0;

    if (usbd_cdc && (usbd_cdc->state > USBD_CDC_STATE_INIT))
    {
	usbd_cdc->state = USBD_CDC_STATE_RESET;

	USBD_CDC_SetRxBuffer(stm32l4_usbd_cdc_device.USBD, &usbd_cdc->rx_data[usbd_cdc->rx_write]);
	USBD_CDC_ReceivePacket(stm32l4_usbd_cdc_device.USBD);

	stm32l4_usbd_cdc_device.rx_busy = 1;
    }

    USBD_RegisterCallbacks(stm32l4_usbd_cdc_sof_callback, stm32l4_usbd_cdc_suspend_callback, stm32l4_usbd_cdc_resume_callback);
}

static void stm32l4_usbd_cdc_deinit(void)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    if (usbd_cdc)
    {
	usbd_cdc->state = USBD_CDC_STATE_RESET;
    }

    USBD_RegisterCallbacks(NULL, NULL, NULL);

    stm32l4_usbd_cdc_device.control = 0;
    stm32l4_usbd_cdc_device.suspended = 0;

    if (stm32l4_usbd_cdc_device.tx_busy && !stm32l4_usbd_cdc_device.tx_flush)
    {
	if (usbd_cdc && (usbd_cdc->events & USBD_CDC_EVENT_TRANSMIT))
	{
	    (*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_TRANSMIT);
	}
    }

    stm32l4_usbd_cdc_device.rx_busy = 0;
    stm32l4_usbd_cdc_device.tx_busy = 0;
    stm32l4_usbd_cdc_device.tx_flush = 0;

    stm32l4_usbd_cdc_device.USBD = NULL;
}

static void stm32l4_usbd_cdc_control(uint8_t command, uint8_t *data, uint16_t length)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    if (command == USBD_CDC_GET_LINE_CODING)
    {
	data[0] = (uint8_t)(stm32l4_usbd_cdc_info.dwDTERate >> 0);
	data[1] = (uint8_t)(stm32l4_usbd_cdc_info.dwDTERate >> 8);
	data[2] = (uint8_t)(stm32l4_usbd_cdc_info.dwDTERate >> 16);
	data[3] = (uint8_t)(stm32l4_usbd_cdc_info.dwDTERate >> 24);
	data[4] = stm32l4_usbd_cdc_info.bCharFormat;
	data[5] = stm32l4_usbd_cdc_info.bParityType;
	data[6] = stm32l4_usbd_cdc_info.bDataBits;     
    }
    else
    {
	if (command == USBD_CDC_SET_LINE_CODING)
	{
	    stm32l4_usbd_cdc_info.dwDTERate   = (uint32_t)((data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
	    stm32l4_usbd_cdc_info.bCharFormat = data[4];
	    stm32l4_usbd_cdc_info.bParityType = data[5];
	    stm32l4_usbd_cdc_info.bDataBits   = data[6];
	}

	if (command == USBD_CDC_SET_CONTROL_LINE_STATE)
	{
	    stm32l4_usbd_cdc_info.lineState = (uint16_t)((data[2] << 0) | (data[3] << 8));
	}

	if ((command == USBD_CDC_SET_LINE_CODING) || (command == USBD_CDC_SET_CONTROL_LINE_STATE))
	{
	    if (!stm32l4_usbd_cdc_device.control)
	    {
		stm32l4_usbd_cdc_device.control = 1;

		if (usbd_cdc && (usbd_cdc->state == USBD_CDC_STATE_RESET))
		{
		    usbd_cdc->state = USBD_CDC_STATE_READY;
		}
	    }

	    if ((stm32l4_usbd_cdc_info.dwDTERate == 1200) && !(stm32l4_usbd_cdc_info.lineState & 1))
	    {
		/* start reset timer */
		stm32l4_usbd_cdc_device.timeout = 250;
	    }
	    else
	    {
		/* stop reset timer */
		stm32l4_usbd_cdc_device.timeout = 0;
	    }
	}
    }
}

static void stm32l4_usbd_cdc_rx_ready(uint8_t *data, uint32_t length)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    stm32l4_usbd_cdc_device.rx_busy = 0;

    if (usbd_cdc && (usbd_cdc->state > USBD_CDC_STATE_INIT))
    {
	usbd_cdc->rx_write += length;

	armv7m_atomic_add(&usbd_cdc->rx_count, length);

	if ((usbd_cdc->rx_write + USBD_CDC_DATA_MAX_PACKET_SIZE) > usbd_cdc->rx_size)
	{
	    usbd_cdc->rx_wrap = usbd_cdc->rx_write;
	    usbd_cdc->rx_write = 0;
	}

	if (usbd_cdc->state != USBD_CDC_STATE_RESET)
	{
	    if ((usbd_cdc->rx_wrap - usbd_cdc->rx_count) >= USBD_CDC_DATA_MAX_PACKET_SIZE)
	    {
		USBD_CDC_SetRxBuffer(stm32l4_usbd_cdc_device.USBD, &usbd_cdc->rx_data[usbd_cdc->rx_write]);
		USBD_CDC_ReceivePacket(stm32l4_usbd_cdc_device.USBD);
		
		stm32l4_usbd_cdc_device.rx_busy = 1;
	    }
	}

	if (usbd_cdc->events & USBD_CDC_EVENT_RECEIVE)
	{
	    (*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_RECEIVE);
	}
    }
}

static void stm32l4_usbd_cdc_tx_done(void)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    stm32l4_usbd_cdc_device.tx_busy = 0;

    if (stm32l4_usbd_cdc_device.tx_flush)
    {
	stm32l4_usbd_cdc_device.tx_flush = 0;
    }
    else
    {
	if (usbd_cdc && (usbd_cdc->state > USBD_CDC_STATE_INIT))
	{
	    if (usbd_cdc->events & USBD_CDC_EVENT_TRANSMIT)
	    {
		(*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_TRANSMIT);
	    }
	}
    }
}

const USBD_CDC_ItfTypeDef stm32l4_usbd_cdc_interface = {
    stm32l4_usbd_cdc_init,
    stm32l4_usbd_cdc_deinit,
    stm32l4_usbd_cdc_control,
    stm32l4_usbd_cdc_rx_ready,
    stm32l4_usbd_cdc_tx_done,
};

bool stm32l4_usbd_cdc_create(stm32l4_usbd_cdc_t *usbd_cdc)
{
    usbd_cdc->state = USBD_CDC_STATE_INIT;

    usbd_cdc->rx_data  = NULL;
    usbd_cdc->rx_size  = 0;
    usbd_cdc->rx_read  = 0;
    usbd_cdc->rx_write = 0;
    usbd_cdc->rx_wrap  = 0;
    usbd_cdc->rx_count = 0;

    usbd_cdc->callback = NULL;
    usbd_cdc->context = NULL;
    usbd_cdc->events = 0;

    stm32l4_usbd_cdc_driver.instances[0] = usbd_cdc;

    return true;
}

bool stm32l4_usbd_cdc_destroy(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state != USBD_CDC_STATE_INIT)
    {
	return false;
    }

    stm32l4_usbd_cdc_driver.instances[0] = NULL;

    return true;
}

bool stm32l4_usbd_cdc_enable(stm32l4_usbd_cdc_t *usbd_cdc, uint8_t *rx_data, uint16_t rx_size, uint32_t option, stm32l4_usbd_cdc_callback_t callback, void *context, uint32_t events)
{
    if (usbd_cdc->state != USBD_CDC_STATE_INIT)
    {
	return false;
    }

    usbd_cdc->rx_data  = rx_data;
    usbd_cdc->rx_size  = rx_size;
    usbd_cdc->rx_read  = 0;
    usbd_cdc->rx_write = 0;
    usbd_cdc->rx_wrap  = rx_size;
    usbd_cdc->rx_count = 0;

    usbd_cdc->option = option;

    usbd_cdc->callback = callback;
    usbd_cdc->context = context;
    usbd_cdc->events = events;

    if (stm32l4_usbd_cdc_device.USBD)
    {
	if (stm32l4_usbd_cdc_device.suspended)
	{
	    usbd_cdc->state = USBD_CDC_STATE_SUSPENDED;
	}
	else
	{
	    usbd_cdc->state = stm32l4_usbd_cdc_device.control ? USBD_CDC_STATE_READY : USBD_CDC_STATE_RESET;
	}

	USBD_CDC_SetRxBuffer(stm32l4_usbd_cdc_device.USBD, &usbd_cdc->rx_data[usbd_cdc->rx_write]);
	USBD_CDC_ReceivePacket(stm32l4_usbd_cdc_device.USBD);

	stm32l4_usbd_cdc_device.rx_busy = 1;
    }
    else
    {
	usbd_cdc->state = USBD_CDC_STATE_RESET;
    }

    return true;
}

bool stm32l4_usbd_cdc_disable(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state < USBD_CDC_STATE_READY)
    {
	return false;
    }

    usbd_cdc->state = USBD_CDC_STATE_INIT;

    usbd_cdc->rx_data  = NULL;
    usbd_cdc->rx_size  = 0;
    usbd_cdc->rx_read  = 0;
    usbd_cdc->rx_write = 0;
    usbd_cdc->rx_wrap  = 0;
    usbd_cdc->rx_count = 0;

    usbd_cdc->callback = NULL;
    usbd_cdc->context = NULL;
    usbd_cdc->events = 0;

    return true;
}

bool stm32l4_usbd_cdc_notify(stm32l4_usbd_cdc_t *usbd_cdc, stm32l4_usbd_cdc_callback_t callback, void *context, uint32_t events)
{
    if (usbd_cdc->state < USBD_CDC_STATE_READY)
    {
	return false;
    }

    usbd_cdc->events = 0;

    usbd_cdc->callback = callback;
    usbd_cdc->context = context;
    usbd_cdc->events = events;

    return true;
}

unsigned int stm32l4_usbd_cdc_receive(stm32l4_usbd_cdc_t *usbd_cdc, uint8_t *rx_data, uint16_t rx_count)
{
    uint32_t rx_total, rx_size, rx_read, rx_wrap;

    if (usbd_cdc->state < USBD_CDC_STATE_READY)
    {
	return false;
    }

    rx_size = usbd_cdc->rx_count;

    if (rx_count > rx_size)
    {
	rx_count = rx_size;
    }

    rx_total = rx_count;

    rx_read = usbd_cdc->rx_read;
    rx_wrap = usbd_cdc->rx_wrap;
    rx_size = rx_total;

    if ((rx_read + rx_size) > rx_wrap)
    {
	rx_size = rx_wrap - rx_read;
    }

    memcpy(rx_data, &usbd_cdc->rx_data[rx_read], rx_size);

    rx_read += rx_size;
    rx_total -= rx_size;

    if (rx_read == rx_wrap)
    {
	rx_read = 0;

	usbd_cdc->rx_wrap = usbd_cdc->rx_size;
    }

    usbd_cdc->rx_read = rx_read;

    armv7m_atomic_sub(&usbd_cdc->rx_count, rx_size);

    if (rx_total)
    {
	memcpy(rx_data + rx_size, &usbd_cdc->rx_data[rx_read], rx_total);

	rx_read += rx_total;

	usbd_cdc->rx_read = rx_read;

	armv7m_atomic_sub(&usbd_cdc->rx_count, rx_total);
    }

    if (!stm32l4_usbd_cdc_device.rx_busy && (usbd_cdc->state != USBD_CDC_STATE_RESET))
    {
	if ((usbd_cdc->rx_wrap - usbd_cdc->rx_count) >= USBD_CDC_DATA_MAX_PACKET_SIZE)
	{
	    USBD_CDC_SetRxBuffer(stm32l4_usbd_cdc_device.USBD, &usbd_cdc->rx_data[usbd_cdc->rx_write]);
	    USBD_CDC_ReceivePacket(stm32l4_usbd_cdc_device.USBD);
	    
	    stm32l4_usbd_cdc_device.rx_busy = 1;
	}
    }

    return rx_count;
}

unsigned int stm32l4_usbd_cdc_count(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state < USBD_CDC_STATE_READY)
    {
	return 0;
    }

    return usbd_cdc->rx_count;
}

int stm32l4_usbd_cdc_peek(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state < USBD_CDC_STATE_READY)
    {
	return -1;
    }

    if (!usbd_cdc->rx_count)
    {
	return -1;
    }

    return usbd_cdc->rx_data[usbd_cdc->rx_read];
}

bool stm32l4_usbd_cdc_transmit(stm32l4_usbd_cdc_t *usbd_cdc, const uint8_t *tx_data, uint32_t tx_count)
{
    int status;

    /* QTG_FS interrupts need to be disabled while calling
     * USBD_CDC_TransmitPacket. It seems that on short 
     * transmits HAL_PCD_EP_Transmit()/USB_EPStartXfer()
     * can take a completion interrupt before being done
     * setting up the transfer completele. I don't want
     * to mess around in the USB/HAL code, so the simple
     * WAR is to avoid the race condition by blocking
     * the interrupt till the transfer setup is complete.
     */

#if defined(STM32L476xx) || defined(STM32L496xx)
    NVIC_DisableIRQ(OTG_FS_IRQn);
#else
    NVIC_DisableIRQ(USB_IRQn);
#endif

    if ((usbd_cdc->state != USBD_CDC_STATE_READY) || stm32l4_usbd_cdc_device.tx_busy)
    {
#if defined(STM32L476xx) || defined(STM32L496xx)
	NVIC_EnableIRQ(OTG_FS_IRQn);
#else
	NVIC_EnableIRQ(USB_IRQn);
#endif

	return false;
    }
    else
    {
	stm32l4_usbd_cdc_device.tx_busy = 1;
	
	USBD_CDC_SetTxBuffer(stm32l4_usbd_cdc_device.USBD, tx_data, tx_count);
	
	status = USBD_CDC_TransmitPacket(stm32l4_usbd_cdc_device.USBD);
	
#if defined(STM32L476xx) || defined(STM32L496xx)
	NVIC_EnableIRQ(OTG_FS_IRQn);
#else
	NVIC_EnableIRQ(USB_IRQn);
#endif
	
	if (status != 0)
	{
	    return false;
	}
	
	return true;
    }
}

bool stm32l4_usbd_cdc_done(stm32l4_usbd_cdc_t *usbd_cdc)
{
    return ((usbd_cdc->state == USBD_CDC_STATE_READY) && !stm32l4_usbd_cdc_device.tx_busy);
}

void stm32l4_usbd_cdc_poll(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state >= USBD_CDC_STATE_READY)
    {
#if defined(STM32L476xx) || defined(STM32L496xx)
	OTG_FS_IRQHandler();
#else
	USB_IRQHandler();
#endif
    }
}
