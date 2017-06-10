/*
 * Copyright (c) 2017 Thomas Roell.  All rights reserved.
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
#include "stm32l4_usbd_dap.h"
#include "usbd_hid.h"
#include "DAP_config.h"
#include "DAP.h"

#define DAP_PACKET_INDEX (DAP_PACKET_COUNT-1)
#define DAP_PACKET_WRAP  (DAP_PACKET_COUNT)

DAP_PINS_t DAP_PINS;

typedef struct _stm32l4_usbd_dap_device_t {
    struct _USBD_HandleTypeDef     *USBD;
    volatile uint16_t              rx_read;
    volatile uint16_t              rx_write;
    volatile uint16_t              tx_read;
    volatile uint16_t              tx_write;
    volatile uint8_t               tx_busy;
    uint8_t                        rx_data[DAP_PACKET_COUNT][DAP_PACKET_SIZE];
    uint8_t                        tx_data[DAP_PACKET_COUNT][DAP_PACKET_SIZE];
    uint16_t                       tx_size[DAP_PACKET_COUNT];
} stm32l4_usbd_dap_device_t;

static stm32l4_usbd_dap_device_t stm32l4_usbd_dap_device;

void stm32l4_usbd_dap_initialize(uint16_t pin_swclk, uint16_t pin_swdio)
{
    DAP_PINS.SWCLK_GPIO = (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * ((pin_swclk & GPIO_PIN_GROUP_MASK) >> GPIO_PIN_GROUP_SHIFT));
    DAP_PINS.SWCLK_BIT = 1ul << ((pin_swclk & GPIO_PIN_INDEX_MASK) >> GPIO_PIN_INDEX_SHIFT);
    DAP_PINS.SWCLK_PIN = pin_swclk;
    DAP_PINS.SWCLK_OUTPUT = 1ul << (((pin_swclk & GPIO_PIN_INDEX_MASK) >> GPIO_PIN_INDEX_SHIFT) * 2);
    
    DAP_PINS.SWDIO_GPIO = (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * ((pin_swdio & GPIO_PIN_GROUP_MASK) >> GPIO_PIN_GROUP_SHIFT));
    DAP_PINS.SWDIO_BIT = 1ul << ((pin_swdio & GPIO_PIN_INDEX_MASK) >> GPIO_PIN_INDEX_SHIFT);
    DAP_PINS.SWDIO_PIN = pin_swdio;
    DAP_PINS.SWDIO_OUTPUT = 1ul << (((pin_swdio & GPIO_PIN_INDEX_MASK) >> GPIO_PIN_INDEX_SHIFT) * 2);

    DAP_Setup();
}

static void stm32l4_usbd_dap_routine(void)
{
    uint32_t rx_read, rx_index, rx_queue, tx_write, tx_index, status;

    rx_read = stm32l4_usbd_dap_device.rx_read;
    tx_write = stm32l4_usbd_dap_device.tx_write;

    while (rx_read ^ stm32l4_usbd_dap_device.rx_write)
    {
	if ((tx_write ^ stm32l4_usbd_dap_device.tx_read) == DAP_PACKET_WRAP)
	{
	    break;
	}

	rx_index = rx_read & DAP_PACKET_INDEX;
	tx_index = tx_write & DAP_PACKET_INDEX;

	/* If the current packet starts with a DAP_QueueCommands, scan forward to see
	 * whether there is a non-DAP_QueueCommands in the queue. If so modify
	 * the packets to be DAP_ExecuteCommands and execute.
	 */

	if (stm32l4_usbd_dap_device.rx_data[rx_index][0] == ID_DAP_QueueCommands)
	{
	    rx_queue = (rx_read + 1) & (DAP_PACKET_WRAP | DAP_PACKET_INDEX);

	    while (rx_queue ^ stm32l4_usbd_dap_device.rx_write)
	    {
		if (stm32l4_usbd_dap_device.rx_data[rx_queue & DAP_PACKET_INDEX][0] != ID_DAP_QueueCommands)
		{
		    stm32l4_usbd_dap_device.rx_data[rx_index][0] = ID_DAP_ExecuteCommands;

		    break;
		}

		rx_queue = (rx_queue + 1) & (DAP_PACKET_WRAP | DAP_PACKET_INDEX);
	    }

	    if (stm32l4_usbd_dap_device.rx_data[rx_index][0] == ID_DAP_QueueCommands)
	    {
		break;
	    }
	}

	memset(&stm32l4_usbd_dap_device.tx_data[tx_index][0], 0, DAP_PACKET_SIZE);

	status = DAP_ExecuteCommand(&stm32l4_usbd_dap_device.rx_data[rx_index][0], &stm32l4_usbd_dap_device.tx_data[tx_index][0]);

	// stm32l4_usbd_dap_device.tx_size[tx_index] = status & 0xffff; 

	stm32l4_usbd_dap_device.tx_size[tx_index] = DAP_PACKET_SIZE;

	tx_write = (tx_write + 1) & (DAP_PACKET_WRAP | DAP_PACKET_INDEX);

	stm32l4_usbd_dap_device.tx_write = tx_write;

	if (!stm32l4_usbd_dap_device.tx_busy)
	{
	    stm32l4_usbd_dap_device.tx_busy = 1;

#if defined(STM32L476xx) || defined(STM32L496xx)
	    NVIC_DisableIRQ(OTG_FS_IRQn);
#else
	    NVIC_DisableIRQ(USB_IRQn);
#endif
	    tx_index = stm32l4_usbd_dap_device.tx_read & DAP_PACKET_INDEX;

	    USBD_HID_SendReport(stm32l4_usbd_dap_device.USBD, &stm32l4_usbd_dap_device.tx_data[tx_index][0], stm32l4_usbd_dap_device.tx_size[tx_index]);

#if defined(STM32L476xx) || defined(STM32L496xx)
	    NVIC_EnableIRQ(OTG_FS_IRQn);
#else
	    NVIC_EnableIRQ(USB_IRQn);
#endif
	}

	rx_read = (rx_read + 1) & (DAP_PACKET_WRAP | DAP_PACKET_INDEX);

	stm32l4_usbd_dap_device.rx_read = rx_read;
    }
}

