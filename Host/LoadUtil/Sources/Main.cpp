#include <cstdlib>
#include <iostream>

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"
#include <fmt/format.h>
#include <rang.hpp>
#include <tabulate/table.hpp>

#include <LibLoad.h>

extern void GetInfo(LibLoad::Device *device);

/**
 * @brief Initialize the load library
 *
 * We'll initialize the library (which in turn sets up USB and friends) and register exit handlers
 * so the library is deinitialized on shutdown.
 */
static void InitLib() {
    LibLoad::Init();

    atexit([](){
        LibLoad::DeInit();
    });
}

/**
 * @brief List all connected devices
 *
 * Enumerate via LibLoad to get all connected devices.
 */
static void PrintDeviceList() {
    tabulate::Table devices;
    devices.add_row({"Connection", "Serial"});
    for(size_t i = 0; i < 2; i++) {
        devices[0][i].format().font_align(tabulate::FontAlign::center)
            .font_style({tabulate::FontStyle::bold});
    }

    LibLoad::EnumerateDevices([&](auto device) -> bool {
        std::string serial(device.serial);

        switch(device.method) {
            case LibLoad::DeviceInfo::ConnectionMethod::Usb:
                devices.add_row({"USB", serial});
                break;

            default:
                devices.add_row({"(unknown)", serial});
                break;
        }

        return true;
    });

    std::cout << rang::style::bold << "Connected programmable load devices:"
        << rang::style::reset << std::endl << devices << std::endl;
}

/**
 * @brief Program entry point
 *
 * This is the entry point for the utility. We'll parse the command line here to determine what
 * device to connect to, and what actions to perform.
 */
int main(int argc, const char **argv) {
    std::string serial;

    // initialize
    InitLib();

    CLI::App app{"Utility for interfacing with programmable load"};

    // options for connecting to a device
    auto connectGroup = app.add_option_group("device", "Define what device to connect to");

    connectGroup->add_option("--device-sn,-S", serial, "Device serial number");

    // informational commands
    app.add_subcommand("list-devices", "Print a list of all connected programmable loads")
        ->callback([](){
        PrintDeviceList();
    })->excludes(connectGroup);

    app.add_subcommand("info", "Print detailed information about a single device")
        ->callback([&](){
        auto dev = LibLoad::Connect(serial);
        if(dev) {
            GetInfo(dev);
        } else {
            std::cerr << rang::fg::red
                << fmt::format("Failed to connect to device S/N '{}'", serial)
                << rang::style::reset << std::endl;
        }
    })->needs(connectGroup);

    // perform parsing
    app.require_subcommand(1);
    CLI11_PARSE(app, argc, argv);
}
