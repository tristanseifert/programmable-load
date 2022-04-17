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



/**
 * @brief Draws a string into the specified bounding box.
 *
 * The specified text string is drawn, with the given alignment, into the bounding box that is
 * provided.
 *
 * You can specify the following options to customize drawing:
 * - Any alignment (HAlignMask): Horizontal text alignment
 * - WordWrap: Wrap on word (space) boundaries, rather than character boundaries
 * - DrawPartialLine: When set, partial glyphs are drawn
 *
 * @param str An UTF-8 encoded string to draw
 * @param fb Framebuffer to draw to
 * @param origin Top left origin for the string
 * @param flags Customizations for drawing
 */
void Font::draw(const etl::string_view str, Framebuffer &fb, const Rect bounds,
        const FontRenderFlags flags) const {
    // sanity checking
    if(str.empty()) {
        return;
    }

    // draw until we reach the end of the string
    auto s = str.data();
    Rect remainingBounds{bounds};
    int height = bounds.size.height;

    while(*s) {
        // draw this line of text
        if(this->processLine(fb, s, remainingBounds, flags)) {
            goto beach;
        }

        // prepare for next row
        height -= this->lineHeight;
        remainingBounds.origin.y += this->lineHeight;
        remainingBounds.size.height -= (remainingBounds.size.height >= this->lineHeight) ?
            this->lineHeight : remainingBounds.size.height;

        // insufficient vertical space for another line
        if(height < this->lineHeight && !TestFlags(flags & FontRenderFlags::DrawPartialLine)) {
            goto beach;
        }
    }

beach:;
    // clean up
}

/**
 * @brief Helper method to draw a single vertical line of text
 *
 * This operates in two stages:
 *
 * 1. Calculate the total number of characters we can fit horizontally. If word wrapping is
 *    enabled, we backtrack here to find the first space or punctuation character to break at, or
 *    if we get to the start of the line, just break at the character boundary anyways.
 *    Either way, we store the range of characters (start/end) and the measured width for that
 *    segment of the string.
 * 2. Render the string; for left or center aligned text, we start from the left (optionally with
 *    an offset for center alignedd) whereas right aligned text is laid out from the right to the
 *    left based on the size of the glyph. This is done in a separate drawing routine.
 *
 * When this is invoked, it's guaranteed that the line should be drawn. If the line is not tall
 * enough for a full line, the characters are cut off, regardless of the value in render flags.
 *
 * @remark This method has a qurik in that if the width of the bounding rectangle is smaller than
 *         a single character, we'll still draw that character - it just might be cut off.
 *
 * @param bounds Rect bounds to draw in
 *
 * @return Whether we've reached the end of the string
 */
