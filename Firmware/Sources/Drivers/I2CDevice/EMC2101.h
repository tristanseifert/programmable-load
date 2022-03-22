#ifndef DRIVERS_I2CDEVICE_EMC2101_H
#define DRIVERS_I2CDEVICE_EMC2101_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief EMC2101 Fan controller + temperature sensor
 *
 * Provides either a PWM or DAC for controlling a fan speed, as well as an internal (and external
 * channel) for a temperature sensor. The chip is capable of controlling the fan speed completely
 * autonomously, by means of a fan control lookup table.
 *
 * @remark Automatic operation samples the external temperature sensor only. It is not possible to
 *         use this mode using the internal sensor.
 */
class EMC2101 {
    public:
        /**
         * @brief Error codes emitted by the device
         */
        enum Errors: int {
            /**
             * @brief Specified fan control map is too small or invalid
             */
            InvalidMap                  = -5100,
            /**
             * @brief Controller is in invalid mode for this call
             *
             * This can happen when trying to set the fan speed in automatic control, or program
             * the lookup table while automatic control is enabled.
             */
            InvalidMode                 = -5101,
        };

        /**
         * @brief Controller configuration
         *
         * Defines how the controller should be configured when initializing. These parameters are
         * meant to stay constant once the device has initialized.
         */
        struct Config {
            /**
             * @brief Fan DAC mode
             *
             * Set to enable an analog voltage at the FAN control output, which is proportional to
             * the desired fan speed, rather than a PWM signal.
             */
            uint8_t analogFan:1{0};

            /**
             * @brief PWM output polarity
             *
             * This bit corresponds to the value of the PWM waveform during its on period; if set,
             * it is inverted.
             */
            uint8_t pwmPolarity:1{0};

            /**
             * @brief Enable tachometer input
             *
             * Set to enable the tachometer input. Clear to use it as an open-drain interrupt
             * output instead.
             */
            uint8_t tach:1{1};

            /**
             * @brief Automatic fan control hysteresis
             *
             * Amount of hysteresis to apply to temperatures when in automatic temperature control
             * mode, in °C.
             */
            uint8_t autoHysteresis:5{4};

            /**
             * @brief Minimum fan speed
             *
             * Minimum speed the fan is expected to rotate at, when set to its lowest speed. This
             * is specified in RPM. A value of 0 disables this detection.
             */
            uint16_t minRpm{0};
        };

        /**
         * @brief A single entry in a fan control map
         *
         * Defines a pair of temperature and fan speed for use in automatic fan control.
         */
        struct FanMapEntry {
            /**
             * @brief Temperature
             *
             * The temperature value at which this map entry is to be applied, in °C.
             */
            int8_t temp;

            /**
             * @brief Desired fan speed
             *
             * How fast the fan should run at or above this temperature. A value of 0 indicates the
             * fan is off, 0xFF indicates full speed.
             */
            uint8_t speed;
        };

