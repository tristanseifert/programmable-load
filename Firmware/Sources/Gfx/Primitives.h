/**
 * @file
 *
 * @brief Graphic primitives
 *
 * Implementations of a few low level graphics drawing primitives
 */
#ifndef GFX_PRIMITIVES_H
#define GFX_PRIMITIVES_H

#include <stddef.h>
#include <stdint.h>

#include "Types.h"

namespace Gfx {
class Framebuffer;

void DrawLine(Framebuffer &fb, const Point start, const Point end, const uint32_t strokeColor);

void StrokeRect(Framebuffer &fb, const Point p1, const Point p2, const uint32_t strokeColor);
void FillRect(Framebuffer &fb, const Point p1, const Point p2, const uint32_t fillColor);
/**
 * @brief Draw a filled and outlined rectangle.
 *
 * The rectangle's filling will be inset one pixel to accomodate the border.
 *
 * @param fb Framebuffer to draw on
 * @param p1 Top left point
 * @param p2 Bottom right point
 * @param strokeColor Color for the border of the rectangle
 * @param fillColor Color for the fill of the rectangle
 */
inline void DrawRect(Framebuffer &fb, const Point p1, const Point p2,
        const uint32_t strokeColor, const uint32_t fillColor) {
    FillRect(fb, MakePoint<int>(p1.x + 1, p1.y + 1), MakePoint(p2.x - 1, p2.y - 1), fillColor);
    StrokeRect(fb, p1, p2, strokeColor);
}

void StrokeCircle(Framebuffer &fb, const Point center, const uint16_t radius, const uint32_t strokeColor);
void FillCircle(Framebuffer &fb, const Point center, const uint16_t radius, const uint32_t fillColor);
/**
 * @brief Draw a filled and outlined circle.
 *
 * This renders a circle, first by filling it, then stroking the outline.
 *
 * @param fb Framebuffer to draw on
 * @param center Center of the circle
 * @param radius Radius of circle, in pixels
 * @param strokeColor Color to paint the border with
 * @param fillColor Color to fill the circle with
 */
inline void DrawCircle(Framebuffer &fb, const Point center, const uint16_t radius,
        const uint32_t strokeColor, const uint32_t fillColor) {
    FillCircle(fb, center, radius, fillColor);
    StrokeCircle(fb, center, radius, strokeColor);
}

void StrokeArc(Framebuffer &fb, const Point center, const Point start, const float theta,
        const uint32_t strokeColor);

}

#endif
