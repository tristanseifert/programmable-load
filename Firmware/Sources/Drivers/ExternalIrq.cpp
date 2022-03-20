#include "ExternalIrq.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

using namespace Drivers;

bool ExternalIrq::gEnabled{false};
uint16_t ExternalIrq::gLinesEnabled{0};

/**
 * @brief Initialize the external interrupt controller
 *
 * All registers are reset to their default values, clocking is configured, and then  the
 * controller is enabled.
 */
void ExternalIrq::Init() {
    // reset to defaults
    ExternalIrq::Reset();

    /*
     * CTRLA: Control A
     *
     * Use external clock
     */
    EIC->CTRLA.reg = 0;

    /**
     * Debounce prescaler
     *
     * Use external low frequency (32.768kHz) clock, divide by 64, require 7 samples (max signal
     * frequency ~146Hz)
     */
    EIC->DPRESCALER.reg = EIC_DPRESCALER_TICKON |
        EIC_DPRESCALER_STATES1 | EIC_DPRESCALER_PRESCALER1(0x5) |
        EIC_DPRESCALER_STATES0 | EIC_DPRESCALER_PRESCALER0(0x5);

    // enable the controller
    Enable();
}

/**
 * @brief Apply an interrupt line's configuration
 *
 * @param line Input line to configure ([0, 15])
 * @param conf Configuration structure
 *
 * @remark All external interrupts are disabled while this configuration change takes place, as the
 *         EIC needs to be disabled.
 *
 * @remark This does _not_ configure the NVIC; you need to manually configure the IRQn's priority
 *         and enable it there to actually receive an interrupt.
 */
void ExternalIrq::ConfigureLine(const uint8_t line, const Config &conf) {
    REQUIRE(line <= 15, "invalid EIC line %u", line);

    taskENTER_CRITICAL();

    // disable if needed
    if(gEnabled) {
        Disable();
    }

    // configure the irq and event flags
    const uint32_t bit{(1UL << static_cast<uint32_t>(line))};

    if(conf.irq) {
        EIC->INTENSET.reg = bit;
    } else {
        EIC->INTENCLR.reg = bit;
    }

    if(conf.event) {
        EIC->EVCTRL.reg |= bit;
    } else {
        EIC->EVCTRL.reg &= (~bit) & EIC_EVCTRL_MASK;
    }

    // configure debounce
    if(conf.debounce) {
        EIC->DEBOUNCEN.reg |= bit;
    } else {
        EIC->DEBOUNCEN.reg &= (~bit) & EIC_DEBOUNCEN_MASK;
    }

    // build up the 4 bits for the sense mode and filter
    uint8_t sense{0};

    if(conf.filter) {
        sense |= EIC_CONFIG_FILTEN0;
    }

    sense |= EIC_CONFIG_SENSE0(static_cast<uint8_t>(conf.mode) & 0x7);

    // shift to the appropriate position and write it in
    uint32_t temp;
    const uint32_t lineShift{static_cast<uint32_t>((line & 0x7) * 4)};
    const uint32_t config{static_cast<uint32_t>(sense & 0xf) << lineShift};

    temp = EIC->CONFIG[sense / 8].reg;
    temp &= ~(0xf << lineShift);
    temp |= config;

    Logger::Trace("EIC CONFIG[%u] = $%08x", sense / 8, temp);

    EIC->CONFIG[sense / 8].reg = temp;

    // re-enable the peripheral
    Enable();

    taskEXIT_CRITICAL();
}

/**
 * @brief Reset the EIC and all registers to default values.
 */
void ExternalIrq::Reset() {
    taskENTER_CRITICAL();

    // assert reset
    EIC->CTRLA.reg = EIC_CTRLA_SWRST;

    // wait for sync bit to clear
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "EIC %s timed out", "reset sync");
    } while(!!EIC->SYNCBUSY.bit.SWRST);

    // wait for reset bit to clear
    timeout = kEnableSyncTimeout;
    do {
        REQUIRE(--timeout, "EIC %s timed out", "reset");
    } while(!!EIC->CTRLA.bit.SWRST);

    gEnabled = false;
    gLinesEnabled = 0;
    taskEXIT_CRITICAL();
}

/**
 * @brief Enable the controller
 *
 * This waits up to kEnableSyncTimeout loops before giving up.
 */
void ExternalIrq::Enable() {
    REQUIRE(!gEnabled, "SPI already enabled");

    // set the bit
    taskENTER_CRITICAL();

    EIC->CTRLA.reg |= EIC_CTRLA_ENABLE;
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "EIC %s timed out", "enable");
    } while(!!EIC->SYNCBUSY.bit.ENABLE);

    gEnabled = true;
    taskEXIT_CRITICAL();
}

/**
 * @brief Disable the controller
 */
void ExternalIrq::Disable() {
    REQUIRE(gEnabled, "SPI already disabled");

    // clear the bit
    taskENTER_CRITICAL();

    EIC->CTRLA.reg &= ~EIC_CTRLA_ENABLE;
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "EIC %s timed out", "disable");
    } while(!!EIC->SYNCBUSY.bit.ENABLE);

    gEnabled = false;
    taskEXIT_CRITICAL();
}
