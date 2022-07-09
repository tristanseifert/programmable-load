#include <stdint.h>

#include <etl/algorithm.h>
#include <etl/array.h>

#include <cbor.h>
#include <string.h>

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "../../Types.h"
#include "Handler.h"
#include "Service.h"

using namespace Rpc::Confd;

/**
 * @brief Initialize the confd service
 *
 * This is a small wrapper around the underlying message transport, which handles encoding
 * requests to be sent to the confd on the host.
 */
Service::Service(Handler *_handler) : handler(_handler) {
    this->cacheLock = xSemaphoreCreateMutex();
    REQUIRE(this->cacheLock, "%s failed", "xSemaphoreCreateMutex");
}

/**
 * @brief Clean up allocated resources
 *
 * Release all packet buffers and other structures.
 */
Service::~Service() {
    if(xSemaphoreTake(this->cacheLock, pdMS_TO_TICKS(10)) == pdTRUE) {
        // release all memory owned by the cache
        while(!this->cache.empty()) {
            void *ptr;
            this->cache.pop_into(ptr);

            vPortFree(ptr);
        }

        xSemaphoreGive(this->cacheLock);
    }

    vSemaphoreDelete(this->cacheLock);
}

/**
 * @brief Get a packet buffer
 *
 * Returns a cached packet buffer, or allocates one if needed.
 *
 * @return Packet buffer, or `nullptr` on error
 *
 * @remark When done with the packet buffer, you should invoke discardPacketBuffer to return it
 *         to the internal queue.
 */
void *Service::getPacketBuffer() {
    void *ptr{nullptr};
    auto ok = xSemaphoreTake(this->cacheLock, portMAX_DELAY);
    REQUIRE(ok == pdTRUE, "failed to acquire %s", "confd packet cache lock");

    // check cache first
    if(!this->cache.empty()) {
        this->cache.pop_into(ptr);
        goto beach;
    }
    // can we allocate a buffer?
    if(this->cacheTotal < kMaxPacketBuffers) {
        ptr = pvPortMalloc(kMaxPacketSize);
    }
    // if not, we don't have any available buffers right now

beach:;
    // unlock and return ptr; it's already NULL if we haven't found a buffer anywhere
    xSemaphoreGive(this->cacheLock);
    return ptr;
}

/**
 * @brief Release a packet buffer
 *
 * Place a previously allocated packet buffer (via getPacketBuffer) back onto the buffer queue.
 *
 * @param buffer A buffer pointer as returned by getPacketBuffer
 */
void Service::discardPacketBuffer(void *buffer) {
    auto ok = xSemaphoreTake(this->cacheLock, portMAX_DELAY);
    REQUIRE(ok == pdTRUE, "failed to acquire %s", "confd packet cache lock");

    REQUIRE(!this->cache.full(), "confd packet cache full!");
    this->cache.push(buffer);

    xSemaphoreGive(this->cacheLock);
}



/**
 * @brief Common code to send a query
 *
 * Serializes a request for the given key into a packet buffer, then sends it (while blocking on a
 * response) to the host.
 *
 * @param key Key name to query
 * @param outBlock Variable to receive the allocated info block
 * @param outFound Whether the key was found
 *
 * @remark The allocated info block must be deleted by the caller when done.
 */
int Service::getCommon(const etl::string_view &key, Handler::InfoBlock* &outBlock,
        bool &outFound) {
    int err;
    etl::span<uint8_t> packet;

    // format and send request
    err = this->serializeQuery(key, packet);
    if(err) {
        return err;
    }

    err = this->handler->sendRequestAndBlock(packet, outBlock);

    this->discardPacketBuffer(packet.data());

    // handle error case
    if(err) {
        if(err == 1) {
            return Status::Timeout;
        }
        return err;
    }

    // no error, so pull out some info
    const auto res = etl::get_if<Handler::InfoBlock::GetResponse>(&outBlock->response);
    REQUIRE(res, "invalid confd response type (expected %s)", "get");

    outFound = res->keyFound;
    return 0;
}

/**
 * @brief Read a binary configuration value
 *
 * @param key Property key to query
 * @param outValue Buffer to receive the configuration value
 * @param outNumBytes Variable to receive the actual number of bytes written
 *
 * @return One of the Status enum values (or a negative system error code)
 *
 * @remark The returned data may be truncated, if its length is larger than either the maximum
 *         receive buffer size, or the specified buffer's size.
 */
