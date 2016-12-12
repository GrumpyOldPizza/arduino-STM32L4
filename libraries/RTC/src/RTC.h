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

#ifndef _RTC_H
#define _RTC_H

#include "Arduino.h"

class RTCClass {
public:

    enum AlarmMatch: uint8_t {
	MATCH_ANY          = 0,      // Every Second
 	MATCH_SS           = 1,      // Every Minute
	MATCH_MMSS         = 3,      // Every Hour
	MATCH_HHMMSS       = 7,      // Every Day
    };

    enum PinMode: uint8_t {
 	NONE               = 0,
 	INPUT_SYNC         = 1,
 	OUTPUT_512HZ       = 2,
 	OUTPUT_1HZ         = 3,
    };

    void enableAlarm(AlarmMatch match);
    void disableAlarm();

    void attachInterrupt(void(*callback)(void));
    void detachInterrupt();
  
    // Get Functions
    uint8_t getSeconds();
    uint8_t getMinutes();
    uint8_t getHours();
  
    uint8_t getDay();
    uint8_t getMonth();
    uint8_t getYear();
  
    uint8_t getAlarmSeconds();
    uint8_t getAlarmMinutes();
    uint8_t getAlarmHours();

    uint8_t getAlarmDay();

    // Set Functions
    void setSeconds(uint8_t seconds);
    void setMinutes(uint8_t minutes);
    void setHours(uint8_t hours);
    void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds);

    void setDay(uint8_t day);
    void setMonth(uint8_t month);
    void setYear(uint8_t year);
    void setDate(uint8_t day, uint8_t month, uint8_t year);

    void setAlarmSeconds(uint8_t seconds);
    void setAlarmMinutes(uint8_t minutes);
    void setAlarmHours(uint8_t hours);
    void setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds);

    void setAlarmDay(uint8_t day);

    // Epoch Functions
    uint32_t getEpoch();
    uint32_t getY2kEpoch();
    void setEpoch(uint32_t ts);
    void setY2kEpoch(uint32_t ts);

    // STM32L4 EXTENSION: atomic set time/date
    void setCalendar(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day, uint8_t month, uint8_t year);

    // STM32L4 EXTENSION: ticks [0..32767]
    uint16_t getTicks();

    // STM32L4 EXTENSION: clock calibration [-511..512]
    int32_t getCalibration();
    void setCalibration(int32_t calibration);

    // STM32L4 EXTENSION: clock fine tuning [-32767..32768]
    void adjustTicks(int32_t ticks);

    // STM32L4 EXTENSION: external sync pulse
    bool syncOccured();
    uint16_t getSyncTicks();
    uint8_t getSyncSeconds();
    uint8_t getSyncMinutes();
    uint8_t getSyncHours();
    uint8_t getSyncDay();
    uint8_t getSyncMonth();
    void onSync(void(*callback)(void));

    // STM32L4 EXTENSION: PC13 control
    void pinMode(PinMode mode);

    // STM32L4 EXTENSION: battery backed registers
    uint32_t read(unsigned int idx);
    void write(unsigned int idx, uint32_t val);

private:
    uint8_t _alarm_init;
    uint8_t _alarm_enable;
    uint8_t _alarm_match;
    uint8_t _alarm_day;
    uint8_t _alarm_hour;
    uint8_t _alarm_minute;
    uint8_t _alarm_second;

    void (*_alarm_callback)(void);

    void InitAlarm();
    void SyncAlarm();

    uint8_t _sync_month;
    uint8_t _sync_day;
    uint8_t _sync_hour;
    uint8_t _sync_minute;
    uint8_t _sync_second;
    uint16_t _sync_ticks;

    void (*_sync_callback)(void);

    static void _syncCallback(void *context);

    friend class RTCZero;
};

extern RTCClass RTC;

#endif // _RTC_H
