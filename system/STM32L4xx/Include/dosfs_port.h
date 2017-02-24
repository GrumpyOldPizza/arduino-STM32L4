/*
 * Copyright (c) 2014 Thomas Roell.  All rights reserved.
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

#if !defined(_DOSFS_PORT_h)
#define _DOSFS_PORT_h

#include "stm32l4_rtc.h"

#ifdef __cplusplus
 extern "C" {
#endif

static inline void stm32l4_system_timedate(uint16_t *p_time, uint16_t *p_date)
{
    stm32l4_rtc_time_t rtc_time;

    stm32l4_rtc_get_time(&rtc_time);

    *p_time = ((rtc_time.second >> 1) | (rtc_time.minute << 5) | (rtc_time.hour << 11));
    *p_date = ((rtc_time.day << 0) | (rtc_time.month << 5) | ((rtc_time.year + 20) << 9));
}

#define DOSFS_PORT_CORE_TIMEDATE(_ctime, _cdate) stm32l4_system_timedate((_ctime),(_cdate))

#ifdef __cplusplus
}
#endif

#endif /* _DOSFS_PORT_h */
