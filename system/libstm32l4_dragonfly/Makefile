TOOLS    = /opt/gcc-arm-none-eabi-4.8.3-2014q1
CPP      = $(TOOLS)/bin/arm-none-eabi-cpp
CC       = $(TOOLS)/bin/arm-none-eabi-gcc
CXX      = $(TOOLS)/bin/arm-none-eabi-g++
AS       = $(TOOLS)/bin/arm-none-eabi-as
AR       = $(TOOLS)/bin/arm-none-eabi-ar
LD       = $(TOOLS)/bin/arm-none-eabi-ld
CFLAGS   = -mcpu=cortex-m4 -mthumb -c -g -Os $(WARNINGS) -std=gnu11 -ffunction-sections -fdata-sections -nostdlib --param max-inline-insns-single=500 -MMD $(EXTRAS) $(DEFINES) $(INCLUDES)
CXXFLAGS = -mcpu=cortex-m4 -mthumb -c -g -Os $(WARNINGS) -std=gnu++11 -ffunction-sections -fdata-sections -fno-threadsafe-statics -nostdlib --param max-inline-insns-single=500 -fno-rtti -fno-exceptions -MMD $(EXTRAS) $(DEFINES) $(INCLUDES)
ASFLAGS  = -c -g -x assembler-with-cpp $(EXTRAS) $(DEFINES) $(INCLUDES)
WARNINGS = -w
EXTRAS   = -DSTM32L476xx -D__FPU_PRESENT=1 -march=armv7e-m -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mabi=aapcs -mslow-flash-data
DEFINES  = -F_CPU=80000000L -DARDUINO=10606 -D_ARDUINO_STM32L4_DRAGONFLY -DARDUINO_ARCH_STM32L4
INCLUDES = -I../../system/libstm32l4_dragonfly/CMSIS/Include -I../../system/libstm32l4_dragonfly/CMSIS/Device/ST/STM32L4xx/Include -I../../system/libstm32l4_dragonfly/USB/HAL/Inc -I../../system/libstm32l4_dragonfly/USB/Core/Inc -I../../system/libstm32l4_dragonfly/USB/Class/CDC/Inc -I../../system/libstm32l4_dragonfly/USB/Class/MSC/Inc -I../../system/libstm32l4_dragonfly/USB -I../../system/libstm32l4_dragonfly/ -I../../variants/dragonfly -I. 

SRCS = \
	./USB/Class/CDC/Src/usbd_cdc.c \
	./USB/Class/MSC/Src/usbd_msc.c \
	./USB/Class/MSC/Src/usbd_msc_bot.c \
	./USB/Class/MSC/Src/usbd_msc_data.c \
	./USB/Class/MSC/Src/usbd_msc_scsi.c \
	./USB/Core/Src/usbd_core.c \
	./USB/Core/Src/usbd_ctlreq.c \
	./USB/Core/Src/usbd_ioreq.c \
	./USB/HAL/Src/stm32l4xx_hal_pcd.c \
	./USB/HAL/Src/stm32l4xx_hal_pcd_ex.c \
	./USB/HAL/Src/stm32l4xx_ll_usb.c \
	./USB/usbd_cdc_msc.c \
	./USB/usbd_conf.c \
	./USB/usbd_desc.c \
	armv7m_atomic.c \
	armv7m_clock.c \
	armv7m_core.c \
	armv7m_orchid.c \
	armv7m_pendsv.c \
	armv7m_svcall.c \
	armv7m_systick.c \
	armv7m_timer.c \
	dosfs_core.c \
	dosfs_sdcard.c \
	dosfs_sflash.c \
	dosfs_storage.c \
	stm32l4_adc.c \
	stm32l4_dac.c \
	stm32l4_dma.c \
	stm32l4_exti.c \
	stm32l4_i2c.c \
	stm32l4_gpio.c \
	stm32l4_nvic.c \
	stm32l4_qspi.c \
	stm32l4_spi.c \
	stm32l4_servo.c \
	stm32l4_system.c \
	stm32l4_timer.c \
	stm32l4_uart.c \
	stm32l4_usbd_cdc.c \
	system_stm32l4xx.c \
	startup_stm32l476xx.S

OBJS = \
	./USB/Class/CDC/Src/usbd_cdc.o \
	./USB/Class/MSC/Src/usbd_msc.o \
	./USB/Class/MSC/Src/usbd_msc_bot.o \
	./USB/Class/MSC/Src/usbd_msc_data.o \
	./USB/Class/MSC/Src/usbd_msc_scsi.o \
	./USB/Core/Src/usbd_core.o \
	./USB/Core/Src/usbd_ctlreq.o \
	./USB/Core/Src/usbd_ioreq.o \
	./USB/HAL/Src/stm32l4xx_hal_pcd.o \
	./USB/HAL/Src/stm32l4xx_hal_pcd_ex.o \
	./USB/HAL/Src/stm32l4xx_ll_usb.o \
	./USB/usbd_cdc_msc.o \
	./USB/usbd_conf.o \
	./USB/usbd_desc.o \
	armv7m_atomic.o \
	armv7m_clock.o \
	armv7m_core.o \
	armv7m_orchid.o \
	armv7m_pendsv.o \
	armv7m_svcall.o \
	armv7m_systick.o \
	armv7m_timer.o \
	dosfs_core.o \
	dosfs_sdcard.o \
	dosfs_sflash.o \
	dosfs_storage.o \
	stm32l4_adc.o \
	stm32l4_dac.o \
	stm32l4_dma.o \
	stm32l4_exti.o \
	stm32l4_i2c.o \
	stm32l4_gpio.o \
	stm32l4_nvic.o \
	stm32l4_qspi.o \
	stm32l4_servo.o \
	stm32l4_spi.o \
	stm32l4_system.o \
	stm32l4_timer.o \
	stm32l4_uart.o \
	stm32l4_usbd_cdc.o \
	system_stm32l4xx.o \
	startup_stm32l476xx.o

all:: libstm32l4_dragonfly.a

libstm32l4_dragonfly.a:: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

clean::
	rm -f libstm32l4_dragonfly.a *~ $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)


