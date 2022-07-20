#include "Task.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "Rpc/Types.h"
#include "Rpc/MessageHandler.h"
#include "Rpc/Rpc.h"

#include <cbor.h>
#include <string.h>

using namespace App::Rpmsg;

Task *Task::gShared{nullptr};

/**
 * @brief Initialize the RPC message handler
 */
void App::Rpmsg::Start() {
    Task::gShared = new Task;
}

/**
 * @brief Initialize the control task
 */
Task::Task() {
    // create the task
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, this->stack, &this->tcb);

    // also, create the timer (to force sampling of data)
    this->sampleTimer = xTimerCreateStatic("rpmsg measurement send timer",
        // automagically reload
        pdMS_TO_TICKS(kMeasureInterval), pdTRUE,
        this, [](auto timer) {
            Task::NotifyTask(Task::TaskNotifyBits::SendMeasurements);
        }, &this->sampleTimerBuf);
    REQUIRE(this->sampleTimer, "rpmsg: %s", "failed to allocate timer");
}

/**
 * @brief Clean up task resources.
 */
Task::~Task() {
    vTaskDelete(this->task);
    xTimerDelete(this->sampleTimer, portMAX_DELAY);
}



/**
 * @brief Message handler main loop
 *
 * Wait for an event to take place so we can do something about it.
 */
void Task::main() {
    int err;
    BaseType_t ok;
    uint32_t note;
    bool remoteAlive{false};

    // set up the RPC channel
    Logger::Trace("rpmsg: %s", "announce endpoint");

    err = Rpc::GetHandler()->registerEndpoint(kRpmsgName, this, kRpmsgAddress);
    REQUIRE(!err, "failed to register rpc ep %s: %d", kRpmsgName.data(), err);

    // wait for the endpoint to come up
    Logger::Trace("rpmsg: %s", "wait for remote");
    for(size_t i = 0; i < 5; i++) {
        remoteAlive = this->waitForRemote(pdMS_TO_TICKS(1000));
        if(remoteAlive) {
            Logger::Trace("rpmsg: %s", "remote alive");
            break;
        }

        Logger::Notice("rpmsg: waiting for remote (attempt %u)", i);
    }
    REQUIRE(remoteAlive, "failed to get %s:%x remote", kRpmsgName.data(), kRpmsgAddress);

    xTimerStart(this->sampleTimer, portMAX_DELAY);

    // event loop
    Logger::Trace("rpmsg: %s", "start message loop");
    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0, TaskNotifyBits::All, &note,
                portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        if(note & TaskNotifyBits::SendMeasurements) {
            this->sendMeasurements();
        }
    }
}

/**
 * @brief Send the current measurement values to the host
 *
 * Capture the current measured voltage, current, and temperature values; then send them to the
 * host for processing.
 */
void Task::sendMeasurements() {
    int err;
    size_t totalNumBytes;
    CborEncoder encoder, encoderMap;

    // prepare RPC header
    auto hdr = reinterpret_cast<struct rpc_header *>(this->txBuffer.data());
    memset(hdr, 0, sizeof(*hdr));

    hdr->version = kRpcVersionLatest;
    hdr->type = static_cast<uint8_t>(MsgType::Measurement);
    hdr->flags = kRpcFlagBroadcast;

    // set up the CBOR encoder for the remaining memory
    const auto maxPayloadSize = kMaxPacketSize - sizeof(*hdr);
    cbor_encoder_init(&encoder, hdr->payload, maxPayloadSize, 0);

    err = cbor_encoder_create_map(&encoder, &encoderMap, 3);
    if(err) {
        Logger::Warning("%s failed: %d", "cbor_encoder_create_map", err);
        return;
    }

    // write the body
    static float t{0.};

    // TODO: read actual values instead of made-up stuff here
    // voltage
    cbor_encode_text_stringz(&encoderMap, "v");
    cbor_encode_float(&encoderMap, fabsf(sinf(t)));
    // current
    cbor_encode_text_stringz(&encoderMap, "i");
    cbor_encode_float(&encoderMap, fabsf(cosf(t)));
    // temperature
    cbor_encode_text_stringz(&encoderMap, "t");
    cbor_encode_float(&encoderMap, 20.f + fabsf(50.f * cosf(t)));

    t += 0.1f;

    // finish encoding
    err = cbor_encoder_close_container(&encoder, &encoderMap);
    if(err) {
        Logger::Warning("%s failed: %d", "cbor_encoder_close_container", err);
        return;
    }

    // return the buffer of just the message
    totalNumBytes = sizeof(*hdr) + cbor_encoder_get_buffer_size(&encoder, hdr->payload);

    hdr->length = totalNumBytes;

    // send the message
    err = Rpc::GetHandler()->sendTo(this->ep,
            {reinterpret_cast<uint8_t *>(this->txBuffer.data()), totalNumBytes},
            this->ep->dest_addr, pdMS_TO_TICKS(10));

    if(err < 0) {
        Logger::Warning("%s failed: %d", "MessageHandler::sendTo", err);
        return;
    }
}



/**
 * @brief Handle an incoming rpmsg message
 *
 * This handles all requests from the Linux side; these requests will be to change operating
 * parameters of the load, for example. Measurement data (and other state changes) are
 * broadcast to the remote endpoint periodically.
 *
 * @remark This is called in the context of the virtio message processing task. Use care when
 *         accessing task internal state.
 */
void Task::handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) {
    Endpoint::handleMessage(message, srcAddr);

    // bail early if it's 0 length (sent to notify us of the remote endpoint becoming alive)
    if(message.empty()) {
        return;
    }

    // discard if not large enough for an rpc header
    if(message.size() < sizeof(struct rpc_header)) {
        Logger::Warning("%s: discarding message (%p, %lu) from %08x: %s", kRpmsgName.data(),
                message.data(), message.size(), srcAddr, "msg too short");
        return;
    }

    // basic header validation
    auto hdr = reinterpret_cast<const struct rpc_header *>(message.data());
    if(hdr->length < sizeof(struct rpc_header)) {
        Logger::Warning("%s: discarding message (%p, %lu) from %08x: %s", kRpmsgName.data(),
                message.data(), message.size(), srcAddr, "invalid hdr length");
        return;
    }
    else if(hdr->version != kRpcVersionLatest) {
        Logger::Warning("%s: discarding message (%p, %lu) from %08x: %s", kRpmsgName.data(),
                message.data(), message.size(), srcAddr, "invalid rpc version");
        return;
    }

    // invoke the appropriate handler
    Logger::Trace("rpmsg: msg %p (%u bytes) from %x", message.data(), message.size(), srcAddr);

    switch(hdr->type) {
        // no-op
        case static_cast<uint8_t>(MsgType::NoOp):
            break;

        default:
            Logger::Warning("rpmsg: unknown message type %02x (from %08x)", hdr->type, srcAddr);
    }
}
