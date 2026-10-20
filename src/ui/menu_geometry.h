#pragma once

#include <string>

// Pure menu-bar geometry: no backend, no allocations. The menu system and
// the unit tests share these so a padding/layout change stays consistent
// between header buttons, click rects, and dropdown positions.

namespace menu_geometry {

// Horizontal padding inside a menu-bar header button, per side (Word95:
// ~7px). Callers scale it with theme::layout::scale().
inline constexpr float HEADER_PADDING_X = 6.0f;  // 6-8px Win95 range; scale() applies display factor

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

// Mark prefixes the dropdown button draws in front of a label
// (MenuUISystem builds the same strings). One source of truth so the
// warm-measure enumeration below cannot drift from what the dropdown
// actually measures: "  " reserves the mark column for an unmarked
// item, "\xE2\x88\x9A " is the U+221A checkmark, "\xE2\x80\xA2 " the
// U+2022 radio bullet, "- " the dash.
inline constexpr const char* MARK_PREFIX_NONE = "  ";
inline constexpr const char* MARK_PREFIX_CHECKMARK = "\xE2\x88\x9A ";
inline constexpr const char* MARK_PREFIX_RADIO = "\xE2\x80\xA2 ";
inline constexpr const char* MARK_PREFIX_DASH = "- ";

// Font size (px) the renderer draws menu labels at: the UI default font,
// Proportional (no Settings scale). Menu code must measure shortcut
// boxes at this size, NOT via theme::MeasureUIText (which applies the
// Settings ui_scale): at ui_scale != 1 the two disagree, so a box
// sized from the scaled measure fits text that is never drawn —
// and at ui_scale < 1 the shortcut text is wider than its box,
// clips, and trips the overflow overlay.
inline constexpr float DRAWN_FONT_SIZE_PX = 14.0f;

// Where the dropdown measures a string (Perf Round 6). The menu bar
// measures the same string at two size conventions, so a warm pass
// must cover both or the first open still pays a fontstash walk:
// - HeaderWidth / MnemonicWidth: MenuUISystem's own
//   theme::MeasureUIText calls (base size 16, Settings-scaled).
// - Drawn: the autolayout + renderer size, DRAWN_FONT_SIZE_PX
//   (Proportional scaling, no Settings scale), for every label the
//   button actually draws — including shortcuts, whose boxes are
//   sized from the Drawn-size measure.
enum class MenuWarmSite { HeaderWidth, MnemonicWidth, Drawn };

// Every (string, site) pair the dropdown measures for these menus,
// for the startup warm pass. Pure / backend-free so unit tests can
// pin the exact set: item labels are emitted with every mark prefix
// (the button draws markPrefix + showLabel, and marks change at
// runtime — View checkmarks, Track Changes — so warming only the
// current prefix would leave the checked form cold), with the
// display label (ellipsized recent path) preferred over the raw
// label, exactly as the dropdown measures. Separators measure
// nothing. Menus is duck-typed ({.label, .items[] with .label,
// .shortcut, .separator, .displayLabel}) so tests need no win95 types.
template <typename Menus, typename Fn>
void forEachMenuWarmString(const Menus& menus, Fn&& fn) {
    const char* prefixes[] = {MARK_PREFIX_NONE, MARK_PREFIX_CHECKMARK,
                              MARK_PREFIX_RADIO, MARK_PREFIX_DASH};
    for (const auto& menu : menus) {
        fn(menu.label, MenuWarmSite::HeaderWidth);
        if (!menu.label.empty()) {
            fn(menu.label.substr(0, 1), MenuWarmSite::MnemonicWidth);
            fn(menu.label.substr(0, 1), MenuWarmSite::Drawn);
        }
        fn(menu.label, MenuWarmSite::Drawn);
        for (const auto& item : menu.items) {
            if (item.separator) continue;
            const std::string& show =
                item.displayLabel.empty() ? item.label : item.displayLabel;
            for (const char* prefix : prefixes) {
                fn(std::string(prefix) + show, MenuWarmSite::Drawn);
            }
            if (!item.shortcut.empty()) {
                fn(item.shortcut, MenuWarmSite::Drawn);
            }
        }
    }
}

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
