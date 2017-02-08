/*
  Servo.h - Interrupt driven Servo library for Arduino using 16 bit timers- Version 2
  Copyright (c) 2009 Michael Margolis.  All right reserved.
  Copyright (c) 2015 Thomas Roell.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* 
  A servo is activated by creating an instance of the Servo class passing 
  the desired pin to the attach() method.
  The servos are pulsed in the background using the value most recently 
  written using the write() method.

  The methods are:

    Servo - Class for manipulating servo motors connected to Arduino pins.

    attach(pin )  - Attaches a servo motor to an i/o pin.
    attach(pin, min, max  ) - Attaches to a pin setting min and max values in microseconds
    default min is 544, max is 2400  
 
    write()     - Sets the servo angle in degrees.  (invalid angle that is valid as pulse in microseconds is treated as microseconds)
    writeMicroseconds() - Sets the servo pulse width in microseconds 
    read()      - Gets the last written servo pulse width as an angle between 0 and 180. 
    readMicroseconds()   - Gets the last written servo pulse width in microseconds. (was read_us() in first release)
    attached()  - Returns true if there is a servo attached. 
    detach()    - Stops an attached servos from pulsing its i/o pin. 

    
  STM32L4 notes:

    The min/max values are really a way to rescale the intended angle to a width in microseconds. 

    Servos usually don't have the full 180 range, so that the min/max values will be outside the
    legal 1000 to 2000 microsecond range.

    The write()/writeMicroseconds() APIs clamp the values to the 1000 to 2000 microsecond range to
    avoid damaging the servos.

    The default values (although been kept for compatibility) do not make sense. The midpoint
    (aka 90 degrees) is at 1472, which does not correspond to the correct 1500 microseconds.
 */

#ifndef Servo_h
#define Servo_h

#include <inttypes.h>

#define Servo_VERSION           2     // software version of this library

#define MIN_PULSE_WIDTH      1000     // the shortest pulse sent to a servo  
#define MAX_PULSE_WIDTH      2000     // the longest pulse sent to a servo 
#define DEFAULT_PULSE_WIDTH  1500     // default pulse width when servo is attached
#define REFRESH_INTERVAL    20000     // minumim time to refresh servos in microseconds 

// NOTE: to maintain a strict refresh interval the user needs to not exceed 2250us pulse width
#define MAX_SERVOS              8

class Servo
{
public:
  Servo();
  uint8_t attach(int pin);           // attach the given pin to the next free channel, sets pinMode, returns channel number or 0 if failure
  uint8_t attach(int pin, int min, int max); // as above but also sets min and max values for writes. 
  void detach();
  void write(int angle);             // if value is < 200 its treated as an angle, otherwise as pulse width in microseconds 
  void writeMicroseconds(int width); // Write pulse width in microseconds 
  int read();                        // returns current pulse width as an angle between 0 and 180 degrees
  int readMicroseconds();            // returns current pulse width in microseconds for this servo (was read_us() in first release)
  bool attached();                   // return true if this servo is attached, otherwise false 
private:
   uint8_t servoIndex;               // index into the channel data for this servo
   int16_t min;                      // minimum corresponding to 0 degrees
   int16_t max;                      // maximum corresponding to 180 degrees
};

#endif
