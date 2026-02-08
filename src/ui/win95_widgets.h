#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../rl.h"

// Win95-style menu types
// These are data structures used by MenuComponent and menu_setup.
// All drawing/interaction is now handled by Afterhours UI in MenuUISystem.
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

}  // namespace win95
