#include "win95_widgets.h"

#include <algorithm>
#include <cstring>

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

bool DrawButton(raylib::Rectangle rect, const char* text, bool enabled) {
    bool clicked = false;
    ButtonState state = enabled ? ButtonState::Normal : ButtonState::Disabled;

    raylib::Vector2 mousePos = input::getMousePosition();
    bool hover = raylib::CheckCollisionPointRec(mousePos, rect);
    bool pressing =
        hover && IsMouseButtonDown(raylib::MOUSE_LEFT_BUTTON);

    if (enabled) {
        if (pressing) {
            state = ButtonState::Pressed;
        } else if (hover) {
            state = ButtonState::Hover;
            if (IsMouseButtonReleased(raylib::MOUSE_LEFT_BUTTON)) {
                clicked = true;
            }
        }
    }

    // Draw button background
    raylib::DrawRectangleRec(rect, theme::BUTTON_BG);

    // Draw 3D border based on state
    if (state == ButtonState::Pressed) {
        DrawSunkenBorder(rect, theme::layout::scaleInt(2));
    } else {
        DrawRaisedBorder(rect, theme::layout::scaleInt(2));
    }

    // Calculate text position
    int fontSize = 14;
    int textWidth = theme::MeasureUIText(text, fontSize);
    int textX = static_cast<int>(rect.x + (rect.width - textWidth) / 2);
    int textY = static_cast<int>(rect.y + (rect.height - theme::layout::scaleInt(fontSize)) / 2);

    // Offset text when pressed
    if (state == ButtonState::Pressed) {
        textX += theme::layout::scaleInt(1);
        textY += theme::layout::scaleInt(1);
    }

    // Draw text
    raylib::Color textColor =
        enabled ? theme::TEXT_COLOR : theme::MENU_DISABLED;
    theme::DrawUIText(text, textX, textY, fontSize, textColor);

    return clicked;
}

