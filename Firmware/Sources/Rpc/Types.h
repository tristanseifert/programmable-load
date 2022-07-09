#ifndef RPC_TYPES_H
#define RPC_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define kRpcVersionLatest 0x0100

/**
 * @brief RPC message header
 *
 * All RPC messages carried over the rpmsg interface use this. The meaning of the message type
 * field varies between endpoints, however type 0 is always a no-op.
 */
struct rpc_header {
    /// protocol version: use kRpcVersionLatest
    uint16_t version;
    /// total length of message, in bytes (including this header)
    uint16_t length;

    /// message endpoint
    uint8_t type;
    /// message tag: used to identify its response
    uint8_t tag;
    /// flags: currently only 0x01 is defined, which is set for replies to requests
    uint8_t flags;

    /// reserved, not currently used: set to 0
    uint8_t reserved;

    /// optional message payload (dependant upon tag)
    uint8_t payload[];
} __attribute__((packed));

#endif
