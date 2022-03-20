#ifndef DRIVERS_TIMERCOUNTER_H
#define DRIVERS_TIMERCOUNTER_H

#include <stddef.h>
#include <stdint.h>

#include <vendor/sam.h>

namespace Drivers {
/**
 * @brief Timer/Counter
 *
 * Implements a 8-bit counter, with two capture/compare channels each. Each TC is capable of also
 * generating waveforms, particularly well suited to PWM.
 *
 * A total of 8 instances of timers exist in the chip.
 *
 * @remark This implementation always operates the TC in 8-bit mode. The 16-bit mode (and 32-bit
 *         mode, where two counters are combined) are not supported. Additionally, input capture
 *         and interrupts aren't implemented.
 */
class TimerCounter {
    public:
        /// Represents a particular timer/counter unit
        enum class Unit: uint8_t {
            Tc0                         = 0,
            Tc1                         = 1,
            Tc2                         = 2,
            Tc3                         = 3,
            Tc4                         = 4,
            Tc5                         = 5,
        };

        /// Waveform generation mode
        enum class WaveformMode: uint8_t {
            /**
             * @brief Normal frequency
             */
            NFRQ                        = 0x0,
            /**
             * @brief Match frequency
             */
            MFRQ                        = 0x1,
            /**
             * @brief Normal PWM
             */
            NPWM                        = 0x2,
            /**
             * @brief Match PWM
             */
            MPWM                        = 0x3,
        };

        /**
         * @brief Configuration for a timer
         *
         * Encapsulates the initial values of configuration for a timer/counter. Some of these
         * values can be changed after the initialization process.
         */
        struct Config {
            /**
             * @brief Direction
             *
             * Whether the counter counts down (1) or up (0).
             */
            uint8_t countDown:1{0};

            /**
             * @brief Stop counter
             *
             * Whether the counter is started on initialization or not
             */
            uint8_t stop:1{1};

            /**
             * @brief Invert waveform 0
             *
             * The output waveform 0 is inverted.
             */
            uint8_t invertWo0:1{0};
            /**
             * @brief Invert waveform 1
             *
             * The output waveform 1 is inverted.
             */
            uint8_t invertWo1:1{0};

            /**
             * @brief Waveform generation mode
             *
             * Defines the way that waveforms are generated on the two WO pads.
             */
            WaveformMode wavegen{WaveformMode::NFRQ};

            /**
             * @brief Frequency for timer
             *
             * This value is used, in combination with the input frequency for the timer, to
             * calculate the best combination of prescaler and period value to be written into the
             * configuration to achieve this frequency in PWM mode.
             */
            uint32_t frequency{0};

            /**
             * @brief Compare values for each output channel
             *
             * In PWM mode, this is used to set the duty cycle.
             */
            uint8_t compare[2]{0,0};
        };

    public:
        TimerCounter(const Unit unit, const Config &conf);
        ~TimerCounter();

        void reset();
        bool enable();
        bool disable();

        void setFrequency(const uint32_t freq);
        void setDutyCycle(const uint8_t line, const uint8_t duty);

    private:
        void applyConfiguration(const Config &);

        /**
         * @brief Get register base for unit
         *
         * @param unit Timer/counter unit ([0, 7])
         *
         * @return Peripheral register base address
         */
        constexpr static inline ::Tc *MmioFor(const Unit unit) {
            switch(unit) {
                case Unit::Tc0:
                    return TC0;
                case Unit::Tc1:
                    return TC1;
                case Unit::Tc2:
                    return TC2;
                case Unit::Tc3:
                    return TC3;
                case Unit::Tc4:
                    return TC4;
                case Unit::Tc5:
                    return TC5;
            }
        }

        static bool CalculateFrequency(const Unit unit, const uint32_t freq, uint16_t &outPrescaler,
                uint8_t &outPeriod);

    private:
        /**
         * @brief Total number of timer/counter instances
         *
         * Up to 8 instances are supported in this family, but the chip we target has only 6.
         */
        constexpr static const size_t kNumInstances{6};

        /**
         * @brief Enable timeout
         *
         * Number of cycles to wait for the enable bit to synchronize.
         *
         * @remark This should be large enough so that it will never expire, even with the slowest
         *         reference clock for any timer in the system.
         */
        constexpr static const size_t kEnableSyncTimeout{1000};

        /**
         * @brief Reset timeout
         *
         * Number of cycles to wait for the reset bit to synchronize.
         *
         * @remark This should be large enough so that it will never expire, even with the slowest
         *         reference clock for any timer in the system.
         */
        constexpr static const size_t kResetSyncTimeout{kEnableSyncTimeout};

        /**
         * @brief Timer input clocks
         *
         * Map each of the timers' units to their corresponding input clock frequency. These values
         * are based on what is configured in Atmel START, and secreted into the
         * `peripheral_clk_config.h` file.
         */
        static const uint32_t kTimerClocks[kNumInstances];

        /**
         * @brief Bitmask of enabled counters
         *
         * Each timer/counter, when enabled, sets the corresponding bit here. This is used so that
         * we can abort if trying to initialize the same counter twice.
         */
        static uint8_t gInitialized;

    private:
        /// Unit this object corresponds to
        Unit unit;
        /// Is the counter enabled?
        bool enabled{false};
        /// Current period value (shadow of actual value)
        uint8_t period{0};

        /// Counter base registers
        ::Tc *regs;
};
}

#endif
