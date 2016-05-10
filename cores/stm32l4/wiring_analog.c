/*
  Copyright (c) 2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"
#include "wiring_private.h"

#ifdef __cplusplus
extern "C" {
#endif

static stm32l4_adc_t stm32l4_adc;
static stm32l4_dac_t stm32l4_dac;
static stm32l4_timer_t stm32l4_pwm[PWM_INSTANCE_COUNT];

static const unsigned int stm32l4_pwm_xlate_instance[PWM_INSTANCE_COUNT] = {
    TIMER_INSTANCE_TIM1,
    TIMER_INSTANCE_TIM3,
    TIMER_INSTANCE_TIM4,
    TIMER_INSTANCE_TIM5,
};

static int _readResolution = 10;
static int _writeResolution = 8;

static int _writeFrequency[PWM_INSTANCE_COUNT];
static int _writeRange[PWM_INSTANCE_COUNT];

static uint8_t _writeCalibrate = 3;

void analogReference(eAnalogReference reference)
{
}

void analogReadResolution(int resolution)
{
    _readResolution = resolution;
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
  
    if ( g_APinDescription[pin].attr & PIN_ATTR_DAC )
    {
	channel = ((pin == A0) ? DAC_CHANNEL_1 : DAC_CHANNEL_2);

	if (!(_writeCalibrate & (1ul << channel)))
	{
	    stm32l4_dac_channel(&stm32l4_dac, channel, 0, DAC_CONTROL_DISABLE);

	    _writeCalibrate |= (1ul << channel);
	}
    }

    if (stm32l4_adc.state == ADC_STATE_NONE)
    {
	stm32l4_adc_create(&stm32l4_adc, ADC_INSTANCE_ADC1, STM32L4_ADC_IRQ_PRIORITY, 0);
	stm32l4_adc_enable(&stm32l4_adc, 0, NULL, NULL, 0);
	stm32l4_adc_calibrate(&stm32l4_adc);
    }
    else
    {
	stm32l4_adc_enable(&stm32l4_adc, 0, NULL, NULL, 0);
    }

    stm32l4_gpio_pin_configure(g_APinDescription[pin].pin, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG | GPIO_ANALOG_SWITCH));

    input = stm32l4_adc_convert(&stm32l4_adc, g_APinDescription[pin].adc_input);

    stm32l4_adc_disable(&stm32l4_adc);

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

    if (g_APinDescription[pin].attr & PIN_ATTR_DAC)
    {
	if (stm32l4_dac.state == DAC_STATE_NONE)
	{
	    stm32l4_dac_create(&stm32l4_dac, DAC_INSTANCE_DAC, STM32L4_DAC_IRQ_PRIORITY, 0);
	    stm32l4_dac_enable(&stm32l4_dac, 0, NULL, NULL, 0);
	}
    
	stm32l4_gpio_pin_configure(g_APinDescription[pin].pin, (GPIO_PUPD_NONE | GPIO_MODE_ANALOG));
    
	value = mapResolution(value, _writeResolution, 12);

	channel = ((pin == A0) ? DAC_CHANNEL_1 : DAC_CHANNEL_2);

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

    if (g_APinDescription[pin].attr & PIN_ATTR_PWM)
    {
	instance = g_APinDescription[pin].pwm_instance;

	if (stm32l4_pwm[instance].state == TIMER_STATE_NONE)
	{
	    stm32l4_timer_create(&stm32l4_pwm[instance], stm32l4_pwm_xlate_instance[instance], STM32L4_PWM_IRQ_PRIORITY, 0);

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

	stm32l4_gpio_pin_configure(g_APinDescription[pin].pin, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));

	stm32l4_timer_channel(&stm32l4_pwm[instance], g_APinDescription[pin].pwm_channel, value, TIMER_CONTROL_PWM);

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
