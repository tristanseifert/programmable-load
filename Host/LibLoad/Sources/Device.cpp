#include "Device.h"
#include "DeviceTransport.h"

#include <iostream>

#include <fmt/format.h>

using namespace LibLoad::Internal;

/**
 * @brief Read a single property from the device
 *
 * Query the device for the current value of a property. The raw decoded value is then provided to
 * the caller.
 *
 * @param key Property id to query
 *
 * @return Whether the property was read (true) or not
 */
Cborg DeviceImpl::propertyGet(const Property key) {
    std::lock_guard lg(this->lock);

    // send the request
    this->writeCborMessage(Endpoint::PropertyRequest, [key](auto encoder) {
        return encoder.map()
            .key("get").array()
                .item(static_cast<unsigned int>(key))
            .end()
        .end();
    });

    // read response
    Cborg response;
    this->readCborMessage(response);

    return response.find("get").find(static_cast<int32_t>(key));
}

/**
 * @brief Send a CBOR-encoded message to the device
 *
 * Serialize the provided json data as CBOR, then send it to the device with the specified type
 * value.
 *
 * @param type Endpoint type to receive the message
 * @param encoder Function to invoke to encode the message payload. This should return the
 *        finalized CBOR encoder when done.
 *
 * @remark You should be holding the device lock when invoking this
 */
void DeviceImpl::writeCborMessage(const Endpoint type,
        const std::function<Cbore(Cbore)> &encoder) {
    // set up CBOR encoder and invoke the user function to encode into it
    Cbore cbor(this->txBuffer.data(), this->txBuffer.size());
    cbor = encoder(cbor);

    // send it
    std::span<const uint8_t> encoded(this->txBuffer.begin(),
            this->txBuffer.begin() + cbor.getLength());
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
    const auto received = this->transport->read(this->rxBuffer);
    if(!received) {
        return;
    }

    // try to decode it
    message = Cborg(this->rxBuffer.data(), received);
}


