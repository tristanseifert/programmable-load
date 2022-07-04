#ifndef REMOTEPROC_H
#define REMOTEPROC_H

#include <stdint.h>

/**
 * struct resource_table - firmware resource table header
 * @ver: version number
 * @num: number of resource entries
 * @reserved: reserved (must be zero)
 * @offset: array of offsets pointing at the various resource entries
 *
 * The header of the resource table, as expressed by this structure,
 * contains a version number (should we need to change this format in the
 * future), the number of available resource entries, and their offsets
 * in the table.
 */
struct remote_resource_table {
    uint32_t version;
    uint32_t num;
    uint32_t reserved[2];
    uint32_t offset[1];
} __attribute__((packed));

#endif
