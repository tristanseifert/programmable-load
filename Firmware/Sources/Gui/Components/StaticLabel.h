#ifndef GUI_COMPONENTS_STATICLABEL_H
#define GUI_COMPONENTS_STATICLABEL_H

#include "Base.h"
#include "Gfx/Font.h"

#include <etl/string_view.h>

namespace Gui::Components {
/**
 * @brief Static text label
 *
 * Displays a fixed string in the user interface. It does not support any user interactivity.
 *
 * The text may be drawn 
 */
class StaticLabel: public Base {
    public:
        StaticLabel(const Gfx::Rect bounds, etl::string_view content, const Gfx::Font &font) :
            Base(bounds), string(content), font(&font) {}
        StaticLabel(const Gfx::Rect bounds, etl::string_view content, const Gfx::Font &font,
                const Gfx::Font::RenderFlags fontMode) : Base(bounds), string(content),
                font(&font), fontMode(fontMode) {}

        StaticLabel(const Gfx::Rect bounds, etl::string_view content, const Gfx::Font *font) :
            Base(bounds), string(content), font(font) {}
        StaticLabel(const Gfx::Rect bounds, etl::string_view content, const Gfx::Font *font,
                const Gfx::Font::RenderFlags fontMode) : Base(bounds), string(content), font(font),
                fontMode(fontMode) {}

        /**
         * @brief Label contents
         *
         * This is the string that will be drawn inside the label.
         */
        etl::string_view string;

        /**
         * @brief Font to render the label with
         */
        const Gfx::Font *font;

        /**
         * @brief Text rendering mode
         *
         * These are the flags used to render the font.
         */
        Gfx::Font::RenderFlags fontMode{Gfx::Font::RenderFlags::HAlignLeft |
            Gfx::Font::RenderFlags::WordWrap};

        /**
         * @brief Draw label inverted
         *
         * When set, the background will be filled (rather than transparent) and the text will be
         * drawn inverted on top.
         */
        uintptr_t isInverted:1{0};

    public:
        void draw(Gfx::Framebuffer &) override;
};
}

#endif
