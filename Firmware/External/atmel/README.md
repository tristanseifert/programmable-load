# Atmel Vendor-Provided Base
This directory contains sources from the Atmel-provided starter code, as exported from [Atmel START.](https://start.atmel.com) Much of it will likely not end up getting used, but we're gathering it all here for completeness' sake -- and also, because it's a huge pain in the ass to try and separate out. The sources consist of the four top level directories copied out of the bundle:

- config: Peripheral configuration, autogenerated by START.
- hal: Hardware abstraction layer; a high level interface to the peripherals. This is not really compatible with an RTOS so it's not going to be used.
- hpl: Low level utility drivers; thin wrappers around bare register accesses, useful for building larger drivers on
- hri: Definitions for hardware registers

Additionally, the Includes directory contains the contents of the `include` directory in the bundle, and some symlinks created to code internal to the project; this is used so the firmware can just include from this directory, rather than reaching into the innards of the source. (So, when updating it with the new code, be sure to keep those symlinks intact.)
