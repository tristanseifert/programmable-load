#ifndef RPC_OPENAMP_H
#define RPC_OPENAMP_H

#include <etl/string_view.h>

namespace Rpc {
/**
 * @brief OpenAMP driver
 *
 * A wrapper around OpenAMP, glues together with the IPCC mailbox to provide the virtio message
 * exchanging with the host side.
 */
class OpenAmp {
    public:
        /**
         * @brief Initialize OpenAMP
         *
         * @remark Assumes that the IPCC mailbox has already been set up and fully configured at
         *         the time of this call.
         */
        static void Init() {
            InitLibMetal();
            InitVdev();
        }

        /**
         * @brief Get the rpmsg device
         *
         * Returns the rpmsg device used to communicate with the host
         */
        static inline auto &GetRpmsgDev() {
            return gRpmsgDev;
        }

    private:
        static void InitLibMetal();
        static void MtlLogHandler(enum metal_log_level, const char *, ...);

        static void InitVdev();

    private:
        /// Name for the shared memory device
        constexpr static const etl::string_view kShmDeviceName{"STM32_SHM"};
        /// Shared memory device definition
        static struct metal_device gShmDevice;

        /// Physical mapped address of the shared memory region
        static metal_phys_addr_t gShmPhysmap;
        /// IO region for shared memory
        static metal_io_region *gShmIo;
        /// Physical mapped address of resource table
        static metal_phys_addr_t gRscPhysmap;
        /// IO region for resource table
        static metal_io_region *gRscIo;

        /// Virtio device used for rpmsg
        static struct virtio_device *gVdev;

        /// shared memory pool (used for vring buffers)
        static struct rpmsg_virtio_shm_pool gShpool;
        /// rpmsg device wrapper for the underlying virtio device
        static struct rpmsg_virtio_device gRpmsgDev;
};
}

#endif