int Service::get(const etl::string_view &key, etl::span<uint8_t> outBuffer, size_t &outNumBytes) {
    int err;
    Handler::InfoBlock *block{nullptr};
    bool found{false}, typeMatch{false};

    // send and handle common response type
    err = this->getCommon(key, block, found);
    if(err) {
        return err;
    }
    const auto res = etl::get_if<Handler::InfoBlock::GetResponse>(&block->response);

    // extract value
    if(found) {
        using BlobType = Handler::InfoBlock::GetResponse::BlobType;
        // ensure type matches
        if(!etl::holds_alternative<BlobType>(res->value)) {
            goto beach;
        }

        // copy the value out
        const auto &vec = etl::get<BlobType>(res->value);
        auto end = etl::copy_s(vec.begin(), vec.end(), outBuffer.begin(), outBuffer.end());

        typeMatch = true;
        outNumBytes = (end - outBuffer.begin());
    }

beach:;
    // finish up
    delete block;
    return found ? (typeMatch ? Status::Success : Status::ValueTypeMismatch) : Status::KeyNotFound;
}

/**
 * @brief Read a string configuration value
 *
 * @param key Property key to query
 * @param outValue String to receive the configuration data
 *
 * @return One of the Status enum values (or a negative system error code)
 *
 * @remark The returned data may be truncated, if the value is larger than either the maximum
 *         receive buffer size, or because the specified string is too small. In either case, the
 *         call still succeeds; the caller should check whether the string has a null termination
 *         to determine if it was completely received.
 */
int Service::get(const etl::string_view &key, etl::istring &outValue) {
    int err;
    Handler::InfoBlock *block{nullptr};
    bool found{false}, typeMatch{false};

    // send and handle common response type
    err = this->getCommon(key, block, found);
    if(err) {
        return err;
    }
    const auto res = etl::get_if<Handler::InfoBlock::GetResponse>(&block->response);

    // extract value
    if(found) {
        using StringType = Handler::InfoBlock::GetResponse::StringType;
        // ensure type matches
        if(!etl::holds_alternative<StringType>(res->value)) {
            goto beach;
        }

        // yeet out the value
        outValue = etl::get<StringType>(res->value);
        typeMatch = true;
    }

beach:;
    // finish up
    delete block;
    return found ? (typeMatch ? Status::Success : Status::ValueTypeMismatch) : Status::KeyNotFound;
}

/**
 * @brief Read an integer configuration value
 *
 * @param key Property key to query
 * @param outValue Variable to receive the value of this property
 *
 * @return One of the Status enum values (or a negative system error code)
 */
int Service::get(const etl::string_view &key, uint64_t &outValue) {
    int err;
    Handler::InfoBlock *block{nullptr};
    bool found{false}, typeMatch{false};

    // send and handle common response type
    err = this->getCommon(key, block, found);
    if(err) {
        return err;
    }
    const auto res = etl::get_if<Handler::InfoBlock::GetResponse>(&block->response);

    // extract value
    if(found) {
        if(!etl::holds_alternative<uint64_t>(res->value)) {
            goto beach;
        }
        outValue = etl::get<uint64_t>(res->value);
        typeMatch = true;
    }

beach:;
    // finish up
    delete block;
    return found ? (typeMatch ? Status::Success : Status::ValueTypeMismatch) : Status::KeyNotFound;
}

/**
 * @brief Read a floating point configuration value
 *
 * @param key Property key to query
 * @param outValue Variable to receive the value of this property
 *
 * @return One of the Status enum values (or a negative system error code)
 *
 * @remark While real valued numbers are stored as doubles on the host, they're down-converted to
 *         float (32-bit precision) when we request them, as we cannot handle doubles in hardware.
 */
int Service::get(const etl::string_view &key, float &outValue) {
    int err;
    Handler::InfoBlock *block{nullptr};
    bool found{false}, typeMatch{false};

    // send and handle common response type
    err = this->getCommon(key, block, found);
    if(err) {
        return err;
    }
    const auto res = etl::get_if<Handler::InfoBlock::GetResponse>(&block->response);

    // extract value
    if(found) {
        if(!etl::holds_alternative<float>(res->value)) {
            goto beach;
        }
        outValue = etl::get<float>(res->value);
        typeMatch = true;
    }

beach:;
    // finish up
    delete block;
    return found ? (typeMatch ? Status::Success : Status::ValueTypeMismatch) : Status::KeyNotFound;

}

