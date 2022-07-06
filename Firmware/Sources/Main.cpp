#include "Log/Logger.h"
#include "Rtos/Start.h"

#include "BuildInfo.h"

#include "stm32mp1xx_hal_rcc.h"

#include "Drivers/Random.h"
#include "Hw/StatusLed.h"
#include "Rpc/Rpc.h"
#include "Supervisor/Supervisor.h"

/**
 * @brief Early hardware init
 *
 * This un-gates various clocks and enables some basic peripherals (RCC, GPIO) that we will need
 * throughout the life of the software.
 */
static void EarlyHwInit() {
    // enable hardware semaphores
    __HAL_RCC_HSEM_CLK_ENABLE();

    // set up status indicator
    Hw::StatusLed::Init();
    Hw::StatusLed::Set(Hw::StatusLed::Color::Yellow);

    // initialize a few peripherals
    Drivers::Random::Init();
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
    /*
     * Perform early initialization (status indicators, to show sign of life; and clocks for some
     * internal peripherals) then log a message indicating that we're alive before proceeding with
     * the actual initialization.
     */
    EarlyHwInit();

    Logger::Warning("Programmable load rtfw (%s/%s-%s) built on %s by %s@%s",
            gBuildInfo.gitBranch, gBuildInfo.gitHash, gBuildInfo.buildType,
            gBuildInfo.buildDate,
            gBuildInfo.buildUser, gBuildInfo.buildHost);
    Logger::Notice("MPU clock: %u Hz", SystemCoreClock);

    /*
     * Initialize host RPC interface
     *
     * This prepares the tasks used to communicate with the host via the Linux rpmsg interface,
     * and a virtio device managed via OpenAMP.
     *
     * Actual communication won't take place until the FreeRTOS scheduler is started, as a
     * background task handles all the message processing.
     */
    Rpc::Init();

    /*
     * Create the supervisory tasks, which are responsible for thermal control of the system and
     * ensuring all tasks that should be running are, and aren't hung.
     *
     * These in turn feed into the hw watchdog, which is activated from this point on.
     */
    Supervisor::Init();

    /*
     * Last, transfer control to the FreeRTOS scheduler. This will begin executing the app main
     * task, which in turn performs all initialization with full scheduling services available.
     *
     * We do it this way so that our initialization can rely on stuff like timers working.
     */
    Rtos::StartScheduler();
}
