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

void STM32Class::getUID(uint32_t uid[3])
{
    uid[0] = *((uint32_t*)0x1fff7590);
    uid[1] = *((uint32_t*)0x1fff7594);
    uid[2] = *((uint32_t*)0x1fff7598);
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

    vref_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VREFINT, ADC_VREFINT_PERIOD);
    vbat_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VBAT, ADC_VBAT_PERIOD);

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

    vref_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VREFINT, ADC_VREFINT_PERIOD);

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

    vref_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_VREFINT, ADC_VREFINT_PERIOD);
    ts_data = stm32l4_adc_convert(&stm32l4_adc, ADC_CHANNEL_ADC1_TS, ADC_TSENSE_PERIOD);

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

uint32_t STM32Class::resetCause()
{
    return stm32l4_system_reset_cause();
}

uint32_t STM32Class::wakeupReason()
{
    return stm32l4_system_wakeup_reason();
}

void STM32Class::sleep()
{
    __WFE();
    __SEV();
    __WFE();
}

bool STM32Class::stop(uint32_t timeout)
{
    return stm32l4_system_stop(timeout);
}

void STM32Class::standby(uint32_t timeout)
{
    stm32l4_system_standby(0, timeout);
}

void STM32Class::standby(uint32_t pin, uint32_t mode, uint32_t timeout)
{
    uint32_t wakeup;

    if (!(g_APinDescription[pin].attr & PIN_ATTR_WKUP_MASK))
	return;

    if ((mode != RISING) && (mode != FALLING))
	return;

    wakeup = ((mode == RISING) ? SYSTEM_CONFIG_WKUP1_RISING :  SYSTEM_CONFIG_WKUP1_FALLING) << (((g_APinDescription[pin].attr & PIN_ATTR_WKUP_MASK) >> PIN_ATTR_WKUP_SHIFT) - PIN_ATTR_WKUP_OFFSET);

    stm32l4_system_standby(wakeup, timeout);
}

void STM32Class::shutdown(uint32_t timeout)
{
    stm32l4_system_shutdown(0, timeout);
}

void STM32Class::shutdown(uint32_t pin, uint32_t mode, uint32_t timeout)
{
    uint32_t wakeup;

    if (!(g_APinDescription[pin].attr & PIN_ATTR_WKUP_MASK))
	return;

    if ((mode != RISING) && (mode != FALLING))
	return;

    wakeup = ((mode == RISING) ? SYSTEM_CONFIG_WKUP1_RISING :  SYSTEM_CONFIG_WKUP1_FALLING) << (((g_APinDescription[pin].attr & PIN_ATTR_WKUP_MASK) >> PIN_ATTR_WKUP_SHIFT) - PIN_ATTR_WKUP_OFFSET);

    stm32l4_system_shutdown(wakeup, timeout);
}

void STM32Class::reset()
{
    stm32l4_system_reset();
}

void  STM32Class::wdtEnable(uint32_t timeout)
{
    stm32l4_iwdg_enable(timeout);
}

void  STM32Class::wdtReset()
{
    stm32l4_iwdg_reset();
}


bool STM32Class::flashErase(uint32_t address, uint32_t count)
{
    if (address & 2047) {
	return false;
    }

    count = (count + 2047) & ~2047;

    if ((address < FLASHSTART) || ((address + count) > FLASHEND)) {
	return false;
    }

    stm32l4_flash_unlock();
    stm32l4_flash_erase(address, count);
    stm32l4_flash_lock();
    
    return true;
}

bool STM32Class::flashProgram(uint32_t address, const void *data, uint32_t count)
{
    if ((address & 7) || (count & 7)) {
	return false;
    }

    if ((address < FLASHSTART) || ((address + count) > FLASHEND)) {
	return false;
    }

    if (count)
    {
	stm32l4_flash_unlock();
	stm32l4_flash_program(address, (const uint8_t*)data, count);
	stm32l4_flash_lock();
    }

    return true;
}

void STM32Class::lsco(bool enable)
{
    stm32l4_system_lsco_configure((enable ? SYSTEM_LSCO_MODE_LSE : SYSTEM_LSCO_MODE_NONE));
}

STM32Class STM32;
