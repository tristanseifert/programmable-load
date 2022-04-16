#include <cstdlib>
#include <iostream>

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"
#include <tabulate/table.hpp>

#include <LibLoad.h>

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

        devices.add_row({"USB", serial});

        return true;
    });

    std::cout << "Connected programmable load devices:" << std::endl << devices << std::endl;
}

/**
 * @brief Program entry point
 *
 * This is the entry point for the utility. We'll parse the command line here to determine what
 * device to connect to, and what actions to perform.
 */
int main(int argc, const char **argv) {
    bool listDevices{false};

    InitLib();

    // parse arguments
    CLI::App app{"Utility for interfacing with programmable load"};

    app.add_flag("--list-devices", listDevices, "Print a list of all connected programmable loads");

    CLI11_PARSE(app, argc, argv);

    // do the needful
    if(listDevices) {
        PrintDeviceList();
    }
}
