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

#include "Arduino.h"
#include "stm32l4_wiring_private.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PIN_DAC0) || defined(PIN_DAC1)
static stm32l4_dac_t stm32l4_dac;
#endif /* defined(PIN_DAC0) || defined(PIN_DAC1) */
static stm32l4_timer_t stm32l4_pwm[PWM_INSTANCE_COUNT];

static uint8_t _channels[PWM_INSTANCE_COUNT];

static int _readResolution = 10;
static int _readPeriod = 2;
static int _writeResolution = 8;

static uint32_t _writeFrequency[PWM_INSTANCE_COUNT];
static uint32_t _writeRange[PWM_INSTANCE_COUNT];

static uint8_t _writeCalibrate = 3;

bool _analogReadFast = false;

void analogReference(eAnalogReference reference)
{
    (void)reference;
}

void analogReadResolution(int resolution)
{
    _readResolution = resolution;
}

void analogReadPeriod(int period)
{
    _readPeriod = period;
}

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to)
{
    if (from == to)
    {
	return value;
    }

    if (from > to)
    {
	return value >> (from-to);
    }
    else
    {
	return value << (to-from);
    }
}

uint32_t analogRead(uint32_t pin)
{
    uint32_t channel, input;

    if ( pin < A0 )
    {
	pin += A0 ;
    }

    if ( !(g_APinDescription[pin].attr & PIN_ATTR_ADC) )
    {
	return 0;
    }
  
#if defined(PIN_DAC0) || defined(PIN_DAC1)
    if ( g_APinDescription[pin].attr & PIN_ATTR_DAC )
    {
	channel = ((pin == PIN_DAC0) ? DAC_CHANNEL_1 : DAC_CHANNEL_2);

	if (!(_writeCalibrate & (1ul << channel)))
	{
	    stm32l4_dac_channel(&stm32l4_dac, channel, 0, DAC_CONTROL_DISABLE);

	    _writeCalibrate |= (1ul << channel);
	}
    }
#endif /* defined(PIN_DAC0) || defined(PIN_DAC1) */

    if (stm32l4_adc.state == ADC_STATE_NONE)
    {
	stm32l4_adc_create(&stm32l4_adc, ADC_INSTANCE_ADC1, STM32L4_ADC_IRQ_PRIORITY, 0);
	stm32l4_adc_enable(&stm32l4_adc, 0, NULL, NULL, 0);
	stm32l4_adc_calibrate(&stm32l4_adc);
    }
    else if (stm32l4_adc.state != ADC_STATE_READY)
    {
	stm32l4_adc_enable(&stm32l4_adc, 0, NULL, NULL, 0);
    }

    stm32l4_gpio_pin_configure(g_APinDescription[pin].pin, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG | GPIO_ANALOG_SWITCH));

    input = stm32l4_adc_convert(&stm32l4_adc, g_APinDescription[pin].adc_input, _readPeriod);

    if (!_analogReadFast)
    {
	stm32l4_adc_disable(&stm32l4_adc);
    }

    return mapResolution(input, 12, _readResolution);
}

void analogWriteResolution( int resolution )
{
    _writeResolution = resolution;
}

void analogWriteFrequency(uint32_t pin, uint32_t frequency)
{
    uint32_t instance, carrier, modulus, divider;

    if (g_APinDescription[pin].GPIO == NULL)
    {
	return;
    }
  
    if (g_APinDescription[pin].attr & PIN_ATTR_PWM)
    {
	instance = g_APinDescription[pin].pwm_instance;

	if (frequency <= stm32l4_timer_clock(&stm32l4_pwm[instance]))
	{
	    _writeFrequency[instance] = frequency;
	    
	    if (stm32l4_pwm[instance].state == TIMER_STATE_ACTIVE)
	    {
		if (_writeFrequency[instance] && _writeRange[instance])
		{
		    carrier = _writeFrequency[instance] * _writeRange[instance];
		    modulus = _writeRange[instance];
		}
		else
		{
		    carrier = 2000000;
		    modulus = 4095;
		}
		
		divider = stm32l4_timer_clock(&stm32l4_pwm[instance]) / carrier;

		if (divider == 0) 
		{
		    divider = 1;
		}

		stm32l4_timer_stop(&stm32l4_pwm[instance]);
		stm32l4_timer_configure(&stm32l4_pwm[instance], divider -1, modulus -1, 0);
		stm32l4_timer_start(&stm32l4_pwm[instance], false);
	    }
	}
    }
}

