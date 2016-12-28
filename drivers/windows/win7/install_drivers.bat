#@echo off
"%~dp0\wdi-simple" -v 0x0483 -p 0xDF11 -t 0 -m "STMicroelectronics" -n "STM32 BOOTLOADER" -d "stm32-bootloader"
"%~dp0\wdi-simple" -v 0x1209 -p 0x6667 -i 0 -t 3 -m "Tlera Corporation" -n "Dragonfly Virtual Serial Port" -d "dragonfly-cdc-acm"
"%~dp0\wdi-simple" -v 0x1209 -p 0x6668 -i 0 -t 3 -m "Tlera Corporation" -n "Butterfly Virtual Serial Port" -d "dragonfly-cdc-acm"
"%~dp0\wdi-simple" -v 0x1209 -p 0x6669 -i 0 -t 3 -m "Tlera Corporation" -n "Ladybug Virtual Serial Port" -d "dragonfly-cdc-acm"
