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

#include "stm32l4_dac.h"
#include "stm32l4_gpio.h"
#include "stm32l4_system.h"

typedef struct _stm32l4_dac_driver_t {
    stm32l4_dac_t   *instances[DAC_INSTANCE_COUNT];
} stm32l4_dac_driver_t;

static stm32l4_dac_driver_t stm32l4_dac_driver;

bool stm32l4_dac_create(stm32l4_dac_t *dac, unsigned int instance, unsigned int priority, unsigned int mode)
{
    if (instance != DAC_INSTANCE_DAC)
    {
	return false;
    }

    dac->DACx = DAC;
    dac->state = DAC_STATE_INIT;
    dac->instance = instance;
    dac->priority = priority;
    dac->callback = NULL;
    dac->context = NULL;
    dac->events = 0;
    
    stm32l4_dac_driver.instances[dac->instance] = dac;

    return true;
}

bool stm32l4_dac_destroy(stm32l4_dac_t *dac)
{
    if (dac->state != DAC_STATE_INIT)
    {
	return false;
    }

    stm32l4_dac_driver.instances[dac->instance] = NULL;

    dac->state = DAC_STATE_NONE;

    return true;
}

bool stm32l4_dac_enable(stm32l4_dac_t *dac, uint32_t option, stm32l4_dac_callback_t callback, void *context, uint32_t events)
{
    if (dac->state != DAC_STATE_INIT)
    {
	return false;
    }

    dac->state = DAC_STATE_BUSY;

    if (!stm32l4_dac_configure(dac, option))
    {
	dac->state = DAC_STATE_INIT;

	return false;
    }

    stm32l4_dac_notify(dac, callback, context, events);

    dac->state = DAC_STATE_READY;

    return true;
}

bool stm32l4_dac_disable(stm32l4_dac_t *dac)
{
    DAC_TypeDef *DACx = dac->DACx;

    if (dac->state != DAC_STATE_READY)
    {
	return false;
    }

    DACx->CR &= ~(DAC_CR_EN1 | DAC_CR_EN2);

    dac->events = 0;
    dac->callback = NULL;
    dac->context = NULL;

    stm32l4_system_periph_disable(SYSTEM_PERIPH_DAC);

    dac->state = DAC_STATE_INIT;

    return true;
}

bool stm32l4_dac_configure(stm32l4_dac_t *dac, uint32_t option)
{
    if ((dac->state != DAC_STATE_BUSY) && (dac->state != DAC_STATE_READY))
    {
	return false;
    }
    
    if (dac->state == DAC_STATE_BUSY)
    {
	stm32l4_system_periph_enable(SYSTEM_PERIPH_DAC);
    }

    return true;
}


bool stm32l4_dac_notify(stm32l4_dac_t *dac, stm32l4_dac_callback_t callback, void *context, uint32_t events)
{
    if ((dac->state != DAC_STATE_BUSY) && (dac->state != DAC_STATE_READY))
    {
	return false;
    }

    dac->events = 0;

    dac->callback = callback;
    dac->context = context;
    dac->events = events;

    return true;
}

