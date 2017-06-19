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

#ifndef _VARIANT_DRAGONFLY_STM32L476RE_
#define _VARIANT_DRAGONFLY_STM32L476RE_

// The definitions here needs a STM32L4 core >=1.6.6
#define ARDUINO_STM32L4_VARIANT_COMPLIANCE 10606

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

#define STM32L4_CONFIG_LSECLK             32768
#define STM32L4_CONFIG_HSECLK             16000000
#define STM32L4_CONFIG_SYSOPT             SYSTEM_OPTION_VBAT_CHARGING
#define STM32L4_CONFIG_USB_VBUS           GPIO_PIN_PA9

#define USBCON

/** Master clock frequency */
#define VARIANT_MCK			  F_CPU

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
#include "USBAPI.h"
#include "Uart.h"
#endif // __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/

// Number of pins defined in PinDescription array
#define PINS_COUNT           (26u)
#define NUM_DIGITAL_PINS     (20u)
#define NUM_TOTAL_PINS       (45u)
#define NUM_ANALOG_INPUTS    (10u)
#define NUM_ANALOG_OUTPUTS   (2u)
#define analogInputToDigitalPin(p)  ((p < 6u) ? (p) + 14u : -1)

// LEDs
#define PIN_LED_13           (13u)
#define PIN_LED_RXL          (25u)
#define PIN_LED_TXL          (26u)
#define PIN_LED              PIN_LED_13
#define PIN_LED2             PIN_LED_RXL
#define PIN_LED3             PIN_LED_TXL
#define LED_BUILTIN          PIN_LED_13

/*
 * Analog pins
 */
#define PIN_A0               (14ul)
#define PIN_A1               (15ul)
#define PIN_A2               (16ul)
#define PIN_A3               (17ul)
#define PIN_A4               (18ul)
#define PIN_A5               (19ul)
#define PIN_A6               (8ul)
#define PIN_A7               (9ul)
#define PIN_A8               (30ul)
#define PIN_A9               (31ul)
#define PIN_DAC0             (14ul)
#define PIN_DAC1             (15ul)

static const uint8_t A0  = PIN_A0;
static const uint8_t A1  = PIN_A1;
static const uint8_t A2  = PIN_A2;
static const uint8_t A3  = PIN_A3;
static const uint8_t A4  = PIN_A4;
static const uint8_t A5  = PIN_A5;
static const uint8_t A6  = PIN_A6;
static const uint8_t A7  = PIN_A7;
static const uint8_t A8  = PIN_A8;
static const uint8_t A9  = PIN_A9;
static const uint8_t DAC0 = PIN_DAC0;
static const uint8_t DAC1 = PIN_DAC1;
#define ADC_RESOLUTION		12
#define DAC_RESOLUTION		12

// Other pins
#define PIN_ATN              (38ul)
static const uint8_t ATN = PIN_ATN;

#define PIN_BUTTON           (44l)
static const uint8_t BUTTON = PIN_BUTTON;

/*
 * Serial interfaces
 */

#define SERIAL_INTERFACES_COUNT 6

#define PIN_SERIAL1_RX       (0ul)
#define PIN_SERIAL1_TX       (1ul)

#define PIN_SERIAL2_RX       (31ul)
#define PIN_SERIAL2_TX       (30ul)

#define PIN_SERIAL3_RX       (8ul)
#define PIN_SERIAL3_TX       (9ul)

#define PIN_SERIAL4_RX       (10ul)
#define PIN_SERIAL4_TX       (11ul)

#define PIN_SERIAL5_RX       (42ul)
#define PIN_SERIAL5_TX       (43ul)

/*
 * SPI Interfaces
 */
#define SPI_INTERFACES_COUNT 3

#define PIN_SPI_MISO         (12u)
#define PIN_SPI_MOSI         (11u)
#define PIN_SPI_SCK          (13u)

#define PIN_SPI1_MISO        (22u)
#define PIN_SPI1_MOSI        (23u)
#define PIN_SPI1_SCK         (24u)

#define PIN_SPI2_MISO        (4u)
#define PIN_SPI2_MOSI        (5u)
#define PIN_SPI2_SCK         (3u)

static const uint8_t SS	  = 10;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/*
 * Wire Interfaces
 */
#define WIRE_INTERFACES_COUNT 3

#define PIN_WIRE_SDA         (20u)
#define PIN_WIRE_SCL         (21u)

#define PIN_WIRE1_SDA        (18u)
#define PIN_WIRE1_SCL        (19u)

#define PIN_WIRE2_SDA        (4u)
#define PIN_WIRE2_SCL        (3u)

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

/*
 * I2S Interfaces
 */
#define I2S_INTERFACES_COUNT 1

#define PIN_I2S_SCK          (2u)
#define PIN_I2S_FS           (3u)
#define PIN_I2S_SD           (4u)
#define PIN_I2S_MCK          (5u)

/*
 * USB
 */
#define PIN_USB_VBUS        (27ul)
#define PIN_USB_DM          (28ul)
#define PIN_USB_DP          (29ul)

#define PWM_INSTANCE_COUNT  4

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern CDC  Serial;
extern Uart Serial1;
extern Uart Serial2;
extern Uart Serial3;
extern Uart Serial4;
extern Uart Serial5;
#endif

// These serial port names are intended to allow libraries and architecture-neutral
// sketches to automatically default to the correct port name for a particular type
// of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
// the first hardware serial port whose RX/TX pins are not dedicated to another use.
//
// SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
//
// SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
//
// SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
//
// SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
//
// SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
//                            pins are NOT connected to anything by default.
#define SERIAL_PORT_USBVIRTUAL      Serial
#define SERIAL_PORT_MONITOR         Serial
#define SERIAL_PORT_HARDWARE1       Serial1
#define SERIAL_PORT_HARDWARE2       Serial2
#define SERIAL_PORT_HARDWARE3       Serial3
#define SERIAL_PORT_HARDWARE4       Serial4
#define SERIAL_PORT_HARDWARE5       Serial5
#define SERIAL_PORT_HARDWARE_OPEN1  Serial1
#define SERIAL_PORT_HARDWARE_OPEN2  Serial2
#define SERIAL_PORT_HARDWARE_OPEN3  Serial3
#define SERIAL_PORT_HARDWARE_OPEN4  Serial4
#define SERIAL_PORT_HARDWARE_OPEN5  Serial5

// Alias SerialUSB to Serial
#define SerialUSB SERIAL_PORT_USBVIRTUAL

#endif /* _VARIANT_DRAGONFLY_STM32L476RE_ */

