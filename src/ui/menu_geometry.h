#pragma once

#include <string>

// Pure menu-bar geometry: no backend, no allocations. The menu system and
// the unit tests share these so a padding/layout change stays consistent
// between header buttons, click rects, and dropdown positions.

namespace menu_geometry {

// Horizontal padding inside a menu-bar header button, per side (Word95:
// ~7px). Callers scale it with theme::layout::scale().
inline constexpr float HEADER_PADDING_X = 7.0f;

// Total header button width from the measured label width plus padding.
inline float headerWidth(float textWidth, float scaledPaddingX) {
    return textWidth + 2.0f * scaledPaddingX;
}

// Horizontal padding inside a dropdown item, per side. The shortcut
// gutter mirrors it so label and shortcut insets are symmetric.
inline constexpr float ITEM_PADDING_X = 8.0f;

// Dropdown width: label + shortcut + insets, capped (see File menu cap).
inline float dropdownWidth(float labelWidth, float shortcutWidth,
                           float scaledPaddingX, float maxWidth) {
    float w = labelWidth + shortcutWidth + 2.0f * scaledPaddingX;
    if (w < 150.0f) w = 150.0f;
    if (w > maxWidth) w = maxWidth;
    return w;
}

// Middle-ellipsize a path to fit maxChars, keeping the filename intact:
// "/Users/gabeoc...name.doc". Returns the input unchanged when it fits.
// Only when the filename alone exceeds the budget is it end-truncated.
inline std::string ellipsizeMiddle(const std::string& text, std::size_t maxChars) {
    if (text.size() <= maxChars || maxChars < 8) return text;
    std::size_t slash = text.rfind('/');
    std::string filename = slash == std::string::npos ? text : text.substr(slash + 1);
    if (filename.size() + 3 >= maxChars) {
        if (filename.size() + 3 <= maxChars) return "..." + filename;
        return filename.substr(0, maxChars - 3) + "...";
    }
    std::size_t headChars = maxChars - 3 - filename.size();
    return text.substr(0, headChars) + "..." + filename;
}

}  // namespace menu_geometry
