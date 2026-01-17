#include "win95_widgets.h"

#include <algorithm>
#include <cstring>

#include "../testing/test_input.h"

namespace win95 {

void DrawRaisedBorder(raylib::Rectangle rect, int thickness) {
    int x = static_cast<int>(rect.x);
    int y = static_cast<int>(rect.y);
    int w = static_cast<int>(rect.width);
    int h = static_cast<int>(rect.height);

    for (int i = 0; i < thickness; ++i) {
        // Top edge (light)
        raylib::DrawLine(x + i, y + i, x + w - i - 1, y + i,
                         colors::BORDER_LIGHT);
        // Left edge (light)
        raylib::DrawLine(x + i, y + i, x + i, y + h - i - 1,
                         colors::BORDER_LIGHT);
        // Bottom edge (dark)
        raylib::DrawLine(x + i, y + h - i - 1, x + w - i, y + h - i - 1,
                         colors::BORDER_DARK);
        // Right edge (dark)
        raylib::DrawLine(x + w - i - 1, y + i, x + w - i - 1, y + h - i,
                         colors::BORDER_DARK);
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
                         colors::BORDER_DARK);
        // Left edge (dark)
        raylib::DrawLine(x + i, y + i, x + i, y + h - i - 1,
                         colors::BORDER_DARK);
        // Bottom edge (light)
        raylib::DrawLine(x + i, y + h - i - 1, x + w - i, y + h - i - 1,
                         colors::BORDER_LIGHT);
        // Right edge (light)
        raylib::DrawLine(x + w - i - 1, y + i, x + w - i - 1, y + h - i,
                         colors::BORDER_LIGHT);
    }
}

bool DrawButton(raylib::Rectangle rect, const char* text, bool enabled) {
    bool clicked = false;
    ButtonState state = enabled ? ButtonState::Normal : ButtonState::Disabled;

    raylib::Vector2 mousePos = raylib::GetMousePosition();
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
    raylib::DrawRectangleRec(rect, colors::BUTTON_FACE);

    // Draw 3D border based on state
    if (state == ButtonState::Pressed) {
        DrawSunkenBorder(rect, 2);
    } else {
        DrawRaisedBorder(rect, 2);
    }

    // Calculate text position
    int textWidth = raylib::MeasureText(text, 14);
    int textX = static_cast<int>(rect.x + (rect.width - textWidth) / 2);
    int textY = static_cast<int>(rect.y + (rect.height - 14) / 2);

    // Offset text when pressed
    if (state == ButtonState::Pressed) {
        textX += 1;
        textY += 1;
    }

    // Draw text
    raylib::Color textColor =
        enabled ? colors::TEXT_COLOR : colors::TEXT_DISABLED;
    raylib::DrawText(text, textX, textY, 14, textColor);

    return clicked;
}

bool DrawCheckbox(raylib::Rectangle rect, const char* text, bool* checked,
                  bool enabled) {
    bool changed = false;

    // Checkbox box is 13x13 pixels (Win95 authentic)
    constexpr int BOX_SIZE = 13;
    raylib::Rectangle boxRect = {rect.x, rect.y + (rect.height - BOX_SIZE) / 2,
                                 BOX_SIZE, BOX_SIZE};

    raylib::Vector2 mousePos = raylib::GetMousePosition();
    bool hover = raylib::CheckCollisionPointRec(mousePos, rect);

    if (enabled && hover &&
        IsMouseButtonReleased(raylib::MOUSE_LEFT_BUTTON)) {
        *checked = !*checked;
        changed = true;
    }

    // Draw checkbox background
    raylib::DrawRectangleRec(boxRect, colors::TEXT_AREA_BG);
    DrawSunkenBorder(boxRect, 2);

    // Draw checkmark if checked
    if (*checked) {
        int cx = static_cast<int>(boxRect.x) + 2;
        int cy = static_cast<int>(boxRect.y) + BOX_SIZE / 2;
        // Simple checkmark
        raylib::DrawLine(cx + 2, cy, cx + 4, cy + 3, colors::TEXT_COLOR);
        raylib::DrawLine(cx + 4, cy + 3, cx + 9, cy - 3, colors::TEXT_COLOR);
        raylib::DrawLine(cx + 2, cy + 1, cx + 4, cy + 4, colors::TEXT_COLOR);
        raylib::DrawLine(cx + 4, cy + 4, cx + 9, cy - 2, colors::TEXT_COLOR);
    }

    // Draw label
    int textX = static_cast<int>(boxRect.x + BOX_SIZE + 6);
    int textY = static_cast<int>(rect.y + (rect.height - 14) / 2);
    raylib::Color textColor =
        enabled ? colors::TEXT_COLOR : colors::TEXT_DISABLED;
    raylib::DrawText(text, textX, textY, 14, textColor);

    return changed;
}

