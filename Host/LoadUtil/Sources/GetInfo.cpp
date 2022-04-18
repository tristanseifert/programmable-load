/**
 * @file
 *
 * @brief Commands to get device information
 */
#include <iostream>
#include <string>
#include <string_view>

#include <fmt/format.h>
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
    uint32_t maxVoltage{static_cast<uint32_t>(-1)}, maxCurrent{static_cast<uint32_t>(-1)};

    // query hardware info
    device->propertyRead(LibLoad::Device::Property::HwVersion, hwVersion);
    device->propertyRead(LibLoad::Device::Property::MaxVoltage, maxVoltage);
    device->propertyRead(LibLoad::Device::Property::MaxCurrent, maxCurrent);

    // query software info
    device->propertyRead(LibLoad::Device::Property::SwVersion, swVersion);

    // make a pretty table
    tabulate::Table table;

    table.add_row({"Serial", device->getSerialNumber()});
    table.add_row({"Hardware Version", hwVersion.empty() ? "(unknown)" : hwVersion});
    table.add_row({"Software Version", swVersion.empty() ? "(unknown)" : swVersion});

    table.add_row({"Maximum Voltage", (maxVoltage != -1) ? fmt::format("{:.2} V",
                static_cast<double>(maxVoltage) / 1000.) : "(unknown)"});
    table.add_row({"Maximum Current", (maxCurrent != -1) ? fmt::format("{:.2} A",
                static_cast<double>(maxCurrent) / 1000.) : "(unknown)"});

    for(auto &cell : table.column(0)) {
        cell.format().font_align(tabulate::FontAlign::right)
            .font_style({tabulate::FontStyle::bold});
    }

    std::cout << table << std::endl;
}
