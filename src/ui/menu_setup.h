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
    fileMenu.items = {{"New", "Ctrl+N", true, false, nullptr},          // 0
                      {"Open...", "Ctrl+O", true, false, nullptr},      // 1
                      {"Save", "Ctrl+S", true, false, nullptr},         // 2
                      {"Save As...", "", true, false, nullptr},         // 3
                      {"", "", false, true, nullptr},                   // 4 Separator
                      {"Page Setup...", "", true, false, nullptr},      // 5
                      {"", "", false, true, nullptr},                   // 6 Separator
                      {"Exit", "Alt+F4", true, false, nullptr}};        // 7
    menus.push_back(fileMenu);

    // Edit menu
    win95::Menu editMenu;
    editMenu.label = "Edit";
    editMenu.items = {{"Undo", "Ctrl+Z", true, false, nullptr},           // 0
                      {"Redo", "Ctrl+Y", true, false, nullptr},           // 1
                      {"", "", false, true, nullptr},                     // 2 Separator
                      {"Cut", "Ctrl+X", true, false, nullptr},            // 3
                      {"Copy", "Ctrl+C", true, false, nullptr},           // 4
                      {"Paste", "Ctrl+V", true, false, nullptr},          // 5
                      {"", "", false, true, nullptr},                     // 6 Separator
                      {"Select All", "Ctrl+A", true, false, nullptr},     // 7
                      {"", "", false, true, nullptr},                     // 8 Separator
                      {"Find...", "Ctrl+F", true, false, nullptr},        // 9
                      {"Find Next", "F3", true, false, nullptr},          // 10
                      {"Find Previous", "Shift+F3", true, false, nullptr},// 11
                      {"Replace...", "Ctrl+H", true, false, nullptr},     // 12
                      {"", "", false, true, nullptr},                     // 13 Separator
                      {"Go To Bookmark...", "", true, false, nullptr}};   // 14
    menus.push_back(editMenu);

    // View menu
    win95::Menu viewMenu;
    viewMenu.label = "View";
    viewMenu.items = {{"Pageless Mode", "", true, false, nullptr},       // 0
                      {"Paged Mode", "", true, false, nullptr},          // 1
                      {"", "", false, true, nullptr},                    // 2 Separator
                      {"Line Width: Normal", "", true, false, nullptr},  // 3
                      {"Line Width: Narrow", "", true, false, nullptr},  // 4
                      {"Line Width: Wide", "", true, false, nullptr},    // 5
                      {"", "", false, true, nullptr},                    // 6 Separator
                      {"Show Line Numbers", "", true, false, nullptr},   // 7
                      {"Show Outline", "", true, false, nullptr}};       // 8
    menus.push_back(viewMenu);

    // Format menu
    win95::Menu formatMenu;
    formatMenu.label = "Format";
    formatMenu.items = {
        // Paragraph styles (0-8)
        {"Normal", "Ctrl+Alt+0", true, false, nullptr},
        {"Title", "", true, false, nullptr},
        {"Subtitle", "", true, false, nullptr},
        {"Heading 1", "Ctrl+Alt+1", true, false, nullptr},
        {"Heading 2", "Ctrl+Alt+2", true, false, nullptr},
        {"Heading 3", "Ctrl+Alt+3", true, false, nullptr},
        {"Heading 4", "Ctrl+Alt+4", true, false, nullptr},
        {"Heading 5", "Ctrl+Alt+5", true, false, nullptr},
        {"Heading 6", "Ctrl+Alt+6", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (9)
        // Text formatting (10-13)
        {"Bold", "Ctrl+B", true, false, nullptr},
        {"Italic", "Ctrl+I", true, false, nullptr},
        {"Underline", "Ctrl+U", true, false, nullptr},
        {"Strikethrough", "Ctrl+Shift+S", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (14)
        // Text color and highlight dialogs
        {"Text Color...", "", true, false, nullptr},
        {"Highlight Color...", "", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator
        // Alignment
        {"Align Left", "Ctrl+L", true, false, nullptr},
        {"Align Center", "Ctrl+E", true, false, nullptr},
        {"Align Right", "Ctrl+R", true, false, nullptr},
        {"Justify", "Ctrl+J", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator
        // Text color quick pick
        {"Text: Black", "", true, false, nullptr},
        {"Text: Red", "", true, false, nullptr},
        {"Text: Orange", "", true, false, nullptr},
        {"Text: Green", "", true, false, nullptr},
        {"Text: Blue", "", true, false, nullptr},
        {"Text: Purple", "", true, false, nullptr},
        {"Text: Gray", "", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (22)
        // Highlight color (23-30)
        {"Highlight: None", "", true, false, nullptr},
        {"Highlight: Yellow", "", true, false, nullptr},
        {"Highlight: Green", "", true, false, nullptr},
        {"Highlight: Cyan", "", true, false, nullptr},
        {"Highlight: Pink", "", true, false, nullptr},
        {"Highlight: Orange", "", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (29)
        // Font (30-31)
        {"Font: Gaegu", "Ctrl+1", true, false, nullptr},
        {"Font: Garamond", "Ctrl+2", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (32)
        // Font size (33-35)
        {"Increase Size", "Ctrl++", true, false, nullptr},
        {"Decrease Size", "Ctrl+-", true, false, nullptr},
        {"Reset Size", "Ctrl+0", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (36)
        // Alignment (37-40)
        {"Align Left", "Ctrl+L", true, false, nullptr},
        {"Align Center", "Ctrl+E", true, false, nullptr},
        {"Align Right", "Ctrl+R", true, false, nullptr},
        {"Justify", "Ctrl+J", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (41)
        // Indentation (42-43)
        {"Increase Indent", "Ctrl+]", true, false, nullptr},
        {"Decrease Indent", "Ctrl+[", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (44)
        // Line spacing (45-47)
        {"Single Spacing", "Ctrl+Shift+1", true, false, nullptr},
        {"1.5 Line Spacing", "Ctrl+Shift+5", true, false, nullptr},
        {"Double Spacing", "Ctrl+Shift+2", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (48)
        // Lists (49-52)
        {"Bulleted List", "Ctrl+Shift+8", true, false, nullptr},
        {"Numbered List", "Ctrl+Shift+7", true, false, nullptr},
        {"Increase List Level", "", true, false, nullptr},
        {"Decrease List Level", "", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator (53)
        // Paragraph spacing (54-57)
        {"Increase Space Before", "Ctrl+Alt+Up", true, false, nullptr},
        {"Decrease Space Before", "Ctrl+Alt+Down", true, false, nullptr},
        {"Increase Space After", "Ctrl+Shift+Alt+Up", true, false, nullptr},
        {"Decrease Space After", "Ctrl+Shift+Alt+Down", true, false, nullptr}
    };
    menus.push_back(formatMenu);

    // Insert menu
    win95::Menu insertMenu;
    insertMenu.label = "Insert";
    insertMenu.items = {
        {"Page Break", "Ctrl+Enter", true, false, nullptr},      // 0
        {"Section Break", "", true, false, nullptr},             // 1
        {"", "", false, true, nullptr},                          // 2 Separator
        {"Hyperlink...", "Ctrl+K", true, false, nullptr},        // 3
        {"Remove Hyperlink", "", true, false, nullptr},          // 4
        {"Bookmark...", "", true, false, nullptr},               // 5
        {"", "", false, true, nullptr},                          // 6 Separator
        {"Table...", "", true, false, nullptr},                  // 7
        {"", "", false, true, nullptr},                          // 8 Separator
        {"Image...", "", true, false, nullptr},                  // 9
        {"Image Layout...", "", true, false, nullptr},           // 10
        {"Wrap Text", "", true, false, nullptr},                 // 11
        {"", "", false, true, nullptr},                          // 12 Separator
        {"Shape...", "", true, false, nullptr},                  // 13
        {"Line", "", true, false, nullptr},                      // 14 - Drawing: Line
        {"Rectangle", "", true, false, nullptr},                 // 15 - Drawing: Rectangle
        {"Circle", "", true, false, nullptr},                    // 16 - Drawing: Circle
        {"Ellipse", "", true, false, nullptr},                   // 17 - Drawing: Ellipse
        {"Arrow", "", true, false, nullptr},                     // 18 - Drawing: Arrow
        {"Rounded Rectangle", "", true, false, nullptr},         // 19 - Drawing: Rounded Rectangle
        {"Triangle", "", true, false, nullptr},                  // 20 - Drawing: Triangle
        {"", "", false, true, nullptr},                          // 21 Separator
        {"Equation...", "", true, false, nullptr},               // 22
        {"Footnote", "", true, false, nullptr},                  // 23
        {"Special Character...", "", true, false, nullptr},      // 24
        {"", "", false, true, nullptr},                          // 25 Separator
        {"Header", "", true, false, nullptr},                    // 26
        {"Footer", "", true, false, nullptr},                    // 27
        {"Page Number", "", true, false, nullptr},               // 28
        {"Document Body", "", true, false, nullptr},             // 29
        {"", "", false, true, nullptr},                          // 30 Separator
        {"Table of Contents", "", true, false, nullptr}          // 31
    };
    menus.push_back(insertMenu);

    // Table menu
    win95::Menu tableMenu;
    tableMenu.label = "Table";
    tableMenu.items = {
        {"Insert Table...", "", true, false, nullptr},           // 0
        {"", "", false, true, nullptr},                          // 1 Separator
        {"Insert Row Above", "", true, false, nullptr},          // 2
        {"Insert Row Below", "", true, false, nullptr},          // 3
        {"Insert Column Left", "", true, false, nullptr},        // 4
        {"Insert Column Right", "", true, false, nullptr},       // 5
        {"", "", false, true, nullptr},                          // 6 Separator
        {"Delete Row", "", true, false, nullptr},                // 7
        {"Delete Column", "", true, false, nullptr},             // 8
        {"", "", false, true, nullptr},                          // 9 Separator
        {"Merge Cells", "", true, false, nullptr},               // 10
        {"Split Cell", "", true, false, nullptr}                 // 11
    };
    menus.push_back(tableMenu);

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
