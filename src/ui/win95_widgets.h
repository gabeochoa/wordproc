#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../rl.h"

// Win95-style widgets
// Note: Colors are now centralized in theme.h
namespace win95 {

// Menu mark type for standard menu conventions
enum class MenuMark {
    None,       // No mark - regular menu item
    Checkmark,  // Checkmark - current selection or enabled toggle
    Radio,      // Radio bullet - one of a group
    Dash        // Dash - partial/mixed state
};

// Menu item structure
struct MenuItem {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    bool separator = false;
    std::function<void()> action;
    MenuMark mark = MenuMark::None;  // Standard mark (checkmark/radio/dash)
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


// Draw menu bar and handle interaction
// Returns index of clicked menu item, or -1 if none
int DrawMenuBar(std::vector<Menu>& menus, int menuBarY, int menuBarHeight);

// Draw a dropdown menu
// Returns index of selected item, or -1 if none
int DrawDropdownMenu(Menu& menu, int x, int y, int itemHeight);


}  // namespace win95
