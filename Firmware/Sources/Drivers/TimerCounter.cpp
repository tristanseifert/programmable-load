#include "TimerCounter.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>
#include <vendor/config/peripheral_clk_config.h>

using namespace Drivers;

static uint32_t ConvertPrescaler(const uint16_t prescaler);

uint8_t TimerCounter::gInitialized{0};

const uint32_t TimerCounter::kTimerClocks[kNumInstances]{
#ifdef CONF_GCLK_TC0_FREQUENCY
    CONF_GCLK_TC0_FREQUENCY,
#else
    0,
#endif

#ifdef CONF_GCLK_TC1_FREQUENCY
    CONF_GCLK_TC1_FREQUENCY,
#else
    0,
#endif

#ifdef CONF_GCLK_TC2_FREQUENCY
    CONF_GCLK_TC2_FREQUENCY,
#else
    0,
#endif

#ifdef CONF_GCLK_TC3_FREQUENCY
    CONF_GCLK_TC3_FREQUENCY,
#else
    0,
#endif

#ifdef CONF_GCLK_TC4_FREQUENCY
    CONF_GCLK_TC4_FREQUENCY,
#else
    0,
#endif

#ifdef CONF_GCLK_TC5_FREQUENCY
    CONF_GCLK_TC5_FREQUENCY,
#else
    0,
#endif
};


/**
 * @brief Initialize the timer/counter
 *
 * @param unit Peripheral instance to initialize
 * @param conf Configuration to apply to the timer
 */
TimerCounter::TimerCounter(const Unit unit, const Config &conf) : unit(unit),
    regs(MmioFor(unit)) {
    // reset its registers to default state
    REQUIRE(!(gInitialized & (1U << static_cast<uint8_t>(unit))), "cannot re-initialize TC%u",
            static_cast<unsigned int>(unit));
    this->reset();

    // apply configuration (includes frequency)
    this->applyConfiguration(conf);

    // mark as allocated and enable
    gInitialized |= ~(1U << static_cast<uint8_t>(this->unit));
    this->enable();
}

/**
 * @brief Deinitialize the timer/counter
 *
 * Its outputs are disabled, and the counter is subsequently reset.
 */
TimerCounter::~TimerCounter() {
    this->reset();
    gInitialized &= ~(1U << static_cast<uint8_t>(this->unit));
}

/**
 * @brief Reset the timer/counter
 */
void TimerCounter::reset() {
    taskENTER_CRITICAL();

    this->regs->COUNT8.CTRLA.reg = TC_CTRLA_SWRST;
    size_t timeout{kResetSyncTimeout};
    do {
        REQUIRE(--timeout, "TC%d %s timed out", static_cast<unsigned int>(this->unit), "reset");
    } while(!!this->regs->COUNT8.SYNCBUSY.bit.SWRST);

    this->enabled = false;
    taskEXIT_CRITICAL();
}

/**
 * @brief Enable the timer/counter
 *
 * @return The enable status of the timer/counter before this call
 */
bool TimerCounter::enable() {
    if(this->enabled) return true;

    taskENTER_CRITICAL();

    this->regs->COUNT8.CTRLA.reg |= TC_CTRLA_ENABLE;
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "TC%d %s timed out", static_cast<unsigned int>(this->unit), "enable");
    } while(!!this->regs->COUNT8.SYNCBUSY.bit.ENABLE);

    this->enabled = true;
    taskEXIT_CRITICAL();

    return false;
}

/**
 * @brief Disable the timer/counter
 *
 * @return The enable status of the timer/counter before this call
 */
bool TimerCounter::disable() {
    if(!this->enabled) return false;

    taskENTER_CRITICAL();

    this->regs->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "TC%d %s timed out", static_cast<unsigned int>(this->unit), "disable");
    } while(!!this->regs->COUNT8.SYNCBUSY.bit.ENABLE);

    this->enabled = false;
    taskEXIT_CRITICAL();

    return true;
}

/**
 * @brief Update the output frequency
 *
 * Updates the frequency/period of the output signal of the timer.
 */
void TimerCounter::setFrequency(const uint32_t freq) {
    // calculate the required period and prescaler
    uint8_t newPeriod{0};
    uint16_t prescaler{0};

    const auto possible = CalculateFrequency(this->unit, freq, prescaler, newPeriod);
    REQUIRE(possible, "TC%u: cannot attain frequency %u Hz", static_cast<unsigned int>(this->unit),
            freq);

    // update them
    taskENTER_CRITICAL();
    const auto enable = this->disable();

    uint32_t ctrla = this->regs->COUNT8.CTRLA.reg;
    ctrla |= ConvertPrescaler(prescaler);
    this->regs->COUNT8.CTRLA.reg = ctrla;

    this->regs->COUNT8.PER.reg = newPeriod;
    this->period = newPeriod;

    if(enable) {
        this->enable();
    }
    taskEXIT_CRITICAL();
}

/**
 * @brief Update duty cycle
 *
 * @param line Which of the two lines ([0,1]) to change
 * @param duty Duty cycle, where 0xFF is 100%, and 0x00 is 0%.
 */
void TimerCounter::setDutyCycle(const uint8_t line, const uint8_t duty) {
    REQUIRE(line <= 1, "TC%d: invalid line %u", static_cast<unsigned int>(this->unit), line);

    // XXX: this is not right, it should be proportional to the value in period
    this->regs->COUNT8.CC[line].reg = duty;
}

