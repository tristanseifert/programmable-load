#include "Font.h"
#include "Framebuffer.h"

#include "Log/Logger.h"
#include "Util/Unicode.h"

using namespace Gfx;

/**
 * @brief Draw the string to the framebuffer, without formatting.
 *
 * The string provided is drawn to the framebuffer, until the edge is reached.
 *
 * @param str An UTF-8 encoded string to draw
 * @param fb Framebuffer to draw to
 * @param origin Top left origin for the string
 *
 * @return Number of codepoints drawn
 */
int Font::draw(const etl::string_view str, Framebuffer &fb, const Point origin) const {
    size_t drawn{0};
    uint32_t utfState{Util::Unicode::kStateAccept}, utfCodepoint;
    const Glyph *glyph{nullptr};

    // set up for drawing
    Point current{origin};

    auto s = str.data();
    for(; *s; ++s) {
        // have not yet consumed an entire codepoint
        if(Util::Unicode::Decode(*s, utfState, utfCodepoint)) {
            continue;
        }
        // codepoint above 0xFFFF: not yet supported
        REQUIRE(utfCodepoint <= 0xFFFF, "codepoints > 0xFFFF not yet supported");

        // find the associated glyph
        if(!this->findGlyph(utfCodepoint, glyph)) {
            Logger::Warning("No glyph for codepoint $%04x in font %p (%s)", utfCodepoint, this,
                    this->name.data());
            continue;
        }

        // draw the glyph, and update current position
        fb.blit4Bpp(glyph->data, glyph->size, current, Framebuffer::BlitFlags::HasTransparency);
        drawn++;

        current.x += glyph->size.width;
        if(current.x >= fb.size.width) {
            goto beach;
        }
    }

    // drew the whole string, or reached end of display
beach:;
    return drawn;
}

