/*
  Copyright (c) 2015 Arduino LLC. All right reserved.
  Copyright (c) 2016 Thomas Roell. All right reserved.

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

#include "Arduino.h"
#include "stm32l4_wiring_private.h"
#include <Servo.h>

#define INVALID_SERVO         255

static stm32l4_servo_t stm32l4_servo;

static uint8_t ServoAttached = 0;
static volatile bool ServoSync = false;
static stm32l4_servo_table_t ServoTable;

static void servo_event_callback(void *context, uint32_t events)
{
    if (ServoSync)
    {
	ServoSync = 0;

	stm32l4_servo_configure(&stm32l4_servo, &ServoTable);
    }
}

Servo::Servo()
{
    if (stm32l4_servo.state == SERVO_STATE_NONE) {
	stm32l4_servo_create(&stm32l4_servo, TIMER_INSTANCE_TIM6, STM32L4_SERVO_IRQ_PRIORITY);
    }

    if (ServoTable.entries < MAX_SERVOS) {
	this->servoIndex = ServoTable.entries;
	
	ServoTable.slot[this->servoIndex].pin = GPIO_PIN_NONE;
	ServoTable.slot[this->servoIndex].width = DEFAULT_PULSE_WIDTH;

	ServoTable.entries++;
    } else {
	this->servoIndex = INVALID_SERVO;
    }
}

uint8_t Servo::attach(int pin)
{
    return this->attach(pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
}

uint8_t Servo::attach(int pin, int min, int max)
{
    if (g_APinDescription[pin].GPIO == NULL) {
	return INVALID_SERVO;
    }

    if (this->servoIndex >= MAX_SERVOS) {
	return INVALID_SERVO;
    }

    if (ServoTable.slot[this->servoIndex].pin == GPIO_PIN_NONE) {
	ServoAttached++;
    }

    pinMode(pin, OUTPUT);

    ServoTable.slot[this->servoIndex].pin = g_APinDescription[pin].pin;
    
    this->min  = min;
    this->max  = max;

    if (stm32l4_servo.state < SERVO_STATE_READY) {
	stm32l4_servo_enable(&stm32l4_servo, &ServoTable, servo_event_callback, NULL, SERVO_EVENT_SYNC);
    } else {
	if (ServoAttached == 1) {
	    stm32l4_servo_configure(&stm32l4_servo, &ServoTable);
	} else {
	    ServoSync = true;
	}
    }

    return this->servoIndex;
}

void Servo::detach()
{
    if (this->servoIndex >= MAX_SERVOS) {
	return;
    }

    if (ServoTable.slot[this->servoIndex].pin != GPIO_PIN_NONE) {
	ServoAttached--;
    }

    ServoTable.slot[this->servoIndex].pin = GPIO_PIN_NONE;   // store default values

    ServoSync = true;
}

void Servo::write(int angle)
{
    int width;

    if (angle < 200) {
	if (angle < 0) {
	    angle = 0;
	}

	if (angle > 180) {
	    angle = 180;
	}

	width = map(angle, 0, 180, this->min, this->max);
    } else {
	width = angle;
    }

    writeMicroseconds(width);
}

void Servo::writeMicroseconds(int width)
{
    if (this->servoIndex >= MAX_SERVOS) {
	return;
    }

    if (width < this->min) {
	width = this->min;
    }

    if (width > this->max) {
	width = this->max;
    }
    
    ServoTable.slot[this->servoIndex].width = width;
    
    ServoSync = true;
}

int Servo::read() // return the value as degrees
{
    return map(readMicroseconds()+1, this->min, this->max, 0, 180);
}

int Servo::readMicroseconds()
{
    if (this->servoIndex >= MAX_SERVOS) {
	return 0;
    }

    return ServoTable.slot[this->servoIndex].width;
}

bool Servo::attached()
{
    return (ServoTable.slot[this->servoIndex].pin != GPIO_PIN_NONE);
}
