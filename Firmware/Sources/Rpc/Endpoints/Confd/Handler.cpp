#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "../../MessageHandler.h"
#include "../../Rpc.h"
#include "../../Types.h"
#include "Handler.h"
#include "Service.h"

using namespace Rpc::Confd;

/**
 * @brief Initialize the confd handler
 *
 * This sets up the synchronization objects (such as the mutex to protect our internal data)
 */
Handler::Handler() {
    this->lock = xSemaphoreCreateMutex();
    REQUIRE(this->lock, "%s failed", "xSemaphoreCreateMutex");
}

/**
 * @brief Clean up handler resources
 *
 * Wake up any waiting tasks (with an "aborted" status code) and then release all of our resource.
 */
Handler::~Handler() {
    // unblock any waiting tasks
    if(xSemaphoreTake(this->lock, pdMS_TO_TICKS(10)) == pdPASS) {
        // TODO: implement

        xSemaphoreGive(this->lock);
    } else {
        Logger::Error("failed to acquire confd lock during shutdown!");
    }

    // clean up resources
    vSemaphoreDelete(this->lock);
}

/**
 * @brief Attach the confd handler
 *
 * This will cause the channel to be announced to the host.
 */
void Handler::attach(Rpc::MessageHandler *mh) {
    mh->registerEndpoint(kRpmsgName, this, kRpmsgAddress);
}

/**
 * @brief Handle an incoming message
 *
 * This will look the message up (using its tag) to see what task(s) are blocking on it. Since
 * confd never sends unsolicited updates, any packet we receive will either be something we can
 * handle directly (no-op) or correspond to a waiting request.
 */
void Handler::handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) {
    // discard if not large enough for an rpc header
    if(message.size() < sizeof(struct rpc_header)) {
        Logger::Warning("discarding message (%p, %lu) from %08x: %s", message.data(),
                message.size(), srcAddr, "msg too short");
        return;
    }

    // basic header validation
    auto hdr = reinterpret_cast<const struct rpc_header *>(message.data());
    if(hdr->length < sizeof(struct rpc_header)) {
        Logger::Warning("discarding message (%p, %lu) from %08x: %s", message.data(),
                message.size(), srcAddr, "invalid hdr length");
        return;
    }
    else if(hdr->version != kRpcVersionLatest) {
        Logger::Warning("discarding message (%p, %lu) from %08x: %s", message.data(),
                message.size(), srcAddr, "invalid rpc version");
        return;
    }

    // invoke the appropriate handler
    switch(hdr->type) {
        // no-op
        case static_cast<uint8_t>(MsgType::NoOp):
            Logger::Trace("received nop from %08x", srcAddr);
            break;

        // response to a query for a value
        case static_cast<uint8_t>(MsgType::Query):
            this->handleResponse(message, srcAddr,
                    DecoderCallback::create<Service::DeserializeQuery>());
            break;

        // response to a set request
        case static_cast<uint8_t>(MsgType::Update):
            this->handleResponse(message, srcAddr,
                    DecoderCallback::create<Service::DeserializeUpdate>());
            break;

        // unhandled message
        default:
            Logger::Notice("unknown msg type %02x from %08x", hdr->type, srcAddr);
            break;
    }
}

/**
 * @brief Process a response to a previously sent packet
 *
 * Notify the task that originally sent this set request. The flow is the same for get and set
 * requests (and any other type we implement) with the deviation being the custom decoder
 * function that's passed in.
 *
 * @remark This method assumes the rpc header in the provided message is valid.
 */
