#include "Watchdog.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>

using namespace Drivers;

TaskHandle_t Watchdog::gEarlyWarningTask{nullptr};
size_t Watchdog::gEarlyWarningNoteIndex{0};
uintptr_t Watchdog::gEarlyWarningNoteBits{0};

/**
 * @brief Configure watchdog
 *
 * Sets up the watchdog with the specified configuration, but does not enable it yet.
 *
 * @param conf Watchdog configuration to apply
 */
void Watchdog::Configure(const Config &conf) {
    // one time initialization
    Init();

    // configure as window mode
    if(conf.windowMode) {
        WDT->CTRLA.bit.WEN = true;
        while(WDT->SYNCBUSY.bit.WEN) {}

        // prevent from specifying window open > timeout
        REQUIRE(conf.timeout > conf.secondary, "invalid window period: open %u timeout %u",
                conf.secondary, conf.timeout);

        WDT->CONFIG.reg = WDT_CONFIG_PER(static_cast<uint8_t>(conf.timeout) & 0b1111) |
                          WDT_CONFIG_WINDOW(static_cast<uint8_t>(conf.secondary) & 0b1111);
    }
    // otherwise, in normal mode
    else {
        WDT->CTRLA.bit.WEN = false;
        while(WDT->SYNCBUSY.bit.WEN) {}

        WDT->CONFIG.reg = WDT_CONFIG_PER(static_cast<uint8_t>(conf.timeout) & 0b1111);
        WDT->EWCTRL.reg = WDT_EWCTRL_EWOFFSET(static_cast<uint8_t>(conf.secondary) & 0b1111);
    }

    // set up early warning irq
    if(conf.earlyWarningIrq) {
        gEarlyWarningTask = conf.notifyTask;
        gEarlyWarningNoteIndex = conf.notifyIndex;
        gEarlyWarningNoteBits = conf.notifyBits;

        WDT->INTENSET.reg = WDT_INTENSET_EW;
        NVIC_EnableIRQ(WDT_IRQn);
    } else {
        WDT->INTENCLR.reg = WDT_INTENCLR_EW;

        NVIC_DisableIRQ(WDT_IRQn);
    }
}

/**
 * @brief Initialize watchdog
 *
 * This enables the interrupts and clocks required by the watchdog.
 */
void Watchdog::Init() {
    MCLK->APBAMASK.bit.WDT_ = true;

    NVIC_SetPriority(WDT_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
}

/**
 * @brief Enable the watchdog
 *
 * It must have been previously configured, or the device will likely reset immediately.
 */
void Watchdog::Enable() {
    WDT->CTRLA.bit.ENABLE = true;
    while(WDT->SYNCBUSY.bit.ENABLE) {}
}

/**
 * @brief Disable the watchdog
 */
void Watchdog::Disable() {
    WDT->CTRLA.bit.ENABLE = false;
    while(WDT->SYNCBUSY.bit.ENABLE) {}
}


/**
 * @brief Watchdog interrupt handler
 *
 * This is the early warning interrupt from the watchdog.
 */
extern "C" void WDT_Handler() {
    BaseType_t woken{pdFALSE};

    if(Watchdog::gEarlyWarningTask) {
        xTaskNotifyIndexedFromISR(Watchdog::gEarlyWarningTask, Watchdog::gEarlyWarningNoteIndex,
                        static_cast<BaseType_t>(Watchdog::gEarlyWarningNoteBits), eSetBits, &woken);
        portYIELD_FROM_ISR(woken);
    }

    WDT->INTFLAG.reg = WDT_INTFLAG_EW;
}
