#ifndef DRIVERS_DMA_H
#define DRIVERS_DMA_H

#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>
#include <vendor/sam.h>

#include "Rtos/Rtos.h"

extern "C" {
void DMAC_0_Handler();
void DMAC_1_Handler();
void DMAC_2_Handler();
void DMAC_3_Handler();
void DMAC_4_Handler();
}

namespace Drivers {
/**
 * @brief DMA Controller driver
 *
 * Provides an interface to the processor's internal 32-channel DMA controller. It encapsulates the
 * required memory allocations for DMA descriptor buffers.
 *
 * @remark Only a subset of all 32 channels may be initialized to save memory.
 *
 * @remark The linked transfer descriptor feature is not implemented. DMA transfers are always
 *         done with a single descriptor.
 */
class Dma {
    friend void ::DMAC_0_Handler();
    friend void ::DMAC_1_Handler();
    friend void ::DMAC_2_Handler();
    friend void ::DMAC_3_Handler();
    friend void ::DMAC_4_Handler();

    public:
        enum Errors: int {
            /**
             * @brief DMA transfer failed
             *
             * Some error was raised by the DMA controller during this transfer, likely a bus error
             * during the read/write.
             */
            TransferError               = -300,

            /**
             * @brief Transfer is too long
             *
             * The transfer cannot be performed as described in a single transaction because it is
             * too long.
             */
            TooLong                     = -301,

            /**
             * @brief Transfer size is unaligned
             *
             * The length of the transfer is not an exact multiple of the beat size. The DMA can
             * only perform whole beat transfers.
             */
            LengthBeatMismatch          = -302,

            /**
             * @brief Failed to block on transfer
             *
             * Something went wrong while trying to block on a DMA transfer completion.
             */
            BlockError                  = -303,

            /**
             * @brief Invalid descriptor
             *
             * The DMA transfer tried to submit an invalid transfer descriptor.
             */
            InvalidDescriptor           = -304,
        };

        /**
         * @brief Size of a single DMA beat
         *
         * The beat is the smallest unit of a DMA transfer, akin to a single bus cycle. This
         * defines the size of a single beat.
         */
        enum class BeatSize: uint8_t {
            /// Transfer one byte per beat
            Byte                        = 0x0,
            /// Transfer two bytes per beat
            HalfWord                    = 0x1,
            /// Transfer four bytes per beat
            Word                        = 0x2,
        };

        /**
         * @brief Transfer FIFO threshold
         *
         * These are values for CHCTRLA.THRESHOLD, and define the number of beat transfers from the
         * source to complete before writing to the destination.
         */
        enum class FifoThreshold: uint8_t {
            x1                          = 0x0,
            x2                          = 0x1,
            x4                          = 0x2,
            x8                          = 0x3,
        };

        /**
         * @brief Transfer trigger action
         *
         * Define what happens when an external trigger is received for a DMA channel.
         */
        enum class TriggerAction: uint8_t {
            /// Transfer one block per trigger
            Block                       = 0x0,
            /// Transfer one burst per trigger
            Burst                       = 0x2,
            /// Transfer one transaction per trigger
            Transaction                 = 0x3,
        };

        /**
         * @brief Total number of available DMA channels
         *
         * Maximum number of DMA channels that are actually enabled, implemented and may be used
         * by application code. A smaller number than the full 32 can be used to reduce the .bss
         * requirements (for descriptors and their writeback area)destination
         */
        constexpr static const size_t kNumChannels{8};
        static_assert(kNumChannels <= 32, "invalid maximum channel count");

    public:
        Dma() = delete;

        static void Init();

        static void ConfigureChannel(const uint8_t channel, const FifoThreshold threshold,
                const uint8_t burstLength, const TriggerAction trigger,
                const uint8_t triggerSource = 0, const uint8_t priority = 0);
        static void EnableChannel(const uint8_t channel);
        static void DisableChannel(const uint8_t channel);
        static void ResetChannel(const uint8_t channel);

        static int ConfigureTransfer(const uint8_t channel, const BeatSize size,
                const void *source, const bool srcIncrement, void *destination,
                const bool destIncrement, const size_t transferLength);

        static void Trigger(const uint8_t channel);
        static int WaitForCompletion(const uint8_t channel);

    private:
        static void HandleIrq(const uint8_t channel);
        static void SignalChannelComplete(const uint8_t channel, const int reason = 0);

    private:
        /**
         * @brief Transfer descriptors buffer
         *
         * Contains the first transfer descriptor for each of the DMA channels.
         */
        static DmacDescriptor gDescriptors[kNumChannels];

        /**
         * @brief Writeback descriptor buffers
         *
         * When a DMA transfer is interrupted (or suspended by software) its current state is
         * stored in the appropriate slot in the writeback descriptor buffers.
         */
        static DmacDescriptor gWritebackDescriptors[kNumChannels];

        /**
         * @brief Tasks blocked on a DMA transfer
         *
         * This array contains a task handle, one for each DMA channel, corresponding to the task
         * that requested the DMA transfer (and is waiting for its completion.)
         */
        static etl::array<TaskHandle_t, kNumChannels> gBlockedTasks;

        /**
         * @brief DMA transfer completion reason
         *
         * Filled in by the interrupt handlers, immediately before notifying blocked task that a
         * transfer completed.
         */
        static etl::array<int, kNumChannels> gCompletionReason;
};
}

#endif
