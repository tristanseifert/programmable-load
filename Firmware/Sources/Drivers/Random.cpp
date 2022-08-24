#include "stm32mp1xx.h"
#include "stm32mp1xx_hal_rcc.h"

#include "Log/Logger.h"
#include "Rpc/Rpc.h"
#include "Rpc/Endpoints/ResourceManager/Service.h"

#include "Random.h"


using namespace Drivers;

/**
 * @brief Initialize random number generator
 *
 * This enables the clock and reset the TRNG.
 */
void Random::Init() {
    using ResMgr = Rpc::ResourceManager::Service;

    int err;
    volatile size_t timeout;

    // acquire RNG2 and configure its clock
    ResMgr::ClockConfig requestedClk{
        .index = 0,
        // this is what's specified for PLL4R in the device tree config
        .rate = 40'000'000,
    }, actualClk;

    err = Rpc::GetResMgrService()->setConfig(Rpc::ResourceManager::RESMGR_ID_RNG2, nullptr,
            requestedClk, actualClk, pdMS_TO_TICKS(1000));
    REQUIRE(!err, "failed to set resmgr cfg: %d", err);

    // enable clocks
    __HAL_RCC_RNG2_CLK_ENABLE();
    __HAL_RCC_RNG2_CONFIG(RCC_RNG2CLKSOURCE_PLL4);

    // reset RNG
    __HAL_RCC_RNG2_FORCE_RESET();
    timeout = 100;
    while(timeout--) {}

    __HAL_RCC_RNG2_RELEASE_RESET();

    // enable RNG; enable clock error detection
    RNG2->CR = 0;
    RNG2->CR = RNG_CR_CED;
    RNG2->CR = RNG_CR_CED | RNG_CR_RNGEN;

    // wait for random number ready (and no error)
    timeout = kInitTimeout;
    while(!(RNG2->SR & RNG_SR_DRDY)) {
        // check for errors
        REQUIRE(!(RNG2->SR & (RNG_SR_CECS | RNG_SR_SECS)), "RNG init failed: SR=%08x", RNG2->SR);

        // handle timeout
        REQUIRE(--timeout, "RNG %s timed out (SR=%08x)", "init", RNG2->SR);

        // break out if data is now ready
        if(RNG2->SR & RNG_SR_DRDY) {
            goto beach;
        }
    }

beach:;
}

uint32_t Random::Get() {
    // wait for data ready
    volatile size_t timeout{kRefillTimeout};
    while(!(RNG2->SR & RNG_SR_DRDY)) {
        REQUIRE(--timeout, "RNG %s timed out (SR=%08x)", "read", RNG2->SR);
    }

    // return data
    const auto value = RNG2->DR;
    REQUIRE(value, "RNG read invalid (SR=%08x, DR=%08x)", RNG2->SR, value);

    return value;
}

