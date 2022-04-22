#ifndef DRIVERS_I2CDEVICE_DAC60501_H
#define DRIVERS_I2CDEVICE_DAC60501_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief DAC60501 – 12-bit DAC
 *
 * @remark This could probably support also the DAC80501 and DAC70501, as they have the same
 *         register sets but output 16-bit and 14-bit codes respectively.
 */
class DAC60501 {
    /// Maximum DAC code (12 bits)
    constexpr static const uint16_t kMaxCode{0xfff};

    public:
        /**
         * @brief Specify the output gain
         *
         * This is the gain applied to the output code, which is performed with a combination of
         * doubling the output gain, and dividing the input reference voltage.
         */
        enum class Gain: uint8_t {
            Half, Unity, Double
        };

    public:
        DAC60501(Drivers::I2CBus *bus, const uint8_t address, const Gain gain = Gain::Unity);

        /**
         * @brief Reset the DAC
         *
         * Perform a soft reset.
         */
        inline int reset() {
            auto err =  this->writeRegister(Reg::Trigger, 0b1010);
            if(!err) {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            return err;
        }

        /**
         * @brief Set DAC output (raw code)
         *
         * Sets the raw DAC output code value
         *
         * @param code Raw DAC code to set
         *
         * @return 0 on success, or negative error code
         */
        int operator() (const uint16_t code) {
            return this->writeRegister(Reg::OutputCode, (code & 0xfff) << 4);
        }
        /**
         * @brief Set DAC output
         *
         * Sets the DAC's output code as a percentage of its full scale value.
         *
         * @param percent Percentage of full scale value to output
         */
        int operator() (const float percent) {
            return (*this)(static_cast<uint16_t>(static_cast<float>(kMaxCode) * percent));
        }

        int setGain(const Gain gain);

        /**
         * @brief Get the current gain setting
         */
        constexpr inline auto getGain() const {
            return this->gain;
        }

        /**
         * @brief Read status register
         *
         * Get the DAC's status register
         */
        inline int getStatus(uint16_t &outStatus) {
            return this->readRegister(Reg::Status, outStatus);
        }

    private:
        /// Internal register names
        enum class Reg: uint8_t {
            NoOp                        = 0,
            DeviceId                    = 1,
            Sync                        = 2,
            Config                      = 3,
            Gain                        = 4,
            Trigger                     = 5,
            Status                      = 7,
            OutputCode                  = 8,
        };

        int writeRegister(const Reg r, const uint16_t value);
        int readRegister(const Reg r, uint16_t &outValue);

    private:
        /// Parent bus
        Drivers::I2CBus *bus;
        /// Device address
        uint8_t deviceAddress;

        /// current gain mode
        Gain gain;
};
}

#endif
