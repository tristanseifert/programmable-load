#ifndef UTIL_INVENTORYROM_H
#define UTIL_INVENTORYROM_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Util {
/**
 * @brief Helpers for working with inventory ROMs
 *
 * Various components of the hardware may feature inventory ROMs, which are small EEPROMs that
 * contain a fixed header, as well as variaable length data packets (atoms) that can be parsed
 * by the application.
 *
 * Essentially, this is a basic TLV parser.
 */
class InventoryRom {
    public:
        /**
         * @brief Error codes unique to inventory ROMs
         */
        enum Errors: int {
            /**
             * @brief The buffer is not large enough for the payload
             */
            InsufficientBuffer          = -50000,

            /**
             * @brief An invalid header was read
             */
            InvalidType                 = -50001,

            /**
             * @brief Invalid IDPROM header
             *
             * The header in the IDPROM was invalid. This could mean the magic value did not
             * match, the version is incorrect, or the size/atom start values are nonsensical.
             */
            InvalidHeader               = -50002,
        };

        /**
         * @brief Inventory ROM header
         *
         * This header is found at the start of the IDPROM.
         */
        struct IdpromHeader {
            /**
             * @brief Magic value
             *
             * This should be set to kMagicValue.
             */
            uint32_t magic;

            /**
             * @brief Header length
             *
             * Size of the header, in bytes, including the magic value.
             */
            uint8_t size;

            /**
             * @brief Header version
             *
             * Defines the version of both the header itself and the data contained within, used
             * to parse the header correctly. It is divided into major and minor versions in the
             * high and low nybbles, respectively.
             *
             * Currently, the only supported version is 0x10.
             */
            uint8_t version;

            /**
             * @brief Offset to atom list
             *
             * Byte offset from the start of the header to the atom list.
             */
            uint16_t firstAtom;

            /// Expected magic value
            constexpr static const uint32_t kMagicValue{'INVi'};
        } __attribute__((packed));
        static_assert(sizeof(IdpromHeader) < 16, "IDPROM header too large!");

        /**
         * @brief Types of atoms
         *
         * Each atom's type is identified by an 8 bit integer at the start.
         */
        enum class AtomType: uint8_t {
            /**
             * @brief End of atoms
             *
             * A zero length atom that indicates there are no more atoms remaining. It must always
             * be the last one written to the ROM.
             */
            End                         = 0x00,

            /**
             * @brief Hardware revision
             *
             * A 16-bit integer that contains the revision number for the hardware.
             */
            HwRevision                  = 0x01,

            /**
             * @brief Descriptive name
             *
             * String containing a short descriptive name of this hardware.
             */
            Name                        = 0x02,

            /**
             * @brief Manufacturer
             *
             * String containing the manufacturer's name
             */
            Manufacturer                = 0x03,

            /**
             * @brief Driver identifier
             *
             * A 16-byte blob containing the binary representation of an UUID, which uniquely
             * matches a software driver to this hardware.
             */
            DriverId                    = 0x04,

            /**
             * @brief First application defined
             *
             * All atom values above this are reserved for the application's interpretation.
             */
            AppSpecific                 = 0x40,

            /**
             * @brief Driver board ratings
             *
             * A list of two 32-bit integers; the first indicates the maximum supported input
             * voltage (in mV) and the second the maximum load current, in mA.
             */
            DriverRating                = AppSpecific + 0,

            /**
             * @brief Invalid header type
             *
             * This indicates an invalid atom. It's got a value of `0xFF` so that the code can
             * easily detect EEPROMs and flash that isn't programmed.
             */
            Invalid                     = 0xFF,
        };

        /**
         * @brief Atom header
         *
         * Each atom starts with this header, which defines its type and length.
         *
         * Data follows immediately after the header. The format of the data is up to the atom
         * type's definitions. Most common data types will be strings (which are NOT zero
         * terminated,) binary blobs, or integers. Integers are stored in big endian byte order.
         */
        struct AtomHeader {
            /// Type of the atom
            AtomType type;
            /**
             * @brief Payload length
             *
             * This is the length of payload data that immediately follows this header, in bytes;
             * values of up to 255 bytes can be supported.
             */
            uint8_t length;
        } __attribute__((packed));

    public:
        /**
         * @brief Config ROM read callback
         *
         * Invoked by the utility class to read a chunk of an inventory ROM. This should perform
         * the needed memory reads or bus transactions, and return the data into the provided
         * buffer.
         *
         * @param address Starting address into the ROM
         * @param length Number of bytes to read
         * @param buffer Place data into this buffer
         * @param ctx Context passed in to the helper function
         *
         * @return 0 on success, or an error code.
         */
        using ReaderCallback = int(*)(uintptr_t address, size_t length,
                etl::span<uint8_t> buffer, void *ctx);

        /**
         * @brief Callback invoked for each atom
         *
         * When iterating over atoms in an inventory ROM, this callback will be invoked.for each
         * atom, and may then decide whether the payload of the atom should be read out. This
         * allows for visiting of all atoms relatively quickly.
         *
         * If the caller desires to read the payload, it must specify a sufficiently large buffer.
         *
         * @param header Reference to the atom's header
         * @param ctx Atom callback context pased in to the helper function
         * @param outBuf Set this buffer to receive the atom's data.
         *
         * @return Whether iteration should continue; data will NOT be read if this is false.
         */
        using AtomCallback = bool(*)(const AtomHeader &header, void *ctx,
                etl::span<uint8_t> &outBuf);

        /**
         * @brief Callback invoked when the payload of an atom is read
         *
         * This callback is invoked in response to an AtomCallback providing an appropriate buffer
         * for the atom's payload, if the read succeeded.
         *
         * @param header Reference to the atom's header
         * @param buffer Buffer that was provided by the atom callback, containing the payload
         * @param ctx Atom data callback context passed in to the helper functions
         */
        using AtomDataCallback = void(*)(const AtomHeader &header,
                etl::span<const uint8_t> buffer, void *ctx);

        static int GetAtoms(ReaderCallback reader, void *readerCtx,
                AtomCallback atomCallback, void *atomCallbackCtx,
                AtomDataCallback atomDataCallback, void *atomDataCallbackCtx);
};
}

#endif
