#pragma once

#include "../rl.h"
#include "../ui/theme.h"

namespace util {

// Draw Win95-style 3D border (raised effect)
inline void drawRaisedBorder(raylib::Rectangle rect) {
    // Top and left (light)
    raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                     static_cast<int>(rect.x + rect.width),
                     static_cast<int>(rect.y), theme::BORDER_LIGHT);
    raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                     static_cast<int>(rect.x),
                     static_cast<int>(rect.y + rect.height),
                     theme::BORDER_LIGHT);
    // Bottom and right (dark)
    raylib::DrawLine(
        static_cast<int>(rect.x), static_cast<int>(rect.y + rect.height),
        static_cast<int>(rect.x + rect.width),
        static_cast<int>(rect.y + rect.height), theme::BORDER_DARK);
    raylib::DrawLine(
        static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y),
        static_cast<int>(rect.x + rect.width),
        static_cast<int>(rect.y + rect.height), theme::BORDER_DARK);
}

// Draw Win95-style 3D border (sunken effect for text area)
inline void drawSunkenBorder(raylib::Rectangle rect) {
    // Top and left (dark)
    raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                     static_cast<int>(rect.x + rect.width),
                     static_cast<int>(rect.y), theme::BORDER_DARK);
    raylib::DrawLine(static_cast<int>(rect.x), static_cast<int>(rect.y),
                     static_cast<int>(rect.x),
                     static_cast<int>(rect.y + rect.height),
                     theme::BORDER_DARK);
    // Bottom and right (light)
    raylib::DrawLine(
        static_cast<int>(rect.x), static_cast<int>(rect.y + rect.height),
        static_cast<int>(rect.x + rect.width),
        static_cast<int>(rect.y + rect.height), theme::BORDER_LIGHT);
    raylib::DrawLine(
        static_cast<int>(rect.x + rect.width), static_cast<int>(rect.y),
        static_cast<int>(rect.x + rect.width),
        static_cast<int>(rect.y + rect.height), theme::BORDER_LIGHT);
}

}  // namespace util
