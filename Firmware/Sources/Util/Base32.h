#ifndef UTIL_BASE32_H
#define UTIL_BASE32_H

#include <stddef.h>

#include <etl/span.h>

namespace Util {
/**
 * @brief Base32 coder
 *
 * This provides helper methods to encode and decode base32 strings.
 *
 * @remark Implementation is taken from google-authenticator-libpam, licensed under the
 *         Apache 2 license.[Original code](https://github.com/google/google-authenticator-libpam/blob/master/src/base32.c)
 */
class Base32 {
    public:
        /**
         * @brief Encode binary data as base32
         *
         * @param input Input buffer to encode
         * @param output Buffer to hold the final string, null terminated if sufficient space
         *
         * @return Number of characters written, or negative error code
         */
        static int Encode(etl::span<const uint8_t> input, etl::span<char> output) {
            constexpr static const char *kChars{"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"};
            size_t length = input.size();
            size_t bufSize = output.size();

            if(length < 0 || length > (1 << 28)) {
                return -1;
            }

            int count{0};
            if(length > 0) {
                uint32_t buffer = input[0];
                size_t next{1};
                size_t bitsLeft{8};

                while(count < bufSize && (bitsLeft > 0 || next < length)) {
                    if(bitsLeft < 5) {
                        if(next < length) {
                            buffer <<= 8;
                            buffer |= input[next++] & 0xFF;
                            bitsLeft += 8;
                        } else {
                            int pad = 5 - bitsLeft;
                            buffer <<= pad;
                            bitsLeft += pad;
                        }
                    }
                    size_t index = 0x1F & (buffer >> (bitsLeft - 5));
                    bitsLeft -= 5;
                    output[count++] = kChars[index];
                }
            }
            if(count < bufSize) {
                output[count] = '\000';
            }
            return count;
        }

        /**
         * @brief Decode base32 data
         *
         * @param input Null-terminated base32 encoded string
         * @param outBuf Buffer to receive the decoded data
         *
         * @return Number of bytes decoded or negative error
         */
        static int Decode(etl::span<const char> input, etl::span<uint8_t> outBuf) {
            size_t bufSize = outBuf.size();

            uint32_t buffer{0};
            size_t bitsLeft{0};
            size_t count{0};

            for(const char *ptr = input.data(); count < bufSize && *ptr; ++ptr) {
                uint8_t ch = static_cast<uint8_t>(*ptr);

                // ignore whitespace
                if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
                    continue;
                }
                buffer <<= 5;

                // Deal with commonly mistyped characters
                if(ch == '0') {
                    ch = 'O';
                } else if(ch == '1') {
                    ch = 'L';
                } else if(ch == '8') {
                    ch = 'B';
                }

                // Look up one base32 digit
                if((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                    ch = (ch & 0x1F) - 1;
                } else if(ch >= '2' && ch <= '7') {
                    ch -= '2' - 26;
                } else {
                    // invalid character
                    return -1;
                }

                buffer |= ch;
                bitsLeft += 5;

                if(bitsLeft >= 8) {
                    outBuf[count++] = buffer >> (bitsLeft - 8);
                    bitsLeft -= 8;
                }
            }

            // add a null byte if needed
            if(count < bufSize) {
                outBuf[count] = '\000';
            }
            return count;
        }
};
}

#endif
