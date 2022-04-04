#include "PCA9955B.h"
#include "Common.h"

#include "Drivers/I2CBus.h"
#include "Log/Logger.h"

using namespace Drivers::I2CDevice;

/**
 * @brief Initialize the LED controller
 *
 * This configures the device registers, namely each channel's mode and full brightness current
 * values.
 */
PCA9955B::PCA9955B(Drivers::I2CBus *bus, const uint8_t busAddress, const uint16_t refCurrent,
        etl::span<const LedConfig, kNumChannels> config) : bus(bus), refCurrent(refCurrent),
        busAddress(busAddress) {
    int err;

    /*
     * Configure global settings:
     *
     * - MODE1: Disable all secondary I²C addresses; enable regular auto-increment mode
     * - MODE2: Change outputs after STOP condition; use exponential brightness ramp
     *
     * Also, reset all the LEDOUT mode for each channel individual brightness control via the
     * PWM pins.
     */
    static etl::array<uint8_t, 7> mode{{
        // write to Mode 1, auto increment
        ((1 << 7) | static_cast<uint8_t>(Regs::Mode1)),
        // MODE1
        0b10000000,
        // MODE2
        0b00010101,
        // LEDOUT 0
        0b10101010,
        0b10101010,
        0b10101010,
        0b10101010,
    }};
    etl::array<I2CBus::Transaction, 1> modeTxns{{
        {
            .address = this->busAddress,
            .read = 0,
            .length = mode.size(),
            .data = mode,
        },
    }};
    err = this->bus->perform(modeTxns);
    REQUIRE(!err, "%s: failed to set %s (%d)", "PCA9955B", "mode registers", err);

    /*
     * Then, build up a buffer for each channel's brightness value, then writes them all at once
     * as one transaction.
     */
    etl::array<uint8_t, kNumChannels+1> irefBuf;
    irefBuf[0] = ((1 << 7) | static_cast<uint8_t>(Regs::IREF0));

    for(size_t i = 0; i < kNumChannels; i++) {
        const auto &led = config[i];
        float proportion = static_cast<float>(led.fullCurrent) / static_cast<float>(refCurrent);
        irefBuf[1+i] = static_cast<uint8_t>(0xFF * proportion);
    }

    etl::array<I2CBus::Transaction, 1> irefBufTxns{{
        {
            .address = this->busAddress,
            .read = 0,
            .length = irefBuf.size(),
            .data = irefBuf,
        },
    }};
    err = this->bus->perform(irefBufTxns);
    REQUIRE(!err, "%s: failed to set %s (%d)", "PCA9955B", "IREF", err);

    /*
     * Lastly, configure each channel's gradation group and its output mode. Each set of four
     * output channels shares one register
     */
    etl::array<uint8_t, 5> gradationGroups{{
        ((1 << 7) | static_cast<uint8_t>(Regs::GradationGroup0)),
        0, 0, 0, 0
    }};
    etl::array<uint8_t, 5> ledMode{{
        ((1 << 7) | static_cast<uint8_t>(Regs::LEDOUT0)),
        0, 0, 0, 0
    }};

    for(size_t i = 0; i < kNumChannels; i++) {
        const auto &led = config[i];

        gradationGroups[1 + (i / 4)] |= (led.gradationGroup << ((i % 4) * 2));

        if(led.enabled) {
            ledMode[1 + (i / 4)] |= (0b10 << ((i % 4) * 2));
        }
    }

    etl::array<I2CBus::Transaction, 2> ledModeTxns{{
        {
            .address = this->busAddress,
            .read = 0,
            .length = gradationGroups.size(),
            .data = gradationGroups,
        },
        {
            .address = this->busAddress,
            .read = 0,
            .length = ledMode.size(),
            .data = ledMode,
        },
    }};
    err = this->bus->perform(ledModeTxns);
    REQUIRE(!err, "%s: failed to set %s (%d)", "PCA9955B", "gradation/channel mode", err);
}

/**
 * @brief Clean up all driver resources
 *
 * This will set the brightness of all channels to zero.
 */
PCA9955B::~PCA9955B() {
    int err;

    // disable all LEDs
    etl::array<uint8_t, 2> toWrite{{
        static_cast<uint8_t>(Regs::PWMALL),
        0
    }};
    etl::array<I2CBus::Transaction, 1> txns{{
        {
            .address = this->busAddress,
            .read = 0,
            .length = toWrite.size(),
            .data = toWrite,
        },
    }};

    err = this->bus->perform(txns);
    if(err) {
        Logger::Warning("%s: failed to set %s (%d)", "PCA9955B", "PWMALL", err);
    }
}

/**
 * @brief Set the brightness of a channel
 *
 * @param channel A channel index, [0,15]
 * @param brightness Brightness value, [0,1]
 *
 * @return 0 on success, or an error code
 */
int PCA9955B::setBrightness(const uint8_t channel, const float level) {
    float adjustedLevel = level > 0 ? level : 0;
    adjustedLevel = level < 1 ? level : 1;

    // build register write struct
    etl::array<uint8_t, 2> toWrite{{
        static_cast<uint8_t>(Regs::PWM0 + channel),
        static_cast<uint8_t>(0xFF * adjustedLevel),
    }};
    etl::array<I2CBus::Transaction, 1> txns{{
        {
            .address = this->busAddress,
            .read = 0,
            .length = toWrite.size(),
            .data = toWrite,
        },
    }};

    return this->bus->perform(txns);
}

