#include "Device.h"
#include "DeviceTransport.h"

#include <iostream>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

using namespace LibLoad::Internal;

/**
 * @brief Read a single property from the device
 *
 * Query the device for the current value of a property. The raw decoded value is then provided to
 * the caller.
 *
 * @param key Property id to query
 * @param outValue JSON object to receive the property value
 *
 * @return Whether the property was read (true) or not
 */
bool DeviceImpl::propertyGet(const Property key, nlohmann::json &outValue) {
    // send the request
    nlohmann::json req{
        {"get", {static_cast<unsigned int>(key)}}
    };

    this->writeCborMessage(Endpoint::PropertyRequest, req);

    // read response
    Cborg response;
    this->readCborMessage(response);

    auto value = response.find("get").find(static_cast<int32_t>(key));
    switch(value.getType()) {
        case CborBase::TypeString: {
            std::string str;
            if(value.getString(str)) {
                outValue = str;
                return true;
            }
            break;
        }

        default:
            throw std::runtime_error(fmt::format("unhandled property type {}", value.getType()));
    }

    // failed to decode or read the property
    return false;
}

/**
 * @brief Send a CBOR-encoded message to the device
 *
 * Serialize the provided json data as CBOR, then send it to the device with the specified type
 * value.
 *
 * @param type Endpoint type to receive the message
 * @param payload A JSON object to encode as the payload of the message
 *
 * @remark You should be holding the device lock when invoking this
 */
void DeviceImpl::writeCborMessage(const Endpoint type, const nlohmann::json &payload) {
    std::vector<uint8_t> encoded = nlohmann::json::to_cbor(payload);
    this->transport->write(static_cast<uint8_t>(type), encoded);
}

/**
 * @brief Receive a CBOR-encoded message from the device
 *
 * Prepare a read transaction from the device, up to the maximum allowed size. We'll then decode
 * the payload as a CBOR object.
 *
 * @param message Object to receive the decoded response
 *
 * @remark Tag and type values of the response are ignored.
 */
void DeviceImpl::readCborMessage(Cborg &message) {
    // receive a message
    const auto received = this->transport->read(this->buffer);
    if(!received) {
        return;
    }

    // try to decode it
    //message = nlohmann::json::from_cbor(payload);
    message = Cborg(this->buffer.data(), received);
}


