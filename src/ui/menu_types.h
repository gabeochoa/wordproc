#pragma once

#include <functional>
#include <string>
#include <vector>

// Menu item structure (independent of raylib)
struct MenuItemData {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    bool isSeparator = false;
};

// Menu structure for testing (without raylib dependencies)
struct MenuData {
    std::string label;
    std::vector<MenuItemData> items;
};

// Create menu bar data (for testing menu structure)
inline std::vector<MenuData> createMenuBarData() {
    std::vector<MenuData> menus;

    // File menu
    MenuData fileMenu;
    fileMenu.label = "File";
    fileMenu.items = {
        {"New", "Ctrl+N", true, false},
        {"Open...", "Ctrl+O", true, false},
        {"Save", "Ctrl+S", true, false},
        {"Save As...", "", true, false},
        {"", "", false, true},  // Separator
        {"Exit", "Alt+F4", true, false}
    };
    menus.push_back(fileMenu);

    // Edit menu
    MenuData editMenu;
    editMenu.label = "Edit";
    editMenu.items = {
        {"Undo", "Ctrl+Z", true, false},
        {"Redo", "Ctrl+Y", true, false},
        {"", "", false, true},  // Separator
        {"Cut", "Ctrl+X", true, false},
        {"Copy", "Ctrl+C", true, false},
        {"Paste", "Ctrl+V", true, false},
        {"", "", false, true},  // Separator
        {"Select All", "Ctrl+A", true, false}
    };
    menus.push_back(editMenu);

    // View menu
    MenuData viewMenu;
    viewMenu.label = "View";
    viewMenu.items = {
        {"Pageless Mode", "", true, false},
        {"Paged Mode", "", true, false},
        {"", "", false, true},  // Separator
        {"Line Width: Normal", "", true, false},
        {"Line Width: Narrow", "", true, false},
        {"Line Width: Wide", "", true, false}
    };
    menus.push_back(viewMenu);

    // Format menu
    MenuData formatMenu;
    formatMenu.label = "Format";
    formatMenu.items = {
        {"Bold", "Ctrl+B", true, false},
        {"Italic", "Ctrl+I", true, false},
        {"", "", false, true},  // Separator
        {"Font: Gaegu", "Ctrl+1", true, false},
        {"Font: Garamond", "Ctrl+2", true, false},
        {"", "", false, true},  // Separator
        {"Increase Size", "Ctrl++", true, false},
        {"Decrease Size", "Ctrl+-", true, false},
        {"Reset Size", "Ctrl+0", true, false}
    };
    menus.push_back(formatMenu);

    // Help menu
    MenuData helpMenu;
    helpMenu.label = "Help";
    helpMenu.items = {
        {"About Wordproc", "", true, false}
    };
    menus.push_back(helpMenu);

    return menus;
}
