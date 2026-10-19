#pragma once

// The Win95 afterhours theme, split out of ui_context.h so it can be built (and
// tested) without the graphics backend: this header only needs the UI theme
// type, not the whole UI plugin.

#include <afterhours/src/plugins/ui/theme.h>

namespace ui_imm {

// Win95 color palette for Afterhours Theme
namespace win95_colors {
constexpr afterhours::Color WINDOW_BG = {192, 192, 192, 255};
constexpr afterhours::Color TITLE_BAR = {0, 0, 128, 255};
constexpr afterhours::Color TEXT = {0, 0, 0, 255};
constexpr afterhours::Color TEXT_WHITE = {255, 255, 255, 255};
constexpr afterhours::Color TEXT_DISABLED = {128, 128, 128, 255};
constexpr afterhours::Color TEXT_AREA = {255, 255, 255, 255};
constexpr afterhours::Color HIGHLIGHT = {0, 0, 128, 255};
constexpr afterhours::Color BUTTON_FACE = {192, 192, 192, 255};
constexpr afterhours::Color BORDER_LIGHT = {255, 255, 255, 255};
constexpr afterhours::Color BORDER_DARK = {128, 128, 128, 255};
constexpr afterhours::Color ERROR_COLOR = {255, 0, 0, 255};
}  // namespace win95_colors

// Focus ring (afterhours draws it for visual_focus_id; feature request #5).
// Win95 draws a 1px focus rectangle just inside the control's edge, so the ring
// is thin and inset rather than the library default 3px at 4px offset. Black,
// because the ring defaults to the theme's font colour, which here is the white
// used for text on the navy title bar and would vanish on the gray chrome.
constexpr float WIN95_FOCUS_RING_THICKNESS = 1.0f;
constexpr float WIN95_FOCUS_RING_OFFSET = 2.0f;
constexpr afterhours::Color WIN95_FOCUS_RING_COLOR = win95_colors::TEXT;

// Popup drop shadow (feature request #10). Win95 casts a hard 2px shadow on the
// right and bottom edges of menus and dropdowns -- no blur, which is what
// ShadowStyle::Hard means.
constexpr float WIN95_POPUP_SHADOW_OFFSET = 2.0f;
constexpr afterhours::Color WIN95_POPUP_SHADOW_COLOR = {0, 0, 0, 96};

// Create Win95 theme for Afterhours UI
inline afterhours::ui::Theme createWin95Theme() {
    afterhours::ui::Theme theme;

    // Font colors
    theme.font = win95_colors::TEXT_WHITE;
    theme.darkfont = win95_colors::TEXT;
    theme.font_muted = win95_colors::TEXT_DISABLED;

    // Background colors
    theme.background = win95_colors::WINDOW_BG;
    theme.surface = win95_colors::TEXT_AREA;

    // UI element colors
    theme.primary = win95_colors::TITLE_BAR;
    theme.secondary = win95_colors::BUTTON_FACE;
    // Accent is the only app-side knob for the text_input / text_area
    // focused border: afterhours hardcodes it as
    // Border::all(theme.accent, 2px) while focused (see feature request
    // #17). Black is the closest Win95 reading — the real control is a
    // dark sunken inset, not a coloured outline — and wordproc uses
    // no upstream radio / switch / tab widget that would also read
    // accent. Do not set it back to HIGHLIGHT: that is the navy ring
    // the Batch 10 review flagged on every focused field.
    theme.accent = win95_colors::TEXT;
    theme.error = win95_colors::ERROR_COLOR;

    // Keyboard focus indicator (WCAG 2.4.7)
    theme.focus = WIN95_FOCUS_RING_COLOR;
    theme.focus_ring_thickness = WIN95_FOCUS_RING_THICKNESS;
    theme.focus_ring_offset = WIN95_FOCUS_RING_OFFSET;

    // Win95 has sharp corners (no rounded corners)
    theme.rounded_corners = std::bitset<4>();
    theme.roundness = 0.0f;
    theme.segments = 0;
    // The library default is 8px, and it is NOT covered by rounded_corners
    // / roundness: any widget that sets corner bits without its own
    // radius (and the upstream tooltip, which reads this field directly)
    // would still draw rounded. Zero it so square is the theme-wide
    // default, not just the common case.
    theme.corner_radius = 0.0f;

    return theme;
}

}  // namespace ui_imm
