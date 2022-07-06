#ifndef RPC_RESOURCETABLE_H
#define RPC_RESOURCETABLE_H

#include <openamp/open_amp.h>

// these are defined by the linker script
extern "C" int __OPENAMP_region_start__[], __OPENAMP_region_end__[];

#define SHM_START_ADDRESS       ((metal_phys_addr_t) __OPENAMP_region_start__)
#define SHM_SIZE                (size_t)((uint8_t *) __OPENAMP_region_end__ - (uint8_t *) __OPENAMP_region_start__)
#define VRING_BUF_ADDRESS       (SHM_START_ADDRESS + 0x2000)

namespace Rpc {
/**
 * @brief Thin wrapper around the remoteproc resource table
 *
 * The resource table is a concept coming from the OpenAMP firmware; it's used to define the
 * buffers and virtio interfaces to communicate between the host (Linux) and us.
 */
class ResourceTable {
    public:
        static struct resource_table &GetTable();
        static void *GetTablePtr();
        static const size_t GetTableSize();

        static struct fw_rsc_vdev &GetVdev();
        static struct fw_rsc_vdev_vring &GetVring0();
        static struct fw_rsc_vdev_vring &GetVring1();
};
}

#endif
