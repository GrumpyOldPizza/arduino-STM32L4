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

#include "stm32l4_system.h"
#include "stm32l4_usbd_cdc.h"

#include "usbd_cdc.h"

volatile stm32l4_usbd_cdc_info_t stm32l4_usbd_cdc_info;

typedef struct _stm32l4_usbd_cdc_driver_t {
    stm32l4_usbd_cdc_t             *instances[USBD_CDC_INSTANCE_COUNT];
} stm32l4_usbd_cdc_driver_t;

static stm32l4_usbd_cdc_driver_t stm32l4_usbd_cdc_driver;

typedef struct _stm32l4_usbd_cdc_device_t {
    struct _USBD_HandleTypeDef     *USBD;
    volatile uint8_t               tx_busy;
    uint8_t                        rx_data[USBD_CDC_DATA_MAX_PACKET_SIZE];
    uint64_t                       connect;
} stm32l4_usbd_cdc_device_t;

static stm32l4_usbd_cdc_device_t stm32l4_usbd_cdc_device;

static void stm32l4_usbd_cdc_init(USBD_HandleTypeDef *USBD)
{
    stm32l4_usbd_cdc_device.USBD = USBD;
    stm32l4_usbd_cdc_device.tx_busy = 0;
    stm32l4_usbd_cdc_device.connect = 0;;

    stm32l4_usbd_cdc_info.dwDTERate = 115200;
    stm32l4_usbd_cdc_info.bCharFormat = 0;
    stm32l4_usbd_cdc_info.bParityType = 0;
    stm32l4_usbd_cdc_info.bDataBits = 8;
    stm32l4_usbd_cdc_info.lineState = 0;

    USBD_CDC_SetRxBuffer(stm32l4_usbd_cdc_device.USBD, &stm32l4_usbd_cdc_device.rx_data[0]);
    USBD_CDC_ReceivePacket(stm32l4_usbd_cdc_device.USBD);
}

static void stm32l4_usbd_cdc_deinit(void)
{
    stm32l4_usbd_cdc_device.USBD = NULL;
    stm32l4_usbd_cdc_device.tx_busy = 0;

    stm32l4_usbd_cdc_info.lineState = 0;
}

static void stm32l4_usbd_cdc_control(uint8_t command, uint8_t *data, uint16_t length)
{
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

	    if ((stm32l4_usbd_cdc_info.lineState & 3) && (stm32l4_usbd_cdc_device.connect == 0))
	    {
		stm32l4_usbd_cdc_device.connect = armv7m_systick_millis();
	    }
	}

	if ((command == USBD_CDC_SET_LINE_CODING) || (command == USBD_CDC_SET_CONTROL_LINE_STATE))
	{
	    if ((stm32l4_usbd_cdc_info.dwDTERate == 1200) && !(stm32l4_usbd_cdc_info.lineState & 1))
	    {
		/* init reset timer */
		armv7m_systick_timeout(stm32l4_system_bootloader, 250);
	    }
	    else
	    {
		/* cancel reset timer */
		armv7m_systick_timeout(NULL, 0);
	    }
	}
    }
}

static void stm32l4_usbd_cdc_data_receive(uint8_t *data, uint32_t length)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    if (usbd_cdc && (usbd_cdc->state > USBD_CDC_STATE_INIT))
    {
	usbd_cdc->rx_data = data;
	usbd_cdc->rx_size = length;

	if (usbd_cdc->events & USBD_CDC_EVENT_RECEIVE)
	{
	    (*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_RECEIVE);
	}

	if (usbd_cdc->rx_index != usbd_cdc->rx_size)
	{
	    if (usbd_cdc->events & USBD_CDC_EVENT_OVERRUN)
	    {
		(*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_OVERRUN);
	    }
	}

	usbd_cdc->rx_data = NULL;
	usbd_cdc->rx_size = 0;
	usbd_cdc->rx_index = 0;
    }

    USBD_CDC_ReceivePacket(stm32l4_usbd_cdc_device.USBD);
}

static void stm32l4_usbd_cdc_data_transmit(void)
{
    stm32l4_usbd_cdc_t *usbd_cdc = stm32l4_usbd_cdc_driver.instances[0];

    stm32l4_usbd_cdc_device.tx_busy = 0;

    if (usbd_cdc && (usbd_cdc->state > USBD_CDC_STATE_INIT))
    {
	if (usbd_cdc->events & USBD_CDC_EVENT_TRANSMIT)
	{
	    (*usbd_cdc->callback)(usbd_cdc->context, USBD_CDC_EVENT_TRANSMIT);
	}
    }
}

const USBD_CDC_ItfTypeDef stm32l4_usbd_cdc_interface = {
    stm32l4_usbd_cdc_init,
    stm32l4_usbd_cdc_deinit,
    stm32l4_usbd_cdc_control,
    stm32l4_usbd_cdc_data_receive,
    stm32l4_usbd_cdc_data_transmit,
};

