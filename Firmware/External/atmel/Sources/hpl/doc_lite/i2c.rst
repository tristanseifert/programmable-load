============
TWI driver
============

The Two-Wire Interface (TWI) is a bidirectional, two-wire communication interface. It is I2C and System
Management Bus (SMBus) compatible. The only external hardware needed to implement the bus is one
pull-up resistor on each bus line. Any device connected to the bus must act as a master or a slave. One bus can
have many slaves and one or several masters that can take control of the bus. An arbitration process
handles priority if more than one master tries to transmit data at the same time. The TWI module supports master and slave functionality. The master and slave functionality are
separated from each other, and can be enabled and configured separately. The master module supports
multi-master bus operation and arbitration. The TWI module will detect START and STOP conditions, bus collisions, and bus errors. Arbitration lost,
errors, collision, and clock hold on the bus are also detected and indicated in separate status flags
available in both master and slave modes.

Features
--------
* Initialization

Applications
------------
* Two-wire interface to communicate external devices like temperature sensor, EEPROM etc

Dependencies
------------
* CLKCTRL for clock
* CPUINT for Interrupt
* PORT for I/O Line and Connections 
* UPDI for debug

Concurrency
-----------
N/A

Limitations
-----------
N/A

Knows issues and workarounds
----------------------------
N/A

