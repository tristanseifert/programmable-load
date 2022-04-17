#ifndef USB_VENDOR_VENDORINTERFACETASK_H
#define USB_VENDOR_VENDORINTERFACETASK_H

#include <stddef.h>
#include <stdint.h>

#include <etl/string_view.h>

#include "Rtos/Rtos.h"

namespace UsbStack::Vendor {
/**
 * @brief Vendor USB interface task
 *
 * Handles sending and receiving packets on the vendor-specific endpoint on the USB device. This
 * endpoint is used for basically all communication with the device. Messages have a small, fixed
 * 4 byte header, and are followed by a payload up to 60 bytes in length.
 *
 * The format of a message payload is defined by its type, though most smaller messages will be
 * encoded with [CBOR](https://cbor.io) to simplify things.
 */
class InterfaceTask {
    public:
        /**
         * @brief Task notification values
         */
        enum TaskNotifyBits: uint32_t {
            /// USB connectivity state changed
            ConnectivityStateChanged    = (1 << 0),

            All                         = (ConnectivityStateChanged),
        };

        /**
         * @brief Message endpoints
         */
        enum class Endpoint: uint8_t {
            /// Read and write properties
            PropertyRequest             = 0x01,
        };

        /**
         * @brief USB packet header
         *
         * All vendor packets are prefixed with this simple 4 byte header. All multibyte values
         * should be stored in big endian order.
         */
        struct PacketHeader {
            /**
             * @brief Message type
             *
             * Defines the format of the content of the message. Each type is associated with a
             * specific type of handler.
             */
            uint8_t type;

            /**
             * @brief Message tag
             *
             * The tag value is used to match up a request to corresponding response from the
             * device.
             */
            uint8_t tag;

            /**
             * @brief Payload length (bytes)
             *
             * If nonzero, this is the number of payload data bytes that follow immediately after
             * the packet header.
             */
            uint16_t payloadLength;
        } __attribute__((packed));

    public:
        static InterfaceTask *Start();

        /**
         * @brief USB host enumerated device
         *
         * The device has been enumerated by the host, so prepare the vendor endpoint for being
         * connected to.
         */
        static void HostConnected() {
            gShared->isActive = true;
            __DSB();
            NotifyTask(TaskNotifyBits::ConnectivityStateChanged);
        }

        /**
         * @brief USB host disconnected from device
         *
         * Reset any state associated with the vendor interface, so that when the host reconnects,
         * we'll be in a clean state.
         */
        static void HostDisconnected() {
            gShared->isActive = false;
            __DSB();
            NotifyTask(TaskNotifyBits::ConnectivityStateChanged);
        }

        /**
         * @brief Send a notification
         *
         * Notify the UI task something happened.
         *
         * @brief bits Notification bits to set
         */
        static inline void NotifyTask(const TaskNotifyBits bits) {
            xTaskNotifyIndexed(gShared->task, kNotificationIndex, bits, eSetBits);
        }

    private:
        InterfaceTask();

        void main();
        void processMessage();

    private:
        /// Priority level for the task
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::AppLow};
        /// Size of the stack, in words
        static const constexpr size_t kStackSize{400};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"USBVendorIntf"};
        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

        /// USB vendor interface index
        static const constexpr size_t kInterfaceIndex{0};
        /// Maximum packet payload size
        static const constexpr size_t kMaxPayload{512};

        /// Static task instance
        static InterfaceTask *gShared;

        /// Task handle
        TaskHandle_t task;
        /// Task information structure
        StaticTask_t tcb;
        /// Preallocated stack for the task
        StackType_t stack[kStackSize];

        /// Whether the vendor interface is enabled
        bool isActive{false};
};
}

#endif