/**
 * @brief Apply timer/counter configuration
 *
 * Sets up the channel in 8-bit mode, and calculates the appropriate prescaler and period for the
 * desired timer frequency.
 */
void TimerCounter::applyConfiguration(const Config &conf) {
    uint8_t newPeriod{0};
    uint16_t prescaler{0};

    /*
     * Try all available prescalers and find which has the smallest error from the requested
     * frequency and period value. This frequency will match the PWM output frequency in NPWM
     * mode.
     */
    const auto possible = CalculateFrequency(unit, conf.frequency, prescaler, newPeriod);
    REQUIRE(possible, "TC%u: cannot attain frequency %u Hz", static_cast<unsigned int>(unit),
            conf.frequency);

    /*
     * CTRLA: Control A
     *
     * Fixed: 8 bit mode; prescaler as calculated above
     */
    uint32_t ctrla{TC_CTRLA_MODE_COUNT8};

    ctrla |= ConvertPrescaler(prescaler);

    this->regs->COUNT8.CTRLA.reg = ctrla;

    /*
     * Waveform generation and inversion
     */
    this->regs->COUNT8.WAVE.reg = TC_WAVE_WAVEGEN(static_cast<uint8_t>(conf.wavegen) & 0x3);

    uint8_t drv{0};
    if(conf.invertWo0) {
        drv |= TC_DRVCTRL_INVEN0;
    }
    if(conf.invertWo1) {
        drv |= TC_DRVCTRL_INVEN1;
    }

    this->regs->COUNT8.DRVCTRL.reg = drv;

    /*
     * Load the counter and period
     */
    this->regs->COUNT8.PER.reg = newPeriod;
    this->period = newPeriod;

    this->regs->COUNT8.CC[0].reg = conf.compare[0];
    this->regs->COUNT8.CC[1].reg = conf.compare[1];
}




/**
 * @brief Calculates the closest period and prescaler value for a given frequency
 *
 * @param unit Timer/counter unit, used to look up its input clock
 * @param freq Desired frequency
 * @param outPrescaler On success, a variable to receive the integer value of the prescaler (which
 *        may be 1, 2, 4, 8, 16, 64, 256, or 1024)
 * @param outPeriod On success, the period value for this frequency
 *
 * @return Whether the frequency is attainable at all
 *
 * @remark The output variables may be written to multiple times for a single invocation, so they
 *         should not be addresses that have side effects.
 */
bool TimerCounter::CalculateFrequency(const Unit unit, const uint32_t freq, uint16_t &outPrescaler,
                uint8_t &outPeriod) {
    bool found{false};

    REQUIRE(freq, "invalid frequency %u Hz", freq);

    // get the input clock
    const uint32_t inFreq = kTimerClocks[static_cast<size_t>(unit)];
    REQUIRE(inFreq, "don't know TC%u input clock", static_cast<unsigned int>(unit));
    Logger::Trace("TC%u: desired freq %u Hz, input %u Hz", static_cast<unsigned int>(unit),
            freq, inFreq);

    constexpr static const size_t kNumPrescalers{8};
    constexpr static const uint16_t kPrescalers[kNumPrescalers]{
        1, 2, 4, 8, 16, 64, 256, 1024
    };

    /*
     * Try all prescalers, and find whichever one has the least error. We successively write out
     * the values to the output values, keeping track of the error.
     */
    uint32_t error{0};

    for(size_t i = 0; i < kNumPrescalers; i++) {
        const uint32_t prescaler = kPrescalers[i];

        // what would be the required period value?
        const uint32_t requiredPer = (inFreq / (freq * prescaler)) - 1;

        // if it's less than 8 bits, calculate the error
        if(requiredPer > 0xff) {
            // period value would be larer than max possible
            continue;
        }

        const uint32_t actual = inFreq / (prescaler * (requiredPer + 1));

        int32_t diff = actual - freq;
        if(diff < 0) {
            // make it always positive
            diff *= -1;
        }

        // if error is less than last error, write it out
        if(!found || diff < error) {
            error = diff;

            outPeriod = static_cast<uint8_t>(requiredPer);
            outPrescaler = prescaler;
            found = true;
        }
    }

    // output the chosen frequency
    if(found) {
        Logger::Debug("TC%u: freq %u Hz: %u Hz / %u, period %u = %u Hz",
                static_cast<unsigned int>(unit), freq, inFreq, outPrescaler, outPeriod,
                (inFreq / (outPrescaler * (outPeriod + 1))));
    }

    return found;
}



/**
 * @brief Convert integral prescaler value to bit field value
 *
 * Converts the integer representation of a prescaler to the appropriate value to go in the
 * CTRLA.PRESCALER field.
 */
static uint32_t ConvertPrescaler(const uint16_t prescaler) {
    switch(prescaler) {
        case 1:
            return TC_CTRLA_PRESCALER_DIV1;
        case 2:
            return TC_CTRLA_PRESCALER_DIV2;
        case 4:
            return TC_CTRLA_PRESCALER_DIV4;
        case 8:
            return TC_CTRLA_PRESCALER_DIV8;
        case 16:
            return TC_CTRLA_PRESCALER_DIV16;
        case 64:
            return TC_CTRLA_PRESCALER_DIV64;
        case 256:
            return TC_CTRLA_PRESCALER_DIV256;
        case 1024:
            return TC_CTRLA_PRESCALER_DIV1024;
        default:
            Logger::Panic("invalid prescaler %u", prescaler);
    }
}
