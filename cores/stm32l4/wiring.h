/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "armv7m.h"

static inline uint32_t millis(void) 
{
  return armv7m_systick_millis();
}

static inline uint64_t micros(void) 
{
  return armv7m_systick_micros();
}

static inline void delay(uint32_t msec) 
{
  if (msec == 0)
    return;

  armv7m_systick_delay(msec);
}

static inline void delayMicroseconds(uint32_t usec) 
{
  if (usec == 0)
    return;

  armv7m_clock_spin(usec * 1000);
}

extern void init(void);

#ifdef __cplusplus
}
#endif
