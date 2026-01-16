#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../rl.h"

// Win95-style color palette
namespace win95 {
namespace colors {
constexpr raylib::Color WINDOW_BG = {192, 192, 192, 255};
constexpr raylib::Color TITLE_BAR_ACTIVE = {0, 0, 128, 255};
constexpr raylib::Color TITLE_BAR_INACTIVE = {128, 128, 128, 255};
constexpr raylib::Color TITLE_TEXT = {255, 255, 255, 255};
constexpr raylib::Color TEXT_AREA_BG = {255, 255, 255, 255};
constexpr raylib::Color TEXT_COLOR = {0, 0, 0, 255};
constexpr raylib::Color TEXT_DISABLED = {128, 128, 128, 255};
constexpr raylib::Color BORDER_LIGHT = {255, 255, 255, 255};
constexpr raylib::Color BORDER_DARK = {128, 128, 128, 255};
constexpr raylib::Color BORDER_DARKER = {64, 64, 64, 255};
constexpr raylib::Color MENU_HIGHLIGHT = {0, 0, 128, 255};
constexpr raylib::Color BUTTON_FACE = {192, 192, 192, 255};
}  // namespace colors

// Button state for tracking interaction
enum class ButtonState { Normal, Hover, Pressed, Disabled };

// Menu item structure
struct MenuItem {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    bool separator = false;
    std::function<void()> action;
};

// Menu structure
struct Menu {
    std::string label;
    std::vector<MenuItem> items;
    bool open = false;
    raylib::Rectangle bounds;
};

// Draw a Win95-style raised border (buttons, panels)
void DrawRaisedBorder(raylib::Rectangle rect, int thickness = 1);

// Draw a Win95-style sunken border (text fields, list boxes)
void DrawSunkenBorder(raylib::Rectangle rect, int thickness = 1);

// Draw a Win95-style button
// Returns true if clicked
bool DrawButton(raylib::Rectangle rect, const char* text, bool enabled = true);

// Draw a Win95-style checkbox
// Returns true if state changed
bool DrawCheckbox(raylib::Rectangle rect, const char* text, bool* checked,
                  bool enabled = true);

// Draw menu bar and handle interaction
// Returns index of clicked menu item, or -1 if none
int DrawMenuBar(std::vector<Menu>& menus, int menuBarY, int menuBarHeight);

// Draw a dropdown menu
// Returns index of selected item, or -1 if none
int DrawDropdownMenu(Menu& menu, int x, int y, int itemHeight);

// Draw a message dialog
// Returns: 0 = OK, 1 = Cancel, -1 = still open
int DrawMessageDialog(raylib::Rectangle dialogRect, const char* title,
                      const char* message, bool hasCancel = false);

// Draw a simple input dialog
// Returns: 0 = OK, 1 = Cancel, -1 = still open
int DrawInputDialog(raylib::Rectangle dialogRect, const char* title,
                    const char* prompt, char* buffer, int bufferSize);

// Dialog state management
struct DialogState {
    bool active = false;
    int result = -1;
    std::string inputBuffer;
};

}  // namespace win95
