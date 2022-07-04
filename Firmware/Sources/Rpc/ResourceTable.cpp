#include <stdint.h>

#include <openamp/open_amp.h>

#include "ResourceTable.h"
#include "Log/Logger.h"

using namespace Rpc;

/**
 * @brief Firmware resource table definition
 *
 * This is a "wrapper" for the resource table's header, as well as the associated content data
 * structs.
 */
struct FwResourceTable {
    uint32_t version;
    // number of entries
    uint32_t num;
    uint32_t reserved[2];
    uint32_t offset[2];

    // trace log buffer
    struct fw_rsc_trace dbgTrace;

    // rpmsg buffers (virtio)
    struct fw_rsc_vdev vdev;
    struct fw_rsc_vdev_vring vring0;
    struct fw_rsc_vdev_vring vring1;
} __attribute__((packed));

/// total number of VRings (fixed for linux compatibility)
constexpr static const uint32_t kNumVRings{2};
/// alignment of vring buffers (fixed for linux compatibility)
constexpr static const uint32_t kVRingAlignment{4};
/// number of vring buffers
constexpr static const uint32_t kVRingNumBufs{8}; // may be up to 8?

/// Identifier for the master-to-remote ring
constexpr static const uint32_t kVRingIdMasterToRemote{0};
/// Identifier for the remote-to-master ring
constexpr static const uint32_t kVRingIdRemoteToMaster{1};

// these are defined by the linker script
extern int __OPENAMP_region_start__[], __OPENAMP_region_end__[];

#define SHM_START_ADDRESS       ((metal_phys_addr_t) __OPENAMP_region_start__)
#define SHM_SIZE                (size_t)((void *) __OPENAMP_region_end__ - (void *) __OPENAMP_region_start__)

#define VRING_RX_ADDRESS        SHM_START_ADDRESS
#define VRING_TX_ADDRESS        (SHM_START_ADDRESS + 0x400)

extern "C" {
/**
 * @brief Coprocessor resource table
 *
 * This table defines all of the interfaces (including RPC endpoints, trace log buffers, etc.) that
 * this firmware exposes.
 */
struct FwResourceTable __attribute__((section(".resource_table"))) __attribute__((used)) rproc_resource{
    .version = 1,
    .num = 2,
    .reserved = {0, 0},
    .offset = {
        offsetof(struct FwResourceTable, dbgTrace),
        offsetof(struct FwResourceTable, vdev),
    },

    // trace log buffer
    .dbgTrace = {
        .type = RSC_TRACE,
        .da = reinterpret_cast<uint32_t>(Logger::gTraceBuffer),
        .len = Logger::kTraceBufferSize,
        .reserved = 0,
        .name = "cm4_log",
    },

    /**
     * RPMSG vdev device entry
     */
    .vdev = {
        .type = RSC_VDEV,
        // virtio RPMSG device id (VIRTIO_ID_RPMSG_)
        .id = 7,
        .notifyid = 0,
        // RPMSG_IPU_C0_FEATURES
        .dfeatures = 1,
        .gfeatures = 0,
        .config_len = 0,
        .status = 0,
        .num_of_vrings = kNumVRings,
        .reserved = {0, 0},
    },
    // vring 0
    .vring0 = {
        .da = static_cast<uint32_t>(-1),
        //.da = static_cast<uint32_t>(VRING_TX_ADDRESS),
        .align = kVRingAlignment,
        .num = kVRingNumBufs,
        .notifyid = kVRingIdMasterToRemote,
        .reserved = 0,
    },
    // vring 1
    .vring1 = {
        .da = static_cast<uint32_t>(-1),
        //.da = static_cast<uint32_t>(VRING_RX_ADDRESS),
        .align = kVRingAlignment,
        .num = kVRingNumBufs,
        .notifyid = kVRingIdRemoteToMaster,
        .reserved = 0,
    },
};
};

/// Get reference to the vdev information structure
const struct fw_rsc_vdev &ResourceTable::GetVdev() {
    return rproc_resource.vdev;
}
/// Get reference to the vring0 (tx direction)
const struct fw_rsc_vdev_vring &ResourceTable::GetVring0() {
    return rproc_resource.vring0;
}
/// Get reference to the vring1 (rx direction)
const struct fw_rsc_vdev_vring &ResourceTable::GetVring1() {
    return rproc_resource.vring1;
}

