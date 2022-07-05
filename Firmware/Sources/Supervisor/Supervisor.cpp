#include "Drivers/Watchdog.h"
#include "Log/Logger.h"

#include "Task.h"
#include "Supervisor.h"

using namespace Supervisor;

/// Supervisor task instance
static Task *gSharedTask{nullptr};

/**
 * @brief Initialize the supervisor
 *
 * Set up the watchdog and supervisor task.
 */
void Supervisor::Init() {
    REQUIRE(!gSharedTask, "cannot re-initialize supervisor");

    // create the task
    gSharedTask = new Task;

    // set up watchdog
    Drivers::Watchdog::Config cfg{
        .divider = Drivers::Watchdog::ClockDivider::Div128,
        .earlyWarningIrq = true,
        .notifyTask = gSharedTask->handle,
        .notifyIndex = Task::kNotificationIndex,
        .notifyBits = Task::TaskNotifyBits::WatchdogWarning,
    };
    Drivers::Watchdog::Configure(cfg);
}
