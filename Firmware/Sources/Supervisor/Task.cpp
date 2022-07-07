#include "Drivers/Watchdog.h"
#include "Hw/StatusLed.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "Task.h"

using namespace Supervisor;


/**
 * @brief Initialize the supervisor task
 */
Task::Task() {
    // create the task
    auto ok = xTaskCreate([](auto ctx) {
        reinterpret_cast<Task *>(ctx)->main();
    }, kName.data(), kStackSize, this, kPriority, &this->handle);
    REQUIRE(ok == pdPASS, "failed to create supervisor task");

    // create a timer (for periodically checking state)
    this->checkinTimer = xTimerCreate("supervisor checkin",
            // specify interval; it's self-reloading (repeating)
            pdMS_TO_TICKS(kCheckinInterval), pdTRUE, this, [](auto timer) {
        auto task = reinterpret_cast<Task *>(pvTimerGetTimerID(timer));
        xTaskNotifyIndexed(task->handle, kNotificationIndex,
                static_cast<uint32_t>(TaskNotifyBits::WatchdogWarning), eSetBits);

        Logger::Notice("yeet");
    });
}

/**
 * @brief Clean up the supervisor task
 */
Task::~Task() {
    vTaskDelete(this->handle);
    xTimerDelete(this->checkinTimer, portMAX_DELAY);
}

/**
 * @brief Main loop
 *
 * Wait for the watchdog early warning notification, during which time we'll assess the system's
 * state and pet the watchdog.
 */
void Task::main() {
    BaseType_t ok;
    uint32_t note;

    Logger::Notice("Supervisor: %s", "task start");

    // enable watchdog
    if(kUseTimer) {
        xTimerStart(this->checkinTimer, portMAX_DELAY);
    }

    Drivers::Watchdog::Enable();
    Logger::Notice("Supervisor: %s", "wdg enabled!");

    // wait for events
    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0,
                static_cast<uint32_t>(TaskNotifyBits::All), &note, portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        // feed the watchdog
        if(note & TaskNotifyBits::WatchdogWarning) {
            this->wdgEarlyWarning();
        }
    }
}

/**
 * @brief Process a watchdog early warning event
 *
 * Evaluate the system's state, and kick the watchdog if it's valid.
 */
void Task::wdgEarlyWarning() {
    // TODO: implement system status checking
    Drivers::Watchdog::Pet();

    // alternate the status LED
    if(++this->numSuccessfulCheckins & 1) {
        Hw::StatusLed::Set(Hw::StatusLed::Color::Cyan);
    } else {
        Hw::StatusLed::Set(Hw::StatusLed::Color::Green);
    }
}
