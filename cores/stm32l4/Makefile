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
LDFLAGS  = -L../../variants/dragonfly -Os -Wl,--gc-sections -save-temps $(EXTRAS) -T../../variants/dragonfly/linker_scripts/flash.ld -specs=nano.specs --specs=nosys.specs -mcpu=cortex-m4 -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align
WARNINGS = -Wall -Wextra -Wno-unused-parameter
EXTRAS   = -DSTM32L476xx -D__FPU_PRESENT=1 -march=armv7e-m -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mabi=aapcs -mslow-flash-data
DEFINES  = -DF_CPU=80000000L -DARDUINO=10606 -D_ARDUINO_STM32L4_DRAGONFLY -DARDUINO_ARCH_STM32L4
INCLUDES = -I../../system/libstm32l4_dragonfly/CMSIS/Include -I../../system/libstm32l4_dragonfly/CMSIS/Device/ST/STM32L4xx/Include -I../../system/libstm32l4_dragonfly/USB/HAL/Inc -I../../system/libstm32l4_dragonfly/USB/Core/Inc -I../../system/libstm32l4_dragonfly/USB/Class/CDC/Inc -I../../system/libstm32l4_dragonfly/USB/Class/MSC/Inc -I../../system/libstm32l4_dragonfly/USB -I../../system/libstm32l4_dragonfly/ -I../../variants/dragonfly -I../../libraries/SPI/ -I../../libraries/Wire/ -I../../libraries/Servo/src/ -I. 

SRCS = \
	../../libraries/Servo/src/Servo.cpp \
	../../libraries/SPI/SPI.cpp \
	../../libraries/Wire/Wire.cpp \
	../../variants/dragonfly/variant.cpp \
	CDC.cpp \
	FS.cpp \
	HardwareSerial.cpp \
	IPAddress.cpp \
	Print.cpp \
	Reset.cpp \
	RingBuffer.cpp \
	STM32.cpp \
	Stream.cpp \
	Tone.cpp \
	Uart.cpp \
	WInterrupts.c \
	WMath.cpp \
	WString.cpp \
	abi.cpp \
	hooks.c \
	main.cpp \
	new.cpp \
	stdlib_noniso.c \
	wiring.c \
	wiring_analog.c \
	wiring_digital.c \
	wiring_pulse.c \
	wiring_shift.c

OBJS = \
	../../libraries/Servo/src/Servo.o \
	../../libraries/SPI/SPI.o \
	../../libraries/Wire/Wire.o \
	../../variants/dragonfly/variant.o \
	CDC.o \
	FS.o \
	HardwareSerial.o \
	IPAddress.o \
	Print.o \
	Reset.o \
	RingBuffer.o \
	STM32.o \
	Stream.o \
	Tone.o \
	Uart.o \
	WInterrupts.o \
	WMath.o \
	WString.o \
	abi.o \
	hooks.o \
	main.o \
	new.o \
	stdlib_noniso.o \
	wiring.o \
	wiring_analog.o \
	wiring_digital.o \
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
	$(CC) $(LDFLAGS) -Wl,-Map,flash.map -Wl,--start-group $(OBJS) -Wl,--end-group -lstm32l4_dragonfly -lm -o flash.elf

flash.bin:: flash.elf
	arm-none-eabi-objcopy -O binary flash.elf flash.bin

clean::
	rm -f flash.elf flash.bin flash.map *~ $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)
