#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "stm32mp1xx.h"

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>

#include <stdarg.h>

#include "OpenAmp.h"
#include "Mailbox.h"
#include "ResourceTable.h"

using namespace Rpc;

/**
 * @brief Shared memory device
 *
 * This is used to access and expose our shared memory regions to OpenAMP.
 */
struct metal_device OpenAmp::gShmDevice = {
    .name = kShmDeviceName.data(),
    .num_regions = 2,
    .regions = {
        {.virt = NULL}, /* shared memory */
        {.virt = NULL}, /* rsc_table memory */
    },
    .node = { NULL },
    .irq_num = 0,
    .irq_info = NULL
};

metal_phys_addr_t OpenAmp::gShmPhysmap;
metal_io_region *OpenAmp::gShmIo{nullptr};
metal_phys_addr_t OpenAmp::gRscPhysmap;
metal_io_region *OpenAmp::gRscIo{nullptr};

struct virtio_device *OpenAmp::gVdev{nullptr};
struct rpmsg_virtio_shm_pool OpenAmp::gShpool;
struct rpmsg_virtio_device OpenAmp::gRpmsgDev;


/**
 * @brief Initialize LibMetal
 *
 * This performs all LibMetal related initialization (to the base library) as well as our metal
 * devices for shared memory io.
 */
void OpenAmp::InitLibMetal() {
    int err;
    struct metal_device *device{nullptr};
    struct metal_init_params mtlParams = METAL_INIT_DEFAULTS;

    // libmetal init
    mtlParams.log_handler = MtlLogHandler;
    mtlParams.log_level = METAL_LOG_DEBUG;
    //mtlParams.log_level = METAL_LOG_NOTICE;
    metal_init(&mtlParams);

    // register our generic shared memory device and open it
    err = metal_register_generic_device(&gShmDevice);
    REQUIRE(!err, "%s failed: %d", "metal_register_generic_device", err);

    err = metal_device_open("generic", kShmDeviceName.data(), &device);
    REQUIRE(!err, "%s failed: %d", "metal_device_open", err);

    // map the shared memory region
    gShmPhysmap = SHM_START_ADDRESS;
    metal_io_init(&device->regions[0], reinterpret_cast<void *>(SHM_START_ADDRESS), &gShmPhysmap,
                SHM_SIZE, -1, 0, nullptr);

    Logger::Debug("shm region at %p (%lu bytes)", gShmPhysmap, SHM_SIZE);

    gShmIo = metal_device_io_region(device, 0);
    REQUIRE(gShmIo, "%s failed: %d", "metal_device_io_region", 0);

    // map resource table region
    gRscPhysmap = reinterpret_cast<uintptr_t>(ResourceTable::GetTablePtr());
    metal_io_init(&device->regions[1], ResourceTable::GetTablePtr(), &gRscPhysmap,
            ResourceTable::GetTableSize(), -1, 0, nullptr);

    gRscIo = metal_device_io_region(device, 1);
    REQUIRE(gShmIo, "%s failed: %d", "metal_device_io_region", 1);
}

/**
 * @brief LibMetal log handler
 *
 * Forwards the specified logging message to our logging system.
 */
void OpenAmp::MtlLogHandler(enum metal_log_level level, const char *fmt, ...) {
    va_list va;

    va_start(va, fmt);

    // invoke the appropriate routine for the log level
    Logger::Level lvl{Logger::Level::Error};

    switch(level) {
        case METAL_LOG_EMERGENCY: [[fallthrough]];
        case METAL_LOG_ALERT: [[fallthrough]];
        case METAL_LOG_CRITICAL: [[fallthrough]];
        case METAL_LOG_ERROR:
            lvl = Logger::Level::Error;
            break;
        case METAL_LOG_WARNING:
            lvl = Logger::Level::Warning;
            break;
        case METAL_LOG_NOTICE: [[fallthrough]];
        case METAL_LOG_INFO:
            lvl = Logger::Level::Notice;
            break;
        case METAL_LOG_DEBUG:
            lvl = Logger::Level::Debug;
            break;

        // unknown messages are logged as errors
        default:
            break;
    }

    Logger::Log(lvl, fmt, va);

    va_end(va);
}



/**
 * @brief Initialize the rpmsg vdev and register it
 *
 * Create the virtio device, as well as the two associated vrings used for rpmsg communication with
 * the host.
 */
void OpenAmp::InitVdev() {
    int err;
    struct fw_rsc_vdev_vring *vi;

    // create vdev
    gVdev = rproc_virtio_create_vdev(RPMSG_REMOTE, 7, &ResourceTable::GetVdev(), gRscIo, nullptr,
            Mailbox::Notify, nullptr);
    REQUIRE(gVdev, "%s failed", "rproc_virtio_create_vdev");

    Logger::Trace("vdev created %p", gVdev);
    rproc_virtio_wait_remote_ready(gVdev);
    Logger::Trace("remote ready!");

    // create vring0
    vi = &ResourceTable::GetVring0();
    Logger::Trace("vring%u @ %p", 0, vi->da);

    err = rproc_virtio_init_vring(gVdev, 0, vi->notifyid, reinterpret_cast<void *>(vi->da), gShmIo,
            vi->num, vi->align);
    REQUIRE(!err, "%s failed: %d", "rproc_virtio_init_vring", err);

    // create vring1
    vi = &ResourceTable::GetVring1();
    Logger::Trace("vring%u @ %p", 1, vi->da);

    err = rproc_virtio_init_vring(gVdev, 1, vi->notifyid, reinterpret_cast<void *>(vi->da), gShmIo,
            vi->num, vi->align);
    REQUIRE(!err, "%s failed: %d", "rproc_virtio_init_vring", err);

    // create shared memory pool (vdev0buffer); note the size is hardcoded for now
    rpmsg_virtio_init_shm_pool(&gShpool, reinterpret_cast<void *>(VRING_BUF_ADDRESS), 0xA000);

    // initialize the rpmsg vdev
    rpmsg_init_vdev(&gRpmsgDev, gVdev, [](auto rdev, auto name, auto dest) {
        Logger::Debug("rpmsg ns: %s = %08x", name, dest);
    }, gShmIo, &gShpool);
}
