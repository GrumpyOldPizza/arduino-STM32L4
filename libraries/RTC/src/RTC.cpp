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
#include "RTC.h"
#include <time.h>

#define EPOCH_TIME_OFF      946684800  // This is 1st January 2000, 00:00:00 in epoch time
#define EPOCH_TIME_YEAR_OFF 100        // years since 1900

void RTCClass::enableAlarm(AlarmMatch match)
{
    if (!_alarm_init) {
	InitAlarm();
    }

    _alarm_enable = 1;
    _alarm_match = match;

    SyncAlarm();
}

void RTCClass::disableAlarm()
{
    _alarm_enable = 0;

    stm32l4_rtc_set_alarm(0, 0, NULL, NULL, NULL);
}

void RTCClass::attachInterrupt(void(*callback)(void))
{
    if (!_alarm_init) {
	InitAlarm();
    }

    _alarm_callback = callback;

    SyncAlarm();
}

void RTCClass::detachInterrupt()
{
    _alarm_callback = NULL; 

    SyncAlarm();
}

/*
 * Get Functions
 */

uint8_t RTCClass::getSeconds()
{
    stm32l4_rtc_time_t rtc_time;

    stm32l4_rtc_get_time(&rtc_time);

    return rtc_time.second;
}

uint8_t RTCClass::getMinutes()
{
    stm32l4_rtc_time_t rtc_time;

    stm32l4_rtc_get_time(&rtc_time);

    return rtc_time.minute;
}

uint8_t RTCClass::getHours()
{
    stm32l4_rtc_time_t rtc_time;

    stm32l4_rtc_get_time(&rtc_time);

    return rtc_time.hour;
}

uint8_t RTCClass::getDay()
{
    stm32l4_rtc_time_t rtc_time;

    stm32l4_rtc_get_time(&rtc_time);

    return rtc_time.day;
}

uint8_t RTCClass::getMonth()
{
    stm32l4_rtc_time_t rtc_time;

    stm32l4_rtc_get_time(&rtc_time);

    return rtc_time.month;
}

uint8_t RTCClass::getYear()
{
    stm32l4_rtc_time_t rtc_time;

    stm32l4_rtc_get_time(&rtc_time);

    return rtc_time.year;
}

uint8_t RTCClass::getAlarmSeconds()
{
    if (!_alarm_init) {
	InitAlarm();
    }

    return _alarm_second;
}

uint8_t RTCClass::getAlarmMinutes()
{
    if (!_alarm_init) {
	InitAlarm();
    }

    return _alarm_minute;
}

uint8_t RTCClass::getAlarmHours()
{
    if (!_alarm_init) {
	InitAlarm();
    }

    return _alarm_hour;
}

uint8_t RTCClass::getAlarmDay()
{
    if (!_alarm_init) {
	InitAlarm();
    }

    return _alarm_day;
}

/*
 * Set Functions
 */

void RTCClass::setSeconds(uint8_t seconds)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.second = seconds;

    stm32l4_rtc_set_time(RTC_TIME_MASK_SECOND, &rtc_time);
}

void RTCClass::setMinutes(uint8_t minutes)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.minute = minutes;

    stm32l4_rtc_set_time(RTC_TIME_MASK_MINUTE, &rtc_time);
}

void RTCClass::setHours(uint8_t hours)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.hour = hours;

    stm32l4_rtc_set_time(RTC_TIME_MASK_HOUR, &rtc_time);
}

void RTCClass::setTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.hour = hours;
    rtc_time.minute = minutes;
    rtc_time.second = seconds;

    stm32l4_rtc_set_time((RTC_TIME_MASK_SECOND | RTC_TIME_MASK_MINUTE | RTC_TIME_MASK_HOUR), &rtc_time);
}

void RTCClass::setDay(uint8_t day)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.day = day;

    stm32l4_rtc_set_time(RTC_TIME_MASK_DAY, &rtc_time);
}

void RTCClass::setMonth(uint8_t month)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.month = month;

    stm32l4_rtc_set_time(RTC_TIME_MASK_MONTH, &rtc_time);
}

