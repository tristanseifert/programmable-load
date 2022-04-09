#ifndef APP_CONTROL_DUMBLOADDRIVER_H
#define APP_CONTROL_DUMBLOADDRIVER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>

#include "LoadDriver.h"

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
        int readInputVoltage(uint32_t &outVoltage) override;
        int setExternalVSense(const bool isExternal) override;

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

    private:
        /// How long to energize the latching relay coils for, in msec
        constexpr static const size_t kRelayPulseWidth{50};

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

        /// IO expander pin connected to the set coil
        constexpr static const uint8_t kRelaySetPin{1};
        /// IO expander pin connected to the reset coil
        constexpr static const uint8_t kRelayResetPin{2};

    private:
        /// When set, the relay de-energization timer fired, and they are turned off next IRQ
        bool deenergizeRelays{false};

        /// IO expander on the load board, driving the VSense relay and indicator
        Drivers::I2CDevice::PI4IOE5V9536 ioExpander;

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
