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
        {"New from Template...", "", true, false},
        {"Open...", "Ctrl+O", true, false},
        {"Save", "Ctrl+S", true, false},
        {"Save As...", "", true, false},
        {"", "", false, true},  // Separator
        {"Export PDF...", "", true, false},
        {"Export HTML...", "", true, false},
        {"Export RTF...", "", true, false},
        {"", "", false, true},  // Separator
        {"Page Setup...", "", true, false},
        {"", "", false, true},  // Separator
        {"Exit", "Alt+F4", true, false}};
    menus.push_back(fileMenu);

    // Edit menu
    MenuData editMenu;
    editMenu.label = "Edit";
    editMenu.items = {{"Undo", "Ctrl+Z", true, false},
                      {"Redo", "Ctrl+Y", true, false},
                      {"", "", false, true},  // Separator
                      {"Track Changes", "", true, false},
                      {"Accept All Changes", "", true, false},
                      {"Reject All Changes", "", true, false},
                      {"", "", false, true},  // Separator
                      {"Cut", "Ctrl+X", true, false},
                      {"Copy", "Ctrl+C", true, false},
                      {"Paste", "Ctrl+V", true, false},
                      {"", "", false, true},  // Separator
                      {"Select All", "Ctrl+A", true, false},
                      {"", "", false, true},  // Separator
                      {"Find...", "Ctrl+F", true, false},
                      {"Find Next", "F3", true, false},
                      {"Find Previous", "Shift+F3", true, false},
                      {"Replace...", "Ctrl+H", true, false},
                      {"", "", false, true},  // Separator
                      {"Go To Bookmark...", "", true, false}};
    menus.push_back(editMenu);

    // View menu
    MenuData viewMenu;
    viewMenu.label = "View";
    viewMenu.items = {{"Pageless Mode", "", true, false},
                      {"Paged Mode", "", true, false},
                      {"", "", false, true},  // Separator
                      {"Zoom In", "Ctrl+Alt+=", true, false},
                      {"Zoom Out", "Ctrl+Alt+-", true, false},
                      {"Zoom Reset", "Ctrl+Alt+0", true, false},
                      {"", "", false, true},  // Separator
                      {"Focus Mode", "F11", true, false},
                      {"Split View", "", true, false},
                      {"Dark Mode", "", true, false},
                      {"", "", false, true},  // Separator
                      {"Line Width: Normal", "", true, false},
                      {"Line Width: Narrow", "", true, false},
                      {"Line Width: Wide", "", true, false},
                      {"", "", false, true},  // Separator
                      {"Show Line Numbers", "", true, false},
                      {"Show Outline", "", true, false}};
    menus.push_back(viewMenu);

    // Format menu
    MenuData formatMenu;
    formatMenu.label = "Format";
    formatMenu.items = {{"Normal", "Ctrl+Alt+0", true, false},
                        {"Title", "", true, false},
                        {"Subtitle", "", true, false},
                        {"Heading 1", "Ctrl+Alt+1", true, false},
                        {"Heading 2", "Ctrl+Alt+2", true, false},
                        {"Heading 3", "Ctrl+Alt+3", true, false},
                        {"Heading 4", "Ctrl+Alt+4", true, false},
                        {"Heading 5", "Ctrl+Alt+5", true, false},
                        {"Heading 6", "Ctrl+Alt+6", true, false},
                        {"", "", false, true},
                        {"Bold", "Ctrl+B", true, false},
                        {"Italic", "Ctrl+I", true, false},
                        {"Underline", "Ctrl+U", true, false},
                        {"Strikethrough", "Ctrl+Shift+S", true, false},
                        {"Superscript", "Ctrl+Shift+=", true, false},
                        {"Subscript", "Ctrl+Shift+-", true, false},
                        {"", "", false, true},
                        {"Text Color...", "", true, false},
                        {"Highlight Color...", "", true, false},
                        {"", "", false, true},
                        {"Align Left", "Ctrl+L", true, false},
                        {"Align Center", "Ctrl+E", true, false},
                        {"Align Right", "Ctrl+R", true, false},
                        {"Justify", "Ctrl+J", true, false},
                        {"", "", false, true},
                        {"Text: Black", "", true, false},
                        {"Text: Red", "", true, false},
                        {"Text: Orange", "", true, false},
                        {"Text: Green", "", true, false},
                        {"Text: Blue", "", true, false},
                        {"Text: Purple", "", true, false},
                        {"Text: Gray", "", true, false},
                        {"", "", false, true},
                        {"Highlight: None", "", true, false},
                        {"Highlight: Yellow", "", true, false},
                        {"Highlight: Green", "", true, false},
                        {"Highlight: Cyan", "", true, false},
                        {"Highlight: Pink", "", true, false},
                        {"Highlight: Orange", "", true, false},
                        {"", "", false, true},
                        {"Font: Gaegu", "Ctrl+1", true, false},
                        {"Font: Garamond", "Ctrl+2", true, false},
                        {"", "", false, true},
                        {"Increase Size", "Ctrl++", true, false},
                        {"Decrease Size", "Ctrl+-", true, false},
                        {"Reset Size", "Ctrl+0", true, false},
                        {"", "", false, true},
                        {"Align Left", "Ctrl+L", true, false},
                        {"Align Center", "Ctrl+E", true, false},
                        {"Align Right", "Ctrl+R", true, false},
                        {"Justify", "Ctrl+J", true, false},
                        {"", "", false, true},
                        {"Increase Indent", "Ctrl+]", true, false},
                        {"Decrease Indent", "Ctrl+[", true, false},
                        {"", "", false, true},
                        {"Single Spacing", "Ctrl+Shift+1", true, false},
                        {"1.5 Line Spacing", "Ctrl+Shift+5", true, false},
                        {"Double Spacing", "Ctrl+Shift+2", true, false},
                        {"", "", false, true},
                        {"Bulleted List", "Ctrl+Shift+8", true, false},
                        {"Numbered List", "Ctrl+Shift+7", true, false},
                        {"Increase List Level", "", true, false},
                        {"Decrease List Level", "", true, false},
                        {"", "", false, true},
                        {"Increase Space Before", "Ctrl+Alt+Up", true, false},
                        {"Decrease Space Before", "Ctrl+Alt+Down", true, false},
                        {"Increase Space After", "Ctrl+Shift+Alt+Up", true, false},
                        {"Decrease Space After", "Ctrl+Shift+Alt+Down", true, false},
                        {"", "", false, true},
                        {"Drop Cap", "", true, false},
                        {"Tab Width...", "", true, false}};
    menus.push_back(formatMenu);

    // Insert menu
    MenuData insertMenu;
    insertMenu.label = "Insert";
    insertMenu.items = {{"Page Break", "Ctrl+Enter", true, false},
                        {"Section Break", "", true, false},
                        {"", "", false, true},
                        {"Hyperlink...", "Ctrl+K", true, false},
                        {"Remove Hyperlink", "", true, false},
                        {"Bookmark...", "", true, false},
                        {"Comment...", "", true, false},
                        {"", "", false, true},
                        {"Table...", "", true, false},
                        {"", "", false, true},
                        {"Image...", "", true, false},
                        {"Image Layout...", "", true, false},
                        {"Wrap Text", "", true, false},
                        {"", "", false, true},
                        {"Shape...", "", true, false},
                        {"Line", "", true, false},
                        {"Rectangle", "", true, false},
                        {"Circle", "", true, false},
                        {"Ellipse", "", true, false},
                        {"Arrow", "", true, false},
                        {"Rounded Rectangle", "", true, false},
                        {"Triangle", "", true, false},
                        {"", "", false, true},
                        {"Equation...", "", true, false},
                        {"Footnote", "", true, false},
                        {"Special Character...", "", true, false},
                        {"", "", false, true},
                        {"Header", "", true, false},
                        {"Footer", "", true, false},
                        {"Page Number", "", true, false},
                        {"Document Body", "", true, false},
                        {"", "", false, true},
                        {"Table of Contents", "", true, false}};
    menus.push_back(insertMenu);

    // Table menu
    MenuData tableMenu;
    tableMenu.label = "Table";
    tableMenu.items = {{"Insert Table...", "", true, false},
                       {"", "", false, true},
                       {"Insert Row Above", "", true, false},
                       {"Insert Row Below", "", true, false},
                       {"Insert Column Left", "", true, false},
                       {"Insert Column Right", "", true, false},
                       {"", "", false, true},
                       {"Delete Row", "", true, false},
                       {"Delete Column", "", true, false},
                       {"", "", false, true},
                       {"Merge Cells", "", true, false},
                       {"Split Cell", "", true, false}};
    menus.push_back(tableMenu);

    // Help menu
    MenuData helpMenu;
    helpMenu.label = "Help";
    helpMenu.items = {{"Keyboard Shortcuts...", "F1", true, false},
                      {"", "", false, true},
                      {"About Wordproc", "", true, false}};
    menus.push_back(helpMenu);

    // Tools menu
    MenuData toolsMenu;
    toolsMenu.label = "Tools";
    toolsMenu.items = {{"Word Count...", "", true, false}};
    menus.push_back(toolsMenu);

    return menus;
}
