#ifndef UTIL_UNICODE_H
#define UTIL_UNICODE_H

#include <stdint.h>

namespace Util {
/**
 * @brief Helpers for working with Unicode strings
 */
class Unicode {
    private:
        constexpr static const uint8_t kUtf8d[] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
            7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
            8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
            0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
            0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
            0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
            1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
            1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
            1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
        };

    public:
        /// Valid codepoint encountered
        constexpr static const uint32_t kStateAccept{0};
        /// An invalid codepoint was encountered
        constexpr static const uint32_t kStateReject{1};

        /**
         * @brief Decode an UTF-8 character
         *
         * It is based on Bjoern Hoehrmann's "Flexible and Economical UTF-8 Decoder." Please see
         * [this page](http://bjoern.hoehrmann.de/utf-8/decoder/dfa/) for more information about
         * the implementation and how to use it.
         *
         * @param byte Current byte in the UTF-8 stream
         * @param outState Current state of the UTF-8 decoder. Must be initialized to kStateAccept,
         *        and will be set to this state again once a valid codepoint is decoded.
         * @param outCodepoint If a valid codepoint was decoded, this variable holds it.
         *
         * @return Current state
         */
        inline constexpr static uint32_t Decode(const uint8_t byte, uint32_t &outState,
                uint32_t &outCodepoint) {
            uint32_t type = kUtf8d[byte];

            outCodepoint = (outState != kStateAccept) ?
                (byte & 0x3fu) | (outCodepoint << 6) :
                (0xff >> type) & (byte);

            outState = kUtf8d[256 + (outState * 16) + type];
            return outState;
        }

        /**
         * @brief Count the number of codepoints in an UTF-8 string
         *
         * @param string Input string
         *
         * @return Number of codepoints, or -1 if malformed
         */
        inline constexpr static int Strlen(const char *string) {
            uint32_t codepoint;
            uint32_t state{kStateAccept};
            size_t count{0};

            for(count = 0; *string; ++string) {
                if(!Decode(*string, state, codepoint)) {
                    count++;
                }
            }

            if(state != kStateAccept) {
                return -1;
            }
            return count;
        }
};
}

#endif
