#ifndef DRIVERS_I2CDEVICE_MCP3421_H
#define DRIVERS_I2CDEVICE_MCP3421_H


#include <stddef.h>
#include <stdint.h>

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief MCP3421 – Up to 18-bit ADC
 *
 * The MCP3421 is a differential single input ADC, with a programmable sample rate between 240 sps
 * and 3.75sps. Additionally, it has a programmable gain amplifier (PGA) which can be set to 1x,
 * 2x, 4x or 8x gain.
 *
 * Output codes are 12 to 18 bits, and are decided by the sample rate:
 *
 * - 12 bits: 240 sps
 * - 14 bits: 60 sps
 * - 16 bits: 15 sps
 * - 18 bits: 3.75 sps
 *
 * Lastly, the driver exposes the ability to place the device into either continuous or one
 * shot conversion mode.
 */
class MCP3421 {
    public:
        /// Errors unique to this device
        enum Errors: int {
            /**
             * @brief Conversion not ready
             *
             * When the device is in one-shot conversion mode, and a conversion result is not yet
             * available to read.
             */
            NotReady                    = -5500,
        };

        /**
         * @brief Output code resolution
         *
         * This defines both the resolution of output codes, and the sample rate with it.
         */
        enum class SampleDepth: uint8_t {
            /// 12 bits (240 sps)
            Low                         = 0b00,
            /// 14 bits (60 sps)
            Medium                      = 0b01,
            /// 16 bits (15 sps)
            High                        = 0b10,
            /// 18 bits (3.75 sps)
            Highest                     = 0b11,
        };

        /**
         * @brief Settings for the PGA
         */
        enum class Gain: uint8_t {
            /// 1x gain
            Unity                       = 0b00,
            /// 2x gain
            x2                          = 0b01,
            /// 4x gain
            x4                          = 0b10,
            /// 8x gain
            x8                          = 0b11,
        };

    public:
        MCP3421(Drivers::I2CBus *bus, const uint8_t address,
                const SampleDepth depth = SampleDepth::Low, const Gain gain = Gain::Unity);
        ~MCP3421();

        /**
         * @brief Set the gain of the ADC input stage
         *
         * Configure the built in PGA
         *
         * @return 0 on success, or a negative error code
         */
        inline int setGain(const Gain newGain) {
            this->gain = newGain;
            return this->updateConfig();
        }

        /**
         * @brief Get the current gain setting
         *
         * @remark This is the last set setting, not read from the device.
         */
        inline const Gain getGain() const {
            return this->gain;
        }

        /**
         * @brief Get the current gain factor
         *
         * @return Gain value (integer factor)
         */
        constexpr inline size_t getGainFactor() const {
            return GainToFactor(this->gain);
        }

        /**
         * @brief Set the bit depth/sampling rate
         *
         * Configure the ADC's sampling rate and conversion resolution.
         *
         * @return 0 on success, or a negative error code
         */
        inline int setSampleDepth(const SampleDepth newDepth) {
            this->depth = newDepth;
            return this->updateConfig();
        }

        int read(int32_t &out);

        /**
         * @brief Read the input voltage at the ADC
         *
         * This performs a read of the raw code, then converts it to the voltage.
         *
         * @param outVoltage Variable to receive the input voltage, in µV
         * @param outCode Raw sample read from ADC
         *
         * @return 0 on success, or negative error code.
         */
        int readVoltage(int &outVoltage, uint16_t &outCode) {
            int err;
            int32_t code;

            // read raw ADC value
            err = this->read(code);
            if(err) {
                return err;
            }

            // convert it
            outVoltage = CodeToVoltage(code, this->depth, this->gain);
            outCode = code;
            return 0;
        }

        /// Read the input voltage at the ADC
        int readVoltage(int &outVoltage) {
            uint16_t temp;
            return this->readVoltage(outVoltage, temp);
        }


        /**
         * @brief Get weight of least significant bit in a given sample depth
         *
         * @return LSB weight, in µV
         */
        static constexpr inline float DepthToLsb(const SampleDepth depth) {
            switch(depth) {
                case SampleDepth::Low:
                    return 1000.f;
                case SampleDepth::Medium:
                    return 250.f;
                case SampleDepth::High:
                    return 62.5f;
                case SampleDepth::Highest:
                    return 15.625f;
            }
        }

        /**
         * @brief Convert gain constant to cain factor
         */
        static constexpr inline int GainToFactor(const Gain gain) {
            switch(gain) {
                case Gain::Unity:
                    return 1;
                case Gain::x2:
                    return 2;
                case Gain::x4:
                    return 4;
                case Gain::x8:
                    return 8;
            }
        }

        /**
         * @brief Return the next lowest gain
         *
         * @remark If the minimum gain is specified, that value is returned.
         */
        static constexpr inline Gain LowerGain(const Gain gain) {
            switch(gain) {
                case Gain::Unity:
                    return Gain::Unity;
                case Gain::x2:
                    return Gain::Unity;
                case Gain::x4:
                    return Gain::x2;
                case Gain::x8:
                    return Gain::x4;
            }
        }

        /**
         * @brief Return the next highest gain
         *
         * @remark If the maximum gain is specified, that value is returned.
         */
        static constexpr inline Gain HigherGain(const Gain gain) {
            switch(gain) {
                case Gain::Unity:
                    return Gain::x2;
                case Gain::x2:
                    return Gain::x4;
                case Gain::x4:
                    return Gain::x8;
                case Gain::x8:
                    return Gain::x8;
            }
        }

        /**
         * @brief Convert a raw ADC reading to a voltage
         *
         * @param code Raw, signed ADC reading
         * @param depth Sample bit depth
         * @param gain Current ADC gain setting
         *
         * @return ADC input voltage, in µV
         */
        static constexpr inline int CodeToVoltage(const int32_t code, const SampleDepth depth,
                const Gain gain) {
            // convert PGA setting to gain factor
            float pga = GainToFactor(gain);
            // get value of the least significant bit (in µV)
            float lsb = DepthToLsb(depth);

            return static_cast<float>(code) * (lsb / pga);
        }

    private:
        int updateConfig();

    private:
        /// Parent bus
        Drivers::I2CBus *bus;
        /// Device address
        uint8_t deviceAddress;

        /// Current sampling rate
        SampleDepth depth{SampleDepth::Low};
        /// PGA setting
        Gain gain{Gain::Unity};
        /// Is one shot conversion mode enabled?
        bool isOneShot{false};
};
}

#endif
