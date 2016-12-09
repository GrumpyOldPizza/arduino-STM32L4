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

#include <stdlib.h>

#include "armv7m.h"

#include "stm32l4_rtc.h"

typedef struct _stm32l4_rtc_device_t {
    stm32l4_rtc_callback_t alarm_callback[2];
    void                   *alarm_context[2];
    stm32l4_rtc_callback_t sync_callback;
    void                   *sync_context;
} stm32l4_rtc_device_t;

static stm32l4_rtc_device_t stm32l4_rtc_device;

void stm32l4_rtc_configure(unsigned int priority)
{
    armv7m_atomic_or(&EXTI->IMR1, (EXTI_IMR1_IM18 | EXTI_IMR1_IM19));
    armv7m_atomic_or(&EXTI->EMR1, (EXTI_EMR1_EM18 | EXTI_EMR1_EM19 | EXTI_EMR1_EM20));
    armv7m_atomic_or(&EXTI->RTSR1, (EXTI_RTSR1_RT18 | EXTI_RTSR1_RT19 | EXTI_RTSR1_RT20));

    EXTI->PR1 = (EXTI_PR1_PIF18 | EXTI_PR1_PIF19 | EXTI_PR1_PIF20);

    NVIC_SetPriority(TAMP_STAMP_IRQn, priority);
    NVIC_EnableIRQ(TAMP_STAMP_IRQn);

    NVIC_SetPriority(RTC_Alarm_IRQn, priority);
    NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

void stm32l4_rtc_set_time(unsigned int mask, const stm32l4_rtc_time_t *time)
{
    uint32_t n_tr, n_dr, m_tr, m_dr, o_tr, o_dr;
    
    n_tr = 0;
    n_dr = 0;

    m_tr = 0;
    m_dr = 0;
    
    if (mask & RTC_TIME_MASK_SECOND)
    {
	n_tr |= (((time->second % 10) << RTC_TR_SU_Pos) | ((time->second / 10) << RTC_TR_ST_Pos));
	m_tr |= (RTC_TR_SU_Msk | RTC_TR_ST_Msk);
    }
    
    if (mask & RTC_TIME_MASK_MINUTE)
    {
	n_tr |= (((time->minute % 10) << RTC_TR_MNU_Pos) | ((time->minute / 10) << RTC_TR_MNT_Pos));
	m_tr |= (RTC_TR_MNU_Msk | RTC_TR_MNT_Msk);
    }

    if (mask & RTC_TIME_MASK_HOUR)
    {
	n_tr |= (((time->hour % 10) << RTC_TR_HU_Pos) | ((time->hour / 10) << RTC_TR_HT_Pos));
	m_tr |= (RTC_TR_HU_Msk | RTC_TR_HT_Msk);
    }

    if (mask & RTC_TIME_MASK_DAY)
    {
	n_dr |= (((time->day % 10) << RTC_DR_DU_Pos) | ((time->day / 10) << RTC_DR_DT_Pos));
	m_dr |= (RTC_DR_DU_Msk | RTC_DR_DT_Msk);
    }

    if (mask & RTC_TIME_MASK_MONTH)
    {
	n_dr |= (((time->month % 10) << RTC_DR_MU_Pos) | ((time->month / 10) << RTC_DR_MT_Pos));
	m_dr |= (RTC_DR_MU_Msk | RTC_DR_MT_Msk);
    }

    if (mask & RTC_TIME_MASK_YEAR)
    {
	n_dr |= (((time->year % 10) << RTC_DR_YU_Pos) | ((time->year / 10) << RTC_DR_YT_Pos));
	m_dr |= (RTC_DR_YU_Msk | RTC_DR_YT_Msk);
    }
    
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;
    
    RTC->ISR |= RTC_ISR_INIT;
    
    while (!(RTC->ISR & RTC_ISR_INITF))
    {
    }
    
    o_tr = RTC->TR;
    o_dr = RTC->DR;

    RTC->TR = (o_tr & ~m_tr) | (n_tr & m_tr);
    RTC->DR = (o_dr & ~m_dr) | (n_dr & m_dr);
    RTC->CR |= RTC_CR_BYPSHAD;
    
    RTC->ISR &= ~RTC_ISR_INIT;
    
    RTC->WPR = 0x00;
}

void stm32l4_rtc_get_time(stm32l4_rtc_time_t *p_time_return)
{
    uint32_t o_tr, o_dr, o_ssr;

    do
    {
	o_ssr = RTC->SSR;
	o_tr = RTC->TR;
	o_dr = RTC->DR;
    }
    while (o_ssr != RTC->SSR);

    p_time_return->year   = ((o_dr & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos) + (((o_dr & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos) * 10);
    p_time_return->month  = ((o_dr & RTC_DR_MU_Msk) >> RTC_DR_MU_Pos) + (((o_dr & RTC_DR_MT_Msk) >> RTC_DR_MT_Pos) * 10);
    p_time_return->day    = ((o_dr & RTC_DR_DU_Msk) >> RTC_DR_DU_Pos) + (((o_dr & RTC_DR_DT_Msk) >> RTC_DR_DT_Pos) * 10);
    p_time_return->hour   = ((o_tr & RTC_TR_HU_Msk) >> RTC_TR_HU_Pos) + (((o_tr & RTC_TR_HT_Msk) >> RTC_TR_HT_Pos) * 10);
    p_time_return->minute = ((o_tr & RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos) + (((o_tr & RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos) * 10);
    p_time_return->second = ((o_tr & RTC_TR_SU_Msk) >> RTC_TR_SU_Pos) + (((o_tr & RTC_TR_ST_Msk) >> RTC_TR_ST_Pos) * 10);
    p_time_return->ticks  = 32767 - (o_ssr & 32767);
}

static const uint16_t stm32l4_rtc_days_since_month[2][12] = {
    {   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, },
    {   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335, },
};

uint64_t stm32l4_rtc_get_count(void)
{
    uint32_t o_tr, o_dr, o_ssr;
    uint32_t year, month, day, hour, minute, second, ticks, epoch;

    do
    {
	o_ssr = RTC->SSR;
	o_tr = RTC->TR;
	o_dr = RTC->DR;
    }
    while (o_ssr != RTC->SSR);

    year   = ((o_dr & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos) + (((o_dr & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos) * 10);
    month  = ((o_dr & RTC_DR_MU_Msk) >> RTC_DR_MU_Pos) + (((o_dr & RTC_DR_MT_Msk) >> RTC_DR_MT_Pos) * 10);
    day    = ((o_dr & RTC_DR_DU_Msk) >> RTC_DR_DU_Pos) + (((o_dr & RTC_DR_DT_Msk) >> RTC_DR_DT_Pos) * 10);
    hour   = ((o_tr & RTC_TR_HU_Msk) >> RTC_TR_HU_Pos) + (((o_tr & RTC_TR_HT_Msk) >> RTC_TR_HT_Pos) * 10);
    minute = ((o_tr & RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos) + (((o_tr & RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos) * 10);
    second = ((o_tr & RTC_TR_SU_Msk) >> RTC_TR_SU_Pos) + (((o_tr & RTC_TR_ST_Msk) >> RTC_TR_ST_Pos) * 10);
    ticks  = 32767 - (o_ssr & 32767);

    epoch = ((((((((year ? (year * 365 + 1 + ((year - 1) / 4)) : 0) + stm32l4_rtc_days_since_month[(year & 3) == 0][month -1] + (day -1)) * 24) + hour) * 60) + minute) * 60) + second);
    
    return (((uint64_t)epoch * 32768) + (uint64_t)ticks);
}

void stm32l4_rtc_set_calibration(int32_t calibration)
{
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;

    if (calibration > 0)
    {
	if (calibration > 512) {
	    calibration = 512;
	}

	RTC->CALR = RTC_CALR_CALP | ((uint32_t)(512 - calibration) << RTC_CALR_CALM_Pos);
    }
    else
    {
	if (calibration < -511) {
	    calibration = -511;
	}

	RTC->CALR = ((uint32_t)(-calibration) << RTC_CALR_CALM_Pos);
    }

    while (RTC->ISR & RTC_ISR_RECALPF)
    {
    }

    RTC->WPR = 0x00;
}

int32_t stm32l4_rtc_get_calibration(void)
{
    if (RTC->CALR & RTC_CALR_CALP)
    {
	return 512 - ((int32_t)((RTC->CALR & RTC_CALR_CALM_Msk) >> RTC_CALR_CALM_Pos));
    }
    else
    {
	return - ((int32_t)((RTC->CALR & RTC_CALR_CALM_Msk) >> RTC_CALR_CALM_Pos));
    }
}

void stm32l4_rtc_pin_configure(unsigned int mode)
{
    uint32_t rtc_cr;

    rtc_cr = RTC->CR & ~(RTC_CR_COE | RTC_CR_OSEL | RTC_CR_POL | RTC_CR_COSEL | RTC_CR_TSE | RTC_CR_TSEDGE);

    RTC->WPR = 0xca;
    RTC->WPR = 0x53;

    switch (mode) {
    default:
    case RTC_PIN_MODE_GPIO:
	RTC->CR = rtc_cr;
	break;
    case RTC_PIN_MODE_INPUT_SYNC:
	RTC->CR = rtc_cr | RTC_CR_TSE;
	break;
    case RTC_PIN_MODE_OUTPUT_512HZ:
	RTC->CR = rtc_cr | RTC_CR_COE;
	break;
    case RTC_PIN_MODE_OUTPUT_1HZ:
	RTC->CR = rtc_cr | RTC_CR_COE | RTC_CR_COSEL;
	break;
    case RTC_PIN_MODE_OUTPUT_ALARM_A:
	RTC->CR = rtc_cr | RTC_CR_OSEL_0;
	break;
    case RTC_PIN_MODE_OUTPUT_ALARM_B:
	RTC->CR = rtc_cr | RTC_CR_OSEL_1;
	break;
    case RTC_PIN_MODE_OUTPUT_WAKEUP:
	RTC->CR = rtc_cr | RTC_CR_OSEL_0 | RTC_CR_OSEL_1;
	break;
    }

    RTC->WPR = 0x00;
}

void stm32l4_rtc_adjust_ticks(int32_t ticks)
{
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;

    if (ticks > 0) 
    {
	if (ticks > 32768) {
	    ticks = 32768;
	}

	RTC->SHIFTR = RTC_SHIFTR_ADD1S | ((uint32_t)(32768 - ticks) << RTC_SHIFTR_SUBFS_Pos);
    }
    else
    {
	if (ticks < -32767) {
	    ticks = -32767;
	}

	RTC->SHIFTR = ((uint32_t)(-ticks) << RTC_SHIFTR_SUBFS_Pos);
    }

    RTC->WPR = 0x00;

    while (RTC->ISR & RTC_ISR_SHPF)
    {
    }
}

uint32_t stm32l4_rtc_get_ticks(void)
{
    return 32767 - (RTC->SSR & 32767);
}

void stm32l4_rtc_alarm(unsigned int channel, unsigned int match, const stm32l4_rtc_alarm_t *alarm, stm32l4_rtc_callback_t callback, void *context)
{
    uint32_t alrmr;

    channel &= 1;

    RTC->WPR = 0xca;
    RTC->WPR = 0x53;

    RTC->CR &= ~((RTC_CR_ALRAIE | RTC_CR_ALRAE) << channel);

    RTC->ISR &= ~RTC_ISR_ALRAF;

    stm32l4_rtc_device.alarm_callback[channel] = callback;
    stm32l4_rtc_device.alarm_context[channel] = context;

    if (callback)
    {
	alrmr = 0;

	if (match & RTC_ALARM_MATCH_SECOND) 
	{
	    alrmr |= (((alarm->second % 10) << RTC_ALRMAR_SU_Pos) | ((alarm->second / 10) << RTC_ALRMAR_ST_Pos));
	}
	else 
	{
	    alrmr |= RTC_ALRMAR_MSK1;
	}

	if (match & RTC_ALARM_MATCH_MINUTE) 
	{
	    alrmr |= (((alarm->minute % 10) << RTC_ALRMAR_MNU_Pos) | ((alarm->minute / 10) << RTC_ALRMAR_MNT_Pos));
	}
	else 
	{
	    alrmr |= RTC_ALRMAR_MSK2;
	}

	if (match & RTC_ALARM_MATCH_HOUR) 
	{
	    alrmr |= (((alarm->hour % 10) << RTC_ALRMAR_HU_Pos) | ((alarm->hour / 10) << RTC_ALRMAR_HT_Pos));
	}
	else 
	{
	    alrmr |= RTC_ALRMAR_MSK3;
	}

	if (match & RTC_ALARM_MATCH_DAY) 
	{
	    alrmr |= (((alarm->day % 10) << RTC_ALRMAR_DU_Pos) | ((alarm->day / 10) << RTC_ALRMAR_DT_Pos));
	}
	else 
	{
	    alrmr |= RTC_ALRMAR_MSK4;
	}

	while (!(RTC->ISR & (RTC_ISR_ALRAWF << channel)))
	{
	}

	if (channel == 0)
	{
	    RTC->ALRMAR = alrmr;
	    RTC->ALRMASSR = 0;
	}
	else
	{
	    RTC->ALRMBR = alrmr;
	    RTC->ALRMBSSR = 0;
	}

	RTC->CR |= ((RTC_CR_ALRAIE | RTC_CR_ALRAE) << channel);
    }

    RTC->WPR = 0x00;
}

void stm32l4_rtc_wakeup(uint32_t timeout)
{
    uint32_t ticks;

    RTC->WPR = 0xca;
    RTC->WPR = 0x53;

    RTC->CR &= ~(RTC_CR_WUTIE | RTC_CR_WUTE);

    RTC->ISR &= ~RTC_ISR_WUTF;

    EXTI->PR1 = EXTI_PR1_PIF20;

    while (!(RTC->ISR & RTC_ISR_WUTWF))
    {
    }

    if (timeout)
    {
        if (timeout <= (((65536 * 16) / 32768) * 1000))
	{
	    ticks = (timeout * 32768 + 500) / 1000;

	    if (ticks <= (65536 * 2))
	    {
		RTC->WUTR = (ticks / 2) - 1;
		RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL) | (RTC_CR_WUCKSEL_1 | RTC_CR_WUCKSEL_0);
	    }
	    else if (ticks <= (65536 * 4))
	    {
		RTC->WUTR = (ticks / 4) - 1;
		RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL) | (RTC_CR_WUCKSEL_1);
	    }
	    else if (ticks <= (65536 * 8))
	    {
		RTC->WUTR = (ticks / 8) -1;
		RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL) | (RTC_CR_WUCKSEL_0);
	    }
	    else
	    {
		RTC->WUTR = (ticks / 16) - 1;
		RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL);
	    }
	}
	else
	{
	    if (timeout <= (65536 * 1000))
	    {
		RTC->WUTR = ((timeout + 500) / 1000) - 1;
		RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL) | (RTC_CR_WUCKSEL_2);
	    }
	    else
	    {
		RTC->WUTR = ((timeout - (65536 * 1000) + 500) / 1000) - 1;
		RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL) | (RTC_CR_WUCKSEL_2 | RTC_CR_WUCKSEL_1);
	    }
	}

	RTC->CR |= (RTC_CR_WUTIE | RTC_CR_WUTE);
    }

    RTC->WPR = 0x00;
}


bool stm32l4_rtc_get_sync(stm32l4_rtc_sync_t *p_sync_return)
{
    uint32_t s_tr, s_dr, s_ssr;

    s_tr = RTC->TSTR;
    s_dr = RTC->TSDR;
    s_ssr = RTC->TSSSR;

    p_sync_return->month  = ((s_dr & RTC_TSDR_MU_Msk) >> RTC_TSDR_MU_Pos) + (((s_dr & RTC_TSDR_MT_Msk) >> RTC_TSDR_MT_Pos) * 10);
    p_sync_return->day    = ((s_dr & RTC_TSDR_DU_Msk) >> RTC_TSDR_DU_Pos) + (((s_dr & RTC_TSDR_DT_Msk) >> RTC_TSDR_DT_Pos) * 10);
    p_sync_return->hour   = ((s_tr & RTC_TSTR_HU_Msk) >> RTC_TSTR_HU_Pos) + (((s_tr & RTC_TSTR_HT_Msk) >> RTC_TSTR_HT_Pos) * 10);
    p_sync_return->minute = ((s_tr & RTC_TSTR_MNU_Msk) >> RTC_TSTR_MNU_Pos) + (((s_tr & RTC_TSTR_MNT_Msk) >> RTC_TSTR_MNT_Pos) * 10);
    p_sync_return->second = ((s_tr & RTC_TSTR_SU_Msk) >> RTC_TSTR_SU_Pos) + (((s_tr & RTC_TSTR_ST_Msk) >> RTC_TSTR_ST_Pos) * 10);
    p_sync_return->ticks  = 32767 - (s_ssr & 32767);

    if (!(RTC->ISR & RTC_ISR_TSF)) {
	return false;
    }

    RTC->ISR &= ~RTC_ISR_TSF;

    return true;
}

void stm32l4_rtc_notify_sync(stm32l4_rtc_callback_t callback, void *context)
{
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;

    RTC->CR &= ~RTC_CR_TSIE;

    RTC->ISR &= ~RTC_ISR_TSF;

    stm32l4_rtc_device.sync_callback = callback;
    stm32l4_rtc_device.sync_context = context;

    if (callback) {
	RTC->CR |= RTC_CR_TSIE;
    }

    RTC->WPR = 0x00;
}

uint32_t stm32l4_rtc_read_backup(unsigned int index)
{
    if (index <= 31) {
	return (&RTC->BKP0R)[index];
    } else {
	return 0x00000000;
    }
}

void stm32l4_rtc_write_backup(unsigned int index, uint32_t data)
{
    if (index <= 31) {
	(&RTC->BKP0R)[index] = data;
    }
}

void TAMP_STAMP_IRQHandler(void)
{
    if (RTC->ISR & RTC_ISR_TSF)
    {
	RTC->ISR &= ~RTC_ISR_TSF;

	if (stm32l4_rtc_device.sync_callback) {
	    (*stm32l4_rtc_device.sync_callback)(stm32l4_rtc_device.sync_context);
	}
    }

    EXTI->PR1 = EXTI_PR1_PIF19;
}

void RTC_Alarm_IRQHandler(void)
{
    if (RTC->ISR & RTC_ISR_ALRAF)
    {
	RTC->ISR &= ~RTC_ISR_ALRAF;

	if (stm32l4_rtc_device.alarm_callback[0]) {
	    (*stm32l4_rtc_device.alarm_callback[0])(stm32l4_rtc_device.alarm_context[0]);
	}
    }

    if (RTC->ISR & RTC_ISR_ALRBF)
    {
	RTC->ISR &= ~RTC_ISR_ALRBF;

	if (stm32l4_rtc_device.alarm_callback[1]) {
	    (*stm32l4_rtc_device.alarm_callback[1])(stm32l4_rtc_device.alarm_context[1]);
	}
    }

    EXTI->PR1 = EXTI_PR1_PIF18;
}
