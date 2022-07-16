Here is where the hardware part of the project lives. This consists of a few different boards:

- analog: A 300W load board, based around up to four individual channels, each with their own MOSFET and current sense resistor. Each channel may be individually controlled, and its current individually sensed via a 24-bit ADC. Both the ADC and current drive DAC are connected via a high-speed SPI bus. The board also features a precision 2.5V voltage reference shared by all analog components, as well as a programmable clock generator for the ADC sampling clock.
- controller: Main board, in charge of communicating with external devices (via USB or Ethernet,) driving the user interface, and running the actual control loop for whatever algorithm is desired. It also contains the analog circuitry for sensing input voltage from one of several sources.
- front: Front panel board; features some indicator lights, buttons, and breakout for the display. An [RP2040](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html) acts as a system controller, driving the LEDs, reading inputs, and talking to the display controller. The board is mounted vertically to front panel of the system.

All of these boards connect to the controller in some way, shape or form. Each board in turn has an EEPROM, which identifies its type, and provides a unique serial number. This is used by the software to figure out what's connected, and what features to offer.

## Front Panels
The `panels` directory contains front and rear panels, designed with [Front Panel Designer](https://www.frontpanelexpress.com/front-panel-designer) and to be manufactured by Front Panel Express. These panels are meant to replace the existing bare aluminum panels that come standard with a [Hammond 1598JBK](https://www.hammfg.com/part/1598JBK) enclosure, for which the boards are designed.
