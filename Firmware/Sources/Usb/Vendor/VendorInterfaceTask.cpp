#include "VendorInterfaceTask.h"
#include "PropertyRequest.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <tusb.h>
#include <etl/array.h>

using namespace UsbStack::Vendor;

InterfaceTask *InterfaceTask::gShared{nullptr};

/**
 * @brief Initialize the USB stack's vendor interface task
 */
InterfaceTask *InterfaceTask::Start() {
    static uint8_t gBuf[sizeof(InterfaceTask)] __attribute__((aligned(alignof(InterfaceTask))));
    auto ptr = reinterpret_cast<InterfaceTask *>(gBuf);

    gShared = new (ptr) InterfaceTask();
    return gShared;
}



/**
 * @brief Launch the vendor interface task
 */
InterfaceTask::InterfaceTask() {
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<InterfaceTask *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, this->stack, &this->tcb);
}

/**
 * @brief Vendor interface task function
 *
 * This is the run function of the vendor interface task. We'll block waiting on notification bits
 * until the interface is enabled (when the host connects to the device) then enter a loop reading
 * from the endpoint to receive a header, followed by its payload.
 */
void InterfaceTask::main() {
    uint32_t note;
    BaseType_t ok;

    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0, TaskNotifyBits::All, &note,
                portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        /*
         * Ignore the notification bits for now; as long as the interface is active (as indicated
         * by the active flag, set by HostConnected() and HostDisconnected()) we'll sit and process
         * messages.
         */
        while(this->isActive) {
            // ensure vendor endpoint is connected
            if(!tud_vendor_n_mounted(kInterfaceIndex)) {
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            // if no readable data, wait (TODO: is this necessary?)
            if(!tud_vendor_n_available(kInterfaceIndex)) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            this->processMessage();
        }

        /*
         * We no longer wish to process USB messages. Reset any previously read data, and also
         * clear the state of any internal state.
         */
        tud_vendor_n_read_flush(kInterfaceIndex);
    }
}

/**
 * @brief Process a single message
 */
void InterfaceTask::processMessage() {
    uint32_t read, written;

    PacketHeader hdr;
    static etl::array<uint8_t, kMaxPayload> payload;
    static etl::array<uint8_t, kMaxPayload> response;

    // read packet header
    read = tud_vendor_n_read(kInterfaceIndex, &hdr, sizeof(hdr));
    if(read != sizeof(hdr)) {
        Logger::Warning("USB: %s (%d)", "failed to read vendor packet header", read);
        return;
    }

    hdr.payloadLength = __builtin_bswap16(hdr.payloadLength);

    // read packet payload
    if(hdr.payloadLength) {
        // validate length
        if(hdr.payloadLength > kMaxPayload) {
            // too much; discard remaining data
            Logger::Warning("USB: %s (%d)", "invalid payload length", hdr.payloadLength);
            tud_vendor_n_read_flush(kInterfaceIndex);
            return;
        }

        // read payload into payload buffer
        read = tud_vendor_n_read(kInterfaceIndex, payload.data(), hdr.payloadLength);
        if(read != hdr.payloadLength) {
            Logger::Warning("USB: %s (%d)", "failed to read vendor payload", read);
            return;
        }
    }

    // run type specific handler
    size_t replyBytes{0};

    switch(hdr.type) {
        case static_cast<uint8_t>(Endpoint::PropertyRequest):
            replyBytes = PropertyRequest::Handle(hdr, {payload.data(), hdr.payloadLength},
                    {response.data() + sizeof(hdr), response.size() - sizeof(hdr)});
            break;

        default:
            Logger::Warning("USB: received unknown packet (type %02x, tag %02x, len %u)",
                    hdr.type, hdr.tag, hdr.payloadLength);
            break;
    }

    // send response
    REQUIRE(replyBytes <= response.size()-sizeof(hdr), "reply too large (%u)", replyBytes);

    if(replyBytes) {
        // format the reply's header
        auto replyHdr = reinterpret_cast<PacketHeader *>(response.data());
        replyHdr->type = hdr.type;
        replyHdr->tag = hdr.tag;

        replyHdr->payloadLength = __builtin_bswap16(replyBytes);
        const auto responseSize = sizeof(*replyHdr) + replyBytes;

        // write it to host
        written = tud_vendor_n_write(kInterfaceIndex, response.data(), responseSize);
        if(written != responseSize) {
            Logger::Warning("*** Failed to send response: %u", written);
        }
    }
}
