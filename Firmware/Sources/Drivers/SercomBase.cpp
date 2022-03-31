#include "SercomBase.h"
#include "ClockMgmt.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>
#include <vendor/config/peripheral_clk_config.h>

using namespace Drivers;

SercomBase::Handler SercomBase::gHandlers[kNumHandlers]{};
uint32_t SercomBase::gUsed{0};
bool SercomBase::gSlowClockEnabled{false};

/*
 * SERCOM fast/core clocks, from peripheral_clk_config
 */
const uint32_t SercomBase::kFastClocks[kNumUnits]{
#ifdef CONF_GCLK_SERCOM0_CORE_FREQUENCY
    CONF_GCLK_SERCOM0_CORE_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM1_CORE_FREQUENCY
    CONF_GCLK_SERCOM1_CORE_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM2_CORE_FREQUENCY
    CONF_GCLK_SERCOM2_CORE_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM3_CORE_FREQUENCY
    CONF_GCLK_SERCOM3_CORE_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM4_CORE_FREQUENCY
    CONF_GCLK_SERCOM4_CORE_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM5_CORE_FREQUENCY
    CONF_GCLK_SERCOM5_CORE_FREQUENCY,
#else
    0,
#endif
};

/*
 * SERCOM fast/slow clocks, from peripheral_clk_config
 */
const uint32_t SercomBase::kSlowClocks[kNumUnits]{
#ifdef CONF_GCLK_SERCOM0_SLOW_FREQUENCY
    CONF_GCLK_SERCOM0_SLOW_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM1_SLOW_FREQUENCY
    CONF_GCLK_SERCOM1_SLOW_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM2_SLOW_FREQUENCY
    CONF_GCLK_SERCOM2_SLOW_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM3_SLOW_FREQUENCY
    CONF_GCLK_SERCOM3_SLOW_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM4_SLOW_FREQUENCY
    CONF_GCLK_SERCOM4_SLOW_FREQUENCY,
#else
    0,
#endif
#ifdef CONF_GCLK_SERCOM5_SLOW_FREQUENCY
    CONF_GCLK_SERCOM5_SLOW_FREQUENCY,
#else
    0,
#endif
};

/**
 * @brief Mapping of SERCOM unit to peripheral clock port
 */
constexpr static const ClockMgmt::Peripheral gClockPeripherals[]{
    ClockMgmt::Peripheral::SERCOM0Core, ClockMgmt::Peripheral::SERCOM1Core,
    ClockMgmt::Peripheral::SERCOM2Core, ClockMgmt::Peripheral::SERCOM3Core,
    ClockMgmt::Peripheral::SERCOM4Core, ClockMgmt::Peripheral::SERCOM5Core,
};


/**
 * @brief Marks a SERCOM instance as used
 *
 * It sets the bit in the usage bitmask, and asserts it was clear before.
 */
void SercomBase::MarkAsUsed(const Unit unit) {
    const uint32_t bit{1UL << static_cast<uint8_t>(unit)};

    taskENTER_CRITICAL();

    // enable SERCOM slow clocks, if not yet done
    if(!gSlowClockEnabled) {
        ClockMgmt::EnableClock(ClockMgmt::Peripheral::SharedSlow, ClockMgmt::Clock::LowSpeed);
        gSlowClockEnabled = true;
    }

    // mark it as used and enable its clock
    REQUIRE(!(gUsed & bit), "SERCOM %u already in use!", static_cast<unsigned int>(unit));
    gUsed |= bit;

    SetApbClock(unit, true);
    ClockMgmt::EnableClock(gClockPeripherals[static_cast<size_t>(unit)],
            ClockMgmt::Clock::HighSpeed);

    taskEXIT_CRITICAL();
}

/**
 * @brief Marks a SERCOM instance as avaialble
 *
 * It clears the bit in the usage bitmask, and asserts it was set before. Also, any handlers will
 * be removed.
 */
void SercomBase::MarkAsAvailable(const Unit unit) {
    const uint32_t bit{1UL << static_cast<uint8_t>(unit)};

    taskENTER_CRITICAL();
    REQUIRE((gUsed & bit), "SERCOM %u already in use!", static_cast<unsigned int>(unit));
    gUsed &= ~bit;

    for(size_t i = 0; i < 3; i++) {
        gHandlers[HandlerOffset(static_cast<uint8_t>(unit), i)].reset();
    }

    ClockMgmt::DisableClock(gClockPeripherals[static_cast<size_t>(unit)]);
    SetApbClock(unit, false);

    taskEXIT_CRITICAL();
}

/**
 * @brief Register a new SERCOM interrupt handler
 *
 * Install the specified function for the given SERCOM unit and interrupt index.
 *
 * @param unit SERCOM unit ([0, 5])
 * @param irq Interrupt number in the SERCOM ([0, 3])
 * @param fn Function to install, which is called as part of the irq
 * @param ctx Optional context value passed to the function
 *
 * @remark This call will panic if an interrupt is already installed.
 */