bool stm32l4_dac_channel(stm32l4_dac_t *dac, unsigned int channel, uint32_t output, uint32_t control)
{
    DAC_TypeDef *DACx = dac->DACx;
    uint32_t trim, step;

    if (dac->state != DAC_STATE_READY)
    {
	return false;
    }

    if (channel == DAC_CHANNEL_1)
    {
	DACx->CR &= ~(DAC_CR_EN1 | DAC_CR_TEN1 | DAC_CR_WAVE1 | DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1 | DAC_CR_CEN1);

	if (control & (DAC_CONTROL_INTERNAL | DAC_CONTROL_EXTERNAL))
	{
	    if (control & DAC_CONTROL_EXTERNAL)
	    {
		armv7m_atomic_modify(&DACx->MCR, DAC_MCR_MODE1, (0));
	    }
	    else
	    {
		armv7m_atomic_modify(&DACx->MCR, DAC_MCR_MODE1, (DAC_MCR_MODE1_0));
	    }

	    if (control & DAC_CONTROL_CALIBRATE)
	    {
		DACx->DHR12R1 = 0x800;

		DACx->CR |= DAC_CR_CEN1;

		trim = 16;
		step = 8;

		while (step)
		{
		    DACx->CCR = (DACx->CCR & ~DAC_CCR_OTRIM1) | (trim << 0);
		
		    armv7m_core_udelay(50);
		
		    if (DACx->SR & DAC_SR_CAL_FLAG1)
		    {
			trim -= step;
		    }
		    else
		    {
			trim += step;
		    }

		    step >>= 1;
		}

		DACx->CCR = (DACx->CCR & ~DAC_CCR_OTRIM1) | (trim << 0);

		armv7m_core_udelay(50);

		if (!(DACx->SR & DAC_SR_CAL_FLAG1))
		{
		    trim++;

		    DACx->CCR = (DACx->CCR & ~DAC_CCR_OTRIM1) | (trim << 0);

		    armv7m_core_udelay(50);
		}

		DACx->CR &= ~DAC_CR_CEN1;
	    }

	    DACx->CR |= DAC_CR_EN1;

	    DACx->DHR12R1 = output & DAC_DHR12R1_DACC1DHR;
	}
    }
    else
    {
	DACx->CR &= ~(DAC_CR_EN2 | DAC_CR_TEN2 | DAC_CR_WAVE2 | DAC_CR_DMAEN2 | DAC_CR_DMAUDRIE2 | DAC_CR_CEN2);

	if (control & (DAC_CONTROL_INTERNAL | DAC_CONTROL_EXTERNAL))
	{
	    if (control & DAC_CONTROL_EXTERNAL)
	    {
		armv7m_atomic_modify(&DACx->MCR, DAC_MCR_MODE2, (0));
	    }
	    else
	    {
		armv7m_atomic_modify(&DACx->MCR, DAC_MCR_MODE2, (DAC_MCR_MODE2_0));
	    }

	    if (control & DAC_CONTROL_CALIBRATE)
	    {
		DACx->DHR12R2 = 0x800;

		DACx->CR |= DAC_CR_CEN2;

		trim = 16;
		step = 8;

		while (step)
		{
		    DACx->CCR = (DACx->CCR & ~DAC_CCR_OTRIM2) | (trim << 16);
		
		    armv7m_core_udelay(50);
		
		    if (DACx->SR & DAC_SR_CAL_FLAG2)
		    {
			trim -= step;
		    }
		    else
		    {
			trim += step;
		    }

		    step >>= 1;
		}

		DACx->CCR = (DACx->CCR & ~DAC_CCR_OTRIM2) | (trim << 16);

		armv7m_core_udelay(50);

		if (!(DACx->SR & DAC_SR_CAL_FLAG2))
		{
		    trim++;

		    DACx->CCR = (DACx->CCR & ~DAC_CCR_OTRIM2) | (trim << 16);

		    armv7m_core_udelay(50);
		}

		DACx->CR &= ~DAC_CR_CEN2;
	    }

	    DACx->CR |= DAC_CR_EN2;

	    DACx->DHR12R2 = output & DAC_DHR12R2_DACC2DHR;
	}
    }
	
    return true;
}

bool stm32l4_dac_convert(stm32l4_dac_t *dac, unsigned int channel, uint32_t output)
{
    DAC_TypeDef *DACx = dac->DACx;

    if (dac->state != DAC_STATE_READY)
    {
	return false;
    }

    if (channel == DAC_CHANNEL_1)
    {
	DACx->DHR12R1 = output & DAC_DHR12R1_DACC1DHR;
    }
    else
    {
	DACx->DHR12R2 = output & DAC_DHR12R2_DACC2DHR;
    }
    
    return true;
}
