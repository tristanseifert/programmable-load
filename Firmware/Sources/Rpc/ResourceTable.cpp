#include "remoteproc.h"

extern "C" {
/**
 * @brief Coprocessor resource table
 *
 * This table defines all of the interfaces (including RPC endpoints, trace log buffers, etc.) that
 * this firmware exposes.
 */
struct remote_resource_table __attribute__((section("resource_table"))) __attribute__((used)) rproc_resource{
    .version = 1,
    .num = 0,
    .reserved = {0, 0},
    .offset = { 0 },
};
};
