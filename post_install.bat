@echo off
ver | find "XP" >NUL
if %ERRORLEVEL% == 0 goto winxp
cd "%~dp0\drivers\windows\win7"
.\wdi-simple -v 0x0483 -p 0xDF11 -t 0 -m "STMicroelectronics" -n "STM32 BOOTLOADER" -d "stm32-bootloader"
.\wdi-simple -v 0x1209 -p 0x6667 -i 0 -t 3 -m "Tlera Corporation" -n "Dragonfly Virtual Serial Port" -d "dragonfly-cdc-acm"
.\wdi-simple -v 0x1209 -p 0x6668 -i 0 -t 3 -m "Tlera Corporation" -n "Butterfly Virtual Serial Port" -d "butterfly-cdc-acm"
.\wdi-simple -v 0x1209 -p 0x6669 -i 0 -t 3 -m "Tlera Corporation" -n "Ladybug Virtual Serial Port" -d "ladybug-cdc-acm"
exit /b 0
:winxp
cd "%~dp0\drivers\windows\winxp"
.\dpinst_x86
exit /b 0
