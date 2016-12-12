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

#if !defined(_STM32L4_RTC_H)
#define _STM32L4_RTC_H

#include "armv7m.h"

#include "stm32l4xx.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define RTC_TIME_MASK_SECOND     0x00000001
#define RTC_TIME_MASK_MINUTE     0x00000002
#define RTC_TIME_MASK_HOUR       0x00000004
#define RTC_TIME_MASK_DAY        0x00000008
#define RTC_TIME_MASK_MONTH      0x00000010
#define RTC_TIME_MASK_YEAR       0x00000020

typedef struct _stm32l4_rtc_time_t {
    uint8_t    year;
    uint8_t    month;
    uint8_t    day;
    uint8_t    hour;
    uint8_t    minute;
    uint8_t    second;
    uint16_t   ticks;
} stm32l4_rtc_time_t;

#define RTC_PIN_MODE_GPIO             0 
#define RTC_PIN_MODE_INPUT_SYNC       1
#define RTC_PIN_MODE_OUTPUT_512HZ     2
#define RTC_PIN_MODE_OUTPUT_1HZ       3
#define RTC_PIN_MODE_OUTPUT_ALARM_A   4
#define RTC_PIN_MODE_OUTPUT_ALARM_B   5
#define RTC_PIN_MODE_OUTPUT_WAKEUP    6

#define RTC_ALARM_MATCH_SECOND        0x00000001
#define RTC_ALARM_MATCH_MINUTE        0x00000002
#define RTC_ALARM_MATCH_HOUR          0x00000004
#define RTC_ALARM_MATCH_DAY           0x00000008
#define RTC_ALARM_MATCH_ENABLE        0x80000000

typedef struct _stm32l4_rtc_alarm_t {
    uint8_t    day;
    uint8_t    hour;
    uint8_t    minute;
    uint8_t    second;
} stm32l4_rtc_alarm_t;

typedef void (*stm32l4_rtc_callback_t)(void *context);

typedef struct _stm32l4_rtc_sync_t {
    uint8_t    month;
    uint8_t    day;
    uint8_t    hour;
    uint8_t    minute;
    uint8_t    second;
    uint16_t   ticks;
} stm32l4_rtc_sync_t;

extern void stm32l4_rtc_configure(unsigned int priority);
extern void stm32l4_rtc_set_time(unsigned int mask, const stm32l4_rtc_time_t *time);
extern void stm32l4_rtc_get_time(stm32l4_rtc_time_t *p_time_return);
extern void stm32l4_rtc_set_calibration(int32_t calibration);
extern int32_t stm32l4_rtc_get_calibration(void);
extern void stm32l4_rtc_adjust_ticks(int32_t ticks);
extern uint32_t stm32l4_rtc_get_ticks(void);
extern void stm32l4_rtc_pin_configure(unsigned int mode);
extern void stm32l4_rtc_set_alarm(unsigned int channel, unsigned int match, const stm32l4_rtc_alarm_t *alarm, stm32l4_rtc_callback_t callback, void *context);
extern void stm32l4_rtc_get_alarm(unsigned int channel, stm32l4_rtc_alarm_t *p_alarm_return);
extern void stm32l4_rtc_wakeup(uint32_t timeout);
extern bool stm32l4_rtc_get_sync(stm32l4_rtc_sync_t *p_sync_return);
extern void stm32l4_rtc_notify_sync(stm32l4_rtc_callback_t callback, void *context);
extern uint32_t stm32l4_rtc_read_backup(unsigned int index);
extern void stm32l4_rtc_write_backup(unsigned int index, uint32_t data);

#ifdef __cplusplus
}
#endif

#endif /* _STM32L4_RTC_H */

