# Programmable Load Firmware
This directory contains the real-time firmware for the programmable load, to be built using the llvm (version 13) `arm-none-unknown-eabi` toolchain. It runs on the M4 core of the [STM32MP151 SoC](https://www.st.com/en/microcontrollers-microprocessors/stm32mp151.html) the programmable load is built around, running its own RTOS. It communicates with the host software (there, its counterpart is `loadd`) for remote control using the OpenAMP framework's rpmsg facilities.

Notable features include:
- Based around FreeRTOS
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
Currently, only the rev 3 programmable load controller board is supported, though adding support for later reivsions is simple: this firmware has a very limited set of peripherals under its purview.

### Analog Board Support
Various types of analog boards can be supported, connected via the dedicated SPI + I²C busses. Boards are identified by means of an AT24CS32-style EEPROM on the I²C bus, which is then used to instantiate the appropriate driver in the firmware. This EEPROM also provides the board's unique serial number, as well as some additional manufacturing information, and any auxiliary board-specific data the driver needs to store (such as calibration or limits) and access at runtime.

Supported analog boards (and their drivers) are:

** None yet! Check back later :) **