void Handler::handleResponse(etl::span<const uint8_t> message, const uint32_t srcAddr,
        etl::delegate<int(etl::span<const uint8_t>, InfoBlock *)> decoder) {
    int err;
    BaseType_t ok;
    InfoBlock *info{nullptr};

    auto hdr = reinterpret_cast<const struct rpc_header *>(message.data());
    const auto tag = hdr->tag;

    // retrieve the appropriate info block
    ok = xSemaphoreTake(this->lock, portMAX_DELAY);
    REQUIRE(ok == pdTRUE, "failed to acquire %s", "confd lock");

    if(!this->requests.count(tag)) {
        Logger::Warning("got confd reply (tag %02x) but no such request!", tag);

        xSemaphoreGive(this->lock);
        return;
    }

    info = this->requests.at(tag);
    REQUIRE(info, "failed to get request info");

    this->requests.erase(tag);

    if(info->abandoned) {
        // bail if the wait was abandoned
        xSemaphoreGive(this->lock);
        return;
    }

    xSemaphoreGive(this->lock);

    // if we get here, the task should still be blocking, so deserialize the message fully
    err = decoder(message.subspan<offsetof(struct rpc_header, payload)>(), info);
    if(err) {
        Logger::Warning("%s failed: %d", "Service::DeserializeQuery", err);
        info->error = err;
    }

    // lastly, notify the task
    ok = xTaskNotifyIndexed(info->notificationTask, Rtos::DriverPrivate, info->notificationBits,
            eSetBits);
    REQUIRE(ok == pdTRUE, "%s failed", "xTaskNotifyIndexed");
}

/**
 * @brief Send the specified packet and wait for a response
 *
 * Transmit the given packet (assumed to have an rpc_header in the first 8 bytes) to the host,
 * then block the calling task until a response arrives (or the timeout expires.)
 *
 * @param message Packet to send to the confd on the host
 * @param outInfoBlock Variable to receive the allocated info block
 * @param timeout How long to wait for a response (in FreeRTOS ticks)
 *
 * @return 0 on success, 1 on timeout, or a negative error.
 *
 * @remark If a non-null info block is returned, the caller is responsible for deleting it when
 *         the results of the call are no longer needed. Otherwise, memory will be leaked.
 *
 * @remark The rpc_header of the packet should be mostly filled in, with the exception of tag.
 */
int Handler::sendRequestAndBlock(etl::span<uint8_t> message, InfoBlock* &outInfoBlock,
        TickType_t timeout) {
    int err;
    BaseType_t ok;
    uint32_t note;

    // message must at least contain an rpc header
    if(message.size() < sizeof(struct rpc_header)) {
        return -1;
    }

    // first, try to allocate the info block and fill it in
    auto info = new InfoBlock;
    if(!info) {
        return -1;
    }

    info->notificationTask = xTaskGetCurrentTaskHandle();
    info->notificationBits = kNotifyBit;

    // clear the notification bit to ensure we recover from previous time out
    ulTaskNotifyValueClearIndexed(nullptr, Rtos::DriverPrivate, kNotifyBit);

    // figure out the next tag value, and store the info block
    ok = xSemaphoreTake(this->lock, timeout);
    if(ok == pdFALSE) {
        delete info;
        return -1;
    }

    do {
        info->tag = ++this->nextTag;
    } while(!info->tag || this->requests.count(info->tag));

    this->requests[info->tag] = info;

    xSemaphoreGive(this->lock);

    // update the rpc header
    auto hdr = reinterpret_cast<struct rpc_header *>(message.data());
    hdr->tag = info->tag;

    // request message transmission
    err = Rpc::GetHandler()->sendTo(this->ep, message, this->ep->dest_addr, timeout);

    if(err < 0) {
        delete info;
        return err;
    }

    // block this task
    ok = xTaskNotifyWaitIndexed(Rtos::DriverPrivate, 0, kNotifyBit, &note, timeout);

    if(ok == pdFALSE) {
        // mark as abandoned so we don't try to process it
        info->abandoned = true;

        // try to remove it from our internal bookkeeping
        if(xSemaphoreTake(this->lock, timeout)) {
            // remove from map
            this->requests.erase(info->tag);

            // restore lock
            xSemaphoreGive(this->lock);
        }

        // finally, deallocate it
        delete info;
        return 1;
    }

    // hey, we're back, return the info block
    outInfoBlock = info;
    return 0;
}