/**
 * @brief Acquire a packet buffer and encode a "get" request for the given key
 *
 * Allocate a packet buffer (which may fail!) and then place inside it an rpc_header, as well as
 * the CBOR payload which will contain a serialized query for the given property key.
 *
 * @param key Property key to request
 * @param outPacket Variable to hold the packet buffer on success
 *
 * @return 0 on success or a negative error code
 */
int Service::serializeQuery(const etl::string_view &key, etl::span<uint8_t> &outPacket) {
    int err;
    size_t totalNumBytes{0};
    CborEncoder encoder, encoderMap;

    // get a buffer first
    auto buffer = this->getPacketBuffer();
    if(!buffer) {
        return -1;
    }

    // reserve space for the rpc header and prepare it
    auto hdr = reinterpret_cast<struct rpc_header *>(buffer);

    hdr->version = kRpcVersionLatest;
    hdr->type = static_cast<uint8_t>(Handler::MsgType::Query);

    // set up the CBOR encoder for the remaining memory
    const auto maxPayloadSize = kMaxPacketSize - sizeof(*hdr);
    cbor_encoder_init(&encoder, hdr->payload, maxPayloadSize, 0);

    err = cbor_encoder_create_map(&encoder, &encoderMap, 2);
    if(err) {
        Logger::Warning("%s failed: %d", "cbor_encoder_create_map", err);
        goto fail;
    }

    // write the body
    cbor_encode_text_stringz(&encoderMap, "key");
    cbor_encode_text_string(&encoderMap, key.data(), key.length());

    cbor_encode_text_stringz(&encoderMap, "forceFloat");
    cbor_encode_boolean(&encoderMap, true);

    // finish encoding
    err = cbor_encoder_close_container(&encoder, &encoderMap);
    if(err) {
        Logger::Warning("%s failed: %d", "cbor_encoder_close_container", err);
        goto fail;
    }

    // return the buffer of just the message
    totalNumBytes = sizeof(*hdr) + cbor_encoder_get_buffer_size(&encoder, hdr->payload);

    hdr->length = totalNumBytes;
    outPacket = {reinterpret_cast<uint8_t *>(buffer), totalNumBytes};

    return 0;

fail:;
    // handle an error
    this->discardPacketBuffer(buffer);
    return err;
}

/**
 * @brief Decode the CBOR-encoded payload provided
 *
 * @param payload Buffer containing the payload of the rpc packet
 * @param info Information block to receive the decoded query information
 *
 * @remark This is one gigantic function and it kind of sucks :(
 */
