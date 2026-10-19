#pragma once

#include <filesystem>
#include <vector>

#include "menu_geometry.h"
#include "win95_widgets.h"

namespace menu_setup {

// Label of the Window menu's current-document item ("1 <name>"),
// shared by createMenuBar and by the MenuUISystem rebuild check so
// the label follows the open document.
inline std::string windowDocLabel(const std::string& filePath) {
    std::string name;
    if (!filePath.empty()) {
        name = std::filesystem::path(filePath).filename().string();
    }
    if (name.empty()) name = "Untitled";
    return "1 " + name;
}

// Create the standard Win95-style menu bar for the word processor
inline std::vector<win95::Menu> createMenuBar(
    const std::vector<std::string>& recentFiles = {},
    const std::string& currentFilePath = {}) {
    std::vector<win95::Menu> menus;

    // File menu
    win95::Menu fileMenu;
    fileMenu.label = "File";
    fileMenu.items = {{"New", "Ctrl+N", true, false, nullptr},              // 0
                      {"New from Template...", "", true, false, nullptr},  // 1
                      {"Open...", "Ctrl+O", true, false, nullptr},          // 2
                      {"Save", "Ctrl+S", true, false, nullptr},             // 3
                      {"Save As...", "F12", true, false, nullptr},           // 4
                      {"Version History...", "", true, false, nullptr},    // 5 (Docs-era)
                      {"", "", false, true, nullptr},                       // 6 Separator
                      {"Export PDF...", "", true, false, nullptr},          // 7
                      {"Export HTML...", "", true, false, nullptr},         // 8
                      {"Export RTF...", "", true, false, nullptr},          // 9
                      {"", "", false, true, nullptr},                       // 10 Separator
                      {"Page Setup...", "", true, false, nullptr},          // 11
                      {"Print Preview", "", true, false, nullptr},         // 12
                      {"Print...", "Ctrl+P", true, false, nullptr},        // 13
                      {"", "", false, true, nullptr}};                      // 14 Separator

    if (!recentFiles.empty()) {
        for (const auto& path : recentFiles) {
            win95::MenuItem item{"Recent: " + path, "", true, false, nullptr};
            // Middle-ellipsize for display so a long path can't stretch
            // the dropdown; `label` keeps the full path for dispatch.
            item.displayLabel = "Recent: " + menu_geometry::ellipsizeMiddle(
                                                path, menu_geometry::RECENT_PATH_MAX_CHARS);
            fileMenu.items.push_back(std::move(item));
        }
        fileMenu.items.push_back({"", "", false, true, nullptr});  // Separator
    }

    fileMenu.items.push_back({"Exit", "Alt+F4", true, false, nullptr});  // Last
    menus.push_back(fileMenu);

    // Edit menu
    win95::Menu editMenu;
    editMenu.label = "Edit";
    editMenu.items = {{"Undo", "Ctrl+Z", true, false, nullptr},           // 0
                      {"Redo", "Ctrl+Y", true, false, nullptr},           // 1
                      {"", "", false, true, nullptr},                     // 2 Separator
                      {"Track Changes", "", true, false, nullptr},        // 3
                      {"Accept All Changes", "", true, false, nullptr},   // 4
                      {"Reject All Changes", "", true, false, nullptr},   // 5
                      {"", "", false, true, nullptr},                     // 6 Separator
                      {"Cut", "Ctrl+X", true, false, nullptr},            // 7
                      {"Copy", "Ctrl+C", true, false, nullptr},           // 8
                      {"Paste", "Ctrl+V", true, false, nullptr},          // 9
                      {"", "", false, true, nullptr},                     // 10 Separator
                      {"Select All", "Ctrl+A", true, false, nullptr},     // 11
                      {"", "", false, true, nullptr},                     // 12 Separator
                      {"Find...", "Ctrl+F", true, false, nullptr},        // 13
                      {"Find Next", "F3", true, false, nullptr},          // 14
                      {"Find Previous", "Shift+F3", true, false, nullptr},// 15
                      {"Replace...", "Ctrl+H", true, false, nullptr},     // 16
                      {"", "", false, true, nullptr},                     // 17 Separator
                      {"Go To...", "Ctrl+G", true, false, nullptr},    // 18 (Batch 9: Word has ONE Go To; bookmark jumping lives inside it)
                      {"", "", false, true, nullptr},                     // 19 Separator
                      {"Repeat", "F4", true, false, nullptr}};            // 20
    menus.push_back(editMenu);

    // View menu
    win95::Menu viewMenu;
    viewMenu.label = "View";
    // Word 95 View order: Normal, Page Layout, Outline (Outline
    // sits below, after Show Line Numbers). Normal IS the pageless
    // view, renamed: continuous text + dashed computed page-break
    // lines. Page Layout IS the paged view, renamed. Dispatch is
    // by index, so the renames change no case numbers.
    // Word 95 View order: "Header and Footer" sits after Ruler
    // (Batch 9); everything below it shifts by one — the dispatch
    // case numbers in menu_actions.h follow this list.
    viewMenu.items = {{"Normal", "", true, false, nullptr},              // 0
                      {"Page Layout", "", true, false, nullptr},         // 1
                      {"", "", false, true, nullptr},                    // 2 Separator
                      {"Ruler", "", true, false, nullptr},               // 3 (checkmark = shown)
                      {"Header and Footer", "", true, false, nullptr},  // 4 (checkmark = editing)
                      {"", "", false, true, nullptr},                    // 5 Separator
                      {"Zoom In", "Ctrl+Alt+=", true, false, nullptr},   // 6
                      {"Zoom Out", "Ctrl+Alt+-", true, false, nullptr},  // 7
                      {"Zoom Reset", "Ctrl+Alt+0", true, false, nullptr},// 8
                      {"", "", false, true, nullptr},                    // 9 Separator
                      {"Focus Mode", "F11", true, false, nullptr},       // 10
                      {"Split View", "Ctrl+Alt+V", true, false, nullptr}, // 11
                      {"Dark Mode", "Ctrl+Alt+D", true, false, nullptr}, // 12
                      {"", "", false, true, nullptr},                    // 13 Separator
                      {"Line Width: Normal", "", true, false, nullptr},  // 14
                      {"Line Width: Narrow", "", true, false, nullptr},  // 15
                      {"Line Width: Wide", "", true, false, nullptr},    // 16
                      {"", "", false, true, nullptr},                    // 17 Separator
                      {"Show Line Numbers", "", true, false, nullptr},   // 18
                      {"Outline", "", true, false, nullptr},             // 19 (checkmark = outline mode)
                      {"", "", false, true, nullptr},                    // 20 Separator
                      {"Standard Toolbar", "", true, false, nullptr},    // 21 (checkmark = shown)
                      {"Formatting Toolbar", "", true, false, nullptr},  // 22 (checkmark = shown)
                      {"", "", false, true, nullptr},                    // 23 Separator
                      {"UI Scale...", "", true, false, nullptr}};        // 24 (moved from Tools)
    menus.push_back(viewMenu);

    // Format menu
    win95::Menu formatMenu;
    formatMenu.label = "Format";
    formatMenu.items = {
        // Font dialog (0), then paragraph styles (1-9)
        {"Font...", "", true, false, nullptr},
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
        {"Superscript", "Ctrl+Shift+=", true, false, nullptr},
        {"Subscript", "Ctrl+Shift+-", true, false, nullptr},
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
        // Alignment / spacing: Paragraph... first (Word 95 order),
        // then alignment, indents, line spacing, lists, spacing.
        {"Paragraph...", "", true, false, nullptr},
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
        {"Decrease Space After", "Ctrl+Shift+Alt+Down", true, false, nullptr},
        {"", "", false, true, nullptr},  // Separator
        {"Drop Cap", "", true, false, nullptr},
        {"Tab Width...", "", true, false, nullptr},
        // Borders and Shading... sits just before Columns... (which
        // stays last: test_menu_setup pins Columns... as the tail,
        // and Paragraph... must directly precede Align Left).
        {"Borders and Shading...", "", true, false, nullptr},
        {"Columns...", "", true, false, nullptr}
    };
    // formatMenu pushed later in correct order (Index 4)

    // Insert menu (shapes consolidated)
    win95::Menu insertMenu;
    insertMenu.label = "Insert";
    insertMenu.items = {
        {"Page Break", "Ctrl+Enter", true, false, nullptr},      // 0
        {"Section Break", "", true, false, nullptr},             // 1
        {"", "", false, true, nullptr},                          // 2 Separator
        {"Hyperlink...", "Ctrl+K", true, false, nullptr},        // 3
        {"Remove Hyperlink", "", true, false, nullptr},          // 4
        {"Bookmark...", "", true, false, nullptr},               // 5
        {"Comment...", "", true, false, nullptr},                // 6
        {"", "", false, true, nullptr},                          // 7 Separator
        {"Table...", "", true, false, nullptr},                  // 8
        {"Image...", "", true, false, nullptr},                  // 9
        {"Image Layout...", "", true, false, nullptr},           // 10
        {"Wrap Text", "", true, false, nullptr},                 // 11
        {"", "", false, true, nullptr},                          // 12 Separator
        {"Shape: Line", "", true, false, nullptr},               // 13
        {"Shape: Rectangle", "", true, false, nullptr},          // 14
        {"Shape: Circle", "", true, false, nullptr},             // 15
        {"Shape: Ellipse", "", true, false, nullptr},            // 16
        {"Shape: Arrow", "", true, false, nullptr},              // 17
        {"Shape: Rounded Rect", "", true, false, nullptr},       // 18
        {"Shape: Triangle", "", true, false, nullptr},           // 19
        {"", "", false, true, nullptr},                          // 20 Separator
        {"Equation...", "", true, false, nullptr},               // 21
        {"Footnote", "", true, false, nullptr},                  // 22
        {"Date and Time...", "", true, false, nullptr},          // 23
        {"Special Character...", "", true, false, nullptr},      // 24
        {"", "", false, true, nullptr},                          // 25 Separator
        {"Header", "", true, false, nullptr},                    // 26
        {"Footer", "", true, false, nullptr},                    // 27
        {"Page Number", "", true, false, nullptr},               // 28
        {"Document Body", "", true, false, nullptr},             // 29
        {"", "", false, true, nullptr},                          // 30 Separator
        {"Table of Contents", "", true, false, nullptr},         // 31
        {"Update Table of Contents", "", true, false, nullptr}   // 32 (enabled only with a marked TOC: MenuUISystem syncs it)
    };
    menus.push_back(insertMenu);   // Index 3: Insert

    menus.push_back(formatMenu);   // Index 4: Format

    // Tools menu in Word 95 order: Spelling, Word Count, AutoCorrect,
    // separator, Grammar, AutoFormat, separator, Mail Merge,
    // separator, Options. The modern UI Scale... moved to View
    // (bottom); the disabled Preferences... stub is gone.
    win95::Menu toolsMenu;
    toolsMenu.label = "Tools";
    toolsMenu.items = {
        {"Spelling...", "F7", true, false, nullptr},             // 0
        {"Word Count...", "Ctrl+Shift+W", true, false, nullptr}, // 1
        {"AutoCorrect...", "", true, false, nullptr},           // 2
        {"", "", false, true, nullptr},                          // 3 Separator
        {"Grammar...", "", true, false, nullptr},                // 4
        {"AutoFormat...", "", true, false, nullptr},            // 5
        {"Revisions...", "", true, false, nullptr},             // 6 (Docs-era suggesting mode)
        {"", "", false, true, nullptr},                          // 7 Separator
        {"Mail Merge...", "", false, false, nullptr},           // 8 DISABLED: no merge engine / data source, grayed like Word shows unavailable commands
        {"", "", false, true, nullptr},                          // 9 Separator
        {"Options...", "", true, false, nullptr},               // 10
        {"", "", false, true, nullptr},                         // 11 Separator
        {"Thesaurus...", "Shift+F7", true, false, nullptr}      // 12
    };
    menus.push_back(toolsMenu);    // Index 5: Tools

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
        {"Split Cell", "", true, false, nullptr},                // 11
        {"", "", false, true, nullptr},                          // 12 Separator
        {"Sort Ascending", "", true, false, nullptr},           // 13
        {"Sort Descending", "", true, false, nullptr},          // 14
        {"Formula...", "", true, false, nullptr}                 // 15
    };
    menus.push_back(tableMenu);    // Index 6: Table

