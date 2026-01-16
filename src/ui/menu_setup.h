#pragma once

#include <vector>

#include "win95_widgets.h"

namespace menu_setup {

// Create the standard Win95-style menu bar for the word processor
inline std::vector<win95::Menu> createMenuBar() {
    std::vector<win95::Menu> menus;

    // File menu
    win95::Menu fileMenu;
    fileMenu.label = "File";
    fileMenu.items = {{"New", "Ctrl+N", true, false, nullptr},
                      {"Open...", "Ctrl+O", true, false, nullptr},
                      {"Save", "Ctrl+S", true, false, nullptr},
                      {"Save As...", "", true, false, nullptr},
                      {"", "", false, true, nullptr},  // Separator
                      {"Exit", "Alt+F4", true, false, nullptr}};
    menus.push_back(fileMenu);

    // Edit menu
    win95::Menu editMenu;
    editMenu.label = "Edit";
    editMenu.items = {{"Undo", "Ctrl+Z", true, false, nullptr},
                      {"Redo", "Ctrl+Y", true, false, nullptr},
                      {"", "", false, true, nullptr},  // Separator
                      {"Cut", "Ctrl+X", true, false, nullptr},
                      {"Copy", "Ctrl+C", true, false, nullptr},
                      {"Paste", "Ctrl+V", true, false, nullptr},
                      {"", "", false, true, nullptr},  // Separator
                      {"Select All", "Ctrl+A", true, false, nullptr}};
    menus.push_back(editMenu);

    // View menu
    win95::Menu viewMenu;
    viewMenu.label = "View";
    viewMenu.items = {{"Pageless Mode", "", true, false, nullptr},
                      {"Paged Mode", "", true, false, nullptr},
                      {"", "", false, true, nullptr},  // Separator
                      {"Line Width: Normal", "", true, false, nullptr},
                      {"Line Width: Narrow", "", true, false, nullptr},
                      {"Line Width: Wide", "", true, false, nullptr}};
    menus.push_back(viewMenu);

    // Format menu
    win95::Menu formatMenu;
    formatMenu.label = "Format";
    formatMenu.items = {
        // Paragraph styles
        {"Normal", "Ctrl+Alt+0", true, false, nullptr},
        {"Title", "", true, false, nullptr},
        {"Subtitle", "", true, false, nullptr},
        {"Heading 1", "Ctrl+Alt+1", true, false, nullptr},
        {"Heading 2", "Ctrl+Alt+2", true, false, nullptr},
        {"Heading 3", "Ctrl+Alt+3", true, false, nullptr},
        {"Heading 4", "Ctrl+Alt+4", true, false, nullptr},
        {"Heading 5", "Ctrl+Alt+5", true, false, nullptr},
        {"Heading 6", "Ctrl+Alt+6", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator
        // Text formatting
        {"Bold", "Ctrl+B", true, false, nullptr},
        {"Italic", "Ctrl+I", true, false, nullptr},
        {"Underline", "Ctrl+U", true, false, nullptr},
        {"Strikethrough", "Ctrl+Shift+S", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator
        {"Font: Gaegu", "Ctrl+1", true, false, nullptr},
        {"Font: Garamond", "Ctrl+2", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator
        {"Increase Size", "Ctrl++", true, false, nullptr},
        {"Decrease Size", "Ctrl+-", true, false, nullptr},
        {"Reset Size", "Ctrl+0", true, false, nullptr}
    };
    menus.push_back(formatMenu);

    // Help menu
    win95::Menu helpMenu;
    helpMenu.label = "Help";
    helpMenu.items = {{"Keyboard Shortcuts...", "F1", true, false, nullptr},
                      {"", "", false, true, nullptr},  // Separator
                      {"About Wordproc", "", true, false, nullptr}};
    menus.push_back(helpMenu);

    return menus;
}

}  // namespace menu_setup