static void stm32l4_usbd_dap_init(USBD_HandleTypeDef *USBD)
{
    stm32l4_usbd_dap_device.USBD = USBD;
    stm32l4_usbd_dap_device.rx_read = 0;
    stm32l4_usbd_dap_device.rx_write = 0;
    stm32l4_usbd_dap_device.tx_read = 0;
    stm32l4_usbd_dap_device.tx_write = 0;
    stm32l4_usbd_dap_device.tx_busy = 0;
}

static void stm32l4_usbd_dap_deinit(void)
{
    stm32l4_usbd_dap_device.tx_busy = 0;

    stm32l4_usbd_dap_device.USBD = NULL;
}

static uint32_t stm32l4_usbd_dap_get_report(uint8_t type, uint8_t id, uint8_t *data)
{
  return 0;
}

static void stm32l4_usbd_dap_set_report(uint8_t type, uint8_t id, const uint8_t *data, uint32_t length)
{
}

static void stm32l4_usbd_dap_rx_ready(const uint8_t *data, uint32_t length)
{
    uint32_t rx_write, rx_index;

    if (length)
    {
	if (data[0] == ID_DAP_TransferAbort)
	{
	    DAP_TransferAbort = 1;
	}
	else
	{
	    rx_write = stm32l4_usbd_dap_device.rx_write;

	    if ((rx_write ^ stm32l4_usbd_dap_device.rx_read) != DAP_PACKET_WRAP)
	    {
		rx_index = rx_write & DAP_PACKET_INDEX;

		memset(&stm32l4_usbd_dap_device.tx_data[rx_index][0], 0, DAP_PACKET_SIZE);

		memcpy(&stm32l4_usbd_dap_device.rx_data[rx_index][0], data, length);

		stm32l4_usbd_dap_device.rx_write = (rx_write + 1) & (DAP_PACKET_WRAP | DAP_PACKET_INDEX);

		armv7m_pendsv_enqueue((armv7m_pendsv_routine_t)stm32l4_usbd_dap_routine, NULL, 0);
	    }
	}
    }
}

static void stm32l4_usbd_dap_tx_done(void)
{
    uint32_t tx_read, tx_index;

    tx_read = (stm32l4_usbd_dap_device.tx_read + 1) & (DAP_PACKET_WRAP | DAP_PACKET_INDEX);
    
    stm32l4_usbd_dap_device.tx_read = tx_read;

    if (tx_read ^ stm32l4_usbd_dap_device.tx_write)
    {
	tx_index = tx_read & DAP_PACKET_INDEX;
	
	USBD_HID_SendReport(stm32l4_usbd_dap_device.USBD, &stm32l4_usbd_dap_device.tx_data[tx_index][0], stm32l4_usbd_dap_device.tx_size[tx_index]);
    }
    else
    {
	stm32l4_usbd_dap_device.tx_busy = 0;
    }

    stm32l4_usbd_dap_device.tx_busy = 0;
}

const USBD_HID_ItfTypeDef stm32l4_usbd_dap_interface = {
    stm32l4_usbd_dap_init,
    stm32l4_usbd_dap_deinit,
    stm32l4_usbd_dap_get_report,
    stm32l4_usbd_dap_set_report,
    stm32l4_usbd_dap_rx_ready,
    stm32l4_usbd_dap_tx_done,
};
