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
#include "wiring_private.h"

uint64_t STM32Class::getSerial()
{
    uint32_t serial0, serial1, serial2;

    /* This crummy value is what the USB/DFU bootloader uses.
     */

    serial0 = *((uint32_t*)0x1fff7590);
    serial1 = *((uint32_t*)0x1fff7594);
    serial2 = *((uint32_t*)0x1fff7598);

    serial0 += serial2;

    return (((uint64_t)serial0 << 16) | ((uint64_t)serial1 >> 16));
}

float STM32Class::getVBAT()
{
    int32_t vbat_data, vref_data, vrefint;
    
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

    vref_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VREFINT);
    vbat_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VBAT);

    stm32l4_adc_disable(&stm32l4_adc);

    /* Datasheet 3.15.1 */
    vrefint = *((uint16_t*)0x1fff75aa);

    return 3.0 * ((3.0 * vbat_data * vrefint) / (4095.0 * vref_data));
}

float STM32Class::getVREF()
{
    int32_t vref_data, vrefint;
    
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

    vref_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VREFINT);

    stm32l4_adc_disable(&stm32l4_adc);

    /* Datasheet 3.15.1 */
    vrefint = *((uint16_t*)0x1fff75aa);

    return (3.0 * vrefint) / vref_data;
}

float STM32Class::getTemperature()
{
    int32_t ts_data, ts_cal1, ts_cal2, vref_data, vrefint;

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

    vref_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VREFINT);
    ts_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_TS);

    stm32l4_adc_disable(&stm32l4_adc);

    /* Datasheet 3.15.1 */
    ts_cal1  = *((uint16_t*)0x1fff75a8);
    ts_cal2  = *((uint16_t*)0x1fff75ca);

    /* Datasheet 3.15.2 */
    vrefint = *((uint16_t*)0x1fff75aa);

    /* Compensate TS_DATA for VDDA vs. 3.0 */
    ts_data = (ts_data * vrefint) / vref_data;

    return (30.0 + ((float)(110.0 - 30.0) * (float)(ts_data - ts_cal1)) / (float)(ts_cal2 - ts_cal1));
}

STM32Class STM32;
