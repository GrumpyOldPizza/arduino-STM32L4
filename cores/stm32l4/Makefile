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
LDFLAGS  = -L../../system/STM32L4xx/Lib -L../../system/CMSIS/Lib -Os -Wl,--gc-sections -save-temps $(EXTRAS) -T../../variants/STM32L433CC-Butterfly/linker_scripts/STM32L433CC_FLASH.ld --specs=nano.specs -mcpu=cortex-m4 -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align
WARNINGS = -Wall -Wextra -Wno-unused-parameter
EXTRAS   = -DSTM32L433xx -D__FPU_PRESENT=1 -march=armv7e-m -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mabi=aapcs -mslow-flash-data
DEFINES  = -D_SYSTEM_CORE_CLOCK_=80000000L -DARDUINO=10606 -D_ARDUINO_STM32L4 -DARDUINO_ARCH_STM32L4
INCLUDES = -I../../system/CMSIS/Include -I../../system/CMSIS/Device/ST/STM32L4xx/Include -I../../system/STM32L4xx/Include -I../../variants/STM32L433CC-Butterfly -I../../libraries/RTC/src  -I../../libraries/I2S/src -I../../libraries/SPI/src -I../../libraries/Wire/src -I../../libraries/Servo/src -I. 

SRCS = \
	../../libraries/I2S/src/I2S.cpp \
	../../libraries/RTC/src/RTC.cpp \
	../../libraries/Servo/src/Servo.cpp \
	../../libraries/SPI/src/SPI.cpp \
	../../libraries/Wire/src/Wire.cpp \
	../../variants/STM32L433CC-Butterfly/variant.cpp \
	avr/dtostrf.c \
	avr/eeprom.c \
	avr/fdevopen.c \
	CDC.cpp \
	FS.cpp \
	IPAddress.cpp \
	Print.cpp \
	RingBuffer.cpp \
	STM32.cpp \
	Stream.cpp \
	USBCore.cpp \
	Uart.cpp \
	WMath.cpp \
	WString.cpp \
	abi.cpp \
	hooks.c \
	itoa.c \
	main.cpp \
	new.cpp \
	stm32l4_wiring.c \
	stm32l4_wiring_analog.c \
	stm32l4_wiring_digital.c \
	stm32l4_wiring_interrupts.c \
	stm32l4_wiring_pulse.c \
	stm32l4_wiring_shift.c \
	stm32l4_wiring_tone.c

OBJS = \
	../../libraries/I2S/src/I2S.o \
	../../libraries/RTC/src/RTC.o \
	../../libraries/Servo/src/Servo.o \
	../../libraries/SPI/src/SPI.o \
	../../libraries/Wire/src/Wire.o \
	../../variants/STM32L433CC-Butterfly/variant.o \
	avr/dtostrf.o \
	avr/eeprom.o \
	avr/fdevopen.o \
	CDC.o \
	FS.o \
	IPAddress.o \
	Print.o \
	RingBuffer.o \
	STM32.o \
	Stream.o \
	USBCore.o \
	Uart.o \
	WMath.o \
	WString.o \
	abi.o \
	hooks.o \
	itoa.o \
	main.o \
	new.o \
	stm32l4_wiring.o \
	stm32l4_wiring_analog.o \
	stm32l4_wiring_digital.o \
	stm32l4_wiring_interrupts.o \
	stm32l4_wiring_pulse.o \
	stm32l4_wiring_shift.o \
	stm32l4_wiring_tone.o

all:: flash.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

flash.elf:: $(OBJS)
	$(CC) $(LDFLAGS) -Wl,-Map,flash.map -Wl,--start-group $(OBJS) -Wl,--end-group -lstm32l433 -larm_cortexM4lf_math -lc -lm -o flash.elf

flash.bin:: flash.elf
	arm-none-eabi-objcopy -O binary flash.elf flash.bin

clean::
	rm -f flash.elf flash.bin flash.map *~ $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)