void SercomBase::RegisterHandler(const Unit unit, const uint8_t irq, void (*fn)(void *ctx),
        void *ctx) {
    REQUIRE(static_cast<uint8_t>(unit) <= 5, "invalid sercom %s: %u", "unit", unit);
    REQUIRE(irq <= 3, "invalid sercom %s: %u", "irq", irq);

    // ensure there's no handler
    const auto idx = HandlerOffset(static_cast<uint8_t>(unit), irq);
    if(gHandlers[idx]) {
        Logger::Panic("already have sercom irq handler for %u:%u!", unit, irq);
    }

    // otherwise, go and install it
    taskENTER_CRITICAL();

    gHandlers[idx] = {
        .fn = fn,
        .ctx = ctx
    };

    __DSB();
    taskEXIT_CRITICAL();
}



/**
 * @brief Enable the SERCOM APB clock
 *
 * Enable the APB clock for the specified SERCOM unit. This may be on any one of APBA - APBD.
 *
 * @param unit The SERCOM unit whose APB clock to configure
 * @param state Whether the clock is enabled (`true`) or not (`false`)
 */
void SercomBase::SetApbClock(const Unit unit, const bool state) {
    switch(unit) {
        case Unit::Unit0:
            MCLK->APBAMASK.bit.SERCOM0_ = state ? 1 : 0;
            break;
        case Unit::Unit1:
            MCLK->APBAMASK.bit.SERCOM1_ = state ? 1 : 0;
            break;
        case Unit::Unit2:
            MCLK->APBBMASK.bit.SERCOM2_ = state ? 1 : 0;
            break;
        case Unit::Unit3:
            MCLK->APBBMASK.bit.SERCOM3_ = state ? 1 : 0;
            break;
        case Unit::Unit4:
            MCLK->APBDMASK.bit.SERCOM4_ = state ? 1 : 0;
            break;
        case Unit::Unit5:
            MCLK->APBDMASK.bit.SERCOM5_ = state ? 1 : 0;
            break;
    }
}



#define CallHandler(unit,irq) { \
    using S = Drivers::SercomBase;\
    const auto &h = S::gHandlers[S::HandlerOffset(unit, irq)];\
    if(!h.fn) {\
        Logger::Panic("unhandled SERCOM%u irq %u", unit, irq);\
    }\
    h.fn(h.ctx);\
}

void SERCOM0_0_Handler(void) { /* SERCOM0_0 */
    CallHandler(0, 0);
}

void SERCOM0_1_Handler(void) { /* SERCOM0_1 */
    CallHandler(0, 1);
}
void SERCOM0_2_Handler(void) { /* SERCOM0_2 */
    CallHandler(0, 2);
}
void SERCOM0_3_Handler(void) { /* SERCOM0_3, SERCOM0_4, SERCOM0_5, SERCOM0_6 */
    CallHandler(0, 3);
}

void SERCOM1_0_Handler(void) { /* SERCOM1_0 */
    CallHandler(1, 0);
}
void SERCOM1_1_Handler(void) { /* SERCOM1_1 */
    CallHandler(1, 1);
}
void SERCOM1_2_Handler(void) { /* SERCOM1_2 */
    CallHandler(1, 2);
}
void SERCOM1_3_Handler(void) { /* SERCOM1_3, SERCOM1_4, SERCOM1_1, SERCOM1_6 */
    CallHandler(1, 3);
}

void SERCOM2_0_Handler(void) { /* SERCOM2_0 */
    CallHandler(2, 0);
}
void SERCOM2_1_Handler(void) { /* SERCOM2_1 */
    CallHandler(2, 1);
}
void SERCOM2_2_Handler(void) { /* SERCOM2_2 */
    CallHandler(2, 2);
}
void SERCOM2_3_Handler(void) { /* SERCOM2_3, SERCOM2_4, SERCOM2_2, SERCOM2_6 */
    CallHandler(2, 3);
}

void SERCOM3_0_Handler(void) { /* SERCOM3_0 */
    CallHandler(3, 0);
}
void SERCOM3_1_Handler(void) { /* SERCOM3_1 */
    CallHandler(3, 1);
}
void SERCOM3_2_Handler(void) { /* SERCOM3_2 */
    CallHandler(3, 2);
}
void SERCOM3_3_Handler(void) { /* SERCOM3_3, SERCOM3_4, SERCOM3_3, SERCOM3_6 */
    CallHandler(3, 3);
}

void SERCOM4_0_Handler(void) { /* SERCOM4_0 */
    CallHandler(4, 0);
}
void SERCOM4_1_Handler(void) { /* SERCOM4_1 */
    CallHandler(4, 1);
}
void SERCOM4_2_Handler(void) { /* SERCOM4_2 */
    CallHandler(4, 2);
}
void SERCOM4_3_Handler(void) { /* SERCOM4_3, SERCOM4_4, SERCOM4_4, SERCOM4_6 */
    CallHandler(4, 3);
}

void SERCOM5_0_Handler(void) { /* SERCOM5_0 */
    CallHandler(5, 0);
}
void SERCOM5_1_Handler(void) { /* SERCOM5_1 */
    CallHandler(5, 1);
}
void SERCOM5_2_Handler(void) { /* SERCOM5_2 */
    CallHandler(5, 2);
}
void SERCOM5_3_Handler(void) {
    CallHandler(5, 3);
}