bool DrawCheckbox(raylib::Rectangle rect, const char* text, bool* checked,
                  bool enabled) {
    bool changed = false;

    // Checkbox box is 13x13 pixels (Win95 authentic)
    const int BOX_SIZE = theme::layout::scaleInt(13);
    raylib::Rectangle boxRect = {rect.x, rect.y + (rect.height - BOX_SIZE) / 2,
                                 static_cast<float>(BOX_SIZE), static_cast<float>(BOX_SIZE)};

    raylib::Vector2 mousePos = input::getMousePosition();
    bool hover = raylib::CheckCollisionPointRec(mousePos, rect);

    if (enabled && hover &&
        IsMouseButtonReleased(raylib::MOUSE_LEFT_BUTTON)) {
        *checked = !*checked;
        changed = true;
    }

    // Draw checkbox background
    raylib::DrawRectangleRec(boxRect, theme::TEXT_AREA_BG);
    DrawSunkenBorder(boxRect, theme::layout::scaleInt(2));

    // Draw checkmark if checked
    if (*checked) {
        int cx = static_cast<int>(boxRect.x) + 2;
        int cy = static_cast<int>(boxRect.y) + BOX_SIZE / 2;
        // Simple checkmark
        raylib::DrawLine(cx + 2, cy, cx + 4, cy + 3, theme::TEXT_COLOR);
        raylib::DrawLine(cx + 4, cy + 3, cx + 9, cy - 3, theme::TEXT_COLOR);
        raylib::DrawLine(cx + 2, cy + 1, cx + 4, cy + 4, theme::TEXT_COLOR);
        raylib::DrawLine(cx + 4, cy + 4, cx + 9, cy - 2, theme::TEXT_COLOR);
    }

    // Draw label
    int fontSize = 14;
    int textX = static_cast<int>(boxRect.x + BOX_SIZE + theme::layout::scaleInt(6));
    int textY = static_cast<int>(rect.y + (rect.height - theme::layout::scaleInt(fontSize)) / 2);
    raylib::Color textColor =
        enabled ? theme::TEXT_COLOR : theme::MENU_DISABLED;
    theme::DrawUIText(text, textX, textY, fontSize, textColor);

    return changed;
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

int DrawMessageDialog(raylib::Rectangle dialogRect, const char* title,
                      const char* message, bool hasCancel) {
    int result = -1;

    // Dim background
    raylib::DrawRectangle(0, 0, raylib::GetScreenWidth(),
                          raylib::GetScreenHeight(), {0, 0, 0, 128});

    // Draw dialog background
    raylib::DrawRectangleRec(dialogRect, theme::WINDOW_BG);
    DrawRaisedBorder(dialogRect, theme::layout::scaleInt(2));

    // Draw title bar
    int fontSize = 14;
    raylib::Rectangle titleRect = {dialogRect.x + theme::layout::scale(2), 
                                   dialogRect.y + theme::layout::scale(2),
                                   dialogRect.width - theme::layout::scale(4), 
                                   theme::layout::scale(20)};
    raylib::DrawRectangleRec(titleRect, theme::TITLE_BAR);
    theme::DrawUIText(title, static_cast<int>(titleRect.x) + theme::layout::scaleInt(4),
                     static_cast<int>(titleRect.y) + theme::layout::scaleInt(3), fontSize, theme::TITLE_TEXT);

    // Draw message
    int messageX = static_cast<int>(dialogRect.x) + theme::layout::scaleInt(16);
    int messageY = static_cast<int>(dialogRect.y) + theme::layout::scaleInt(40);
    theme::DrawUIText(message, messageX, messageY, fontSize, theme::TEXT_COLOR);

    // Draw buttons
    int buttonWidth = theme::layout::scaleInt(75);
    int buttonHeight = theme::layout::scaleInt(23);
    int buttonY =
        static_cast<int>(dialogRect.y + dialogRect.height) - buttonHeight - theme::layout::scaleInt(12);

    if (hasCancel) {
        int okX = static_cast<int>(dialogRect.x + dialogRect.width) -
                  2 * buttonWidth - theme::layout::scaleInt(24);
        int cancelX = static_cast<int>(dialogRect.x + dialogRect.width) -
                      buttonWidth - theme::layout::scaleInt(12);

        if (DrawButton({static_cast<float>(okX), static_cast<float>(buttonY),
                        static_cast<float>(buttonWidth),
                        static_cast<float>(buttonHeight)},
                       "OK", true)) {
            result = 0;
        }
        if (DrawButton(
                {static_cast<float>(cancelX), static_cast<float>(buttonY),
                 static_cast<float>(buttonWidth),
                 static_cast<float>(buttonHeight)},
                "Cancel", true)) {
            result = 1;
        }
    } else {
        int okX = static_cast<int>(dialogRect.x +
                                   (dialogRect.width - buttonWidth) / 2);
        if (DrawButton({static_cast<float>(okX), static_cast<float>(buttonY),
                        static_cast<float>(buttonWidth),
                        static_cast<float>(buttonHeight)},
                       "OK", true)) {
            result = 0;
        }
    }

    return result;
}

int DrawInputDialog(raylib::Rectangle dialogRect, const char* title,
                    const char* prompt, char* buffer, int bufferSize) {
    int result = -1;

    // Dim background
    raylib::DrawRectangle(0, 0, raylib::GetScreenWidth(),
                          raylib::GetScreenHeight(), {0, 0, 0, 128});

    // Draw dialog background
    raylib::DrawRectangleRec(dialogRect, theme::WINDOW_BG);
    DrawRaisedBorder(dialogRect, theme::layout::scaleInt(2));

    // Draw title bar
    int fontSize = 14;
    raylib::Rectangle titleRect = {dialogRect.x + theme::layout::scale(2), 
                                   dialogRect.y + theme::layout::scale(2),
                                   dialogRect.width - theme::layout::scale(4), 
                                   theme::layout::scale(20)};
    raylib::DrawRectangleRec(titleRect, theme::TITLE_BAR);
    theme::DrawUIText(title, static_cast<int>(titleRect.x) + theme::layout::scaleInt(4),
                     static_cast<int>(titleRect.y) + theme::layout::scaleInt(3), fontSize, theme::TITLE_TEXT);

    // Draw prompt
    int promptX = static_cast<int>(dialogRect.x) + theme::layout::scaleInt(16);
    int promptY = static_cast<int>(dialogRect.y) + theme::layout::scaleInt(36);
    theme::DrawUIText(prompt, promptX, promptY, fontSize, theme::TEXT_COLOR);

    // Draw input field
    raylib::Rectangle inputRect = {dialogRect.x + theme::layout::scale(16), 
                                   dialogRect.y + theme::layout::scale(56),
                                   dialogRect.width - theme::layout::scale(32), 
                                   theme::layout::scale(22)};
    raylib::DrawRectangleRec(inputRect, theme::TEXT_AREA_BG);
    DrawSunkenBorder(inputRect, theme::layout::scaleInt(2));

    // Handle text input
    int key = GetCharPressed();
    int len = static_cast<int>(std::strlen(buffer));
    while (key > 0) {
        if (key >= 32 && key <= 126 && len < bufferSize - 1) {
            buffer[len] = static_cast<char>(key);
            buffer[len + 1] = '\0';
            len++;
        }
        key = GetCharPressed();
    }

    // Handle backspace
    if (IsKeyPressed(raylib::KEY_BACKSPACE) && len > 0) {
        buffer[len - 1] = '\0';
    }

    // Handle Enter for submit
    if (IsKeyPressed(raylib::KEY_ENTER)) {
        result = 0;
    }

    // Handle Escape for cancel
    if (IsKeyPressed(raylib::KEY_ESCAPE)) {
        result = 1;
    }

    // Draw input text
    theme::DrawUIText(buffer, static_cast<int>(inputRect.x) + theme::layout::scaleInt(4),
                     static_cast<int>(inputRect.y) + theme::layout::scaleInt(4), fontSize, theme::TEXT_COLOR);

    // Draw buttons
    int buttonWidth = theme::layout::scaleInt(75);
    int buttonHeight = theme::layout::scaleInt(23);
    int buttonY =
        static_cast<int>(dialogRect.y + dialogRect.height) - buttonHeight - theme::layout::scaleInt(12);
    int okX = static_cast<int>(dialogRect.x + dialogRect.width) -
              2 * buttonWidth - theme::layout::scaleInt(24);
    int cancelX =
        static_cast<int>(dialogRect.x + dialogRect.width) - buttonWidth - theme::layout::scaleInt(12);

    if (DrawButton(
            {static_cast<float>(okX), static_cast<float>(buttonY),
             static_cast<float>(buttonWidth), static_cast<float>(buttonHeight)},
            "OK", true)) {
        result = 0;
    }
    if (DrawButton(
            {static_cast<float>(cancelX), static_cast<float>(buttonY),
             static_cast<float>(buttonWidth), static_cast<float>(buttonHeight)},
            "Cancel", true)) {
        result = 1;
    }

    return result;
}

}  // namespace win95