bool Font::processLine(Framebuffer &fb, const char* &str, Rect &bounds,
        const FontRenderFlags flags) const {
    bool endOfString{false};
    unsigned int lineWidth{0};
    size_t codepoints{0}, byteOffset{0}, codepointBytes{0};

    // prepare Unicode reading state machine
    uint32_t utfState{Util::Unicode::kStateAccept}, utfCodepoint;
    const Glyph *glyph{nullptr};

    // store the last break point for word wrapping
    const char *wrapEnd{nullptr};
    size_t wrapEndCodepoints{0};
    int wrapEndLineWidth{0};

    // record the start of the line string
    const char *strStart = str;

    // iterate over the string til we fill out a line
    for(; *str; ++str, ++byteOffset, ++codepointBytes) {
        // have not yet consumed an entire codepoint
        if(Util::Unicode::Decode(*str, utfState, utfCodepoint)) {
            continue;
        }
        // figure out how many bytes this codepoint took up, should we need to rewind
        const auto toRewind = codepointBytes;
        codepointBytes = 0;

        // codepoint above 0xFFFF: not yet supported
        REQUIRE(utfCodepoint <= 0xFFFF, "codepoints > 0xFFFF not yet supported");

        // handle control characters
        if(utfCodepoint == '\n') { // newline
            goto draw;
        }
        // skip whitespace at start of line
        else if(utfCodepoint == ' ' && !codepoints) {
            continue;
        }
        // find the associated glyph
        else if(!this->findGlyph(utfCodepoint, glyph)) {
            Logger::Warning("No glyph for codepoint $%04x in font %p (%s)", utfCodepoint, this,
                    this->name.data());
            continue;
        }

        /*
         * Check whether this glyph would make the line too wide; if so, bail out and draw it up
         * until this point. We have to rewind the string then by the number of bytes for this
         * codepoint, so that we can start with it on the next line.
         *
         * Note that this does not apply if we haven't drawn any characters on this line yet. We'll
         * always draw the first character of the line.
         */
        if(codepoints) {
            // TODO: this is where we'd implement word wrapping
            if((lineWidth + glyph->size.width) > bounds.size.width) {
                // if word wrapping, reset to the last break point
                if(TestFlags(flags & FontRenderFlags::WordWrap)) {
                    //this->handleWordWrap(strStart, str, codepoints, lineWidth);
                    if(wrapEnd) {
                        str = wrapEnd;
                        codepoints = wrapEndCodepoints;
                        lineWidth = wrapEndLineWidth;
                    }
                }

                // handle wrapping on multi-byte boundaries
                if(toRewind > 1) {
                    str -= (toRewind - 1);
                }
                goto draw;
            }
        }

        /*
         * Record information for this glyph.
         */
        codepoints++;
        lineWidth += glyph->size.width;

        /*
         * If word wrapping is enabled, record the current position into the line if the character
         * is a point at which we can wrap.
         */
        if(TestFlags(flags & FontRenderFlags::WordWrap)) {
            if(IsWrapPoint(utfCodepoint)) {
                wrapEnd = str;
                wrapEndCodepoints = codepoints;
                wrapEndLineWidth = lineWidth;

                // when breaking on whitespace, don't include it in the size calculation
                if(utfCodepoint == ' ') {
                    wrapEndCodepoints--;
                    wrapEndLineWidth -= glyph->size.width;
                }
            }
        }
    }
    // if we fall through to here, we've reached the end of the string
    endOfString = true;

    // jump down here if the line is too long, rather than us reaching the end of the string
draw:;
    // render the line
    const auto hAlignFlag = (flags & FontRenderFlags::HAlignMask);
    int xOffset{0};

    // to center align, move it right half the remaining space
    if(hAlignFlag == FontRenderFlags::HAlignCenter) {
        xOffset = (bounds.size.width - lineWidth) / 2;
    }
    // for right align, it move it right all the remaining space
    else if(hAlignFlag == FontRenderFlags::HAlignRight) {
        xOffset = (bounds.size.width - lineWidth);
    }

    this->renderLine(fb, strStart, bounds, codepoints, xOffset, flags);

    // return end-of-string indicator
    return endOfString;
}

/**
 * @brief Render a single line of text from the left to right.
 *
 * @remark Only codepoints below 0xffff are currently supported.
 */
void Font::renderLine(Framebuffer &fb, const char *str, const Rect bounds,
        const size_t numCodepoints, const int xOffset, const FontRenderFlags flags) const {
    size_t drawn{0};
    uint32_t utfState{Util::Unicode::kStateAccept}, utfCodepoint;
    const Glyph *glyph{nullptr};

    // handle flags
    auto blitFlags = Framebuffer::BlitFlags::HasTransparency;

    if(TestFlags(flags & FontRenderFlags::Invert)) {
        blitFlags = static_cast<Framebuffer::BlitFlags>(blitFlags | Framebuffer::BlitFlags::Invert);
    }

    // set up for drawing
    Point current{bounds.origin};
    current.x += xOffset;

    for(; *str && drawn < numCodepoints; ++str) {
        if(Util::Unicode::Decode(*str, utfState, utfCodepoint)) {
            // have not yet consumed an entire codepoint, read another byte
            continue;
        }
        if(!this->findGlyph(utfCodepoint, glyph)) {
            // failed to find a glyph for the codepoint
            continue;
        }

        // draw the glyph, and update current position
        auto glyphSize{glyph->size};
        if(glyphSize.height > bounds.size.height) {
            glyphSize.height = bounds.size.height;
        }

        fb.blit4Bpp(glyph->data, glyphSize, current, blitFlags);
        drawn++;

        current.x += glyph->size.width;

        // bail if we hit the edge
        if((current.x - bounds.origin.x) >= bounds.size.width) {
            return;
        }
    }
}

