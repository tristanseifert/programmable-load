#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stddef.h>
#include <stdint.h>
#include <etl/string_view.h>

#include <tusb.h>

namespace UsbStack {
/**
 * @brief USB device descriptors
 *
 * Encapsulates the fixed descriptors, required for USB enumeration.
 *
 * This is also where endpoints are allocated.
 */
class Descriptors {
    public:
        /**
         * @brief String descriptor indices
         *
         * Defines the indices of various USB-related strings presented as string descriptors to
         * the host. Most of these strings are read directly from flash.
         */
        enum StringDescriptor: uint8_t {
            /// Strings language ID
            Language                    = 0,
            /// Manufacturer name
            ManufacturerName            = 1,
            /// Product name
            ProductName                 = 2,
            /**
             * @brief CDC interface name
             *
             * Identifies the virtual serial port exposed by the device.
             */
            CdcInterfaceName            = 3,
            /**
             * @brief Vendor interface name
             *
             * Identifies the vendor-specific interface, used for custom host side tools to
             * communicate with the device.
             */
            VendorInterfaceName         = 4,

            /**
             * @brief Size of string table
             *
             * Number of string table indices to allocate space for. Requests for string indices
             * larger than this will be ignored.
             */
            MaxConstStringDescriptor,

            /**
             * @brief Serial number
             *
             * There is no corresponding string table entry for this value; instead, it 's read at
             * runtime from the system configuration block.
             */
            SerialNumber                = 10,
        };

        /**
         * @brief Interface indices
         */
        enum Interfaces: uint8_t {
            /**
             * @brief Vendor-specific interface
             *
             * This interface receives raw read/write packets, which are decoded according to our
             * custom binary protocol.
             */
            Vendor                      = 0,
            /// Total number of interfaces
            Total,
        };

        /**
         * @brief Endpoint indices
         *
         * Allocation of endpoints to various services/classes. Note that the ATSAMD5x devices can
         * support maximum of 8 endpoints in each direction.
         */
        enum Endpoints: uint8_t {
            /// CDC notification IN
            ConsoleNotifyIN             = 0x81,
            /// CDC data OUT
            ConsoleOUT                  = 0x02,
            /// CDC data IN
            ConsoleIN                   = 0x82,

            /// Vendor endpoint OUT
            VendorOUT                   = 0x03,
            /// Vendor endpoint IN
            VendorIN                    = 0x83,
        };

    public:
        /**
         * @brief USB Vendor ID
         *
         * This vendor ID corresponds to the one from [pid.codes](https://pid.codes), a repository
         * of USB PID assignments for open-source projects. Eventually, when this project is more
         * polished, we'll have to go and register the project.
         */
        constexpr static const uint16_t kVendorId{0x1209};

        /**
         * @brief USB Product ID
         *
         * Used to identify the device, for driver matching on the host.
         *
         * @note Currently, this is just a testing value. It needs to be replaced with a proper PID
         *       allocation at some point. It should only be used in test environments as it's not
         *       unique; [see here](https://pid.codes/1209/0009/) for details.
         *
         * @todo Register a proper PID from pid.codes.
         */
        constexpr static const uint16_t kProductId{0x0009};

        /**
         * @brief Device descriptor
         *
         * The device descriptor identifies the USB device to the host, including the strings
         * presented in the user interface. It also defines how many configurations are available,
         * which in turn define the implemented classes.
         */
        static const tusb_desc_device_t kDeviceDescriptor;

        /**
         * @brief Number of string descriptors
         *
         * Defines the size of the string descriptor table.
         */
        constexpr static const size_t kNumStringDescriptors{5};

        /**
         * @brief String descriptor table
         *
         * Various USB descriptors refer back to strings in this table. Each string is an UTF-16
         * null-terminated.
         *
         * @remark Each string must be prefixed with the string descriptor identifier: that is, a
         *         0x0300 | (strlen(str)+1). Keep in mind this needs to be specified as byte-
         *         swapped, so you should specify something like \x0503.
         */
        constexpr static const etl::u16string_view kStrings[kNumStringDescriptors]{
            // 0: Language index (English)
            {u"\x0302""\x0904"},
            // 1. Manufacturer name
            {u"\x030C""Trist"},
            // 2. Product name
            {u"\x0324""Programmable Load"},
            // 3. CDC interface name
            {u"\x031A""Debug Console"},
            // 4. Vendor interface name
            {u"\x031E""Spicy Interface"},
        };

        /**
         * @brief Number of configurations
         *
         * Defines how many configurations the device exposes, and in turn, the number of device
         * configuration descriptors.
         */
        constexpr static const size_t kNumConfigDescriptors{1};

        /**
         * @brief Default device configuration
         *
         * USB configuration descriptor for the device's normal operating mode.
         */
        static const uint8_t kDefaultCfgDescriptor[];

        /**
         * @brief Configuration descriptors
         *
         * An array containing the addresses of USB configuration descriptors. Each configuration
         * in turn specifies the available interfaces and their endpoints.
         */
        constexpr static const uint8_t *kConfigurations[kNumConfigDescriptors]{
            kDefaultCfgDescriptor,
        };

    private:
};
}

#endif
