#include <openamp/open_amp.h>

#include "Hw/StatusLed.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "Endpoint.h"
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
        // handle a shutdown request
        if(note & TaskNotifyBits::ShutdownRequest) {
            Logger::Warning("Shutdown request received!");
            // TODO: ensure it doesn't get overwritten by watchdog blinker
            Hw::StatusLed::Set(Hw::StatusLed::Color::Red);

            // when we return, all tasks will have been notified and done, so ack shutdown
            Hw::StatusLed::Set(Hw::StatusLed::Color::Off);
            Mailbox::AckShutdownRequest();
        }

        xSemaphoreGive(this->lock);
    }
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

    Logger::Debug("%s: registered endpoint '%s' = %p", "MsgHandler", epName.data(), handler);

    return 0;
}

