#ifndef GFX_FONT_H
#define GFX_FONT_H

#include <bitflags.h>
#include <stdint.h>
#include <stddef.h>

#include <etl/utility.h>
#include <etl/span.h>
#include <etl/string_view.h>

#include "Types.h"

namespace Gfx {
class Framebuffer;
/**
 * @brief Font drawing modifiers
 *
 * Some of these can be combined (via bitwise OR) to affect the font rendering.
 */
enum class FontRenderFlags: uintptr_t {
    None                        = 0,

    /// Mask for the horizontal alignment
    HAlignMask                  = (0x7 << 0),
    /// Align text to the left
    HAlignLeft                  = (0x0 << 0),
    /// Align text to the right
    HAlignRight                 = (0x1 << 0),
    /// Align text in the middle
    HAlignCenter                = (0x2 << 0),

    /**
     * @brief Enable word wrapping
     *
     * When set, the string is broken at word boundaries rather than whenever it reaches
     * the edge.
     */
    WordWrap                    = (1 << 8),

    /**
     * @brief Render partial lines
     *
     * If the vertical space is insufficient to draw a full line, and this flag is set,
     * a partial line (glyphs are cut off at some point before the full line height) will
     * be drawn. Otherwise, the space is left empty.
     */
    DrawPartialLine             = (1 << 9),
};
ENUM_FLAGS_EX(FontRenderFlags, uintptr_t);

/**
 * @brief Font descriptor
 *
 * Describes the characters (and associated glyphs for drawing) for a single font, and contains the
 * logic for drawing the font to a framebuffer.
 */
struct Font {
    public:
        /**
         * @brief A renderable character (glyph) in a font
         *
         * Describes a bitmap that contains a single character, as well as how to render and lay it
         * out.
         */
        struct Glyph {
            /**
             * @brief Glyph bitmap data
             *
             * An array containing 4bpp bitmap data for the character. In the case that the width
             * is odd, the last byte in each row will have the lower nybble set to zero.
             *
             * It should be densely packed, with no additional padding between rows.
             */
            etl::span<const uint8_t> data;

            /**
             * @brief Glyph size
             *
             * Defines the size of the glyph on screen, in pixels. The first parameter is width,
             * the second is the height.
             */
            Size size;

            // unused
            uint8_t block;
        };

        /**
         * @brief A single character in a font
         *
         * Encapsulates information about a single character in the font.
         */
        struct Character {
            /**
             * @brief Character codepoint
             *
             * UTF-16 codepoint corresponding to this character.
             */
            wchar_t codepoint;

            /**
             * @brief Glyph data
             *
             * Information on how to render this character, including the bitmap and its size.
             */
            const Glyph glyph;
        };

    public:
        int draw(const etl::string_view str, Framebuffer &fb, const Point origin) const;
        void draw(const etl::string_view str, Framebuffer &fb, const Rect bounds,
                const FontRenderFlags flags = FontRenderFlags::None) const;

        /**
         * @brief Search the font for a glyph for the given codepoint
         *
         * Iterate through all characters in the font to find a glyph with the given codepoint,
         * then output the corresponding glyph structure.
         *
         * @param codepoint UTF-16 codepoint to search for
         * @param outGlyph Variable to receive the address of the glyph structure
         *
         * @return Whether the glyph was found or not
         *
         * @TODO Implement binary search to take advantage of the character list being sorted
         */
        inline bool findGlyph(const wchar_t codepoint, const Glyph* &outGlyph) const {
            for(const auto &ch : this->characters) {
                if(ch.codepoint != codepoint) continue;

                outGlyph = &ch.glyph;
                return true;
            }

            return false;
        }

    public:
        /**
         * @brief Font name
         *
         * This is the name of this font. It's not used anywhere in the code (yet)
         */
        etl::string_view name;

        /**
         * @brief Character array
         *
         * An array containing character information structures for all characters in this font. It
         * should be ordered in ascending codepoint order.
         */
        etl::span<const Character> characters;

        /// Font size (points)
        uint16_t fontSize;

        /**
         * @brief Line height (pixels)
         *
         * The vertical distance between consecutive lines to be added.
         */
        uint16_t lineHeight;

    public:
        /**
         * @brief Number font, XL
         *
         * An extra large font (size 26 points) containing numbers, and a few characters/symbols.
         * Primarily intended for display of current/voltage values on the main screen.
         */
        static const Font gNumbers_XL;
        /**
         * @brief Large number font
         *
         * A large font (20 points) containing numbers and some characters/symbols.
         */
        static const Font gNumbers_L;

        /**
         * @brief Large UI font
         *
         * A large 16 point font, with a condensed X spacing.
         */
        static const Font gGeneral_16_Condensed;
        /**
         * @brief Large UI font (bold)
         *
         * A large 16 point font, in bold.
         */
        static const Font gGeneral_16_Bold;
        /**
         * @brief Large UI font (condensed, bold)
         *
         * A large 16 point font, in bold with a condensed X spacing.
         */
        static const Font gGeneral_16_BoldCondensed;

        /**
         * @brief General UI font
         *
         * Regular weight 14 point font
         */
        static const Font gGeneral_14;

        /**
         * @brief Small, information dense font
         *
         * This is a small 10 point font.
         */
        static const Font gSmall;

    private:
        bool processLine(Framebuffer &fb, const char* &str, Rect &bounds,
                const FontRenderFlags flags) const;
        void renderLine(Framebuffer &, const char *, const Rect, const size_t, const int,
                const FontRenderFlags) const;

        /**
         * @brief Check if the given character is a word wrap point.
         *
         * This includes punctuation and spacing.
         */
        static constexpr bool IsWrapPoint(const uint32_t ch) {
            return (ch == ' ') || (ch == '.') || (ch == '!') || (ch == '?');
        }
};
}

#endif
