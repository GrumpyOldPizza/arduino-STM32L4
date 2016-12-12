/*
  RTC library for Arduino Zero.
  Copyright (c) 2015 Arduino LLC. All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef RTC_ZERO_H
#define RTC_ZERO_H

#include "RTC.h"
#include "STM32.h"

typedef void(*voidFuncPtr)(void);

class RTCZero {
public:

  enum Alarm_Match: uint8_t // Should we have this enum or just use the identifiers from /component/rtc.h ?
  {
    MATCH_OFF          = 0,                                   // Never
    MATCH_SS           = 1,                                   // Every Minute
    MATCH_MMSS         = 3,                                   // Every Hour
    MATCH_HHMMSS       = 7,                                   // Every Day
  };

  inline void begin(bool resetTime = false) { if (resetTime) { RTC.setCalendar(0,0,0,1,1,0); }

  inline void enableAlarm(Alarm_Match match) { if (match == MATCH_OFF) { RTC.disableAlarm(); } else { RTC.enableAlarm((RTCClass::AlarmMatch)match); } }
  inline void disableAlarm() { RTC.disableAlarm(); }

  inline void attachInterrupt(voidFuncPtr callback) { RTC.attachInterrupt(callback); }
  inline void detachInterrupt() { RTC.detachInterrupt(); }
  
  inline void standbyMode() { STM32.stop(); }
  
  /* Get Functions */

  inline uint8_t getSeconds() { return RTC.getSeconds(); }
  inline uint8_t getMinutes() { return RTC.getMinutes(); }
  inline uint8_t getHours() { return RTC.getHours(); }
  
  inline uint8_t getDay() { return RTC.getDay(); }
  inline uint8_t getMonth() { return RTC.getMonth(); }
  inline uint8_t getYear() { return RTC.getYear(); }
  
  inline uint8_t getAlarmSeconds() { return RTC.getAlarmSeconds(); }
  inline uint8_t getAlarmMinutes() { return RTC.getAlarmMinutes(); }
  inline uint8_t getAlarmHours() { return RTC.getAlarmHours(); }

  inline uint8_t getAlarmDay() { return RTC.getAlarmDay(); }

  /* Set Functions */

  inline void setSeconds(uint8_t seconds) { RTC.setSeconds(seconds); }
  inline void setMinutes(uint8_t minutes) { RTC.setMinutes(minutes); }
  inline void setHours(uint8_t hours) { RTC.setHours(hours); }
  inline void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds) { RTC.setTime(hours, minutes, seconds); }

  inline void setDay(uint8_t day) { RTC.setDay(day); }
  inline void setMonth(uint8_t month) { RTC.setMonth(month); }
  inline void setYear(uint8_t year) { RTC.setYear(year); }
  inline void setDate(uint8_t day, uint8_t month, uint8_t year) { RTC.setDate(day, month, year); }

  inline void setAlarmSeconds(uint8_t seconds) { RTC.setAlarmSeconds(seconds); }
  inline void setAlarmMinutes(uint8_t minutes) { RTC.setAlarmMinutes(minutes); }
  inline void setAlarmHours(uint8_t hours) { RTC.setAlarmHours(hours); }
  inline void setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds) { RTC.setAlarmTime(hours, minutes, seconds); }

  inline void setAlarmDay(uint8_t day) { RTC.setAlarmDay(day); }

  /* Epoch Functions */

  inline uint32_t getEpoch() { return RTC.getEpoch(); }
  inline uint32_t getY2kEpoch() { return RTC.getY2kEpoch(); }
  inline void setEpoch(uint32_t ts) { RTC.setEpoch(ts); }
  inline void setY2kEpoch(uint32_t ts) { RTC.setY2kEpoch(ts); }
};

#endif // RTC_ZERO_H
