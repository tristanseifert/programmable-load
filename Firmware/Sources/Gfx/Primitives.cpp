#include "Primitives.h"
#include "Framebuffer.h"

#include "Log/Logger.h"

#include <math.h>

using namespace Gfx;

/**
 * @brief Draw a line between two points
 *
 * Draws a line between the starting and ending points. This uses the [Extremely Fast Line
 * Algorithm](http://www.edepot.com/algorithm.html) by Po-Han Lin to generate the points to draw;
 * specifically, variation C (using addition) is used.
 *
 * @param fb Framebuffer to draw the line on
 * @param start Starting point
 * @param end Ending point
 * @param color Pixel value to plot the line in
 */
void Gfx::DrawLine(Framebuffer &fb, const Point start, const Point end, const uint32_t color) {
    bool yLonger{false};
    int incrementVal, endVal;

    int shortLen = end.y - start.y;
    int longLen = end.x - start.x;

    if(abs(shortLen) > abs(longLen)) {
        int swap = shortLen;
        shortLen = longLen;
        longLen = swap;
        yLonger = true;
    }

    endVal = longLen;
    if(longLen < 0) {
        incrementVal = -1;
        longLen = -longLen;
    } else {
        incrementVal = 1;
    }

    float decInc;
    if(longLen == 0) {
        decInc = static_cast<float>(shortLen);
    } else {
        decInc = (static_cast<float>(shortLen) / static_cast<float>(longLen));
    }

    float j{0.f};
    if(yLonger) {
        for(int i = 0; i != endVal; i += incrementVal) {
            fb.setPixel(MakePoint<int>((start.x + (int) j), (start.y + i)), color);
            j += decInc;
        }
    } else {
        for(int i = 0; i != endVal; i += incrementVal) {
            fb.setPixel(MakePoint<int>((start.x + i), (start.y + (int) j)), color);
            j += decInc;
        }
    }
}

/**
 * @brief Draw an outlined rectangle
 *
 * Renders a rectangle by stroking its borders.
 *
 * @param fb Framebuffer to draw on
 * @param p1 Top left point
 * @param p2 Bottom right point
 * @param strokeColor Color for the border of the rectangle
 */
void Gfx::DrawRect(Framebuffer &fb, const Point p1, const Point p2, const uint32_t strokeColor) {
    DrawLine(fb, p1, MakePoint(p2.x, p1.y), strokeColor);
    DrawLine(fb, MakePoint(p2.x, p1.y), MakePoint(p2.x, p2.y), strokeColor);
    DrawLine(fb, p2, MakePoint(p1.x, p2.y), strokeColor);
    DrawLine(fb, MakePoint(p1.x, p2.y), p1, strokeColor);
}

/**
 * @brief Draw a filled rectangle
 *
 * A filled rectangle is drawn. The entire region enclosed by the specified points will be filled;
 * to draw a border, draw an outlined rectangle one pixel larger in each direction.
 *
 * @param fb Framebuffer to draw on
 * @param p1 Top left point
 * @param p2 Bottom right point
 * @param fillColor Color for the fill of the rectangle
 */
void Gfx::FillRect(Framebuffer &fb, const Point p1, const Point p2, const uint32_t fillColor) {
    for(uint16_t y = p1.y; y <= p2.y; y++) {
        for(uint16_t x = p1.x; x <= p2.x; x++) {
            fb.setPixel(MakePoint(x, y), fillColor);
        }
    }
}



/**
 * @brief Midpoint circle drawing algorithm helper
 *
 * This draws points in all eight octants of the circle (exploiting its symmetry) for better
 * drawing performance.
 *
 * @param fb Framebuffer to draw on
 * @param center Center point of the circle
 * @param 
 */
static void DrawCircleHelper(Framebuffer &fb, const Point center, const int16_t x, const int16_t y,
        const uint32_t strokeColor) {
    if(!x) {
        fb.setPixel(MakePoint<int>(center.x, center.y + y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x, center.y - y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x + y, center.y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x - y, center.y), strokeColor);
    } else if(x == y) {
        fb.setPixel(MakePoint<int>(center.x + x, center.y + y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x - x, center.y + y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x + x, center.y - y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x - x, center.y - y), strokeColor);
    } else if(x < y) {
        fb.setPixel(MakePoint<int>(center.x + x, center.y + y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x - x, center.y + y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x + x, center.y - y), strokeColor);
        fb.setPixel(MakePoint<int>(center.x - x, center.y - y), strokeColor);

        fb.setPixel(MakePoint<int>(center.x + y, center.y + x), strokeColor);
        fb.setPixel(MakePoint<int>(center.x - y, center.y + x), strokeColor);
        fb.setPixel(MakePoint<int>(center.x + y, center.y - x), strokeColor);
        fb.setPixel(MakePoint<int>(center.x - y, center.y - x), strokeColor);
    }
}

/**
 * @brief Draw an outlined circle
 *
 * Draws a circle, centered at the given point, with the specified radius. It will be stroked with
 * the specified color.
 *
 * This implements the midpoint circle drawing algorithm.
 *
 * @param fb Framebuffer to draw on
 * @param center Center of the circle
 * @param radius Radius of circle, in pixels
 * @param strokeColor Color to paint the border with
 */
void Gfx::DrawCircle(Framebuffer &fb, const Point center, const uint16_t radius,
        const uint32_t strokeColor) {
    int16_t x{0};
    int16_t y = radius;
    int p = (5 - radius*4) / 4;

    DrawCircleHelper(fb, center, x, y, strokeColor);

    while(x < y) {
        x++;

        if(p < 0) {
            p += 2*x+1;
        } else {
            y--;
            p += 2*(x-y)+1;
        }

        DrawCircleHelper(fb, center, x, y, strokeColor);
    }
}

/**
 * @brief Draw a filled circle
 *
 * Draw and fill a circle centered at the given point.
 *
 * @param fb Framebuffer to draw on
 * @param center Center of the circle
 * @param radius Radius of circle, in pixels
 * @param strokeColor Color to fill the circle with
 */
void Gfx::FillCircle(Framebuffer &fb, const Point center, const uint16_t radius,
        const uint32_t fillColor) {
    int r = radius;

    for(int x = -r; x < r; x++) {
        int height = static_cast<int>(sqrtf(r * r - x * x));

        for(int y = -height; y <= height; y++) {
            fb.setPixel(MakePoint<int>(center.x + x, center.y + y), fillColor);
        }
    }
}