    // Window menu (Word 95 order: between Table and Help).
    // Word 95 order: New Window, Arrange All, Split, then the open
    // documents. Arrange All is disabled: a single-window app cannot
    // arrange windows, and grayed is the honest Word behavior. The
    // current-document item is ENABLED and CHECKED (mark renders as
    // the checkmark in MenuUISystem; clicking it is a no-op — the
    // app stays on this document).
    win95::Menu windowMenu;
    windowMenu.label = "Window";
    win95::MenuItem docItem{windowDocLabel(currentFilePath), "", true,
                            false, nullptr};
    docItem.mark = win95::MenuMark::Checkmark;
    windowMenu.items = {{"New Window", "", true, false, nullptr},  // 0
                        {"Arrange All", "", false, false, nullptr},  // 1
                        {"Split", "", true, false, nullptr},          // 2
                        {"", "", false, true, nullptr},  // 3 Separator
                        std::move(docItem)};  // 4 "1 <name>"
    menus.push_back(windowMenu);   // Index 7: Window

    // Help menu (always last per Win95 guidelines)
    win95::Menu helpMenu;
    helpMenu.label = "Help";
    helpMenu.items = {{"Keyboard Shortcuts...", "F1", true, false, nullptr},
                      {"", "", false, true, nullptr},  // Separator
                      {"About Wordproc", "", true, false, nullptr}};
    menus.push_back(helpMenu);     // Index 8: Help

    return menus;
}

}  // namespace menu_setup