int Service::DeserializeQuery(etl::span<const uint8_t> payload, Handler::InfoBlock *info) {
    int err;
    info->response = Handler::InfoBlock::GetResponse{};
    auto &resp = etl::get<Handler::InfoBlock::GetResponse>(info->response);

    CborParser parser;
    CborValue it, map;
    CborType type;

    // set up CBOR decoder
    err = cbor_parser_init(payload.data(), payload.size(), 0, &parser, &it);
    if(err) {
        Logger::Warning("%s failed: %d", "cbor_parser_init", err);
        return err;
    }

    // the first field _must_ be a map
    type = cbor_value_get_type(&it);
    if(type != CborMapType) {
        Logger::Warning("invalid %s in confd response (type=%02x)", "root object", type);
        return Status::MalformedResponse;
    }

    err = cbor_value_enter_container(&it, &map);
    if(err) {
        Logger::Warning("%s failed: %d", "cbor_value_enter_container", err);
        return err;
    }

    // iterate over the contents of the map
    etl::array<char, 12> stringBuf;
    bool isKey{true};
    enum class Key {
        Unknown         = 0,
        KeyName         = 1,
        IsFound         = 2,
        Value           = 3,
    };

    Key next{Key::Unknown};

    // iterate through the map; we'll alternately get keys and values
    while(!cbor_value_at_end(&map)) {
        // set to skip advancing to next item at end of loop
        bool noAdvance{false};
        type = cbor_value_get_type(&map);

        // handle keys
        if(isKey) {
            // keys are always string
            if(type != CborTextStringType) {
                Logger::Warning("invalid %s in confd response (type=%02x)", "key", type);
                return Status::MalformedResponse;
            }

            // so figure out what type of key the string corresponds to
            size_t len = stringBuf.size();
            etl::fill(stringBuf.begin(), stringBuf.end(), '\0');

            err = cbor_value_copy_text_string(&map, stringBuf.data(), &len, &map);
            if(err == CborErrorOutOfMemory) {
                Logger::Warning("invalid %s in confd response (%s)", "key", "too long");

                next = Key::Unknown;
            } else {
                // select the appropriate key
                if(!strncmp(stringBuf.data(), "found", stringBuf.size())) {
                    next = Key::IsFound;
                }
                else if(!strncmp(stringBuf.data(), "key", stringBuf.size())) {
                    next = Key::KeyName;
                }
                else if(!strncmp(stringBuf.data(), "value", stringBuf.size())) {
                    next = Key::Value;
                }
                else {
                    next = Key::Unknown;
                }
            }

            noAdvance = true;
        }
        // handle values
        else {
            switch(next) {
                // whether we found data
                case Key::IsFound:
                    if(type != CborBooleanType) {
                        Logger::Warning("invalid %s in confd response (type=%02x)", "found", type);
                    } else {
                        err = cbor_value_get_boolean(&map, &resp.keyFound);
                        if(err) {
                            Logger::Warning("%s failed: %d", "cbor_value_get_boolean", err);
                            return err;
                        }
                    }
                    break;

                // the resulting data
                case Key::Value:
                    switch(type) {
                        case CborIntegerType: {
                            uint64_t temp;
                            err = cbor_value_get_uint64(&map, &temp);
                            resp.value = temp;
                            break;
                        }
                        // all double values are downcast to float by the server
                        case CborFloatType: {
                            float temp;
                            err = cbor_value_get_float(&map, &temp);
                            resp.value = temp;
                            break;
                        }
                        // strings are copied (as much as fits, anyhow) into the string value
                        case CborTextStringType: {
                            using StringType = Handler::InfoBlock::GetResponse::StringType;
                            size_t len;

                            // default construct an empty string
                            resp.value = StringType{};
                            // get the result string length
                            err = cbor_value_get_string_length(&map, &len);
                            if(err != CborNoError) {
                                goto error;
                            }

                            // resize string and copy
                            auto strRef = etl::get_if<StringType>(&resp.value);
                            strRef->resize(etl::min(len, strRef->capacity()));

                            len = strRef->size();
                            err = cbor_value_copy_text_string(&map, strRef->data(), &len, &map);
                            if(err != CborNoError) {
                                goto error;
                            }

                            // we've already advanced to the next item
                            noAdvance = true;
                            break;
                        }
                        // BLOBs work similarly to strings, sans null termination
                        case CborByteStringType: {
                            using BlobType = Handler::InfoBlock::GetResponse::BlobType;
                            size_t len;

                            // default construct an empty buffer
                            resp.value = BlobType{};
                            err = cbor_value_get_string_length(&map, &len);
                            if(err != CborNoError) {
                                goto error;
                            }

                            // resize buffer and copy
                            auto vecRef = etl::get_if<BlobType>(&resp.value);
                            vecRef->resize(etl::min(len, vecRef->capacity()));

                            len = vecRef->size();
                            err = cbor_value_copy_byte_string(&map, vecRef->data(), &len, &map);
                            if(err != CborNoError) {
                                goto error;
                            }

                            // we've already advanced to the next item
                            noAdvance = true;
                            break;
                        }
                        // null values are represented by the "monostate" value
                        case CborNullType:
                            resp.value = etl::monostate{};
                            break;

                        default:
                            Logger::Warning("invalid %s in confd response (type=%02x)", "value", type);
                            return Status::MalformedResponse;
                    }

                    // process errors from retrieving
                    if(err) {
error:;
                        Logger::Warning("failed to get cbor value: %d (type=%02x)", err, type);
                        return err;
                    }
                    break;

                // ignore other types of keys
                default:
                    break;
            }
        }

        // advance to the next value
        if(!noAdvance) {
            err = cbor_value_advance_fixed(&map);
            if(err) {
                Logger::Warning("%s failed: %d", "cbor_value_advance_fixed", err);
                return err;
            }
        }

        // ensure we alternate between keys and values
        isKey = !isKey;
    }

    cbor_value_leave_container(&it, &map);

    // store result struct info block
    return 0;
}

