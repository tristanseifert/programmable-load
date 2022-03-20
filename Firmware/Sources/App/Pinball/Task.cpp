#include "Task.h"
#include "Hardware.h"

#include "Drivers/ExternalIrq.h"
#include "Drivers/Gpio.h"
#include "Drivers/Spi.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>

using namespace App::Pinball;

StaticTask_t Task::gTcb;
StackType_t Task::gStack[kStackSize];

/**
 * @brief Start the pinball task
 *
 * This initializes the shared pinball task instance.
 */
void App::Pinball::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(alignof(Task))));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    new (ptr) Task();
}

/**
 * @brief Initialize the UI task
 */
Task::Task() {
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Pinball main loop
 *
 * Responds to user interface events (such as button presses, encoder rotations, etc.) and then
 * updates the interface (display, indicators) appropriately.
 */
void Task::main() {
    /*
     * Initialize front panel hardware
     *
     * Reset all hardware for front IO, then begin by initializing the display. After this is
     * complete, read the ID EEPROM on the I2C bus to determine what devices/layout is available on
     * the front panel, and initialize those devices.
     */
    Logger::Trace("pinball: %s", "reset hw");
    Hw::ResetFrontPanel();

    Logger::Trace("pinball: %s", "init display");
    // TODO: initialize display

    Logger::Trace("pinball: %s", "init front panel");
    // TODO: discover front panel

    // begin message handling
    Logger::Trace("pinball: %s", "start message loop");

    while(1) {
        // TODO: implement
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

