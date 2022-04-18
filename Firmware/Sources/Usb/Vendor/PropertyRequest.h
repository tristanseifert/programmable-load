#ifndef USB_VENDOR_PROPERTYREQUEST_H
#define USB_VENDOR_PROPERTYREQUEST_H

#include "VendorInterfaceTask.h"

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

struct CborValue;
struct CborEncoder;

namespace UsbStack::Vendor {
/**
 * @brief Handler for property request messages
 *
 * Allows reading/writing properties of the device.
 */
class PropertyRequest {
    public:
        /// Supported property types
        enum class Property: uint16_t {
            HwSerial                    = 0x01,
            HwVersion                   = 0x02,
            HwInventory                 = 0x03,
            SwVersion                   = 0x04,
            MaxVoltage                  = 0x05,
            MaxCurrent                  = 0x06,

            /// Highest value for an invalid property id
            MaxPropertyId,
        };

    public:
        static size_t Handle(const InterfaceTask::PacketHeader &hdr,
                etl::span<const uint8_t> payload, etl::span<uint8_t> response);

    private:
        static void ProcessGet(CborValue *propertyIds, CborEncoder *map);
        static int GetSingleProperty(const Property what, CborEncoder *valueMap);
};
}

#endif
