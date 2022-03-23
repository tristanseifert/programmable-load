#include "Hardware.h"
#include "Task.h"

#include "Drivers/ExternalIrq.h"
#include "Drivers/Gpio.h"
#include "Drivers/I2C.h"
#include "Drivers/I2CDevice/PCA9543A.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

using namespace App::Main;

Drivers::I2C *Hw::gIoBus{nullptr};
Drivers::I2CDevice::PCA9543A *Hw::gIoMux{nullptr};

/**
 * @brief Initialize the local IO I²C bus
 *
 * Configure SERCOM0 as an I²C master, which is used to control the front panel and rear IO on the
 * system, as well as a few miscellaneous on-board peripherals. The following IOs are used:
 *
 * - PA08: SDA/PAD0
 * - PA09: SCL/PAD1
 * - PA10: /I2C_IRQ: Asserted by switch when either rear or front asserts irq
 * - PA06: /I2C_RESET: Reset the bus multiplexer's I²C state machine
 *
 * @return A reference to the allocated bus
 */
Drivers::I2C *Hw::InitIoBus() {
    /*
     * Configure the IO lines used for the bus
     */
    Drivers::Gpio::ConfigurePin(kIoBusSda, { // SDA
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PA08C_SERCOM0_PAD0,
    });
    Drivers::Gpio::ConfigurePin(kIoBusScl, { // SCL
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PA09C_SERCOM0_PAD1,
    });


    /*
     * Set up the GPIOs, as well as the external interrupt for the /I2C_IRQ line; whenever that IRQ
     * fires, we'll notify the main task, which in turn reads out the multiplexer's state to
     * determine whether the front or rear panel caused the interrupt, and then notifying the
     * appropriate task.
     */
    Drivers::Gpio::ConfigurePin(kIoBusIrq, {
        .mode = Drivers::Gpio::Mode::DigitalIn,
        .pull = Drivers::Gpio::Pull::Up,
        .function = MUX_PA10A_EIC_EXTINT10,
        .pinMuxEnable = 1,
    });

    Drivers::Gpio::ConfigurePin(kIoBusReset, {
        .mode = Drivers::Gpio::Mode::DigitalOut,
        .pull = Drivers::Gpio::Pull::Up,
        .initialOutput = 1,
    });

    // enable irq on falling edge of interrupt line
    Drivers::ExternalIrq::ConfigureLine(10, {
        .irq = 1,
        .event = 0,
        .filter = 1,
        .debounce = 0,
        .mode = Drivers::ExternalIrq::SenseMode::EdgeFalling
    });

    NVIC_SetPriority(EIC_10_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3);
    NVIC_EnableIRQ(EIC_10_IRQn);

    /*
     * Configure the I²C bus now. We run the bus at a standard 400kHz speed.
     */
    static const Drivers::I2C::Config cfg{
        .frequency = 400'000,
    };

    static uint8_t gI2CBuf[sizeof(Drivers::I2C)] __attribute__((aligned(alignof(Drivers::I2C))));
    auto ptr = reinterpret_cast<Drivers::I2C *>(gI2CBuf);
    gIoBus = new (ptr) Drivers::I2C(Drivers::SercomBase::Unit::Unit0, cfg);

    return gIoBus;
}

/**
 * @brief Initialize the IO bus multiplexer
 *
 * This sets up the PCA9543A 2 channel I²C switch, used to create separate front panel and rear IO
 * board busses. (Note that the rear IO bus is used for a fan controller on the controller board
 * as well.)
 *
 * Here, we'll reset the mux (by asserting /I2C_RESET for a bit) and then initialize a driver for
 * it. This will provide us two downstream busses, which we provide.
 *
 * @param outBusses Buffer for the two output busses
 */
void Hw::InitIoBusMux(etl::span<Drivers::I2CBus *, 2> outBusses) {
    // assert reset for at least 10ms
    SetIoBusReset(true);
    vTaskDelay(pdMS_TO_TICKS(10));
    SetIoBusReset(false);

    // initialize the mux driver
    static uint8_t gMuxBuf[sizeof(Drivers::I2CDevice::PCA9543A)]
        __attribute__((aligned(alignof(Drivers::I2CDevice::PCA9543A))));
    auto ptr = reinterpret_cast<Drivers::I2CDevice::PCA9543A *>(gMuxBuf);
    gIoMux = new (ptr) Drivers::I2CDevice::PCA9543A(kIoMuxAddress, gIoBus);

    // output its two busses
    outBusses[0] = gIoMux->getDownstream0();
    outBusses[1] = gIoMux->getDownstream1();
}



/**
 * @brief Query which of the IO busses asserted an interrupt
 *
 * This reads the interrupt status register out of the IO bus mux.
 *
 * @param front When set, the front panel interrupt is asserted
 * @param rear When set, the rear IO interrupt is asserted
 *
 * @return 0 on success
 */
int Hw::QueryIoIrq(bool &front, bool &rear) {
    return gIoMux->readIrqState(front, rear);
}



/**
 * @brief Interrupt handler for IO bus
 *
 * Indicates a falling edge was detected on the /I2C_IRQ line; this notifies the main task to query
 * the bus mux for which bus behind it caused it.
 */
void EIC_10_Handler() {
    BaseType_t woken{pdFALSE};

    if(Drivers::ExternalIrq::HandleIrq(10)) {
        Task::NotifyTaskFromIsr(Task::TaskNotifyBits::IoBusInterrupt, &woken);
    }

    portYIELD_FROM_ISR(woken);
}
