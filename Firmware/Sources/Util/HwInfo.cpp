#include "HwInfo.h"
#include "Base32.h"
#include "Hash.h"

#include <string.h>
#include <etl/array.h>
#include <printf/printf.h>

using namespace Util;

uint16_t HwInfo::gRevision{0};
const char *HwInfo::gSerial{nullptr};

/**
 * @brief Initialize hardware info
 *
 * Reads the serial number and other information from various nonvolatile memories on the chip,
 * and initializes our structures.
 */
void HwInfo::Init() {
    /*
     * Read the 128-bit chip serial number, hash it to a 32-bit value, then then run that through
     * base32 to make it more human friendly, and store its address.
     *
     * The memory addresses from which we read are from section 9.6 (Serial Number) in the "SAM
     * D5x/E5x Family Data Sheet." (Are there symbolic names for them?)
     */
    etl::array<uint8_t, 16> serial;
    memcpy(serial.data(), reinterpret_cast<const void *>(0x008061FC), 4);
    memcpy(serial.data()+4, reinterpret_cast<const void *>(0x00806010), 12);

    const auto serialHash = Hash::MurmurHash3(serial);

    static etl::array<char, 10> gSerialBase32;
    Util::Base32::Encode({reinterpret_cast<const uint8_t *>(&serialHash),
            sizeof(serialHash)}, gSerialBase32);

    gSerial = gSerialBase32.data();

    // XXX: read revision from NVM user row
    gRevision = 1;
}
