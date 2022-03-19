#include "Descriptors.h"
#include "Usb.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <tusb.h>

using namespace UsbStack;

const tusb_desc_device_t Descriptors::kDeviceDescriptor{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

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
        case Descriptors::StringDescriptor::SerialNumber:
            // TODO: implement
            return reinterpret_cast<const uint16_t *>(u"\x0503shit");
            break;
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
 * - CDC (debug console)
 * - Vendor specific interface
 */
constexpr static const size_t kDefaultCfgDescriptorLen{
    TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_VENDOR_DESC_LEN
};

const uint8_t Descriptors::kDefaultCfgDescriptor[]{
    // config descriptor header
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, Interfaces::Total, 0, kDefaultCfgDescriptorLen,
            TUSB_DESC_CONFIG_ATT_SELF_POWERED, 0),

    // CDC: virtual serial port for debug console
    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(Interfaces::CDC, StringDescriptor::CdcInterfaceName,
            Endpoints::ConsoleNotifyIN, 8, Endpoints::ConsoleOUT, Endpoints::ConsoleIN, 64),

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
