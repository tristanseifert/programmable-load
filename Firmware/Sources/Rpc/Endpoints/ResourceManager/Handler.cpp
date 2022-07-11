#include <etl/algorithm.h>

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "../../MessageHandler.h"
#include "../../Rpc.h"
#include "Handler.h"
#include "Service.h"

using namespace Rpc::ResourceManager;

/**
 * @brief Initialize the ResourceManager message handler
 */
Handler::Handler() {
    this->msgRxSem = xSemaphoreCreateBinary();
    REQUIRE(this->msgRxSem, "%s failed", "xSemaphoreCreateBinary");
}

/**
 * @brief Cleans up the ResourceManager message handler
 */
Handler::~Handler() {
    // abort any waiting calls
    if(this->waitingTask) {
        // TODO: notify it (but with timeout error)
    }

    // delete
    vSemaphoreDelete(this->msgRxSem);
}

/**
 * @brief Announce the RPC endpoint
 */
void Handler::attach(Rpc::MessageHandler *mh) {
    int err = mh->registerEndpoint(kRpmsgName, this, kRpmsgAddress);
    REQUIRE(!err, "failed to register rpc ep %s: %d", kRpmsgName.data(), err);
}

/**
 * @brief Handle an incoming message
 *
 * The message is copied into our receive buffer, then the waiting task (if any) is notified to
 * continue processing.
 */
void Handler::handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) {
    Endpoint::handleMessage(message, srcAddr);

    // ignore empty messages, and do some sanity checking
    if(message.empty()) {
        return;
    } else if(message.size() > this->rxBuffer.capacity()) {
        Logger::Warning("ignoring rproc_srm msg from %08x (%s, %u bytes)", srcAddr, "too long",
                message.size());
        return;
    }

    // just copy the message over
    this->rxBuffer.resize(message.size());
    etl::copy_s(message.begin(), message.end(), this->rxBuffer.begin(), this->rxBuffer.end());

    // wake up task
    if(this->waitingTask) {
        xTaskNotifyIndexed(this->waitingTask, Rtos::TaskNotifyIndex::DriverPrivate, kNotifyBit,
                eSetBits);
        this->waitingTask = nullptr;
    }
}

/**
 * @brief Send a message, and wait for its response
 *
 * @param message Message to send to the host
 * @param outRawResponse Variable to receive a pointer to where the message was received
 * @param timeout How long to wait for a response
 *
 * @return 0 on success or a negative error code
 *
 * @remark This might have some raciness issues if we get unsolicted (or multiple) responses from
 *         the host, as the buffer returned out will then get indiscriminately overwritten by the
 *         receive callback.
 *
 * @remark Callers should ensure only one task enters this routine at a time.
 */
int Handler::sendRequestAndBlock(etl::span<uint8_t> message, etl::span<uint8_t> &outRawResponse,
        TickType_t timeout) {
    int err;
    BaseType_t ok;
    uint32_t note;

    // validate args
    if(message.empty()) {
        return -1;
    }

    // set up for wait
    ulTaskNotifyValueClearIndexed(nullptr, Rtos::DriverPrivate, kNotifyBit);

    this->waitingTask = xTaskGetCurrentTaskHandle();

    // wait for remote endpoint to become available
    if(!this->waitForRemote(timeout)) {
        return 1;
    }

    // cool, so send the packet now
    err = Rpc::GetHandler()->sendTo(this->ep, message, this->ep->dest_addr, timeout);

    if(err < 0) {
        return err;
    }

    // …then await the response
    ok = xTaskNotifyWaitIndexed(Rtos::DriverPrivate, 0, kNotifyBit, &note, timeout);

    if(ok == pdFALSE) {
        return 1;
    }

    // output the packet that wsa actually received
    outRawResponse = this->rxBuffer;

    return 0;
}

