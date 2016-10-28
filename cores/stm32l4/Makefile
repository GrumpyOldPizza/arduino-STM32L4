TOOLS    = /opt/gcc-arm-none-eabi-4.8.3-2014q1
CPP      = $(TOOLS)/bin/arm-none-eabi-cpp
CC       = $(TOOLS)/bin/arm-none-eabi-gcc
CXX      = $(TOOLS)/bin/arm-none-eabi-g++
AS       = $(TOOLS)/bin/arm-none-eabi-as
AR       = $(TOOLS)/bin/arm-none-eabi-ar
LD       = $(TOOLS)/bin/arm-none-eabi-ld
CFLAGS   = -mcpu=cortex-m4 -mthumb -c -g3 -Os $(WARNINGS) -std=gnu11 -ffunction-sections -fdata-sections -nostdlib --param max-inline-insns-single=500 -MMD $(EXTRAS) $(DEFINES) $(INCLUDES)
CXXFLAGS = -mcpu=cortex-m4 -mthumb -c -g3 -Os $(WARNINGS) -std=gnu++11 -ffunction-sections -fdata-sections -fno-threadsafe-statics -nostdlib --param max-inline-insns-single=500 -fno-rtti -fno-exceptions -MMD $(EXTRAS) $(DEFINES) $(INCLUDES)
ASFLAGS  = -c -g -x assembler-with-cpp $(EXTRAS) $(DEFINES) $(INCLUDES)
LDFLAGS  = -L../../variants/STM32L476RE-Dragonfly -Os -Wl,--gc-sections -save-temps $(EXTRAS) -T../../variants/STM32L476RE-Dragonfly/linker_scripts/STM32L476RE_FLASH.ld --specs=nano.specs -mcpu=cortex-m4 -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align
WARNINGS = -Wall -Wextra -Wno-unused-parameter
EXTRAS   = -DSTM32L476xx -D__FPU_PRESENT=1 -march=armv7e-m -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mabi=aapcs -mslow-flash-data
DEFINES  = -D_SYSTEM_CORE_CLOCK_=80000000L -DARDUINO=10606 -D_ARDUINO_STM32L4 -DARDUINO_ARCH_STM32L4
INCLUDES = -I../../system/CMSIS/Include -I../../system/CMSIS/Device/ST/STM32L4xx/Include -I../../system/libstm32l4/USB/HAL/Inc -I../../system/libstm32l4/USB/Core/Inc -I../../system/libstm32l4/USB/Class/CDC/Inc -I../../system/libstm32l4/USB/Class/MSC/Inc -I../../system/libstm32l4/USB -I../../system/libstm32l4/ -I../../variants/STM32L476RE-Dragonfly -I../../libraries/RTC/src -I../../libraries/SPI/ -I../../libraries/Wire/ -I../../libraries/Servo/src/ -I. 

SRCS = \
	../../libraries/RTC/src/RTC.cpp \
	../../libraries/Servo/src/Servo.cpp \
	../../libraries/SPI/SPI.cpp \
	../../libraries/Wire/Wire.cpp \
	../../variants/STM32L476RE-Dragonfly/variant.cpp \
	avr/dtostrf.c \
	avr/eeprom.c \
	avr/fdevopen.c \
	CDC.cpp \
	FS.cpp \
	HardwareSerial.cpp \
	IPAddress.cpp \
	Print.cpp \
	RingBuffer.cpp \
	STM32.cpp \
	Stream.cpp \
	Tone.cpp \
	Uart.cpp \
	WMath.cpp \
	WString.cpp \
	abi.cpp \
	hooks.c \
	itoa.c \
	main.cpp \
	new.cpp \
	wiring.c \
	wiring_analog.c \
	wiring_digital.c \
	wiring_interrupts.c \
	wiring_pulse.c \
	wiring_shift.c

OBJS = \
	../../libraries/RTC/src/RTC.o \
	../../libraries/Servo/src/Servo.o \
	../../libraries/SPI/SPI.o \
	../../libraries/Wire/Wire.o \
	../../variants/STM32L476RE-Dragonfly/variant.o \
	avr/dtostrf.o \
	avr/eeprom.o \
	avr/fdevopen.o \
	CDC.o \
	FS.o \
	HardwareSerial.o \
	IPAddress.o \
	Print.o \
	RingBuffer.o \
	STM32.o \
	Stream.o \
	Tone.o \
	Uart.o \
	WMath.o \
	WString.o \
	abi.o \
	hooks.o \
	itoa.o \
	main.o \
	new.o \
	wiring.o \
	wiring_analog.o \
	wiring_digital.o \
	wiring_interrupts.o \
	wiring_pulse.o \
	wiring_shift.o

all:: flash.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

flash.elf:: $(OBJS)
	$(CC) $(LDFLAGS) -Wl,-Map,flash.map -Wl,--start-group $(OBJS) -Wl,--end-group -lstm32l476 -lc -lm -lstm32l476 -o flash.elf

flash.bin:: flash.elf
	arm-none-eabi-objcopy -O binary flash.elf flash.bin

clean::
	rm -f flash.elf flash.bin flash.map *~ $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)