void RTCClass::setYear(uint8_t year)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.year = year;

    stm32l4_rtc_set_time(RTC_TIME_MASK_YEAR, &rtc_time);
}

void RTCClass::setDate(uint8_t day, uint8_t month, uint8_t year)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.year = year;
    rtc_time.month = month;
    rtc_time.day = day;

    stm32l4_rtc_set_time((RTC_TIME_MASK_DAY | RTC_TIME_MASK_MONTH | RTC_TIME_MASK_YEAR), &rtc_time);
}

void RTCClass::setAlarmSeconds(uint8_t seconds)
{
    if (!_alarm_init) {
	InitAlarm();
    }

    _alarm_second = seconds;

    SyncAlarm();
}

void RTCClass::setAlarmMinutes(uint8_t minutes)
{
    if (!_alarm_init) {
	InitAlarm();
    }

    _alarm_minute = minutes;

    SyncAlarm();
}

void RTCClass::setAlarmHours(uint8_t hours)
{
    if (!_alarm_init) {
	InitAlarm();
    }

    _alarm_hour = hours;

    SyncAlarm();
}

void RTCClass::setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    if (!_alarm_init) {
	InitAlarm();
    }

    _alarm_hour = hours;
    _alarm_minute = minutes;
    _alarm_second = seconds;

    SyncAlarm();
}

void RTCClass::setAlarmDay(uint8_t day)
{
    if (!_alarm_init) {
	InitAlarm();
    }

    _alarm_day = day;

    SyncAlarm();
}

uint32_t RTCClass::getEpoch()
{
    stm32l4_rtc_time_t rtc_time;
    struct tm tm;

    stm32l4_rtc_get_time(&rtc_time);

    tm.tm_isdst = -1;
    tm.tm_yday = 0;
    tm.tm_wday = 0;
    tm.tm_year = rtc_time.year + EPOCH_TIME_YEAR_OFF;
    tm.tm_mon = rtc_time.month - 1;
    tm.tm_mday = rtc_time.day;
    tm.tm_hour = rtc_time.hour;
    tm.tm_min = rtc_time.minute;
    tm.tm_sec = rtc_time.second;
    
    return mktime(&tm);
}

uint32_t RTCClass::getY2kEpoch()
{
    return (getEpoch() - EPOCH_TIME_OFF);
}

void RTCClass::setEpoch(uint32_t ts)
{
    stm32l4_rtc_time_t rtc_time;
    time_t t = ts;
    struct tm tm;

    if (ts < EPOCH_TIME_OFF) {
      ts = EPOCH_TIME_OFF;
    }

    t = (time_t)ts;
    gmtime_r(&t, &tm);

    rtc_time.year = tm.tm_year - EPOCH_TIME_YEAR_OFF;
    rtc_time.month = tm.tm_mon + 1;
    rtc_time.day = tm.tm_mday;
    rtc_time.hour = tm.tm_hour;
    rtc_time.minute = tm.tm_min;
    rtc_time.second = tm.tm_sec;

    stm32l4_rtc_set_time((RTC_TIME_MASK_SECOND | RTC_TIME_MASK_MINUTE | RTC_TIME_MASK_HOUR | RTC_TIME_MASK_DAY | RTC_TIME_MASK_MONTH | RTC_TIME_MASK_YEAR), &rtc_time);
}

void RTCClass::setY2kEpoch(uint32_t ts)
{
    setEpoch(ts + EPOCH_TIME_OFF);
}

void RTCClass::setCalendar(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day, uint8_t month, uint8_t year)
{
    stm32l4_rtc_time_t rtc_time;

    rtc_time.year = year;
    rtc_time.month = month;
    rtc_time.day = day;
    rtc_time.hour = hours;
    rtc_time.minute = minutes;
    rtc_time.second = seconds;

    stm32l4_rtc_set_time((RTC_TIME_MASK_SECOND | RTC_TIME_MASK_MINUTE | RTC_TIME_MASK_HOUR | RTC_TIME_MASK_DAY | RTC_TIME_MASK_MONTH | RTC_TIME_MASK_YEAR), &rtc_time);
}