void analogWriteRange(uint32_t pin, uint32_t range)
{
    uint32_t instance, carrier, modulus, divider;

    if (g_APinDescription[pin].GPIO == NULL)
    {
	return;
    }
  
    if (g_APinDescription[pin].attr & PIN_ATTR_PWM)
    {
	instance = g_APinDescription[pin].pwm_instance;

	if (range <= 65536)
	{
	    _writeRange[instance] = range;
	    
	    if (stm32l4_pwm[instance].state == TIMER_STATE_ACTIVE)
	    {
		if (_writeFrequency[instance] && _writeRange[instance])
		{
		    carrier = _writeFrequency[instance] * _writeRange[instance];
		    modulus = _writeRange[instance];
		}
		else
		{
		    carrier = 2000000;
		    modulus = 4095;
		}
		
		divider = stm32l4_timer_clock(&stm32l4_pwm[instance]) / carrier;

		if (divider == 0) 
		{
		    divider = 1;
		}
		
		stm32l4_timer_stop(&stm32l4_pwm[instance]);
		stm32l4_timer_configure(&stm32l4_pwm[instance], divider -1, modulus -1, 0);
		stm32l4_timer_start(&stm32l4_pwm[instance], false);
	    }
	}
    }
}


// Right now, PWM output only works on the pins with
// hardware support.  These are defined in the appropriate
// pins_*.c file.  For the rest of the pins, we default
// to digital output.
void analogWrite(uint32_t pin, uint32_t value)
{
    uint32_t instance, channel, carrier, modulus, divider;

    // Handle the case the pin isn't usable as PIO
    if (g_APinDescription[pin].GPIO == NULL)
    {
	return;
    }

#if defined(PIN_DAC0) || defined(PIN_DAC1)
    if (g_APinDescription[pin].attr & PIN_ATTR_DAC)
    {
	if (stm32l4_dac.state == DAC_STATE_NONE)
	{
	    stm32l4_dac_create(&stm32l4_dac, DAC_INSTANCE_DAC, STM32L4_DAC_IRQ_PRIORITY, 0);
	    stm32l4_dac_enable(&stm32l4_dac, 0, NULL, NULL, 0);
	}
    
	stm32l4_gpio_pin_configure(g_APinDescription[pin].pin, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    
	value = mapResolution(value, _writeResolution, 12);

	channel = ((pin == PIN_DAC0) ? DAC_CHANNEL_1 : DAC_CHANNEL_2);

	if (_writeCalibrate & (1ul << channel))
	{
	    _writeCalibrate &= ~(1ul << channel);

	    stm32l4_dac_channel(&stm32l4_dac, channel, value, DAC_CONTROL_EXTERNAL | DAC_CONTROL_CALIBRATE);
	}
	else
	{
	    stm32l4_dac_convert(&stm32l4_dac, channel, value);
	}

	return;
    }
#endif /* defined(PIN_DAC0) || defined(PIN_DAC1) */

    if (g_APinDescription[pin].attr & PIN_ATTR_PWM)
    {
	instance = g_APinDescription[pin].pwm_instance;

	if (_writeFrequency[instance] && _writeRange[instance])
	{
	    if (value > _writeRange[instance])
	    {
		value = _writeRange[instance];
	    }
	}
	else
	{
	    value = mapResolution(value, _writeResolution, 12);
	}

	if (_channels[instance] & (1u << g_APinDescription[pin].pwm_channel))
	{
	    stm32l4_timer_compare(&stm32l4_pwm[instance], g_APinDescription[pin].pwm_channel, value);
	}
	else
	{
	    _channels[instance] |= (1u << g_APinDescription[pin].pwm_channel);

	    if (stm32l4_pwm[instance].state == TIMER_STATE_NONE)
	    {
		stm32l4_timer_create(&stm32l4_pwm[instance], g_PWMInstances[instance], STM32L4_PWM_IRQ_PRIORITY, 0);
		
		if (_writeFrequency[instance] && _writeRange[instance])
		{
		    carrier = _writeFrequency[instance] * _writeRange[instance];
		    modulus = _writeRange[instance];
		}
		else
		{
		    carrier = 2000000;
		    modulus = 4095;
		}
		
		divider = stm32l4_timer_clock(&stm32l4_pwm[instance]) / carrier;
		
		if (divider == 0)
		{
		    divider = 1;
		}
		
		stm32l4_timer_enable(&stm32l4_pwm[instance], divider -1, modulus -1, 0, NULL, NULL, 0);
		stm32l4_timer_start(&stm32l4_pwm[instance], false);
	    }
	    
	    stm32l4_gpio_pin_configure(g_APinDescription[pin].pin, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
	    
	    stm32l4_timer_channel(&stm32l4_pwm[instance], g_APinDescription[pin].pwm_channel, value, TIMER_CONTROL_PWM);
	}

	return;
    }

    // -- Defaults to digital write
    pinMode(pin, OUTPUT) ;
    value = mapResolution(value, _writeResolution, 8);
    if (value < 128 )
    {
	digitalWrite(pin, LOW);
    }
    else
    {
	digitalWrite(pin, HIGH );
    }
}

#ifdef __cplusplus
}
#endif
