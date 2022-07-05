#include "Log/Logger.h"
#include "Rtos/Start.h"
//#include "Util/HwInfo.h"

#include "BuildInfo.h"

#include "stm32mp1xx_hal_rcc.h"

#include "Hw/StatusLed.h"
#include "Rpc/ResourceTable.h"

/**
 * @brief Early hardware init
 *
 * This un-gates various clocks and enables some basic peripherals (RCC, GPIO) that we will need
 * throughout the life of the software.
 */
static void EarlyHwInit() {
    // enable clocks for RCC, GPIO
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    // enable hardware semaphores
    __HAL_RCC_HSEM_CLK_ENABLE();

    // set up status indicator
    Hw::StatusLed::Init();
    Hw::StatusLed::Set(Hw::StatusLed::Color::Yellow);

    // start the watchdog
}

/**
 * @brief Application entry point
 *
 * We'll jump here after the chip is mostly set up; that is, the RAM regions are established. We
 * should then perform low-level hardware initialization (clocks, peripherals, IOs) and set up the
 * tasks and other OS resources.
 *
 * Lastly, the RTOS scheduler is launched to actually begin execution.
 */
extern "C" int main() {
    // perform initial hardware setup
    EarlyHwInit();

    // log version
    Logger::Warning("**********\nProgrammable load rtfw (%s/%s-%s)\nBuilt on: %s by %s@%s",
            gBuildInfo.gitBranch, gBuildInfo.gitHash, gBuildInfo.buildType,
            gBuildInfo.buildDate,
            gBuildInfo.buildUser, gBuildInfo.buildHost);

    Logger::Notice("Virtio: vring0@%p, vring1@%p", Rpc::ResourceTable::GetVring0().da, Rpc::ResourceTable::GetVring1().da);

    /*
     * Last, transfer control to the FreeRTOS scheduler. This will begin executing the app main
     * task, which in turn performs all initialization with full scheduling services available.
     *
     * We do it this way so that our initialization can rely on stuff like timers working.
     */
    Rtos::StartScheduler();
}
