#pragma once

#include "../rl.h"
#include "../ui/theme.h"
#include <afterhours/src/drawing_helpers.h>

namespace util {

// Draw Win95-style 3D border (sunken effect for text area)
inline void drawSunkenBorder(raylib::Rectangle rect) {
    // Top and left (dark)
    afterhours::draw_line(static_cast<int>(rect.x), static_cast<int>(rect.y),
                          static_cast<int>(rect.x + rect.width),
                          static_cast<int>(rect.y), theme::BORDER_DARK);
    afterhours::draw_line(static_cast<int>(rect.x), static_cast<int>(rect.y),
                          static_cast<int>(rect.x),
                          static_cast<int>(rect.y + rect.height),
                          theme::BORDER_DARK);
    // Bottom and right (light)
    afterhours::draw_line(
        static_cast<int>(rect.x), static_cast<int>(rect.y + rect.height),
        static_cast<int>(rect.x + rect.width),
        static_cast<int>(rect.y + rect.height), theme::BORDER_LIGHT);
    afterhours::draw_line(
        static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y),
        static_cast<int>(rect.x + rect.width),
        static_cast<int>(rect.y + rect.height), theme::BORDER_LIGHT);
}

}  // namespace util
