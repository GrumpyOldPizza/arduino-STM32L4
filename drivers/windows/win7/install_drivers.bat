#@echo off
"%~dp0\wdi-simple" -v 0x0483 -p 0xDF11 -t 0 -m "STMicroelectronics" -n "STM32 BOOTLOADER" -d "dragonfly-dfu"
"%~dp0\wdi-simple" -v 0x1209 -p 0x6667 -i 1 -t 3 -m "Tlera Corporation" -n "Dragonfly Virtual Serial Port" -d "dragonfly-cdc-acm"
