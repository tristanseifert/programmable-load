#ifndef RPC_MAILBOX_H
#define RPC_MAILBOX_H

extern "C" {
void IPCC_RX1_IRQHandler();
void IPCC_TX1_IRQHandler();
}

namespace Rpc {
/**
 * @brief RPC mailbox (IPCC) driver
 *
 * Provides an interface around the HAL IPCC driver to work with the OpenAMP framework; it works
 * in the interrupt driven mode using HAL callbacks.
 *
 * @TODO: Drop the HAL dependency
 */
class Mailbox {
    friend void ::IPCC_RX1_IRQHandler();
    friend void ::IPCC_TX1_IRQHandler();

    public:
        static void Init();

        static int ProcessDeferredIrq(struct virtio_device *vdev);
        static int Notify(void *priv, const uint32_t id);

        /**
         * @brief Set the task to receive mailbox notifications
         *
         * @param task Task handle to receive direct-to-task notifications for mailbox ISRs
         * @param index Notification index to modify
         * @param bits Notification bits to set for mailbox interrupt event
         */
        static inline void SetDeferredIsrHandler(TaskHandle_t task, const size_t index,
                const uintptr_t bits) {
            gNotifyIndex = index;
            gNotifyBits = bits;
            // update this last, as the ISR uses it as a flag to see if we should notify
            gNotifyTask = task;
        }

    private:
        /// State of an IPCC channel
        enum class ChannelStatus: uint8_t {
            /// No events
            Idle                        = 0,
            /// A receive buffer has been released
            RxBufferFreed               = 1,
            /// A new message has been received
            RxBufferAvailable           = 2,
        };

        /// configuration handle for IPCC
        static struct __IPCC_HandleTypeDef gHandle;
        /// number of times an irq for each channel was missed
        static size_t gMissedIrqs[2];
        /// state of each of the channels we monitor
        static ChannelStatus gStatus[2];

        /// Task handle to notify when we receive an irq
        static TaskHandle_t gNotifyTask;
        /// Bits to set in the notify task
        static uintptr_t gNotifyBits;
        /// Notify index to set
        static size_t gNotifyIndex;

        static void InstallCallbacks();
};
}

#endif
