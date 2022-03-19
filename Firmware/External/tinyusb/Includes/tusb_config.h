/**
 * @file
 *
 * @brief TinyUSB stack configuration
 *
 * This file defines the configuration for TinyUSB, to make it work in this project. Specifically,
 * this means enabling the right platform code and OSAL.
 */
#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

/// We're on an Atmel SAMD5x chip
#define CFG_TUSB_MCU                    OPT_MCU_SAMD51

/// Put the controller into USB Full Speed device operation
#define CFG_TUSB_RHPORT0_MODE           (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

/// Enable FreeRTOS compatibility
#define CFG_TUSB_OS                     OPT_OS_FREERTOS

// Align USB buffers to 4 bytes
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN              __attribute__((aligned(4)))


/*
 * Device configuration section
 *
 * This defines which classes are compiled and enabled, though we still have to manually build the
 * appropriate USB descriptors and assign endpoints.
 */

/// Enable USB CDC (simulate a serial port)
#define CFG_TUD_CDC                     1
// CDC FIFO size of TX and RX
#define CFG_TUD_CDC_RX_BUFSIZE          64
#define CFG_TUD_CDC_TX_BUFSIZE          64

/// Enable DFU runtime
#define CFG_TUD_DFU_RUNTIME             0
/// Enable actual DFU support
#define CFG_TUD_DFU                     0
/// Size of the DFU transfer buffer
#define CFG_TUD_DFU_XFER_BUFSIZE        64

/// Enable USB-TMC support (similar GPIB)
#define CFG_TUD_USBTMC                  0

/// Enable vendor-specific interface
#define CFG_TUD_VENDOR                  1
// Vendor interface FIFO size of TX and RX
#define CFG_TUD_VENDOR_RX_BUFSIZE       64
#define CFG_TUD_VENDOR_TX_BUFSIZE       64


#endif