    public:
        EMC2101(Drivers::I2CBus *bus, const Config &conf, const uint8_t address = 0b100'1100);

        int getInternalTemp(float &outInternalTemp);
        int getExternalTemp(float &outExternalTemp);

        int getFanSpeed(unsigned int &outRpm);

        int setFanMap(etl::span<const FanMapEntry> temps);
        int setFanMode(const bool automatic);
        int setFanSpeed(const uint8_t speed);

    private:
        /**
         * @brief Registers indices on the device
         *
         * Some registers have multiple addresses specified in the datasheet for backwards
         * compatibility; in this case, we pick the first of the provided addresses.
         */
        enum class Regs: uint8_t {
            /**
             * @brief Internal temperature
             *
             * Temperature from the internal temperature sensor, in °C. It's a signed 8 bit value.
             */
            InternalTemp                = 0x00,
            /**
             * @brief External temperature, high byte
             *
             * High byte of the external temperature sensor value, in decimal °C.
             */
            ExternalTempHigh            = 0x01,

            /**
             * @brief Device status
             *
             * Indicates the current state of the device, including various fault conditions.
             */
            Status                      = 0x02,

            /**
             * @brief Configuration register
             *
             * Control the basic functionality of the device, including the type of fan control.
             */
            Control                     = 0x03,
            /**
             * @brief Number of conversions per second
             *
             * The low 4 bits encode a value between 1/16 and 32.
             */
            ConversionRate              = 0x04,

            /**
             * @brief External temperature force
             *
             * Write a temperature value to this register (signed 8-bit °C) to force the automatic
             * fan control to proceed as if this were the measured temperature.
             *
             * @remark This temperature is only taken into account if the fan control register's
             *         external temperature override bit is set.
             */
            ExternalTempForce           = 0x0C,

            /**
             * @brief External temperature, low byte
             *
             * Left-aligned within this byte are the three fractional bits of the externally
             * measured temperature.
             *
             * @remark The low byte is automatically latched when the high byte is read, as to
             *         guarantee the two halves will always match.
             */
            ExternalTempLow             = 0x10,

            /**
             * @brief Tachometer counts, low byte
             *
             * Low byte of the current tachometer count. Convert to RPM by dividing 5,400,000 by
             * the 16-bit count.
             */
            TachCountLow                = 0x46,
            /**
             * @brief Tachometer count, high byte
             */
            TachCountHigh               = 0x47,

            /**
             * @brief Tachometer limit, low byte
             *
             * Low byte of the tachometer limit, which is the maximum TACH count (thus, minimum
             * rotational speed) the fan is expected to operate at.
             */
            TachLimitLow                = 0x48,
            /**
             * @brief Tachometer limit, high byte
             */
            TachLimitHigh               = 0x49,

            /**
             * @brief Fan configuration
             */
            FanConfig                   = 0x4A,
            /**
             * @brief Fan spinup configuration
             */
            FanSpinup                   = 0x4B,
            /**
             * @brief Fan setting
             *
             * Read/write register indicating the current fan speed, where 0 is off, and 0x3f is
             * full speed. If automatic fan control (using the fan table) is enabled, this register
             * is read-only.
             */
            FanSetting                  = 0x4C,
            /**
             * @brief PWM frequency
             *
             * The low 5 bits indicate the final PWM frequency and effective resolution. The
             * datasheet suggests setting it to $1F always for maximum resolution.
             */
            PwmFrequency                = 0x4D,
            /**
             * @brief PWM frequency divider
             *
             * All 8 bits are used to configure an alternate PWM frequency divider.
             */
            PwmFreqDivide               = 0x4E,

            /**
             * @brief Lookup table hysteresis
             *
             * Determine the amount of hysteresis applied to conescutive inputs into the fan
             * control table.
             */
            TableHysteresis             = 0x4F,
            /**
             * @brief Lookup table, temperature value
             *
             * Index of the first temperature value in the fan control lookup table. The stride
             * between consecutive temperature registers is 2.
             *
             * Values are specified in °C, with the leading bit being forced to 0.
             */
            TableTemp1                  = 0x50,
            /**
             * @brief Lookup table, speed value
             *
             * The fan speed to use when the measured temperature is at or above. A value of 0
             * means the fan is stopped, 0x3F means full speed.
             *
             * The stride between table entries is 2.
             */
            TableSpeed1                 = 0x51,

            /**
             * @brief Averaging filter configuration
             *
             * Define the configuration of the digital averaging filter, used for the external
             * temperature measurement.
             */
            AvgFilter                   = 0xBF,

            /**
             * @brief Product ID
             *
             * A read-only register that should read back as $16 (EMC2101) or $28 (EMC2101-R)
             */
            ProductId                   = 0xFD,
            /**
             * @brief Manufacturer ID
             *
             * A read-only register identifying the manufacturer as SMSC ($5D)
             */
            ManufacturerId              = 0xFE,
            /**
             * @brief Chip revision
             *
             * A read-only register containing the die revision
             */
            Revision                    = 0xFF,
        };

        void applyConfig(const Config &conf);

        int writeRegister(const Regs reg, const uint8_t value);
        int readRegister(const Regs reg, uint8_t &outValue);

    private:
        /**
         * @brief Is the fan control table used?
         *
         * When set, the fan controller is using automatic fan control. Any attempts to manually
         * alter the fan speed will be ignored.
         */
        uint8_t useFanTable:1{0};

        /**
         * @brief Invert PWM polarity
         *
         * Whether the output polarity of the PWM control signal is inverted.
         */
        uint8_t invertPwm:1{0};

        /**
         * @brief Bus address
         *
         * The I²C address of the fan controller.
         */
        uint8_t address;

        /**
         * @brief Bus
         *
         * The I²C bus to which the controller is connected.
         */
        Drivers::I2CBus *bus;
};
}

#endif
