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

#include "Tone.h"
#include "wiring_private.h"

static GPIO_TypeDef *toneGPIO = NULL;
static uint32_t     toneBit = 0x00000000;
static uint32_t     toneCount = 0;;

static stm32l4_timer_t stm32l4_tone;

static void tone_event_callback(void *context, uint32_t events)
{
  if (toneCount) {
      if (toneCount & 1) {
        toneGPIO->BSRR = toneBit;
      } else {
        toneGPIO->BRR = toneBit;
      }

      if (toneCount < 0xfffffffe) {
	toneCount--;
      } else {
	toneCount ^= 1;
      }
  } else {
    stm32l4_timer_stop(&stm32l4_tone);
    stm32l4_timer_disable(&stm32l4_tone);

    toneGPIO->BRR = toneBit;
    toneGPIO = NULL;
  }
}

void tone (uint32_t pin, uint32_t frequency, uint32_t duration)
{
  if (frequency == 0) {
    return;
  }

  if ( g_APinDescription[pin].GPIO == NULL ) {
    return ;
  }

  if (stm32l4_tone.state == TIMER_STATE_NONE) {
    stm32l4_timer_create(&stm32l4_tone, TIMER_INSTANCE_TIM7, STM32L4_TONE_IRQ_PRIORITY, 0);
  }

  GPIO_TypeDef *GPIO = (GPIO_TypeDef *)g_APinDescription[pin].GPIO;
  uint32_t bit = g_APinDescription[pin].bit;

  if ((toneGPIO != GPIO) || (toneBit != bit)) {
    if (toneGPIO) {
      noTone(pin);
    }

    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }

  toneGPIO  = GPIO;
  toneBit   = bit;
  toneCount = (duration > 0 ? frequency * duration * 2 / 1000UL : 0xfffffffe);

  uint32_t modulus = (stm32l4_timer_clock(&stm32l4_tone) / frequency) / 2;
  uint32_t scale   = 1;

  while (modulus > 65536) {
    modulus /= 2;
    scale++;
  }

  stm32l4_timer_enable(&stm32l4_tone, scale -1, modulus -1, 0, tone_event_callback, NULL, TIMER_EVENT_PERIOD);
  stm32l4_timer_start(&stm32l4_tone, false);

  toneGPIO->BSRR = toneBit;
}

void noTone (uint32_t pin)
{
  stm32l4_timer_stop(&stm32l4_tone);
  stm32l4_timer_disable(&stm32l4_tone);

  digitalWrite(pin, LOW);

  toneGPIO = NULL;
}
