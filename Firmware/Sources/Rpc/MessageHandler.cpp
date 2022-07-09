#include <openamp/open_amp.h>

#include "Hw/StatusLed.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "Endpoints/Handler.h"
#include "OpenAmp.h"
#include "Mailbox.h"
#include "MessageHandler.h"

using namespace Rpc;

/**
 * @brief Initialize the message handler task
 */
MessageHandler::MessageHandler() {
    auto ok = xTaskCreate([](auto ctx) {
        reinterpret_cast<MessageHandler *>(ctx)->main();
    }, kName.data(), kStackSize, this, kPriority, &this->handle);
    REQUIRE(ok == pdPASS, "failed to create task");

    this->lock = xSemaphoreCreateMutex();

    // sign it up for mailbox interrupt events
    Mailbox::SetDeferredIsrHandler(this->handle, kNotificationIndex,
            TaskNotifyBits::MailboxDeferredIrq, TaskNotifyBits::ShutdownRequest);
}

/**
 * @brief Terminate the message handler task
 */
MessageHandler::~MessageHandler() {
    vTaskDelete(this->handle);
    vSemaphoreDelete(this->lock);
}

/**
 * @brief Message handler main loop
 *
 * This waits in perpetuity to be notified (this would typically be from the IPCC mailbox driver)
 * and then processes events.
 */
void MessageHandler::main() {
    BaseType_t ok;
    uint32_t note;

    Logger::Notice("MsgHandler: %s", "task start");


    // process events
    Logger::Trace("MsgHandler: %s", "enter main loop");

    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0,
                static_cast<uint32_t>(TaskNotifyBits::All), &note, portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        // collect the lock
        xSemaphoreTake(this->lock, portMAX_DELAY);

        // process a virtio event
        if(note & TaskNotifyBits::MailboxDeferredIrq) {
            Mailbox::ProcessDeferredIrq(OpenAmp::GetRpmsgDev().vdev);
        }
        xSemaphoreGive(this->lock);

        // handle a shutdown request
        if(note & TaskNotifyBits::ShutdownRequest) {
            this->handleShutdown();
        }
    }
}

/**
 * @brief Handle shutdown request
 *
 * Notify all tasks (which care about this notification) that we're shutting down; then wait for
 * all of them to acknowledge shutdown.
 */
void MessageHandler::handleShutdown() {
    BaseType_t ok;
    uint32_t note;

    // TODO: ensure it doesn't get overwritten by watchdog blinker
    // update indicators
    Hw::StatusLed::Set(Hw::StatusLed::Color::Red);
    Logger::Warning("Shutdown request received!");

    // notify all of the tasks we're shutting down (in reverse order)
    for(auto it = this->shutdownHandlers.rbegin(); it != this->shutdownHandlers.rend(); ++it) {
        auto &info = *it;
        info.callback(this, info.context);
    }

    // wait for all tasks to acknowledge
    if(!this->shutdownHandlers.empty()) {
        const auto shutdownTotal = this->shutdownCounter;

        while(shutdownCounter) {
            Logger::Debug("waiting for shutdown ack (%u/%u)", shutdownTotal - this->shutdownCounter,
                    shutdownTotal);

            // TODO: use a different timeout?
            ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0,
                    static_cast<uint32_t>(TaskNotifyBits::ShutdownAck), &note, portMAX_DELAY);
            REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);
        }
    }

    Logger::Notice("all shutdown acks received, proceeding");

    // TODO: shut down vdev interface to host

    // extinguish the LED and acknowledge shutdown to the host
    Hw::StatusLed::Set(Hw::StatusLed::Color::Off);

    Logger::Notice("acknowledging shutdown request to host");
    Mailbox::AckShutdownRequest();
}

/**
 * @brief Install a shutdown handler
 *
 * Insert the given callback to be invoked when the system receives a shutdown request from the
 * host.
 *
 * @param callback Function to invoke during shutdown
 * @param ctx Additional argument passed to the callback
 *
 * @remark Callbacks are invoked in the reverse order they're added.
 */
void MessageHandler::addShutdownHandler(const ShutdownCallback &callback, void *ctx) {
    taskENTER_CRITICAL();

    this->shutdownHandlers.emplace_back(ShutdownCallbackInfo{callback, ctx});
    this->shutdownCounter++;

    taskEXIT_CRITICAL();
}


/**
 * @brief Acknowledge shutdown notification
 *
 * Acknowledges a shutdown notification sent to a task.
 *
 * @remark Invoke this after you have completed _all_ shutdown-related processing; if there are no
 *         other tasks the shutdown is waiting on, the system powers off immediately after.
 *
 * @remark This method is not ISR safe.
 *
 * @note It is important this method is invoked _only_ if the calling task received a shutdown
 *       notification. Invoking it without receiving such a notification can cause internal state
 *       to be corrupted.
 */
void MessageHandler::ackShutdown() {
    if(!__atomic_sub_fetch(&this->shutdownCounter, 1, __ATOMIC_RELAXED)) {
        // all tasks shut down, pack it in
    }

    // notify the message handler task
    const auto ok = xTaskNotifyIndexed(this->handle, kNotificationIndex,
            static_cast<uint32_t>(TaskNotifyBits::ShutdownAck), eSetBits);
    REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyIndexed", ok);
}




/**
 * @brief Register a message endpoint
 *
 * This allocates the underlying rpmsg endpoint (announcing it to the host as needed) and registers
 * its callbacks.
 *
 * @param epName Name under which the endpoint is registered with the host
 * @param handler Handler class for all messages and events received on this endpoint
 * @param srcAddr A non-negative fixed channel source address, or -1 to automatically assign
 * @param timeout How long to block to acquire the message handler lock
 *
 * @return 0 on success, or a negative error code
 */
int MessageHandler::registerEndpoint(const etl::string_view &epName, Endpoint *handler,
        const uint32_t srcAddr, const TickType_t timeout) {
    REQUIRE(!this->endpoints.full(), "max number of endpoints registered!");

    int err;
    auto info = new EndpointInfo;
    info->handler = handler;

    // acquire the lock
    if(xSemaphoreTake(this->lock, timeout) != pdTRUE) {
        return -1;
    }

    // create the rpmsg endpoint
    err = rpmsg_create_ept(&info->rpmsgEndpoint, &OpenAmp::GetRpmsgDev().rdev, epName.data(),
            srcAddr, RPMSG_ADDR_ANY, [](auto ept, auto data, auto dataLen, auto src, auto priv) -> int {
        auto handler = reinterpret_cast<Endpoint *>(priv);
        auto msgPtr = reinterpret_cast<const uint8_t *>(data);
        handler->handleMessage({msgPtr, msgPtr + dataLen}, src);
        return 0;
    }, [](auto ept) {
        auto handler = reinterpret_cast<Endpoint *>(ept->priv);
        handler->hostDidUnbind();
    });

    REQUIRE(!err, "%s failed: %d", "rpmsg_create_ept", err);
    info->rpmsgEndpoint.priv = handler;

    // store the info
    this->endpoints.insert({epName, info});
    xSemaphoreGive(this->lock);

    // invoke the handler method
    handler->endpointIsAvailable(&info->rpmsgEndpoint);

    Logger::Debug("%s: registered endpoint '%s' = %p", "MsgHandler", epName.data(), handler);

    return 0;
}

