# Programmable Load Host Tools
This directory contains a variety of tools that run on a computer connected to the load via USB or Ethernet.

## libload
This library implements USB communication with the load. Internally, it's a thin wrapper around libusb that provides a nicer C++ interface to the device.

When/if the load is updated to support TCP/IP communication, this library will be updated to handle that as well.

## loadutil
A command line utility, which embeds libload, used to communicate with the programmable load. This allows basic tasks like retrieving device information, controlling the device, and accessing some additional internal functions.
