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

#ifndef _STM32L4_WIRING_
#define _STM32L4_WIRING_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "armv7m.h"

#define retained __attribute__((section(".backup")))

static inline void interrupts(void)
{
    __asm__ volatile ("cpsie i" : : : "memory");
}

static inline  void noInterrupts(void)
{
    __asm__ volatile ("cpsid i" : : : "memory");
}

static inline uint32_t millis(void) 
{
    return armv7m_systick_millis();
}

static inline uint32_t micros(void) 
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

    armv7m_core_udelay(usec);
}

extern void init(void);


typedef enum _eAnalogReference {
  AR_DEFAULT,
} eAnalogReference;

extern void analogReference(eAnalogReference reference);
extern void analogReadResolution(int resolution);
extern void analogReadPeriod(int period);
extern uint32_t analogRead(uint32_t pin);
extern void analogWriteResolution(int resolution);
extern void analogWriteFrequency(uint32_t pin, uint32_t frequency);
extern void analogWriteRange(uint32_t pin, uint32_t range);
extern void analogWrite(uint32_t pin, uint32_t value);

extern void pinMode(uint32_t pin, uint32_t mode);
extern void digitalWrite(uint32_t pin, uint32_t value);
extern int digitalRead(uint32_t pin);

typedef void (*voidFuncPtr)(void);
extern void attachInterrupt(uint32_t pin, voidFuncPtr callback, uint32_t mode);
extern void detachInterrupt(uint32_t pin);

extern uint32_t pulseIn(uint32_t pin, uint32_t state, uint32_t timeout);
#ifdef __cplusplus
extern uint32_t pulseIn(uint32_t pin, uint32_t state, uint32_t timeout = 1000000L);
#endif

extern uint32_t shiftIn( uint32_t ulDataPin, uint32_t ulClockPin, uint32_t ulBitOrder);
extern void shiftOut( uint32_t ulDataPin, uint32_t ulClockPin, uint32_t ulBitOrder, uint32_t ulVal);

extern void tone(uint32_t pin, uint32_t frequency, uint32_t duration);
extern void noTone(uint32_t pin);
#ifdef __cplusplus
extern void tone(uint32_t pin, uint32_t frequency, uint32_t duration = 0);
#endif

#define PIN_ATTR_DAC           (1UL<<0)
#define PIN_ATTR_ADC           (1UL<<1)
#define PIN_ATTR_PWM           (1UL<<2)
#define PIN_ATTR_EXTI          (1UL<<3)
#define PIN_ATTR_WKUP1         (1UL<<4)   /* PA0  */
#define PIN_ATTR_WKUP2         (2UL<<4)   /* PC13 */
#define PIN_ATTR_WKUP3         (3UL<<4)   /* PE6  */
#define PIN_ATTR_WKUP4         (4UL<<4)   /* PA2  */
#define PIN_ATTR_WKUP5         (5UL<<4)   /* PC5  */
#define PIN_ATTR_WKUP_MASK     (7UL<<4)
#define PIN_ATTR_WKUP_SHIFT    4
#define PIN_ATTR_WKUP_OFFSET   1

#define PWM_INSTANCE_NONE      255

#define PWM_CHANNEL_1          0
#define PWM_CHANNEL_2          1
#define PWM_CHANNEL_3          2
#define PWM_CHANNEL_4          3
#define PWM_CHANNEL_NONE       255

#define ADC_INPUT_1            1
#define ADC_INPUT_2            2
#define ADC_INPUT_3            3
#define ADC_INPUT_4            4
#define ADC_INPUT_5            5
#define ADC_INPUT_6            6
#define ADC_INPUT_7            7
#define ADC_INPUT_8            8
#define ADC_INPUT_9            9
#define ADC_INPUT_10           10
#define ADC_INPUT_11           11
#define ADC_INPUT_12           12
#define ADC_INPUT_13           13
#define ADC_INPUT_14           14
#define ADC_INPUT_15           15
#define ADC_INPUT_16           16
#define ADC_INPUT_NONE         255


/* Types used for the table below */
typedef struct _PinDescription
{
  void                    *GPIO;
  uint16_t                bit;
  uint16_t                pin;
  uint8_t                 attr;
  uint8_t                 pwm_instance;
  uint8_t                 pwm_channel;
  uint8_t                 adc_input;
} PinDescription ;

/* Pins table to be instantiated into variant.cpp */
extern const PinDescription g_APinDescription[] ;

extern const unsigned int g_PWMInstances[] ;

#define digitalPinToPort(P)        ( g_APinDescription[P].GPIO )
#define digitalPinToBitMask(P)     ( g_APinDescription[P].bit )
#define portInputRegister(port)    ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x10) ) // IDR
#define portOutputRegister(port)   ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x14) ) // ODR
#define portSetRegister(port)      ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x18) ) // BSRR
#define portClearRegister(port)    ( (volatile uint32_t*)((volatile uint8_t*)(port) + 0x28) ) // BRR
#define digitalPinHasPWM(P)        ( g_APinDescription[P].attr & PIN_ATTR_PWM )

extern uint32_t SystemCoreClock;

#define F_CPU SystemCoreClock

extern uint32_t __FlashBase;
extern uint32_t __FlashLimit;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _STM32L4_WIRING_ */