bool stm32l4_usbd_cdc_create(stm32l4_usbd_cdc_t *usbd_cdc)
{
    usbd_cdc->state = USBD_CDC_STATE_INIT;

    usbd_cdc->rx_data  = NULL;
    usbd_cdc->rx_size  = 0;
    usbd_cdc->rx_index = 0;

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

bool stm32l4_usbd_cdc_enable(stm32l4_usbd_cdc_t *usbd_cdc, uint32_t option, stm32l4_usbd_cdc_callback_t callback, void *context, uint32_t events)
{
    if (usbd_cdc->state != USBD_CDC_STATE_INIT)
    {
	return false;
    }

    usbd_cdc->rx_data  = NULL;
    usbd_cdc->rx_size  = 0;
    usbd_cdc->rx_index = 0;

    usbd_cdc->option = option;

    usbd_cdc->callback = callback;
    usbd_cdc->context = context;
    usbd_cdc->events = events;

    usbd_cdc->state = USBD_CDC_STATE_READY;

    return true;
}

bool stm32l4_usbd_cdc_disable(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state != USBD_CDC_STATE_READY)
    {
	return false;
    }

    usbd_cdc->state = USBD_CDC_STATE_INIT;

    usbd_cdc->rx_data  = NULL;
    usbd_cdc->rx_size  = 0;
    usbd_cdc->rx_index = 0;

    usbd_cdc->callback = NULL;
    usbd_cdc->context = NULL;
    usbd_cdc->events = 0;

    return true;
}

bool stm32l4_usbd_cdc_notify(stm32l4_usbd_cdc_t *usbd_cdc, stm32l4_usbd_cdc_callback_t callback, void *context, uint32_t events)
{
    if (usbd_cdc->state != USBD_CDC_STATE_READY)
    {
	return false;
    }

    usbd_cdc->events = 0;

    usbd_cdc->callback = callback;
    usbd_cdc->context = context;
    usbd_cdc->events = events;

    return true;
}

unsigned int stm32l4_usbd_cdc_receive(stm32l4_usbd_cdc_t *usbd_cdc, uint8_t *rx_data, uint32_t rx_count)
{
    uint32_t rx_size, rx_index;

    if (usbd_cdc->state < USBD_CDC_STATE_READY)
    {
	return false;
    }

    rx_size = 0;

    rx_index = usbd_cdc->rx_index;

    while ((rx_index != usbd_cdc->rx_size) && (rx_size < rx_count))
    {
	rx_data[rx_size++] = usbd_cdc->rx_data[rx_index];

	rx_index++;
    }

    usbd_cdc->rx_index = rx_index;

    return rx_size;
}

bool stm32l4_usbd_cdc_transmit(stm32l4_usbd_cdc_t *usbd_cdc, const uint8_t *tx_data, uint32_t tx_count)
{
    int status;

    if ((usbd_cdc->state != USBD_CDC_STATE_READY) || stm32l4_usbd_cdc_device.tx_busy)
    {
	return false;
    }

    if (stm32l4_usbd_cdc_info.lineState & 3)
    {
	/* QTG_FS interrupts need to be disabled while calling
	 * USBD_CDC_TransmitPacket. It seems that on short 
	 * transmits HAL_PCD_EP_Transmit()/USB_EPStartXfer()
	 * can take a completion interrupt before being done
	 * setting up the transfer completele. I don't want
	 * to mess around in the USB/HAL code, so the simple
	 * WAR is to avoid the race condition by blocking
	 * the interrupt till the transfer setup is complete.
	 */
	NVIC_DisableIRQ(OTG_FS_IRQn);

	stm32l4_usbd_cdc_device.tx_busy = 1;

	USBD_CDC_SetTxBuffer(stm32l4_usbd_cdc_device.USBD, tx_data, tx_count);
	
	status = USBD_CDC_TransmitPacket(stm32l4_usbd_cdc_device.USBD);

	NVIC_EnableIRQ(OTG_FS_IRQn);

	if (status != 0)
	  {
	    return false;
	  }
    }

    return true;
}

bool stm32l4_usbd_cdc_done(stm32l4_usbd_cdc_t *usbd_cdc)
{
    return ((usbd_cdc->state == USBD_CDC_STATE_READY) && !stm32l4_usbd_cdc_device.tx_busy);
}

void stm32l4_usbd_cdc_poll(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state >= USBD_CDC_STATE_READY)
    {
	OTG_FS_IRQHandler();
    }
}

bool stm32l4_usbd_cdc_connected(stm32l4_usbd_cdc_t *usbd_cdc)
{
    if (usbd_cdc->state != USBD_CDC_STATE_READY)
    {
	return false;
    }
	
    return (stm32l4_usbd_cdc_info.lineState & 3) && ((armv7m_systick_millis() - stm32l4_usbd_cdc_device.connect) >= 250);
}
