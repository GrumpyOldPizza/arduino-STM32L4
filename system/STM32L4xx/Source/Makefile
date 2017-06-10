TOOLS    = /opt/gcc-arm-none-eabi-4.8.3-2014q1
CPP      = $(TOOLS)/bin/arm-none-eabi-cpp
CC       = $(TOOLS)/bin/arm-none-eabi-gcc
CXX      = $(TOOLS)/bin/arm-none-eabi-g++
AS       = $(TOOLS)/bin/arm-none-eabi-as
AR       = $(TOOLS)/bin/arm-none-eabi-ar
LD       = $(TOOLS)/bin/arm-none-eabi-ld
CFLAGS   = -mcpu=cortex-m4 -mthumb -c -g -Os $(WARNINGS) -std=gnu11 -ffunction-sections -fdata-sections -nostdlib -MMD $(EXTRAS) $(DEFINES) $(INCLUDES)
CXXFLAGS = -mcpu=cortex-m4 -mthumb -c -g -Os $(WARNINGS) -std=gnu++11 -ffunction-sections -fdata-sections -fno-threadsafe-statics -nostdlib -fno-rtti -fno-exceptions -MMD $(EXTRAS) $(DEFINES) $(INCLUDES)
ASFLAGS  = -c -g -x assembler-with-cpp $(EXTRAS) $(DEFINES) $(INCLUDES)
WARNINGS = -Wall -Wextra -Wno-unused-parameter
EXTRAS   = -D__FPU_PRESENT=1 -march=armv7e-m -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mabi=aapcs -mslow-flash-data
DEFINES  = 
INCLUDES = \
	-I../../../system/CMSIS/Include \
	-I../../../system/CMSIS/Device/ST/STM32L4xx/Include \
	-I../../../system/STM32L4xx/Source/DAP \
	-I../../../system/STM32L4xx/Source/USB/HAL/Inc \
	-I../../../system/STM32L4xx/Source/USB/Core/Inc \
	-I../../../system/STM32L4xx/Source/USB/Class/CDC/Inc \
	-I../../../system/STM32L4xx/Source/USB/Class/MSC/Inc \
	-I../../../system/STM32L4xx/Source/USB/Class/HID/Inc \
	-I../../../system/STM32L4xx/Source/USB \
	-I../../../system/STM32L4xx/Include \
	-I. 

BSRCS = \
	boot_stm32l4xx.c \
	stm32l4_iap.c

LSRCS = \
	./DAP/DAP.c \
	./DAP/SW_DP.c \
	./USB/Class/CDC/Src/usbd_cdc.c \
	./USB/Class/MSC/Src/usbd_msc.c \
	./USB/Class/MSC/Src/usbd_msc_bot.c \
	./USB/Class/MSC/Src/usbd_msc_data.c \
	./USB/Class/MSC/Src/usbd_msc_scsi.c \
	./USB/Class/HID/Src/usbd_hid.c \
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
	armv7m_core.c \
	armv7m_orchid.c \
	armv7m_pendsv.c \
	armv7m_rtlib.S \
	armv7m_svcall.c \
	armv7m_systick.c \
	armv7m_timer.c \
	dosfs_core.c \
	dosfs_device.c \
	dosfs_sflash.c \
	dosfs_storage.c \
	stm32l4_adc.c \
	stm32l4_clib.c \
	stm32l4_dac.c \
	stm32l4_dma.c \
	stm32l4_exti.c \
	stm32l4_flash.c \
	stm32l4_i2c.c \
	stm32l4_iwdg.c \
	stm32l4_gpio.c \
	stm32l4_nvic.c \
	stm32l4_qspi.c \
	stm32l4_rtc.c \
	stm32l4_sai.c \
	stm32l4_sdmmc.c \
	stm32l4_sdspi.c \
	stm32l4_spi.c \
	stm32l4_servo.c \
	stm32l4_system.c \
	stm32l4_timer.c \
	stm32l4_uart.c \
	stm32l4_usbd_cdc.c \
	stm32l4_usbd_dap.c \
	stm32l4_usbd_hid.c

