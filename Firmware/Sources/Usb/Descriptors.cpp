#include "Descriptors.h"
#include "Usb.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"
#include "Util/HwInfo.h"

#include <tusb.h>
#include <string.h>

using namespace UsbStack;

const tusb_desc_device_t Descriptors::kDeviceDescriptor{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = TUSB_CLASS_VENDOR_SPECIFIC,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    // see https://f055.io
    .idVendor           = Descriptors::kVendorId,
    .idProduct          = Descriptors::kProductId,
    .bcdDevice          = 0x0100,

    // string table indices
    .iManufacturer      = StringDescriptor::ManufacturerName,
    .iProduct           = StringDescriptor::ProductName,
    .iSerialNumber      = StringDescriptor::SerialNumber,

    // number of configurations. we have just one
    .bNumConfigurations = Descriptors::kNumConfigDescriptors,
};

/**
 * @brief Read device descriptor
 *
 * Invoked when the USB stack receives a "GET DEVICE DESCRIPTOR" message from the host.
 */
extern "C" uint8_t const * tud_descriptor_device_cb() {
    return reinterpret_cast<const uint8_t *>(&Descriptors::kDeviceDescriptor);
}

/**
 * @brief Read string descriptor
 *
 * Gets a string out of the string table. Most strings are read directly from flash, with the
 * exception of the following:
 *
 * - Serial number: Read out of the system configuration manager
 *
 * @param index Index into string descriptor table
 * @param langId Language identifier for the strings (currently ignored)
 */
extern "C" uint16_t const *tud_descriptor_string_cb(const uint8_t index, const uint16_t langId) {
    // handle special cases
    switch(index) {
        /*
         * Device serial number
         *
         * The serial number is read out of the system configuration block, and converted to UTF-16
         * if this is the first time this call is made.
         */
        case Descriptors::StringDescriptor::SerialNumber: {
            constexpr static const size_t kSerialDescriptorLen{24};
            static uint16_t gSerialDescriptor[kSerialDescriptorLen];
            static bool gSerialDescriptorValid{false};

            if(!gSerialDescriptorValid) {
                memset(gSerialDescriptor, 0, sizeof(gSerialDescriptor));

                const auto serialStr = Util::HwInfo::GetSerial();
                REQUIRE(serialStr, "failed to get serial");

                const auto len = strlen(serialStr);
                REQUIRE(len < (kSerialDescriptorLen-2), "serial too long");

                gSerialDescriptor[0] = 0x0300 | (2 + (len * 2));

                for(size_t i = 0; i < len; i++) {
                    gSerialDescriptor[1+i] = serialStr[i];
                }

                gSerialDescriptorValid = true;
            }

            return gSerialDescriptor;
        }
    }

    // bail if out of range
    if(index >= Descriptors::kNumStringDescriptors) {
        Logger::Warning("request for invalid USB %s: $%02x", "string descriptor", index);
        return nullptr;
    }

    // simply secrete the string; it will have the identifier/length already prepared
    const auto &str = Descriptors::kStrings[index];
    return reinterpret_cast<const uint16_t *>(str.data());
}



/*
 * Default configuration descriptor
 *
 * This advertises the device itself, as well as the following services:
 *
 * - Vendor specific interface
 */
constexpr static const size_t kDefaultCfgDescriptorLen{
    TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN
};

const uint8_t Descriptors::kDefaultCfgDescriptor[]{
    // config descriptor header
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, Interfaces::Total, 0, kDefaultCfgDescriptorLen,
            TUSB_DESC_CONFIG_ATT_SELF_POWERED, 0),

    // Vendor: Custom mailbox interface for binary protocol
    // Interface number, string index, EP Out & EP In address, EP size
    TUD_VENDOR_DESCRIPTOR(Interfaces::Vendor, StringDescriptor::VendorInterfaceName,
            Endpoints::VendorOUT, Endpoints::VendorIN, 64),
};

/**
 * @brief Read configuration descriptor
 *
 * Provides a configuration descriptor, which defines the interfaces (and thus, their endpoints)
 * exposed by the device. This way, we can support multiple different device configurations, based
 * on the operating mode. However, this is not used.
 *
 * @param index Configuration descriptor index (currently ignored)
 */
extern "C" uint8_t const *tud_descriptor_configuration_cb(const uint8_t index) {
    if(index >= Descriptors::kNumConfigDescriptors) {
        Logger::Warning("request for invalid USB %s: $%02x", "config descriptor", index);
        return nullptr;
    }

    return Descriptors::kConfigurations[index];
}
