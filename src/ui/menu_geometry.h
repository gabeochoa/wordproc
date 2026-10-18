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

// Dropdown item buttons sit 2px inside the dropdown edge so the hover
// background never paints over the container's 1px bevel border.
inline constexpr float ITEM_BUTTON_EDGE = 2.0f;

// Label left edge / shortcut right edge inside a dropdown: symmetric
// ~8px insets from the dropdown edge.
inline float dropdownLabelLeft(float dropdownX) {
    return dropdownX + ITEM_PADDING_X;
}
inline float dropdownShortcutRight(float dropdownX, float maxWidth) {
    return dropdownX + maxWidth - ITEM_PADDING_X;
}

// Maximum dropdown width in pixels: long content (notably File > recent
// paths) must not stretch the menu across the screen.
inline constexpr float MAX_DROPDOWN_WIDTH = 400.0f;

// Character budget for a recent-file path inside its "Recent: " label.
// 8 ("Recent: ") + 40 chars at ~7px + 50px slack ≈ 386px < the cap.
inline constexpr std::size_t RECENT_PATH_MAX_CHARS = 40;

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
