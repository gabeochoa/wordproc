#pragma once

#include "../rl.h"

// Win95-style color theme with Mac OS 3.1 accents
// All colors are centralized here for easy theming
namespace theme {

// Window chrome
inline raylib::Color WINDOW_BG = {192, 192, 192, 255};   // Win95 gray
inline raylib::Color TITLE_BAR = {0, 0, 128, 255};       // Win95 blue
inline raylib::Color TITLE_TEXT = {255, 255, 255, 255};  // White

// Text editing area
inline raylib::Color TEXT_AREA_BG = {255, 255, 255, 255};  // White
inline raylib::Color TEXT_COLOR = {0, 0, 0, 255};          // Black
inline raylib::Color CARET_COLOR = {0, 0, 0, 255};         // Black
inline raylib::Color SELECTION_BG = {0, 0, 128, 255};      // Blue highlight
inline raylib::Color SELECTION_TEXT = {255, 255, 255,
                                       255};  // White text on selection

// 3D borders
inline raylib::Color BORDER_LIGHT = {255, 255, 255, 255};  // 3D border light
inline raylib::Color BORDER_DARK = {128, 128, 128, 255};   // 3D border dark

// Status bar
inline raylib::Color STATUS_BAR = {192, 192, 192, 255};  // Status bar gray
inline raylib::Color STATUS_ERROR = {200, 0, 0, 255};    // Error message red
inline raylib::Color STATUS_SUCCESS = {0, 100, 0,
                                       255};  // Success message green

// Menu colors (Win95 style)
inline raylib::Color MENU_BG = {192, 192, 192, 255};
inline raylib::Color MENU_HOVER = {0, 0, 128, 255};
inline raylib::Color MENU_TEXT = {0, 0, 0, 255};
inline raylib::Color MENU_TEXT_HOVER = {255, 255, 255, 255};
inline raylib::Color MENU_DISABLED = {128, 128, 128, 255};
inline raylib::Color MENU_SEPARATOR = {128, 128, 128, 255};

// Dialog colors
inline raylib::Color DIALOG_BG = {192, 192, 192, 255};
inline raylib::Color DIALOG_TITLE_BG = {0, 0, 128, 255};
inline raylib::Color DIALOG_TITLE_TEXT = {255, 255, 255, 255};

// Button colors
inline raylib::Color BUTTON_BG = {192, 192, 192, 255};
inline raylib::Color BUTTON_TEXT = {0, 0, 0, 255};
inline raylib::Color BUTTON_PRESSED_BG = {128, 128, 128, 255};

// UI Font - initialized in preload, used for all UI text rendering
inline raylib::Font UI_FONT;
inline bool UI_FONT_LOADED = false;

// UI layout constants
namespace layout {
constexpr int FONT_SIZE = 18;  // UI font size (title, menus, status bar) - increased for better readability
constexpr int TITLE_BAR_HEIGHT = 24;
constexpr int MENU_BAR_HEIGHT = 20;
constexpr int STATUS_BAR_HEIGHT = 20;
constexpr int BORDER_WIDTH = 3;
constexpr double STATUS_MESSAGE_DURATION =
    3.0;                                      // Seconds to show status messages
constexpr double CARET_BLINK_INTERVAL = 0.5;  // Seconds between caret blinks

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
// Falls back to raylib default font if UI_FONT not loaded
inline void DrawUIText(const char* text, int x, int y, int fontSize, raylib::Color color) {
    if (UI_FONT_LOADED) {
        raylib::DrawTextEx(UI_FONT, text, {static_cast<float>(x), static_cast<float>(y)}, 
                          static_cast<float>(fontSize), 1.0f, color);
    } else {
        raylib::DrawText(text, x, y, fontSize, color);
    }
}

// Overload for std::string
inline void DrawUIText(const std::string& text, int x, int y, int fontSize, raylib::Color color) {
    DrawUIText(text.c_str(), x, y, fontSize, color);
}

// Helper to measure text with UI font
inline int MeasureUIText(const char* text, int fontSize) {
    if (UI_FONT_LOADED) {
        raylib::Vector2 size = raylib::MeasureTextEx(UI_FONT, text, static_cast<float>(fontSize), 1.0f);
        return static_cast<int>(size.x);
    } else {
        return raylib::MeasureText(text, fontSize);
    }
}

}  // namespace theme
