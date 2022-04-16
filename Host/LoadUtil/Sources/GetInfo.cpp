/**
 * @file
 *
 * @brief Commands to get device information
 */
#include <string>
#include <string_view>

#include <rang.hpp>
#include <tabulate/table.hpp>

#include <LibLoad.h>



/**
 * @brief Get general information about a device
 *
 * Given a devie connection, query it for its hardware information (revision, connected
 * peripherals, etc) as well as software information. Then print that in a pretty table to the
 * terminal
 */
void GetInfo(LibLoad::Device *device) {
    // query hardware info

    // query software info
}