int DrawMenuBar(std::vector<Menu>& menus, int menuBarY, int menuBarHeight) {
    int clickedMenu = -1;
    int x = 4;

    raylib::Vector2 mousePos = raylib::GetMousePosition();
    bool mouseInMenuBar =
        mousePos.y >= menuBarY && mousePos.y < menuBarY + menuBarHeight;

    for (std::size_t i = 0; i < menus.size(); ++i) {
        Menu& menu = menus[i];
        int textWidth = raylib::MeasureText(menu.label.c_str(), 14);
        int itemWidth = textWidth + 16;  // Padding on each side

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
            raylib::DrawRectangleRec(menu.bounds, colors::MENU_HIGHLIGHT);
            raylib::DrawText(menu.label.c_str(), x + 8, menuBarY + 3, 14,
                             colors::TITLE_TEXT);
        } else {
            raylib::DrawText(menu.label.c_str(), x + 8, menuBarY + 3, 14,
                             colors::TEXT_COLOR);
        }

        // Draw dropdown if open
        if (menu.open) {
            int selectedItem =
                DrawDropdownMenu(menu, x, menuBarY + menuBarHeight, 20);
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
                int dropdownHeight = static_cast<int>(menu.items.size()) * 20;
                raylib::Rectangle dropdownRect = {
                    menu.bounds.x, menu.bounds.y + menu.bounds.height, 150,
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
    int maxWidth = 150;
    for (const auto& item : menu.items) {
        int w = raylib::MeasureText(item.label.c_str(), 14);
        if (!item.shortcut.empty()) {
            w += raylib::MeasureText(item.shortcut.c_str(), 14) + 20;
        }
        maxWidth = std::max(maxWidth, w + 32);
    }

    int totalHeight = static_cast<int>(menu.items.size()) * itemHeight;
    raylib::Rectangle dropdownRect = {
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(maxWidth), static_cast<float>(totalHeight)};

    // Draw dropdown background
    raylib::DrawRectangleRec(dropdownRect, colors::WINDOW_BG);
    DrawRaisedBorder(dropdownRect, 2);

    // Draw items
    raylib::Vector2 mousePos = raylib::GetMousePosition();
    int itemY = y;

    for (std::size_t i = 0; i < menu.items.size(); ++i) {
        const MenuItem& item = menu.items[i];
        raylib::Rectangle itemRect = {
            static_cast<float>(x + 2), static_cast<float>(itemY),
            static_cast<float>(maxWidth - 4), static_cast<float>(itemHeight)};

        if (item.separator) {
            // Draw separator line
            int sepY = itemY + itemHeight / 2;
            raylib::DrawLine(x + 4, sepY, x + maxWidth - 4, sepY,
                             colors::BORDER_DARK);
            raylib::DrawLine(x + 4, sepY + 1, x + maxWidth - 4, sepY + 1,
                             colors::BORDER_LIGHT);
        } else {
            bool hover = raylib::CheckCollisionPointRec(mousePos, itemRect) &&
                         item.enabled;
            
            // Register menu item text for E2E testing
            test_input::registerVisibleText(item.label);
            
            // Reserve 20 pixels for mark column
            const int markColumnWidth = 20;
            const int textX = x + markColumnWidth;

            if (hover) {
                raylib::DrawRectangleRec(itemRect, colors::MENU_HIGHLIGHT);
                
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
                        raylib::DrawText(markStr, x + 6, itemY + 3, 14,
                                         colors::TITLE_TEXT);
                    }
                }
                
                raylib::DrawText(item.label.c_str(), textX, itemY + 3, 14,
                                 colors::TITLE_TEXT);
                if (!item.shortcut.empty()) {
                    int shortcutX =
                        x + maxWidth -
                        raylib::MeasureText(item.shortcut.c_str(), 14) - 12;
                    raylib::DrawText(item.shortcut.c_str(), shortcutX,
                                     itemY + 3, 14, colors::TITLE_TEXT);
                }

                if (IsMouseButtonReleased(raylib::MOUSE_LEFT_BUTTON)) {
                    selectedItem = static_cast<int>(i);
                }
            } else {
                raylib::Color textColor =
                    item.enabled ? colors::TEXT_COLOR : colors::TEXT_DISABLED;
                
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
                        raylib::DrawText(markStr, x + 6, itemY + 3, 14, textColor);
                    }
                }
                
                raylib::DrawText(item.label.c_str(), textX, itemY + 3, 14,
                                 textColor);
                if (!item.shortcut.empty()) {
                    int shortcutX =
                        x + maxWidth -
                        raylib::MeasureText(item.shortcut.c_str(), 14) - 12;
                    raylib::DrawText(item.shortcut.c_str(), shortcutX,
                                     itemY + 3, 14, textColor);
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
    raylib::DrawRectangleRec(dialogRect, colors::WINDOW_BG);
    DrawRaisedBorder(dialogRect, 2);

    // Draw title bar
    raylib::Rectangle titleRect = {dialogRect.x + 2, dialogRect.y + 2,
                                   dialogRect.width - 4, 20};
    raylib::DrawRectangleRec(titleRect, colors::TITLE_BAR_ACTIVE);
    raylib::DrawText(title, static_cast<int>(titleRect.x) + 4,
                     static_cast<int>(titleRect.y) + 3, 14, colors::TITLE_TEXT);

    // Draw message
    int messageX = static_cast<int>(dialogRect.x) + 16;
    int messageY = static_cast<int>(dialogRect.y) + 40;
    raylib::DrawText(message, messageX, messageY, 14, colors::TEXT_COLOR);

    // Draw buttons
    int buttonWidth = 75;
    int buttonHeight = 23;
    int buttonY =
        static_cast<int>(dialogRect.y + dialogRect.height) - buttonHeight - 12;

    if (hasCancel) {
        int okX = static_cast<int>(dialogRect.x + dialogRect.width) -
                  2 * buttonWidth - 24;
        int cancelX = static_cast<int>(dialogRect.x + dialogRect.width) -
                      buttonWidth - 12;

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
    raylib::DrawRectangleRec(dialogRect, colors::WINDOW_BG);
    DrawRaisedBorder(dialogRect, 2);

    // Draw title bar
    raylib::Rectangle titleRect = {dialogRect.x + 2, dialogRect.y + 2,
                                   dialogRect.width - 4, 20};
    raylib::DrawRectangleRec(titleRect, colors::TITLE_BAR_ACTIVE);
    raylib::DrawText(title, static_cast<int>(titleRect.x) + 4,
                     static_cast<int>(titleRect.y) + 3, 14, colors::TITLE_TEXT);

    // Draw prompt
    int promptX = static_cast<int>(dialogRect.x) + 16;
    int promptY = static_cast<int>(dialogRect.y) + 36;
    raylib::DrawText(prompt, promptX, promptY, 14, colors::TEXT_COLOR);

    // Draw input field
    raylib::Rectangle inputRect = {dialogRect.x + 16, dialogRect.y + 56,
                                   dialogRect.width - 32, 22};
    raylib::DrawRectangleRec(inputRect, colors::TEXT_AREA_BG);
    DrawSunkenBorder(inputRect, 2);

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
    raylib::DrawText(buffer, static_cast<int>(inputRect.x) + 4,
                     static_cast<int>(inputRect.y) + 4, 14, colors::TEXT_COLOR);

    // Draw buttons
    int buttonWidth = 75;
    int buttonHeight = 23;
    int buttonY =
        static_cast<int>(dialogRect.y + dialogRect.height) - buttonHeight - 12;
    int okX = static_cast<int>(dialogRect.x + dialogRect.width) -
              2 * buttonWidth - 24;
    int cancelX =
        static_cast<int>(dialogRect.x + dialogRect.width) - buttonWidth - 12;

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

void applyDarkMode(bool enabled) {
    if (enabled) {
        colors::WINDOW_BG = {48, 48, 48, 255};
        colors::TITLE_BAR_ACTIVE = {32, 32, 64, 255};
        colors::TITLE_BAR_INACTIVE = {64, 64, 64, 255};
        colors::TITLE_TEXT = {255, 255, 255, 255};
        colors::TEXT_AREA_BG = {24, 24, 24, 255};
        colors::TEXT_COLOR = {230, 230, 230, 255};
        colors::TEXT_DISABLED = {120, 120, 120, 255};
        colors::BORDER_LIGHT = {90, 90, 90, 255};
        colors::BORDER_DARK = {20, 20, 20, 255};
        colors::BORDER_DARKER = {10, 10, 10, 255};
        colors::MENU_HIGHLIGHT = {64, 64, 128, 255};
        colors::BUTTON_FACE = {64, 64, 64, 255};
    } else {
        colors::WINDOW_BG = {192, 192, 192, 255};
        colors::TITLE_BAR_ACTIVE = {0, 0, 128, 255};
        colors::TITLE_BAR_INACTIVE = {128, 128, 128, 255};
        colors::TITLE_TEXT = {255, 255, 255, 255};
        colors::TEXT_AREA_BG = {255, 255, 255, 255};
        colors::TEXT_COLOR = {0, 0, 0, 255};
        colors::TEXT_DISABLED = {128, 128, 128, 255};
        colors::BORDER_LIGHT = {255, 255, 255, 255};
        colors::BORDER_DARK = {128, 128, 128, 255};
        colors::BORDER_DARKER = {64, 64, 64, 255};
        colors::MENU_HIGHLIGHT = {0, 0, 128, 255};
        colors::BUTTON_FACE = {192, 192, 192, 255};
    }
}

}  // namespace win95
