#include <openamp/open_amp.h>

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
            TaskNotifyBits::MailboxDeferredIrq);
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

        // feed the watchdog
        if(note & TaskNotifyBits::MailboxDeferredIrq) {
            Mailbox::ProcessDeferredIrq(OpenAmp::GetRpmsgDev().vdev);
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
 * @param timeout How long to block to acquire the message handler lock
 *
 * @return 0 on success, or a negative error code
 */
int MessageHandler::registerEndpoint(const etl::string_view &epName, Endpoint *handler,
        const TickType_t timeout) {
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
            RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, [](auto ept, auto data, auto dataLen, auto src, auto priv) -> int {
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

