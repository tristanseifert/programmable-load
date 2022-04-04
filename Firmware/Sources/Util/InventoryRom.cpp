#include "InventoryRom.h"

#include <etl/array.h>

using namespace Util;

/**
 * @brief Iterate over all atoms in an inventory ROM
 *
 * Reads all atoms in the inventory ROM using the provided IO callback.
 *
 * For each atom, we read the complete header and invoke an user specified callback to determine
 * if we should read the payload, and if so, providing the buffer that it should go into.
 *
 * @param reader Callback to read from ROM
 * @param readerCtx Context to reader
 * @param atomCallback Callback for each atom
 * @param atomCallbackCtx Context for atom callback
 * @param atomDataCallback Callback for atoms whose data was read
 * @param atomDataCallbackCtx Context for atom data callback
 *
 * @param Positive number of atoms read, or a negative error code.
 */
int InventoryRom::GetAtoms(ReaderCallback reader, void *readerCtx,
        AtomCallback atomCallback, void *atomCallbackCtx,
        AtomDataCallback atomDataCallback, void *atomDataCallbackCtx) {
    int err, numRead{0};
    uintptr_t addr{0};
    bool hasNext{true};

    // read the header out of the IDPROM
    etl::array<uint8_t, sizeof(IdpromHeader)> idpromHdrBuf;
    err = reader(0, idpromHdrBuf.size(), idpromHdrBuf, readerCtx);
    if(err) {
        return err;
    }

    const auto &idpromHdr = *reinterpret_cast<const IdpromHeader *>(idpromHdrBuf.data());

    if(idpromHdr.magic != IdpromHeader::kMagicValue || idpromHdr.size < sizeof(idpromHdr) ||
            idpromHdr.version > 0x1F || idpromHdr.firstAtom < idpromHdr.size) {
        return Errors::InvalidHeader;
    }

    addr = idpromHdr.firstAtom;

    // read atoms
    etl::array<uint8_t, sizeof(AtomHeader)> headerBuf;

    do {
        // read the header
        err = reader(addr, headerBuf.size(), headerBuf, readerCtx);
        if(err) goto beach;

        const auto &header = *reinterpret_cast<const AtomHeader *>(headerBuf.data());

        if(header.type == AtomType::Invalid) {
            err = Errors::InvalidType;
            goto beach;
        }

        // invoke callback
        etl::span<uint8_t> readBuf;
        if(!atomCallback(header, atomCallbackCtx, readBuf)) {
            goto done;
        }

        // perform read of the data if requested
        addr += sizeof(AtomHeader);

        if(!readBuf.empty() && header.length) {
            const auto length = (readBuf.size() > header.length) ? header.length : readBuf.size();
            err = reader(addr, length, readBuf, readerCtx);
            if(err) goto beach;
        }
        // otherwise, just increment the address counter to the next header
        else {
            addr += header.length;
        }

done:;
        // prepare for next
        numRead++;
        if(header.type == AtomType::End) {
            hasNext = false;
        }
    } while(hasNext);

    // we want to return the number read
    err = numRead;

beach:;
    // clean up
    return err;
}
