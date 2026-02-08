#include "win95_widgets.h"

#include <algorithm>

// test_input:: available via rl.h -> external.h
#include "input.h"  // For input:: wrappers
#include "theme.h"  // For centralized color scheme

namespace win95 {

void DrawRaisedBorder(raylib::Rectangle rect, int thickness) {
    int x = static_cast<int>(rect.x);
    int y = static_cast<int>(rect.y);
    int w = static_cast<int>(rect.width);
    int h = static_cast<int>(rect.height);

    for (int i = 0; i < thickness; ++i) {
        // Top edge (light)
        raylib::DrawLine(x + i, y + i, x + w - i - 1, y + i,
                         theme::BORDER_LIGHT);
        // Left edge (light)
        raylib::DrawLine(x + i, y + i, x + i, y + h - i - 1,
                         theme::BORDER_LIGHT);
        // Bottom edge (dark)
        raylib::DrawLine(x + i, y + h - i - 1, x + w - i, y + h - i - 1,
                         theme::BORDER_DARK);
        // Right edge (dark)
        raylib::DrawLine(x + w - i - 1, y + i, x + w - i - 1, y + h - i,
                         theme::BORDER_DARK);
    }
}

void DrawSunkenBorder(raylib::Rectangle rect, int thickness) {
    int x = static_cast<int>(rect.x);
    int y = static_cast<int>(rect.y);
    int w = static_cast<int>(rect.width);
    int h = static_cast<int>(rect.height);

    for (int i = 0; i < thickness; ++i) {
        // Top edge (dark)
        raylib::DrawLine(x + i, y + i, x + w - i - 1, y + i,
                         theme::BORDER_DARK);
        // Left edge (dark)
        raylib::DrawLine(x + i, y + i, x + i, y + h - i - 1,
                         theme::BORDER_DARK);
        // Bottom edge (light)
        raylib::DrawLine(x + i, y + h - i - 1, x + w - i, y + h - i - 1,
                         theme::BORDER_LIGHT);
        // Right edge (light)
        raylib::DrawLine(x + w - i - 1, y + i, x + w - i - 1, y + h - i,
                         theme::BORDER_LIGHT);
    }
}


int DrawMenuBar(std::vector<Menu>& menus, int menuBarY, int menuBarHeight) {
    int clickedMenu = -1;
    int x = theme::layout::scaleInt(4);

    raylib::Vector2 mousePos = input::getMousePosition();
    bool mouseInMenuBar =
        mousePos.y >= menuBarY && mousePos.y < menuBarY + menuBarHeight;

    for (std::size_t i = 0; i < menus.size(); ++i) {
        Menu& menu = menus[i];
        int fontSize = 14;
        int textWidth = theme::MeasureUIText(menu.label.c_str(), fontSize);
        int itemWidth = textWidth + theme::layout::scaleInt(16);  // Padding on each side

        menu.bounds = {static_cast<float>(x), static_cast<float>(menuBarY),
                       static_cast<float>(itemWidth),
                       static_cast<float>(menuBarHeight)};

        bool hover = raylib::CheckCollisionPointRec(mousePos, menu.bounds);
        bool clicked =
            hover && IsMouseButtonPressed(raylib::MOUSE_LEFT_BUTTON);

        // Handle menu opening/closing
        if (clicked) {
            // Close other menus and toggle this one
            for (std::size_t j = 0; j < menus.size(); ++j) {
                if (j != i) menus[j].open = false;
            }
            menu.open = !menu.open;
        }

        // If a menu is open and we hover another menu header, switch to it
        bool anyOpen = false;
        for (const auto& m : menus) {
            if (m.open) anyOpen = true;
        }
        if (anyOpen && hover && !menu.open) {
            for (std::size_t j = 0; j < menus.size(); ++j) {
                menus[j].open = (j == i);
            }
        }

        // Register menu label for E2E testing
        test_input::registerVisibleText(menu.label);
        
        // Draw menu header
        if (menu.open || hover) {
            raylib::DrawRectangleRec(menu.bounds, theme::MENU_HOVER);
            theme::DrawUIText(menu.label.c_str(), x + theme::layout::scaleInt(8), 
                             menuBarY + theme::layout::scaleInt(3), fontSize,
                             theme::TITLE_TEXT);
        } else {
            theme::DrawUIText(menu.label.c_str(), x + theme::layout::scaleInt(8), 
                             menuBarY + theme::layout::scaleInt(3), fontSize,
                             theme::TEXT_COLOR);
        }

        // Draw dropdown if open
        if (menu.open) {
            int selectedItem =
                DrawDropdownMenu(menu, x, menuBarY + menuBarHeight, theme::layout::scaleInt(20));
            if (selectedItem >= 0) {
                clickedMenu = static_cast<int>(
                    i * 100 + selectedItem);  // Encode menu and item
                if (menu.items[selectedItem].action) {
                    menu.items[selectedItem].action();
                }
                menu.open = false;
            }
        }

        x += itemWidth;
    }

    // Close menus on click outside
    if (IsMouseButtonPressed(raylib::MOUSE_LEFT_BUTTON) &&
        !mouseInMenuBar) {
        bool clickedInDropdown = false;
        for (const auto& menu : menus) {
            if (menu.open) {
                // Check if click is in dropdown area
                int dropdownHeight = static_cast<int>(menu.items.size()) * theme::layout::scaleInt(20);
                raylib::Rectangle dropdownRect = {
                    menu.bounds.x, menu.bounds.y + menu.bounds.height, 
                    static_cast<float>(theme::layout::scaleInt(150)),
                    static_cast<float>(dropdownHeight)};
                if (raylib::CheckCollisionPointRec(mousePos, dropdownRect)) {
                    clickedInDropdown = true;
                }
            }
        }
        if (!clickedInDropdown) {
            for (auto& menu : menus) {
                menu.open = false;
            }
        }
    }

    return clickedMenu;
}

int DrawDropdownMenu(Menu& menu, int x, int y, int itemHeight) {
    int selectedItem = -1;

    // Calculate dropdown dimensions
    int fontSize = 14;
    int maxWidth = theme::layout::scaleInt(150);
    for (const auto& item : menu.items) {
        int w = theme::MeasureUIText(item.label.c_str(), fontSize);
        if (!item.shortcut.empty()) {
            w += theme::MeasureUIText(item.shortcut.c_str(), fontSize) + theme::layout::scaleInt(20);
        }
        maxWidth = std::max(maxWidth, w + theme::layout::scaleInt(32));
    }

    int totalHeight = static_cast<int>(menu.items.size()) * itemHeight;
    raylib::Rectangle dropdownRect = {
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(maxWidth), static_cast<float>(totalHeight)};

    // Draw dropdown background
    raylib::DrawRectangleRec(dropdownRect, theme::WINDOW_BG);
    DrawRaisedBorder(dropdownRect, theme::layout::scaleInt(2));

    // Draw items
    raylib::Vector2 mousePos = input::getMousePosition();
    int itemY = y;

    for (std::size_t i = 0; i < menu.items.size(); ++i) {
        const MenuItem& item = menu.items[i];
        raylib::Rectangle itemRect = {
            static_cast<float>(x + theme::layout::scaleInt(2)), static_cast<float>(itemY),
            static_cast<float>(maxWidth - theme::layout::scaleInt(4)), static_cast<float>(itemHeight)};

        if (item.separator) {
            // Draw separator line
            int sepY = itemY + itemHeight / 2;
            raylib::DrawLine(x + theme::layout::scaleInt(4), sepY, 
                           x + maxWidth - theme::layout::scaleInt(4), sepY,
                           theme::BORDER_DARK);
            raylib::DrawLine(x + theme::layout::scaleInt(4), sepY + 1, 
                           x + maxWidth - theme::layout::scaleInt(4), sepY + 1,
                           theme::BORDER_LIGHT);
        } else {
            bool hover = raylib::CheckCollisionPointRec(mousePos, itemRect) &&
                         item.enabled;
            
            // Register menu item text for E2E testing
            test_input::registerVisibleText(item.label);
            
            // Reserve 20 pixels for mark column
            const int markColumnWidth = theme::layout::scaleInt(20);
            const int textX = x + markColumnWidth;

            if (hover) {
                raylib::DrawRectangleRec(itemRect, theme::MENU_HOVER);
                
                // Draw mark if present
                if (item.mark != MenuMark::None) {
                    const char* markStr = nullptr;
                    switch (item.mark) {
                        case MenuMark::Checkmark: markStr = "\xE2\x9C\x93"; break;  // ✓
                        case MenuMark::Radio: markStr = "\xE2\x80\xA2"; break;  // •
                        case MenuMark::Dash: markStr = "-"; break;
                        case MenuMark::None:
                        default: break;
                    }
                    if (markStr) {
                        theme::DrawUIText(markStr, x + theme::layout::scaleInt(6), 
                                         itemY + theme::layout::scaleInt(3), fontSize,
                                         theme::TITLE_TEXT);
                    }
                }
                
                theme::DrawUIText(item.label.c_str(), textX, itemY + theme::layout::scaleInt(3), fontSize,
                                 theme::TITLE_TEXT);
                if (!item.shortcut.empty()) {
                    int shortcutX =
                        x + maxWidth -
                        theme::MeasureUIText(item.shortcut.c_str(), fontSize) - theme::layout::scaleInt(12);
                    theme::DrawUIText(item.shortcut.c_str(), shortcutX,
                                     itemY + theme::layout::scaleInt(3), fontSize, theme::TITLE_TEXT);
                }

                if (IsMouseButtonReleased(raylib::MOUSE_LEFT_BUTTON)) {
                    selectedItem = static_cast<int>(i);
                }
            } else {
                raylib::Color textColor =
                    item.enabled ? theme::TEXT_COLOR : theme::MENU_DISABLED;
                
                // Draw mark if present
                if (item.mark != MenuMark::None) {
                    const char* markStr = nullptr;
                    switch (item.mark) {
                        case MenuMark::Checkmark: markStr = "\xE2\x9C\x93"; break;  // ✓
                        case MenuMark::Radio: markStr = "\xE2\x80\xA2"; break;  // •
                        case MenuMark::Dash: markStr = "-"; break;
                        case MenuMark::None:
                        default: break;
                    }
                    if (markStr) {
                        theme::DrawUIText(markStr, x + theme::layout::scaleInt(6), 
                                         itemY + theme::layout::scaleInt(3), fontSize, textColor);
                    }
                }
                
                theme::DrawUIText(item.label.c_str(), textX, itemY + theme::layout::scaleInt(3), fontSize,
                                 textColor);
                if (!item.shortcut.empty()) {
                    int shortcutX =
                        x + maxWidth -
                        theme::MeasureUIText(item.shortcut.c_str(), fontSize) - theme::layout::scaleInt(12);
                    theme::DrawUIText(item.shortcut.c_str(), shortcutX,
                                     itemY + theme::layout::scaleInt(3), fontSize, textColor);
                }
            }
        }

        itemY += itemHeight;
    }

    return selectedItem;
}


}  // namespace win95