BOBJS_L432 = $(patsubst %.c,_out/stm32l432/%.o,$(BSRCS))
LSRCS_L432 = $(LSRCS) startup_stm32l432xx.S
LOBJS_L432 = $(patsubst %.c,_out/stm32l432/%.o,$(filter-out %.S,$(LSRCS_L432))) $(patsubst %.S,_out/stm32l432/%.o,$(filter-out %.c,$(LSRCS_L432)))

BOBJS_L433 = $(patsubst %.c,_out/stm32l433/%.o,$(BSRCS))
LSRCS_L433 = $(LSRCS) startup_stm32l433xx.S
LOBJS_L433 = $(patsubst %.c,_out/stm32l433/%.o,$(filter-out %.S,$(LSRCS_L433))) $(patsubst %.S,_out/stm32l433/%.o,$(filter-out %.c,$(LSRCS_L433)))

BOBJS_L476 = $(patsubst %.c,_out/stm32l476/%.o,$(BSRCS))
LSRCS_L476 = $(LSRCS) startup_stm32l476xx.S
LOBJS_L476 = $(patsubst %.c,_out/stm32l476/%.o,$(filter-out %.S,$(LSRCS_L476))) $(patsubst %.S,_out/stm32l476/%.o,$(filter-out %.c,$(LSRCS_L476)))

BOBJS_L496 = $(patsubst %.c,_out/stm32l496/%.o,$(BSRCS))
LSRCS_L496 = $(LSRCS) startup_stm32l496xx.S
LOBJS_L496 = $(patsubst %.c,_out/stm32l496/%.o,$(filter-out %.S,$(LSRCS_L496))) $(patsubst %.S,_out/stm32l496/%.o,$(filter-out %.c,$(LSRCS_L496)))

all:: boot_stm32l432.o boot_stm32l433.o boot_stm32l476.o boot_stm32l496.o libstm32l432.a libstm32l433.a libstm32l476.a  libstm32l496.a

boot_stm32l432.o:: $(BOBJS_L432)
	$(LD) -r -o $@ $^

libstm32l432.a:: $(LOBJS_L432)
	$(AR) rcs $@ $^

_out/stm32l432/%.o: %.c
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L432xx $(CFLAGS) -c $< -o $@

_out/stm32l432/%.o: %.S
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L432xx $(ASFLAGS) -c $< -o $@


boot_stm32l433.o:: $(BOBJS_L433)
	$(LD) -r -o $@ $^

libstm32l433.a:: $(LOBJS_L433)
	$(AR) rcs $@ $^

_out/stm32l433/%.o: %.c
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L433xx $(CFLAGS) -c $< -o $@

_out/stm32l433/%.o: %.S
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L433xx $(ASFLAGS) -c $< -o $@


boot_stm32l476.o:: $(BOBJS_L476)
	$(LD) -r -o $@ $^

libstm32l476.a:: $(LOBJS_L476)
	$(AR) rcs $@ $^

_out/stm32l476/%.o: %.c
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L476xx $(CFLAGS) -c $< -o $@

_out/stm32l476/%.o: %.S
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L476xx $(ASFLAGS) -c $< -o $@


boot_stm32l496.o:: $(BOBJS_L496)
	$(LD) -r -o $@ $^

libstm32l496.a:: $(LOBJS_L496)
	$(AR) rcs $@ $^

_out/stm32l496/%.o: %.c
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L496xx $(CFLAGS) -c $< -o $@

_out/stm32l496/%.o: %.S
	-@mkdir -p $(dir $@)
	$(CC) -DSTM32L496xx $(ASFLAGS) -c $< -o $@

clean::
	rm -rf boot_stm32l432.o boot_stm32l433.o boot_stm32l476.o boot_stm32l496.o libstm32l432.a libstm32l433.a libstm32l476.a libstm32l496.a _out

-include $(BOBJS_L432:.o=.d)
-include $(BOBJS_L433:.o=.d)
-include $(BOBJS_L476:.o=.d)
-include $(BOBJS_L496:.o=.d)

-include $(LOBJS_L432:.o=.d)
-include $(LOBJS_L433:.o=.d)
-include $(LOBJS_L476:.o=.d)
-include $(LOBJS_L496:.o=.d)


