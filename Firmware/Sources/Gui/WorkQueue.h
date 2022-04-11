#ifndef GUI_WORKQUEUE_H
#define GUI_WORKQUEUE_H

#include <stddef.h>

#include "Rtos/Rtos.h"

namespace Gui {
/**
 * @brief GUI work queue
 *
 * Various work items may be pushed on the GUI work queue, to be executed in the context of the
 * user interface worker task before we draw.
 */
class WorkQueue {
    private:
        /**
         * @brief A single work item
         */
        struct Item {
            void (*callback)(void *context);
            void *context{nullptr};

            /// Invoke the method specified by the work item
            inline void operator()() {
                this->callback(this->context);
            }
        };

    public:
        static void Init();

        /**
         * @brief Process all work items
         */
        static void Drain() {
            while(Work()) {}
        }
        static bool Work();

        static bool Submit(void (*function)(void *ctx), void *context = nullptr);

    private:
        /// Size of the storage area for the queue (max pending work items)
        constexpr static const size_t kQueueSize{5};
        /// Queue to hold work items
        static QueueHandle_t gQueue;
};
}

#endif
