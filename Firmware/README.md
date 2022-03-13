# Programmable Load Firmware
This directory contains the firmware for the programmable load, to be built using the llvm (version 13) `arm-none-unknown-eabi` toolchain. It's meant to run on an Atmel/Microchip SAM D5x series chip; specifically, we target the ATSAME51J20A, although chips with smaller flash/RAM will likely work as well. The ATSAME53J20A (with Ethernet MAC) is also supported, and will in the future support Ethernet/TCP interfacing as well with the same hardware, if an Ethernet PHY is populated.

Notable features include:
- Based around FreeRTOS
- USB stack (powered by [TinyUSB](https://github.com/hathach/tinyusb)) to provide a serial console, firmware upgrade support, and data logging
- Graphic user interface on 256x64 OLED with local buttons
- Versatile set of control modes
- Clean, easily hackable code

## Building
To get started with the code, ensure you have a CMake toolchain file for your ARM toolchain. Inside a build directory (in this case, `build`) issue the following commands:

```
mkdir build
cmake -G Ninja -B build -DCMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Hardware Support
Currently, only the rev 1 controller board, and associated IO boards (rear panel + front panel) are supported. When other boards are added, these are identified by their configuration EEPROMs. For the processor board itself, we have an SPI flash that can be read out to determine board revision/type.
