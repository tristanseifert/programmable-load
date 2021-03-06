#include "Gpio.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>

using namespace Drivers;

static PortGroup *MmioFor(const Gpio::Port port);
static PortGroup *MmioFor(const Gpio::Pin pin) {
    return MmioFor(pin.first);
}

static void DisableIo(const Gpio::Pin pin, const Gpio::PinConfig &config);
static void ConfigureDigitalIo(const Gpio::Pin pin, const Gpio::PinConfig &config);
static void ConfigurePeripheralIo(const Gpio::Pin, const Gpio::PinConfig &config);

static void ConfigurePull(PortGroup *regs, const uint8_t pin, const Gpio::PinConfig &config,
        const uint8_t pinCfgBase = 0);



/**
 * @brief Configure an IO pin
 *
 * Sets up the pin as one of three categories: disabled, digital IO, or peripheral IO. According to
 * the values in the pin config struct, the pin is appropriately configured.
 *
 * @param pin IO pin to configure
 * @param config Detailed pin configuration
 */
void Gpio::ConfigurePin(const Pin pin, const PinConfig &config) {
    REQUIRE(pin.second <= 32, "invalid pin: %u", pin.second);

    switch(config.mode) {
        /*
         * Disable all digital circuitry on the pin (unused)
         */
        case Mode::Off:
            DisableIo(pin, config);
            break;

        /*
         * Put the pin into analog mode.
         *
         * This has the same effect as disabling all digital circuitry.
         */
        case Mode::Analog:
            DisableIo(pin, config);
            // TODO: do we need to do additional stuff to analogize it?
            break;

        /*
         * Digital IOs, controlled directly by PORT
         */
        case Mode::DigitalIn:
        case Mode::DigitalOut:
            ConfigureDigitalIo(pin, config);
            break;

        /**
         * Peripheral IO
         */
        case Mode::Peripheral:
            ConfigurePeripheralIo(pin, config);
            break;
    }
}

/**
 * @brief Set the state of an IO pin
 *
 * Sets the state of an output pin.
 *
 * @param pin GPIO to set
 * @param state Whether the output is high (true) or not
 */
void Gpio::SetOutputState(const Pin pin, const bool state) {
    auto regs = MmioFor(pin);

    if(state) {
        regs->OUTSET.reg = (1U << static_cast<uint32_t>(pin.second));
    } else {
        regs->OUTCLR.reg = (1U << static_cast<uint32_t>(pin.second));
    }
}

/**
 * @brief Read an IO pin
 *
 * Gets the state of an input pin.
 *
 * @param pin IO pin to read
 *
 * @return State of the IO pin
 *
 * @remark The return value is only valid if the pin is configured as an input.
 */
bool Gpio::GetInputState(const Pin pin) {
    auto regs = MmioFor(pin);

    return !!(regs->IN.reg & (1U << static_cast<uint32_t>(pin.second)));
}



/**
 * @brief Get the register base for the given IO port
 *
 * @param port A GPIO port enumeration value
 *
 * @return GPIO register bank for the specified port
 */
static PortGroup *MmioFor(const Gpio::Port p) {
    switch(p) {
        case Gpio::Port::PortA:
            return &PORT->Group[0];
        case Gpio::Port::PortB:
            return &PORT->Group[1];
        case Gpio::Port::PortC:
            return &PORT->Group[2];

        default:
            Logger::Panic("invalid gpio port %u", static_cast<uint8_t>(p));
    }
}



/**
 * @brief Disable an IO pin
 *
 * Disables all digital circuitry on an IO pin. This includes pull up/down resistors, and both the
 * input and output buffers.
 */
static void DisableIo(const Gpio::Pin pin, const Gpio::PinConfig &config) {
    auto regs = MmioFor(pin);

    taskENTER_CRITICAL();

    // DIR = 0, INEN = 0, PULLEN = 0
    regs->DIRCLR.reg = (1UL << static_cast<uint32_t>(pin.second));
    regs->PINCFG[pin.second].reg = 0 & PORT_PINCFG_MASK;

    ConfigurePull(regs, pin.second, config);

    taskEXIT_CRITICAL();
}

