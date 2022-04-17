/**
 * @file
 *
 * @brief Commands to get device information
 */
#include <iostream>
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
    std::string hwVersion, swVersion;

    // query hardware info
    device->propertyRead(LibLoad::Device::Property::HwVersion, hwVersion);

    // query software info
    device->propertyRead(LibLoad::Device::Property::SwVersion, swVersion);

    // make a pretty table
    tabulate::Table table;

    table.add_row({"Serial", device->getSerialNumber()});
    table.add_row({"Hardware Version", hwVersion.empty() ? "(unknown)" : hwVersion});

    table.add_row({"Software Version", swVersion.empty() ? "(unknown)" : swVersion});

    for(auto &cell : table.column(0)) {
        cell.format().font_align(tabulate::FontAlign::right)
            .font_style({tabulate::FontStyle::bold});
    }

    std::cout << table << std::endl;
}
