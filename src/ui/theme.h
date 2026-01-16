#pragma once

#include "../rl.h"

// Win95-style color theme with Mac OS 3.1 accents
// All colors are centralized here for easy theming
namespace theme {

// Window chrome
constexpr raylib::Color WINDOW_BG = {192, 192, 192, 255};     // Win95 gray
constexpr raylib::Color TITLE_BAR = {0, 0, 128, 255};         // Win95 blue
constexpr raylib::Color TITLE_TEXT = {255, 255, 255, 255};    // White

// Text editing area
constexpr raylib::Color TEXT_AREA_BG = {255, 255, 255, 255};  // White
constexpr raylib::Color TEXT_COLOR = {0, 0, 0, 255};          // Black
constexpr raylib::Color CARET_COLOR = {0, 0, 0, 255};         // Black
constexpr raylib::Color SELECTION_BG = {0, 0, 128, 255};      // Blue highlight
constexpr raylib::Color SELECTION_TEXT = {255, 255, 255, 255};// White text on selection

// 3D borders
constexpr raylib::Color BORDER_LIGHT = {255, 255, 255, 255};  // 3D border light
constexpr raylib::Color BORDER_DARK = {128, 128, 128, 255};   // 3D border dark

// Status bar
constexpr raylib::Color STATUS_BAR = {192, 192, 192, 255};    // Status bar gray
constexpr raylib::Color STATUS_ERROR = {200, 0, 0, 255};      // Error message red
constexpr raylib::Color STATUS_SUCCESS = {0, 100, 0, 255};    // Success message green

// Menu colors (Win95 style)
constexpr raylib::Color MENU_BG = {192, 192, 192, 255};
constexpr raylib::Color MENU_HOVER = {0, 0, 128, 255};
constexpr raylib::Color MENU_TEXT = {0, 0, 0, 255};
constexpr raylib::Color MENU_TEXT_HOVER = {255, 255, 255, 255};
constexpr raylib::Color MENU_DISABLED = {128, 128, 128, 255};
constexpr raylib::Color MENU_SEPARATOR = {128, 128, 128, 255};

// Dialog colors
constexpr raylib::Color DIALOG_BG = {192, 192, 192, 255};
constexpr raylib::Color DIALOG_TITLE_BG = {0, 0, 128, 255};
constexpr raylib::Color DIALOG_TITLE_TEXT = {255, 255, 255, 255};

// Button colors
constexpr raylib::Color BUTTON_BG = {192, 192, 192, 255};
constexpr raylib::Color BUTTON_TEXT = {0, 0, 0, 255};
constexpr raylib::Color BUTTON_PRESSED_BG = {128, 128, 128, 255};

// UI layout constants
namespace layout {
constexpr int FONT_SIZE = 16;              // UI font size (title, menus, status bar)
constexpr int TITLE_BAR_HEIGHT = 24;
constexpr int MENU_BAR_HEIGHT = 20;
constexpr int STATUS_BAR_HEIGHT = 20;
constexpr int TEXT_PADDING = 8;
constexpr int BORDER_WIDTH = 3;
constexpr double STATUS_MESSAGE_DURATION = 3.0;  // Seconds to show status messages
constexpr double CARET_BLINK_INTERVAL = 0.5;     // Seconds between caret blinks
} // namespace layout

} // namespace theme
