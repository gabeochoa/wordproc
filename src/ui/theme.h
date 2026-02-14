#pragma once

#include "../rl.h"
#include "../settings.h"
#include <afterhours/src/drawing_helpers.h>

// Win95-style color theme with Mac OS 3.1 accents
// All colors are centralized here for easy theming
namespace theme {

// Use backend-agnostic types
using Color = afterhours::Color;

// Window chrome
inline Color WINDOW_BG = {192, 192, 192, 255};   // Win95 gray
inline Color TITLE_BAR = {0, 0, 128, 255};       // Win95 blue
inline Color TITLE_TEXT = {255, 255, 255, 255};  // White

// Text editing area
inline Color TEXT_AREA_BG = {255, 255, 255, 255};  // White
inline Color TEXT_COLOR = {0, 0, 0, 255};          // Black
inline Color CARET_COLOR = {0, 0, 0, 255};         // Black
inline Color SELECTION_BG = {0, 0, 128, 255};      // Blue highlight
inline Color SELECTION_TEXT = {255, 255, 255,
                                       255};  // White text on selection

// 3D borders
inline Color BORDER_LIGHT = {255, 255, 255, 255};  // 3D border light
inline Color BORDER_DARK = {128, 128, 128, 255};   // 3D border dark

// Status bar
inline Color STATUS_BAR = {192, 192, 192, 255};  // Status bar gray
inline Color STATUS_ERROR = {200, 0, 0, 255};    // Error message red
inline Color STATUS_SUCCESS = {0, 100, 0,
                                       255};  // Success message green

// Menu colors (Win95 style)
inline Color MENU_BG = {192, 192, 192, 255};
inline Color MENU_HOVER = {0, 0, 128, 255};
inline Color MENU_TEXT = {0, 0, 0, 255};
inline Color MENU_TEXT_HOVER = {255, 255, 255, 255};
inline Color MENU_DISABLED = {128, 128, 128, 255};
inline Color MENU_SEPARATOR = {128, 128, 128, 255};

// Dialog colors
inline Color DIALOG_BG = {192, 192, 192, 255};
inline Color DIALOG_TITLE_BG = {0, 0, 128, 255};
inline Color DIALOG_TITLE_TEXT = {255, 255, 255, 255};

// Button colors
inline Color BUTTON_BG = {192, 192, 192, 255};
inline Color BUTTON_FACE = {192, 192, 192, 255};
inline Color BUTTON_TEXT = {0, 0, 0, 255};
inline Color BUTTON_PRESSED_BG = {128, 128, 128, 255};

// Toolbar colors
inline Color TOOLBAR_BG = {192, 192, 192, 255};
inline Color TOOLBAR_HOVER_BG = {220, 220, 220, 255};
inline Color TOOLBAR_PRESSED_BG = {160, 160, 160, 255};
inline Color TOOLBAR_BUTTON_HOVER = {220, 220, 220, 255};
inline Color TOOLBAR_BUTTON_PRESSED = {160, 160, 160, 255};
inline Color TOOLBAR_SEPARATOR = {128, 128, 128, 255};

// Ruler colors
inline Color RULER_BG = {255, 255, 255, 255};
inline Color RULER_MARKS = {0, 0, 0, 255};
inline Color RULER_TEXT = {0, 0, 0, 255};

// UI Font - initialized in preload, used for all UI text rendering
inline afterhours::Font UI_FONT;
inline bool UI_FONT_LOADED = false;

// UI layout constants
namespace layout {
// Base UI scaling functions - multiply all UI sizes by Settings ui_scale
inline float scale(float baseSize) {
    return baseSize * Settings::get().get_ui_scale();
}

inline int scaleInt(int baseSize) {
    return static_cast<int>(scale(static_cast<float>(baseSize)));
}

// Base sizes (before scaling)
constexpr int FONT_SIZE = 18;  // UI font size (title, menus, status bar) - increased for better readability
constexpr int TITLE_BAR_HEIGHT = 26;
constexpr int MENU_BAR_HEIGHT = 23;
constexpr int TOOLBAR_HEIGHT = 28;
constexpr int FORMATTING_BAR_HEIGHT = 28;
constexpr int RULER_HEIGHT = 20;
constexpr int STATUS_BAR_HEIGHT = 20;
constexpr int BORDER_WIDTH = 3;
constexpr double STATUS_MESSAGE_DURATION =
    3.0;                                      // Seconds to show status messages
constexpr double CARET_BLINK_INTERVAL = 0.5;  // Seconds between caret blinks

// Toolbar button sizes
constexpr int TOOLBAR_BUTTON_SIZE = 24;  // Square icon buttons
constexpr int TOOLBAR_BUTTON_PADDING = 2;
constexpr int TOOLBAR_SEPARATOR_WIDTH = 2;

// Minimum window dimensions — ensures all menu headers, toolbar buttons,
// and formatting bar controls remain accessible (not clipped off-screen).
// Derived from: 8 menu headers ≈ 360px + padding, toolbar row ≈ 300px.
constexpr int MIN_WINDOW_WIDTH = 500;
// Chrome height ≈ 145px at scale 1.0; need at least some editing area.
constexpr int MIN_WINDOW_HEIGHT = 300;

// Spacing scale (4/8/16-based rhythm for consistent margins/gutters/padding)
constexpr int SPACING_XS = 4;   // Extra small: tight spacing, icons
constexpr int SPACING_SM = 8;   // Small: text padding, menu items
constexpr int SPACING_MD = 16;  // Medium: section spacing, dialog padding
constexpr int SPACING_LG = 24;  // Large: major sections
constexpr int SPACING_XL = 32;  // Extra large: page margins

// Convenience aliases for common uses
constexpr int TEXT_PADDING = SPACING_SM;
constexpr int ICON_SPACING = SPACING_XS;
constexpr int MENU_ITEM_PADDING = SPACING_SM;
constexpr int DIALOG_PADDING = SPACING_MD;
constexpr int PAGE_MARGIN = SPACING_XL;
}  // namespace layout

inline bool DARK_MODE_ENABLED = false;
void applyDarkMode(bool enabled);

// Helper function to draw text with the UI font
// Falls back to default font if UI_FONT not loaded
// Automatically applies UI scaling to font size
inline void DrawUIText(const char* text, int x, int y, int baseFontSize, Color color) {
    int scaledSize = layout::scaleInt(baseFontSize);
    if (UI_FONT_LOADED) {
        afterhours::draw_text_ex(UI_FONT, text, {static_cast<float>(x), static_cast<float>(y)}, 
                                 static_cast<float>(scaledSize), 1.0f, color);
    } else {
        afterhours::draw_text(text, static_cast<float>(x), static_cast<float>(y),
                              static_cast<float>(scaledSize), color);
    }
}

// Overload for std::string
inline void DrawUIText(const std::string& text, int x, int y, int fontSize, Color color) {
    DrawUIText(text.c_str(), x, y, fontSize, color);
}

// Helper to measure text with UI font
// Automatically applies UI scaling to font size
inline int MeasureUIText(const char* text, int baseFontSize) {
    int scaledSize = layout::scaleInt(baseFontSize);
    if (UI_FONT_LOADED) {
        vec2 size = afterhours::measure_text(UI_FONT, text, static_cast<float>(scaledSize), 1.0f);
        return static_cast<int>(size.x);
    } else {
        return afterhours::graphics::measure_text(text, scaledSize);
    }
}

}  // namespace theme
