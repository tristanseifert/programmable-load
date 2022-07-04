#ifndef RPC_RESOURCETABLE_H
#define RPC_RESOURCETABLE_H

#include <openamp/open_amp.h>

namespace Rpc {
/**
 * @brief Thin wrapper around the remoteproc resource table
 *
 * The resource table is a concept coming from the OpenAMP firmware; it's used to define the
 * buffers and virtio interfaces to communicate between the host (Linux) and us.
 */
class ResourceTable {
    public:
        static const struct fw_rsc_vdev &GetVdev();
        static const struct fw_rsc_vdev_vring &GetVring0();
        static const struct fw_rsc_vdev_vring &GetVring1();
};
}

#endif
