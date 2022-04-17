#include "Task.h"
#include "Vendor/VendorInterfaceTask.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <tusb.h>

using namespace UsbStack;

Task *Task::gShared{nullptr};

/**
 * @brief Set up USB stack task
 *
 * Initialize the USB task class in the preallocated memory.
 */
void Task::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(alignof(Task))));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    gShared = new (ptr) Task();
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
    }, kName.data(), kStackSize, this, kPriority, this->stack, &this->tcb);
}

/**
 * @brief Task entry point
 *
 * Invoked when the task is started; it performs deferred initialization work, then handles USB
 * requests forever.
 */
void Task::main() {
    Logger::Trace("USB: %s", "start");

    // init USB stack and the endpoints
    tusb_init();

    this->vendorInterface = Vendor::InterfaceTask::Start();

    // service requests
    Logger::Trace("USB: %s", "main loop");

    while(1) {
        tud_task();
    }
}



/**
 * @brief USB device configured callback
 */
void tud_mount_cb() {
    Logger::Notice("USB: %s", "device mounted");
    Task::gShared->isConnected = true;

    Vendor::InterfaceTask::HostConnected();
}

/**
 * @brief USB device unmounted (disconnected)
 */
void tud_umount_cb() {
    Logger::Notice("USB: %s", "device unmounted");
    Task::gShared->isConnected = false;

    Vendor::InterfaceTask::HostDisconnected();
}
