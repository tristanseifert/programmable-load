#include "Usb.h"
#include "Task.h"

#include "Drivers/Gpio.h"
#include "Log/Logger.h"

#include <tusb.h>

using namespace UsbStack;

/// USB D+
constexpr static const Drivers::Gpio::Pin kUsbDp{
    Drivers::Gpio::Port::PortA, 25
};
/// USB D-
constexpr static const Drivers::Gpio::Pin kUsbDm{
    Drivers::Gpio::Port::PortA, 24
};

static void InitHardware();

void UsbStack::Init() {
    // set up hardware first
    Logger::Trace("USB init %s", "hardware");
    InitHardware();

    // then set up the driver task
    Logger::Trace("USB init %s", "task");
    Task::Start();
}

/**
 * @brief Initialize USB hardware
 *
 * Configures the IO pins for the USB hardware control and the interrupts and their priorities. The
 * clocks should already have been set up.
 */
static void InitHardware() {
    using Drivers::Gpio;

    // configure the IO pins (PA24 (D-), PA25 (D+))
    Gpio::ConfigurePin(kUsbDm, {
        .mode = Gpio::Mode::Peripheral,
        .function = MUX_PA24H_USB_DM
    });
    Gpio::ConfigurePin(kUsbDp, {
        .mode = Gpio::Mode::Peripheral,
        .function = MUX_PA25H_USB_DP
    });

    // set IRQ priorities
    NVIC_SetPriority(USB_0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_SetPriority(USB_1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_SetPriority(USB_2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_SetPriority(USB_3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
}

/**
 * @brief USB IRQ 0
 *
 * Forwards the USB interrupt to the TinyUSB stack.
 */
extern "C" void USB_0_Handler() {
    tud_int_handler(0);
}

/**
 * @brief USB IRQ 1
 *
 * Forwards the USB interrupt to the TinyUSB stack.
 */
extern "C" void USB_1_Handler() {
    tud_int_handler(0);
}

/**
 * @brief USB IRQ 2
 *
 * Forwards the USB interrupt to the TinyUSB stack.
 */
extern "C" void USB_2_Handler() {
    tud_int_handler(0);
}

/**
 * @brief USB IRQ 3
 *
 * Forwards the USB interrupt to the TinyUSB stack.
 */
extern "C" void USB_3_Handler() {
    tud_int_handler(0);
}
