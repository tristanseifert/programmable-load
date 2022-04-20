#include <hpl_gpio.h>

#include "App/Main/Task.h"
#include "Drivers/Dma.h"
#include "Drivers/ExternalIrq.h"
#include "Drivers/Random.h"
#include "Log/Logger.h"
#include "Rtos/Start.h"
#include "Util/HwInfo.h"

#include "Drivers/Gpio.h"

#include "BuildInfo.h"

#include <timers.h>

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
    Logger::Warning("\n\n**********\nProgrammable load fw (%s/%s-%s)\n%s@%s, on %s)",
            gBuildInfo.gitBranch, gBuildInfo.gitHash, gBuildInfo.buildType,
            gBuildInfo.buildUser, gBuildInfo.buildHost, gBuildInfo.buildDate);

    /*
     * Do early hardware initialization
     *
     * These are peripherals that are embedded in the processor and will always be present and
     * used for something.
     */
    Drivers::Dma::Init();
    Drivers::ExternalIrq::Init();
    Drivers::Random::Init();

    Util::HwInfo::Init();

    /*
     * Create the main app task
     *
     * This task is responsible for performing additional application initialization, including
     * starting other tasks.
     */
    App::Main::Start();

    /*
     * Last, transfer control to the FreeRTOS scheduler. This will begin executing the app main
     * task, which in turn performs all initialization with full scheduling services available.
     *
     * We do it this way so that our initialization can rely on stuff like timers working.
     */
    Rtos::StartScheduler();
}
