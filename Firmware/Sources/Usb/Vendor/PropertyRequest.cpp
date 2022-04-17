#include "PropertyRequest.h"

#include "BuildInfo.h"
#include "Log/Logger.h"
#include "Util/HwInfo.h"

#include <cbor.h>
#include <string.h>
#include <etl/algorithm.h>
#include <etl/array.h>
#include <printf/printf.h>

using namespace UsbStack::Vendor;

/**
 * @brief Handle a request to the property endpoint
 *
 * We'll decode the message, then first process any property set requests, followed by reading of
 * all properties.
 *
 * @param hdr Header of received packet
 * @param payload Packet payload (to be CBOR decoded)
 * @param response Buffer in which the response is built up
 *
 * @param Size of response, or 0 to send none
 */
size_t PropertyRequest::Handle(const InterfaceTask::PacketHeader &hdr,
        etl::span<const uint8_t> payload, etl::span<uint8_t> response) {
    CborError err;

    // set up encoder for response
    CborEncoder encoder, encoderMap;
    cbor_encoder_init(&encoder, response.data(), response.size(), 0);
    err = cbor_encoder_create_map(&encoder, &encoderMap, CborIndefiniteLength);
    if(err) {
        Logger::Warning("PropertyRequest: %s (%s)", "failed to initialize encoder map",
                cbor_error_string(err));
        return 0;
    }

    // set up decoder
    CborParser parser;
    CborValue it, map;
    CborType type;

    err = cbor_parser_init(payload.data(), payload.size(), 0, &parser, &it);
    if(err) {
        Logger::Warning("PropertyRequest: %s (%s)", "failed to initialize parser",
                cbor_error_string(err));
        return 0;
    }

    // the first field _must_ be a map
    type = cbor_value_get_type(&it);
    if(type != CborMapType) {
        Logger::Warning("PropertyRequest: %s (%u)", "malformed request", type);
        return 0;
    }

    err = cbor_value_enter_container(&it, &map);
    if(err) {
        Logger::Warning("PropertyRequest: %s (%s)", "failed to enter map",
                cbor_error_string(err));
        return 0;
    }

    /*
     * Iterate over the contents of the map. It should be string keys, which in turn contains
     * either an array (for `get`) or a map (for `set`)
     */
    enum class Key {
        None, Get, Set
    };

    etl::array<char, 8> stringBuf;
    Key nextKey{Key::None};

    while(!cbor_value_at_end(&map)) {
        type = cbor_value_get_type(&map);

        switch(type) {
            // map key: check if it's `get` or `set`
            case CborTextStringType: {
                // read out the string
                size_t len = stringBuf.size();
                etl::fill(stringBuf.begin(), stringBuf.end(), '\0');

                err = cbor_value_copy_text_string(&map, stringBuf.data(), &len, &map);

                // if string is too long, bail
                if(err == CborErrorOutOfMemory) {
                    Logger::Warning("PropertyRequest: %s", "string key too long");
                    goto noAdvance;
                }

                // ensure it's one of the "allowed" types
                if(!strncmp(stringBuf.data(), "get", stringBuf.size())) {
                    nextKey = Key::Get;
                }
                else {
                    Logger::Notice("PropertyRequest %s (%s)", "invalid key", stringBuf.data());
                    return 0;
                }
                goto noAdvance;
            }

            // array contents (for get)
            case CborArrayType: {
                CborValue getArray;

                if(nextKey != Key::Get) {
                    Logger::Notice("PropertyRequest %s", "unexpected array");
                    return 0;
                }

                // process its contents
                err = cbor_value_enter_container(&map, &getArray);
                if(err) {
                    Logger::Warning("PropertyRequest: %s (%s)", "failed to enter container",
                            cbor_error_string(err));
                }

                ProcessGet(&getArray, &encoderMap);

                // exit the array
                err = cbor_value_leave_container(&map, &getArray);
                if(err) {
                    Logger::Warning("PropertyRequest: %s (%s)", "failed to leave container",
                            cbor_error_string(err));
                }

                nextKey = Key::None;
                goto noAdvance;
            }

            default:
                Logger::Warning("PropertyRequest: %s (%u)", "invalid type", type);
                return 0;
        }

        // advance to next
        err = cbor_value_advance_fixed(&map);
        if(err) {
            Logger::Warning("PropertyRequest: %s (%s)", "failed to advance request",
                    cbor_error_string(err));
            return 0;
        }

noAdvance:;
    }

    // clean up
    cbor_value_leave_container(&it, &map);

    // finish encoding the response
    err = cbor_encoder_close_container(&encoder, &encoderMap);
    if(err) {
        Logger::Warning("PropertyRequest: %s (%s)", "failed to close encoder map",
                cbor_error_string(err));
        return 0;
    }

    return cbor_encoder_get_buffer_size(&encoder, response.data());
}

