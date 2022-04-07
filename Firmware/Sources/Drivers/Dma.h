#ifndef DRIVERS_DMA_H
#define DRIVERS_DMA_H

#include <stddef.h>
#include <stdint.h>

#include <vendor/sam.h>

namespace Drivers {
/**
 * @brief DMA Controller driver
 *
 * Provides an interface to the processor's internal 32-channel DMA controller. It encapsulates the
 * required memory allocations for DMA descriptor buffers.
 *
 * @remark Only a subset of all 32 channels may be initialized to save memory.
 */
class Dma {
    public:
        /**
         * @brief Total number of available DMA channels
         *
         * Maximum number of DMA channels that are actually enabled, implemented and may be used
         * by application code. A smaller number than the full 32 can be used to reduce the .bss
         * requirements (for descriptors and their writeback area)
         */
        constexpr static const size_t kNumChannels{8};
        static_assert(kNumChannels <= 32, "invalid maximum channel count");

    public:
        Dma() = delete;

        static void Init();

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
};
}

#endif
