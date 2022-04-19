Here is where the hardware part of the project lives. This consists of a few different boards:

- controller: Main board, in charge of communicating with external devices (via USB or Ethernet,) driving the user interface, and running the actual control loop for whatever algorithm is desired.
- driver: This is the actual MOSFET-based load part. It features some ADCs and DACs, controlled via I2C, to adjust the current. It also has facilities for temperature sensing and a fan controller.
- front: Front panel board; features some indicator lights, buttons, and breakout for the display. Mounted vertically to front panel of the system.
- rear: Various connectors exposed on the rear of the device, such as USB and Ethernet. Mounted right angle to the rear panel of the system.

All of these boards connect to the controller in some way, shape or form. Each board in turn has an EEPROM, which identifies its type, and provides a unique serial number. This is used by the software to figure out what's connected, and what features to offer.

## Front Panels
The `panels` directory contains front and rear panels, designed with [Front Panel Designer](https://www.frontpanelexpress.com/front-panel-designer) and to be manufactured by Front Panel Express. These panels are meant to replace the existing bare aluminum panels that come standard with a [Hammond 1598JBK](https://www.hammfg.com/part/1598JBK) enclosure, for which the boards are designed.
