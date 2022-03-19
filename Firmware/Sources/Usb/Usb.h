/**
 * @file
 *
 * @brief USB stack
 */
#ifndef USB_USB_H
#define USB_USB_H

/// USB related stuff
namespace UsbStack {
/**
 * @brief Initialize the USB stack
 *
 * The USB driver runs in its own task, and this call initializes the task. Actual initialization
 * of the USB hardware is delayed until the scheduler is started and the task begins execution,
 * but the hardware will already be set up.
 */
void Init();
}

#endif