/**
 * @brief Process all "property get" requests.
 *
 * This handles all property reads; it's provided an array of property IDs, as well as a map into
 * which we'll encode the results.
 *
 * Results are encoded as another map, where each key corresponds to a property ID, and its value
 * is of course the property's value. If the ID is not existing in the map, it means the key could
 * not be read. This means this map _may_ be empty.
 *
 * @param propertyIds A CBOR array containing the property IDs
 * @param response Top level CBOR map for response
 */
void PropertyRequest::ProcessGet(CborValue *propertyIds, CborEncoder *response) {
    CborEncoder values;
    CborType type;
    CborError err;
    int id, propErr;

    // set up map for response values
    cbor_encode_text_stringz(response, "get");
    cbor_encoder_create_map(response, &values, CborIndefiniteLength);

    // iterate over property IDs
    while(!cbor_value_at_end(propertyIds)) {
        type = cbor_value_get_type(propertyIds);

        // we should only allow integers
        if(type != CborIntegerType) {
            Logger::Warning("PropertyRequest: %s", "expected integer");
            goto next;
        }

        // read the property id out
        err = cbor_value_get_int(propertyIds, &id);
        if(err) {
            Logger::Warning("PropertyRequest: %s (%s)", "failed to read property id",
                    cbor_error_string(err));
            goto next;
        }

        // process it
        propErr = GetSingleProperty(static_cast<Property>(id), &values);
        if(propErr) {
            Logger::Warning("PropertyRequest: %s (%u)", "failed to get property", propErr);
            goto next;
        }

next:;
        // advance to next
        err = cbor_value_advance_fixed(propertyIds);
        if(err) {
            Logger::Warning("PropertyRequest: %s (%s)", "failed to advance",
                    cbor_error_string(err));
            goto beach;
        }
    }

beach:;
    // finalize response
    cbor_encoder_close_container(response, &values);
}

/**
 * @brief Read a single property
 *
 * Attempt to read a single property; its result will be written into the provided output map, with
 * the key being the property id.
 *
 * @param what Property to read out
 * @param valueMap Map to write the property value into
 */
int PropertyRequest::GetSingleProperty(const Property what, CborEncoder *valueMap) {
    static char gStringBuf[64];

    // encode the id
    cbor_encode_uint(valueMap, static_cast<uint16_t>(what));

    // then, encode the value
    switch(what) {
        case Property::HwSerial:
            return cbor_encode_text_stringz(valueMap, Util::HwInfo::GetSerial());

        case Property::HwVersion:
            snprintf(gStringBuf, sizeof(gStringBuf), "Rev %u", Util::HwInfo::GetRevision());
            return cbor_encode_text_stringz(valueMap, gStringBuf);

        case Property::SwVersion:
            snprintf(gStringBuf, sizeof(gStringBuf), "%s/%s (%s)", gBuildInfo.gitBranch,
                    gBuildInfo.gitHash, gBuildInfo.buildType);
            return cbor_encode_text_stringz(valueMap, gStringBuf);

        // if we don't know the type, encode an 'undefined' value
        default:
            cbor_encode_undefined(valueMap);
            break;
    }

    return 0;
}

