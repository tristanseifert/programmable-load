#include "Task.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

using namespace App::Control;

StaticTask_t Task::gTcb;
StackType_t Task::gStack[kStackSize];

Task *Task::gShared{nullptr};

/**
 * @brief Start the control task
 *
 * This initializes the shared control task instance.
 */
void App::Control::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(alignof(Task))));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    Task::gShared = new (ptr) Task();
}

/**
 * @brief Initialize the control task
 */
Task::Task() {
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Control main loop
 *
 * Entry point for the control loop task; this monitors the current/voltage sense ADCs, feeds
 * their inputs into the control algorithm, then uses that to drive the current control DACs.
 *
 * The expansion bus is scanned to determine what driver board is connected.
 */
void Task::main() {
    BaseType_t ok;
    uint32_t note;

    // perform setup

    /*
     * Start handling messages
     */
    Logger::Trace("control: %s", "start message loop");

    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0, TaskNotifyBits::All, &note,
                portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        // TODO: handle
        Logger::Warning("control notify: $%08x", note);
    }
}
