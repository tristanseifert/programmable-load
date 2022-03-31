#include "Task.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <tusb.h>

using namespace UsbStack;

StaticTask_t Task::gTcb;
StackType_t Task::gStack[kStackSize];

/**
 * @brief Set up USB stack task
 *
 * Initialize the USB task class in the preallocated memory.
 */
void Task::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(4)));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    new (ptr) Task();
}

/**
 * @brief Initialize the USB task
 *
 * Initializes the statically allocated task structure, so that when the scheduler starts, the
 * task will begin executing.
 */
Task::Task() {
    // create the task
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Task entry point
 *
 * Invoked when the task is started; it performs deferred initialization work, then handles USB
 * requests forever.
 */
void Task::main() {
    // TODO: get serial number for serial number descriptor
    Logger::Trace("%s: start", "USB");

    // init USB stack
    tusb_init();

    // service requests
    while(1) {
        tud_task();
    }
}
