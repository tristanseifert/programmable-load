#include "WorkQueue.h"

#include "App/Pinball/Task.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <etl/array.h>

using namespace Gui;

QueueHandle_t WorkQueue::gQueue{nullptr};

/**
 * @brief Initialize the shared GUI work queue
 */
void WorkQueue::Init() {
    static etl::array<Item, kQueueSize> gStorage;
    static StaticQueue_t gBuf;

    gQueue = xQueueCreateStatic(kQueueSize, sizeof(Item),
            reinterpret_cast<uint8_t *>(gStorage.data()), &gBuf);
    REQUIRE(gQueue, "gui: %s", "failed to allocate work queue");
}

/**
 * @brief Process a single unit of work.
 *
 * @return Whether we processed an item
 */
bool WorkQueue::Work() {
    Item i;
    BaseType_t ok;

    // try to dequeue item
    ok = xQueueReceive(gQueue, &i, 0);
    if(ok != pdTRUE) {
        return false;
    }

    // invoke work item
    i();
    return true;
}

/**
 * @brief Enqueue a work item
 *
 * Submits a work item to the queue.
 */
bool WorkQueue::Submit(void (*function)(void *ctx), void *context) {
    BaseType_t ok;

    // sanity checking
    REQUIRE(function, "invalid work queue submission");

    Item i{
        function, context
    };

    // submit it
    ok = xQueueSendToBack(gQueue, &i, 0);
    if(ok != pdTRUE) {
        return false;
    }

    App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::ProcessWorkQueue);
    return true;
}

