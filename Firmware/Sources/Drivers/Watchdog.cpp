#include "stm32mp1xx.h"
#include "stm32mp1xx_hal_rcc.h"

#include "Hw/StatusLed.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "Common.h"
#include "Watchdog.h"

using namespace Drivers;

TaskHandle_t Watchdog::gEarlyWarningTask{nullptr};
size_t Watchdog::gEarlyWarningNoteIndex{0};
uintptr_t Watchdog::gEarlyWarningNoteBits{0};
uint8_t Watchdog::gCounterValue{0};
uint8_t Watchdog::gWindowValue{0};

/**
 * @brief Configure watchdog
 *
 * Sets up the watchdog with the specified configuration, but does not enable it yet.
 *
 * @param conf Watchdog configuration to apply
 */
void Watchdog::Configure(const Config &conf) {
    // validate inputs
    REQUIRE(conf.windowValue > 0x41 && conf.windowValue <= 0x7f, "invalid window value %u",
            conf.windowValue);
    REQUIRE(conf.counter > 0x40 && conf.counter <= 0x7f, "invalid counter %u", conf.counter);
    REQUIRE(conf.windowValue <= conf.counter, "invalid window period: open %u total count %u",
            conf.windowValue, conf.counter);

    // enable clock for the watchdog
    __HAL_RCC_WWDG1_CLK_ENABLE();

    // configure interrupts
    NVIC_SetPriority(WWDG1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);

    // get watchdog timer base frequency (APB1 / 4096)
    const auto wdgFreq = GetApbClock(2) / 4096;
    const float countFreq = wdgFreq / static_cast<float>(1 << static_cast<uint32_t>(conf.divider));

    Logger::Notice("WWDG clk %u Hz / %u = %u Hz", wdgFreq, 1 << static_cast<uint32_t>(conf.divider),
            static_cast<uint32_t>(countFreq));

    Logger::Notice("WWDG timeout %d msec, window at %d msec",
            static_cast<int>(((1.f / countFreq) * (conf.counter - 0x3f)) * 1000.f),
            static_cast<int>(((1.f / countFreq) * (0x7f - conf.windowValue)) * 1000.f)
            );

    // configuration register
    WWDG1->CFR = (static_cast<uint8_t>(conf.divider) << WWDG_CFR_WDGTB_Pos) |
        (conf.earlyWarningIrq ? WWDG_CFR_EWI : 0) |
        (conf.windowValue ? conf.windowValue : 0x7f) << WWDG_CFR_W_Pos;

    // control register
    gCounterValue = (conf.counter & 0x7f);
    gWindowValue = (conf.windowValue & 0x7f) - 1;

    while(((WWDG1->CR >> WWDG_CR_T_Pos) & 0x7f) >= gWindowValue) {}
    WWDG1->CR = static_cast<uint32_t>(gCounterValue) << WWDG_CR_T_Pos;

    // set up early warning irq
    if(conf.earlyWarningIrq) {
        gEarlyWarningTask = conf.notifyTask;
        gEarlyWarningNoteIndex = conf.notifyIndex;
        gEarlyWarningNoteBits = conf.notifyBits;

        NVIC_EnableIRQ(WWDG1_IRQn);
    } else {
        NVIC_DisableIRQ(WWDG1_IRQn);
    }
}

/**
 * @brief Enable the watchdog
 *
 * It must have been previously configured, or the device will likely reset immediately.
 */
void Watchdog::Enable() {
    WWDG1->CR = WWDG_CR_WDGA | (static_cast<uint32_t>(gCounterValue) << WWDG_CR_T_Pos);
}


/**
 * @brief Watchdog interrupt handler
 *
 * This is the early warning interrupt from the watchdog.
 */
extern "C" void WWDG1_IRQHandler() {
    BaseType_t woken{pdFALSE};

    // notify the task
    if(Watchdog::gEarlyWarningTask) {
        xTaskNotifyIndexedFromISR(Watchdog::gEarlyWarningTask, Watchdog::gEarlyWarningNoteIndex,
                        static_cast<BaseType_t>(Watchdog::gEarlyWarningNoteBits), eSetBits, &woken);
        portYIELD_FROM_ISR(woken);
    }

    // acknowledge interrupt
    WWDG1->SR = 0;
}
