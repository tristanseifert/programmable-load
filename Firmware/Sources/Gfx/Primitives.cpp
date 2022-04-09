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

    int shortLen = end.y-start.y;
    int longLen = end.x-start.x;
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
            fb.setPixel(
                    {static_cast<uint16_t>(start.x + (int) j), static_cast<uint16_t>(start.y + i)},
                    color);
            j+=decInc;
        }
    } else {
        for(int i = 0; i != endVal; i += incrementVal) {
            fb.setPixel(
                    {static_cast<uint16_t>(start.x + i), static_cast<uint16_t>(start.y + (int) j)},
                    color);
            j+=decInc;
        }
    }
}

