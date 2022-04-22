#ifndef APP_CONTROL_DUMBLOADDRIVER_H
#define APP_CONTROL_DUMBLOADDRIVER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>

#include "LoadDriver.h"

#include "Drivers/I2CDevice/DAC60501.h"
#include "Drivers/I2CDevice/MCP3421.h"
#include "Drivers/I2CDevice/PI4IOE5V9536.h"
#include "Rtos/Rtos.h"
#include "Util/Uuid.h"

namespace App::Control {
/**
 * @brief Driver for "dumb" analog board
 *
 * This is a driver for the basic programmable load board, which features a pile of discrete ADCs
 * and DACs to implement the load control. The control loop is implemented in software in the
 * firmware for modes other than constant current.
 *
 * In addition, the driver implements support for remote voltage sense (under automatic control)
 * and temperature reporting of the MOSFETs. The number of MOSFET channels is also configurable
 * via the PROM, to disable the second channel if needed. Lastly, there is an EMC2101 fan 
 * controller on the board, which is used for automatic control of the fan cooling the MOSFETs
 * with a discrete temperature sensor connected to the driver.
 *
 * Calibration values (for the input voltage sense, current sense, and current driver) are read
 * from the IDPROM during initialization and applied.
 */
class DumbLoadDriver: public LoadDriver {
    public:
        /// Driver UUID bytes
        constexpr static const etl::array<uint8_t, Util::Uuid::kByteSize> kUuidBytes{{
            0x32, 0x4E, 0x77, 0xA4, 0x0F, 0xFF, 0x4D, 0x6D, 0xB4, 0x83, 0xAB, 0xB6, 0x6C, 0xC6, 0x51, 0xFB
        }};

        /**
         * @brief Driver UUID
         *
         * Use this UUID in the inventory ROM on the analog load board to match the driver.
         */
        static const Util::Uuid kDriverId;

    public:
        DumbLoadDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom);
        ~DumbLoadDriver();

        void handleIrq() override;

        int setEnabled(const bool isEnabled) override;
        int readInputCurrent(uint32_t &outCurrent) override;
        int setOutputCurrent(const uint32_t current) override {
            return this->setOutputCurrent(current, false);
        }

        int readInputVoltage(uint32_t &outVoltage) override;
        int setExternalVSense(const bool isExternal) override;

        /// Return the maximum input voltage we read from EEPROM during initialization
        int getMaxInputVoltage(uint32_t &outVoltage) override {
            outVoltage = this->maxVoltage;
            return 0;
        }
        /// Return the maximum input current we read from EEPROM during initialization
        int getMaxInputCurrent(uint32_t &outCurrent) override {
            outCurrent = this->maxCurrent;
            return 0;
        }

    private:
        /**
         * @brief Set the state of the LED indicator
         *
         * Controls whether the LED connected to one of the spare outputs of the IO expander is lit.
         *
         * @param isLit When set, the LED is illuminated.
         *
         * @return 0 on success, error code otherwise
         */
        int setIndicatorState(const bool isLit) {
            return this->ioExpander.setOutput(3, !isLit);
        }

        int setOutputCurrent(const uint32_t current, const bool isInternal);

        int readCurrentAdc(Drivers::I2CDevice::MCP3421 &adc, uint32_t &outCurrent);

    private:
        /// Bus address of the IO expander
        constexpr static const uint8_t kExpanderAddress{0b1000001};
        /**
         * @brief Pin configuration for IO expander
         *
         * The 4 bits of the IO expander are allocated as follows:
         *
         * 0. Unused
         * 1. Set VSense relay
         * 2. Reset VSense relay
         * 3. LED (active low)
         */
        constexpr static const etl::array<Drivers::I2CDevice::PI4IOE5V9536::PinConfig, 4> kExpanderPinConfig{{
            Drivers::I2CDevice::PI4IOE5V9536::kPinConfigUnused,
            // set coil
            {
                .input = false,
                .initialOutput = 0,
            },
            // reset coil
            {
                .input = false,
                .initialOutput = 0,
            },
            // bonus blinkenlights
            {
                .input = false,
                .initialOutput = 1,
            }
        }};

        /// How long to energize the latching relay coils for, in msec
        constexpr static const size_t kRelayPulseWidth{50};
        /// IO expander pin connected to the set coil
        constexpr static const uint8_t kRelaySetPin{1};
        /// IO expander pin connected to the reset coil
        constexpr static const uint8_t kRelayResetPin{2};

        /// Bus address for the VSense ADC
        constexpr static const uint8_t kVSenseAdcAddress{0b1101001};
        /// Set the VSense ADC to 16 bit resolution
        constexpr static const auto kVSenseAdcBits{Drivers::I2CDevice::MCP3421::SampleDepth::High};
        /// Gain factor to convert the VSense voltage to input voltage
        constexpr static const float kVSenseGain{50.f};

        /// Set the current ADC to 12 bit resolution
        constexpr static const auto kCurrentAdcBits{Drivers::I2CDevice::MCP3421::SampleDepth::Low};
        /// Bus address for current sense ADC, channel 1
        constexpr static const uint8_t kCurrentAdc1Address{0b1101010};
        /// Bus address for current drive DAC, channel 1
        constexpr static const uint8_t kCurrentDac1Address{0b1001010};

        /// DAC reference voltage, in µV
        constexpr static const float kDacReference{2'500'000.f};
        /// Resistance of the current sense shunt (in Ω)
        constexpr static const float kSenseResistance{0.05f};

    private:
        /// When set, the relay de-energization timer fired, and they are turned off next IRQ
        bool deenergizeRelays{false};
        /// Whether the load is enabled
        bool isEnabled{false};

        /// Current setpoint (µA)
        uint32_t currentSetpoint{0};

        /// IO expander on the load board, driving the VSense relay and indicator
        Drivers::I2CDevice::PI4IOE5V9536 ioExpander;

        /// ADC sampling input voltage
        Drivers::I2CDevice::MCP3421 voltageAdc;

        /// Current sense ADC for channel 1
        Drivers::I2CDevice::MCP3421 currentAdc1;

        /// Current drive DAC for channel 1
        Drivers::I2CDevice::DAC60501 currentDac1;

        /**
         * @brief Relay de-energization timer
         *
         * This is a timer that is set to expire after ~50ms, then triggers a fake hardware
         * interrupt that will in turn call into our interrupt handler, which then will see the
         * relay flag set and deenergize it.
         */
        TimerHandle_t relayTimer;

        /// Maximum permitted input voltage (mV)
        uint32_t maxVoltage{0};
        /// Maximum input current (mA)
        uint32_t maxCurrent{0};
};
}

#endif
