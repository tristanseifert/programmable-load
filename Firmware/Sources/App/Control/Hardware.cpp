#include "Hardware.h"
#include "Task.h"

#include "Drivers/ExternalIrq.h"
#include "Drivers/Gpio.h"
#include "Drivers/I2C.h"

#include "Log/Logger.h"

#include <vendor/sam.h>

using namespace App::Control;

Drivers::I2C *Hw::gBus{nullptr};

/**
 * @brief Initialize control loop hardware
 *
 * It sets up the SERCOM3 as I²C, a few GPIOs and an external interrupt for both the trigger and
 * driver interrupt inputs.
 */
void Hw::Init() {
    int err;

    /*
     * Configure the digital IOs for the driver:
     *
     * - PB06: /DRIVER_RESET
     * - PB09: /DRIVER_IRQ
     * - PB11: /DRIVER_TRIGGER
     */
    Drivers::Gpio::ConfigurePin(kDriverReset, {
        .mode = Drivers::Gpio::Mode::DigitalOut,
        .initialOutput = 1,
    });
    Drivers::Gpio::ConfigurePin(kDriverIrq, {
        .mode = Drivers::Gpio::Mode::DigitalIn,
        .pull = Drivers::Gpio::Pull::Up,
        .function = MUX_PB09A_EIC_EXTINT9,
        .pinMuxEnable = 1,
    });
    Drivers::Gpio::ConfigurePin(kExternalTrigger, {
        .mode = Drivers::Gpio::Mode::DigitalIn,
        .pull = Drivers::Gpio::Pull::Up,
        .function = MUX_PB11A_EIC_EXTINT11,
        .pinMuxEnable = 1,
    });

    /*
     * Configure external interrupts for the driver IRQ line, and the trigger input.
     */
    Drivers::ExternalIrq::ConfigureLine(9, {
        .irq = 1,
        .event = 0,
        .filter = 1,
        .debounce = 0,
        .mode = Drivers::ExternalIrq::SenseMode::EdgeFalling
    });

    NVIC_SetPriority(EIC_9_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_EnableIRQ(EIC_9_IRQn);

    Drivers::ExternalIrq::ConfigureLine(11, {
        .irq = 1,
        .event = 0,
        .filter = 1,
        .debounce = 0,
        .mode = Drivers::ExternalIrq::SenseMode::EdgeFalling
    });

    NVIC_SetPriority(EIC_11_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_EnableIRQ(EIC_11_IRQn);

    /*
     * Set up the IO pins for the I²C bus and then initialize the SERCOM as the bus master. The
     * bus is on PA22 (SDA/PAD0) and PA23 (SCL/PAD1)
     */
    static const Drivers::I2C::Config cfg{
        .frequency = 400'000,
    };

    Drivers::Gpio::ConfigurePin(kDriverSda, { // SDA
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PA22C_SERCOM3_PAD0,
    });
    Drivers::Gpio::ConfigurePin(kDriverScl, { // SCL
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PA23C_SERCOM3_PAD1,
    });

    static uint8_t gI2CBuf[sizeof(Drivers::I2C)] __attribute__((aligned(alignof(Drivers::I2C))));
    auto ptr = reinterpret_cast<Drivers::I2C *>(gI2CBuf);
    gBus = new (ptr) Drivers::I2C(Drivers::SercomBase::Unit::Unit3, cfg);

    /*
     * Pulse the reset line, then issue a reset to the "general call" address on the bus.
     */
    Logger::Trace("control: reset bus");

    PulseReset();

    etl::array<uint8_t, 1> resetData{{0x06}};
    etl::array<Drivers::I2CBus::Transaction, 1> txns{{
        {
            .address = 0x0,
            .read = 0,
            .length = 1,
            .data = resetData
        },
    }};
    err = gBus->perform(txns);
    if(err) {
        Logger::Error("control: I2C general call reset failed: %d", err);
    }
}



/**
 * @brief Pulse the driver reset line.
 *
 * This forces the line low for approximately 20ms, then waits approximately 50ms to allow the
 * devices on the bus to perform their reset.
 */
void Hw::PulseReset() {
    SetResetState(true);
    vTaskDelay(pdMS_TO_TICKS(20));
    SetResetState(false);
    vTaskDelay(pdMS_TO_TICKS(50));
}

/**
 * @brief Set the state of the driver reset line
 *
 * The line is active high, since we'll have the optocoupler on the driver board invert it.
 *
 * @param asserted Whether the reset line is asserted
 */
void Hw::SetResetState(const bool asserted) {
    Drivers::Gpio::SetOutputState(kDriverReset, asserted);
}



/**
 * @brief Driver irq asserted
 *
 * Driver board asserted its interrupt line.
 */
void EIC_9_Handler() {
    BaseType_t woken{0};

    if(Drivers::ExternalIrq::HandleIrq(9)) {
        Task::NotifyFromIsr(Task::TaskNotifyBits::IrqAsserted, &woken);
    }

    portYIELD_FROM_ISR(woken);
}

/**
 * @brief Trigger input interrupt handler
 *
 * Indicates a falling edge was detected on the trigger input.
 */
void EIC_11_Handler() {
    BaseType_t woken{0};

    if(Drivers::ExternalIrq::HandleIrq(11)) {
        Task::NotifyFromIsr(Task::TaskNotifyBits::ExternalTrigger, &woken);
    }

    portYIELD_FROM_ISR(woken);
}