uint16_t RTCClass::getTicks()
{
    return stm32l4_rtc_get_ticks();
}

int32_t RTCClass::getCalibration()
{
    return stm32l4_rtc_get_calibration();
}

void RTCClass::setCalibration(int32_t calibration)
{
    stm32l4_rtc_set_calibration(calibration);
}

void RTCClass::adjustTicks(int32_t ticks)
{
    stm32l4_rtc_adjust_ticks(ticks);
}

bool RTCClass::syncOccured()
{
    stm32l4_rtc_sync_t rtc_sync;

    if (!stm32l4_rtc_get_sync(&rtc_sync)) {
	return false;
    }

    _sync_month = rtc_sync.month;
    _sync_day = rtc_sync.day;
    _sync_hour = rtc_sync.hour;
    _sync_minute = rtc_sync.minute;
    _sync_second = rtc_sync.second;
    _sync_ticks = rtc_sync.ticks;

    return true;
}

uint16_t RTCClass::getSyncTicks()
{
    return _sync_ticks;
}

uint8_t RTCClass::getSyncSeconds()
{
    return _sync_second;
}
 
uint8_t RTCClass::getSyncMinutes()
{
    return _sync_minute;
}

uint8_t RTCClass::getSyncHours()
{
    return _sync_hour;
}

uint8_t RTCClass::getSyncDay()
{
    return _sync_day;
}

uint8_t RTCClass::getSyncMonth()
{
    return _sync_month;
}

void RTCClass::onSync(void(*callback)(void))
{
    _sync_callback = callback;

    if (callback)
    {
	stm32l4_rtc_notify_sync(RTCClass::_syncCallback, (void*)this);
    }
    else
    {
	stm32l4_rtc_notify_sync(NULL, NULL);
    }
}

void RTCClass::pinMode(PinMode mode)
{
    stm32l4_rtc_pin_configure(mode);
}

uint32_t RTCClass::read(unsigned int index)
{
    return stm32l4_rtc_read_backup(index);
}

void RTCClass::write(unsigned int index, uint32_t data)
{
    stm32l4_rtc_write_backup(index, data);
}

void RTCClass::InitAlarm()
{
    stm32l4_rtc_alarm_t rtc_alarm;

    stm32l4_rtc_get_alarm(0, &rtc_alarm);

    _alarm_day = rtc_alarm.day;
    _alarm_hour = rtc_alarm.hour;
    _alarm_minute = rtc_alarm.minute;
    _alarm_second = rtc_alarm.second;

    _alarm_init = 1;
}
    
void RTCClass::SyncAlarm()
{
    uint32_t match;
    stm32l4_rtc_alarm_t rtc_alarm;

    if (_alarm_enable)
    {
	rtc_alarm.day = _alarm_day;
	rtc_alarm.hour = _alarm_hour;
	rtc_alarm.minute = _alarm_minute;
	rtc_alarm.second = _alarm_second;

	match = (_alarm_match & (RTC_ALARM_MATCH_SECOND | RTC_ALARM_MATCH_MINUTE | RTC_ALARM_MATCH_HOUR | RTC_ALARM_MATCH_DAY)) | RTC_ALARM_MATCH_ENABLE;

	stm32l4_rtc_set_alarm(0, match, &rtc_alarm, (stm32l4_rtc_callback_t)_alarm_callback, NULL);
    }
}

void RTCClass::_syncCallback(void *context)
{
    class RTCClass *rtc_class = reinterpret_cast<class RTCClass*>(context);
    stm32l4_rtc_sync_t rtc_sync;

    stm32l4_rtc_get_sync(&rtc_sync);

    rtc_class->_sync_month = rtc_sync.month;
    rtc_class->_sync_day = rtc_sync.day;
    rtc_class->_sync_hour = rtc_sync.hour;
    rtc_class->_sync_minute = rtc_sync.minute;
    rtc_class->_sync_second = rtc_sync.second;
    rtc_class->_sync_ticks = rtc_sync.ticks;

    if (rtc_class->_sync_callback) {
	(*rtc_class->_sync_callback)();
    }
}

RTCClass RTC;