/**
 * @brief Configure an IO pin as digital IO
 *
 * The pin is placed under full control of the PORT controller, and designated as either an input
 * or output pin. It's configurable whether pull up/down resistors are enabled also.
 */
static void ConfigureDigitalIo(const Gpio::Pin pin, const Gpio::PinConfig &config) {
    auto regs = MmioFor(pin);

    taskENTER_CRITICAL();

    // configure the direction
    if(config.mode == Gpio::Mode::DigitalOut) {
        if(config.initialOutput) {
            regs->OUTSET.reg = (1U << static_cast<uint32_t>(pin.second));
        } else {
            regs->OUTCLR.reg = (1U << static_cast<uint32_t>(pin.second));
        }

        regs->DIRSET.reg = (1UL << static_cast<uint32_t>(pin.second));
    } else {
        regs->DIRCLR.reg = (1UL << static_cast<uint32_t>(pin.second));
    }

    // build the base pin config (enable pin mux, if requested)
    uint8_t base{0};

    if(config.pinMuxEnable) {
        base |= PORT_PINCFG_PMUXEN;
    }

    // configure pull resistors for inputs
    if(config.mode == Gpio::Mode::DigitalIn) {
        base |= PORT_PINCFG_INEN;
        ConfigurePull(regs, pin.second, config, base);
    }
    // configure drive strength for outputs
    else {
        if(config.driveStrength) {
            base |= PORT_PINCFG_DRVSTR;
        }
        regs->PINCFG[pin.second].reg = base;
    }

    taskEXIT_CRITICAL();
}

/**
 * @brief Configure an IO pin for peripheral use
 *
 * The IO pin is configuered for exclusive control by a peripheral. Its direction and IO value will
 * instead be controlled by the peripheral.
 */
static void ConfigurePeripheralIo(const Gpio::Pin pin, const Gpio::PinConfig &config) {
    auto regs = MmioFor(pin);

    taskENTER_CRITICAL();

    // specify the pin multiplexer function
    if(pin.second & 1) {
        regs->PMUX[pin.second / 2].bit.PMUXO = config.function;
    } else {
        regs->PMUX[pin.second / 2].bit.PMUXE = config.function;
    }

    // enable pin multiplexer mode
    regs->DIRCLR.reg = (1UL << static_cast<uint32_t>(pin.second));
    const uint32_t base = (PORT_PINCFG_PMUXEN) | (config.driveStrength ? PORT_PINCFG_DRVSTR : 0);

    // optionally configure pull resistors also
    ConfigurePull(regs, pin.second, config, base);

    taskEXIT_CRITICAL();
}

/**
 * @brief Configure the pull up/down resistors on a pin
 *
 * @param pinCfgBase Base value for the pin config register; it is ORed with the appropriate pull
 *        resistor configuration values.
 *
 * @remark This should only be called if the pin is configured as an input or disabled; otherwise,
 *         the results are undefined.
 *
 * @remark Use this function as part of another GPIO config function that takes a critical section.
 */
static void ConfigurePull(PortGroup *regs, const uint8_t pin, const Gpio::PinConfig &config,
        const uint8_t pinCfgBase) {
    switch(config.pull) {
        // disable pull resistors
        case Gpio::Pull::None:
            regs->PINCFG[pin].reg = pinCfgBase;
            break;
        // enable pull up resistors
        case Gpio::Pull::Up:
            regs->OUTSET.reg = (1UL << static_cast<uint32_t>(pin));
            regs->PINCFG[pin].reg = pinCfgBase | PORT_PINCFG_PULLEN;
            break;
        // enable pull down restors
        case Gpio::Pull::Down:
            regs->OUTCLR.reg = (1UL << static_cast<uint32_t>(pin));
            regs->PINCFG[pin].reg = pinCfgBase | PORT_PINCFG_PULLEN;
            break;
    }
}
