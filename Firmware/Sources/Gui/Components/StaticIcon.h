#ifndef GUI_COMPONENTS_STATICICON_H
#define GUI_COMPONENTS_STATICICON_H

namespace Gui::Components {
/**
 * @brief Static icon
 *
 * Renders a Gfx::Icon in the UI
 */
class StaticIcon {
    public:
        /**
         * @brief Draw the icon
         */
        static void Draw(Gfx::Framebuffer &fb, const ComponentData &data) {
            FillRect(fb, data.bounds, 0x0);

            if(!data.staticIcon.hideIcon) {
                data.staticIcon.icon->draw(fb, data.bounds.origin);
            }
        }
};
}

#endif
