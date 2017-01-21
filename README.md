# Arduino Core for STMicroelectronics STM32L4 based boards

## Supported boards

### Tlera Corp
 * [Dragonfly-STM32L476RE](https://www.tindie.com/products/TleraCorp/dragonfly-stm32l476-development-board)
 * [Butterfly-STM32L433CC](https://www.tindie.com/products/TleraCorp/butterfly-stm32l433-development-board)
 * [Ladybug-STM32L432KC](https://www.tindie.com/products/TleraCorp/ladybug-stm32l432-development-board)

### STMicroelectronics
 * NUCLEO-L432
 * NUCLEO-L476

## Installing

### Board Manager

 1. [Download and install the Arduino IDE](https://www.arduino.cc/en/Main/Software) (At least v1.6.8)
 2. Start the Arduino IDE
 3. Go into Preferences
 4. Add ```https://grumpyoldpizza.github.io/arduino-STM32L4/package_STM32L4_boards_index.json``` as an "Additional Board Manager URL"
 5. Open the Boards Manager from the Tools -> Board menu and install "STM32L4 Boards by Tlera Corp"
 6. Select your STM32L4 board from the Tools -> Board menu

#### OS Specific Setup

##### OS X

No additional setup required.

##### Linux

No additional setup required.

#####  Windows

###### STM32 BOOTLOADER driver setup for Tlera Corp boards

 1. Download [Zadig](http://zadig.akeo.ie)
 2. Plugin STM32L4 board and toggle the RESET button while holding down the BOOT button
 3. Let Windows finish searching for drivers
 4. Start ```Zadig```
 5. Select ```Options -> List All Devices```
 6. Select ```STM32 BOOTLOADER``` from the device dropdown
 7. Select ```WinUSB (v6.1.7600.16385)``` as new driver
 8. Click ```Replace Driver```

###### USB Serial driver setup for Tlera Corp boards

 1. Go to ~/AppData/Local/Arduino15/packages/grumpypoldpizza/hardware/stm32l4/```<VERSION>```/drivers/windows
 2. Right-click on ```dpinst-x86.exe``` (32 bit Windows) or ```dpinst-amd64.exe``` (64 bit Windows) and select ```Run as administrartor```
 3. Click on ```Install this driver software anyway``` at the ```Windows Security``` popup as the driver is unsigned

### Flashing the bootloader for Tlera Corp Boards

 1. Select Tools -> Programmer -> STM32 BOOTLOADER
 1. Select Tools -> Burn Bootloader

### From git (for core development)

 1. Follow steps from Board Manager section above
 2. ```cd <SKETCHBOOK>```, where ```<SKETCHBOOK>``` is your Arduino Sketch folder:
  * OS X: ```~/Documents/Arduino```
  * Linux: ```~/Arduino```
  * Windows: ```~/Documents/Arduino```
 3. Create a folder named ```hardware```, if it does not exist, and change directories to it
 4. Clone this repo: ```git clone https://github.com/sandeepmistry/arduino-nRF5.git sandeepmistry/nRF5```
 5. Restart the Arduino IDE

## BLE

This Arduino Core does **not** contain any Arduino style API's for BLE functionality. All the relevant Nordic SoftDevice (S110, S130, S132) header files are included build path when a SoftDevice is selected via the `Tools` menu.

### Recommend BLE Libraries

 * [BLEPeripheral](https://github.com/sandeepmistry/arduino-BLEPeripheral)
   * v0.3.0 and greater, available via the Arduino IDE's library manager.
   * Supports peripheral mode only.

## Credits

This core is based on the [Arduino SAMD Core](https://github.com/arduino/ArduinoCore-samd) and licensed under the same [GPL License](LICENSE)

The following tools are used:

 * [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded) as the compiler
 * A [forked](https://github.com/sandeepmistry/openocd-code-nrf5) version of [OpenOCD](http://openocd.org) to flash sketches
