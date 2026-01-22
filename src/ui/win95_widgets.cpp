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

// Toolbar-specific widget implementations

// Helper function to draw toolbar icons
static void DrawToolbarIcon(const char* iconType, int centerX, int centerY, raylib::Color color, bool enabled) {
    if (!enabled) {
        color = raylib::Color{128, 128, 128, 255};
    }
    
    int size = theme::layout::scaleInt(14);
    int lineThickness = std::max(1, theme::layout::scaleInt(1));
    
    if (strcmp(iconType, "new") == 0) {
        // Document icon - rectangle with folded corner
        raylib::DrawRectangle(centerX - size/2, centerY - size/2, size - 3, size, color);
        raylib::DrawTriangle(
            {static_cast<float>(centerX + size/2 - 3), static_cast<float>(centerY - size/2)},
            {static_cast<float>(centerX + size/2), static_cast<float>(centerY - size/2 + 3)},
            {static_cast<float>(centerX + size/2 - 3), static_cast<float>(centerY - size/2 + 3)},
            theme::WINDOW_BG
        );
        // Lines on document
        for (int i = 0; i < 3; i++) {
            raylib::DrawLineEx(
                {static_cast<float>(centerX - size/2 + 2), static_cast<float>(centerY - size/2 + 3 + i * 3)},
                {static_cast<float>(centerX + size/2 - 5), static_cast<float>(centerY - size/2 + 3 + i * 3)},
                static_cast<float>(lineThickness), theme::WINDOW_BG
            );
        }
    } else if (strcmp(iconType, "open") == 0) {
        // Folder icon
        raylib::DrawRectangle(centerX - size/2, centerY - size/2 + 2, size, size - 4, color);
        raylib::DrawRectangle(centerX - size/2, centerY - size/2, size/2 + 2, 3, color);
        raylib::DrawRectangle(centerX - size/2 + 2, centerY - size/2 + 4, size - 4, size - 10, theme::WINDOW_BG);
    } else if (strcmp(iconType, "save") == 0) {
        // Floppy disk icon
        raylib::DrawRectangle(centerX - size/2, centerY - size/2, size, size, color);
        // Label area
        raylib::DrawRectangle(centerX - size/2 + 2, centerY - size/2 + 1, size - 4, 4, theme::WINDOW_BG);
        // Disk hole
        raylib::DrawRectangle(centerX - size/2 + 2, centerY + size/2 - 5, size - 4, 3, theme::WINDOW_BG);
        // Metal shutter
        raylib::DrawRectangle(centerX - size/2, centerY - size/2 + size - 6, size, 2, raylib::DARKGRAY);
    } else if (strcmp(iconType, "print") == 0) {
        // Printer icon
        raylib::DrawRectangle(centerX - size/2 + 2, centerY - size/2, size - 4, 3, color);
        raylib::DrawRectangle(centerX - size/2, centerY - size/2 + 3, size, size/2, color);
        raylib::DrawRectangle(centerX - size/2 + 2, centerY, size - 4, size/2 - 1, theme::WINDOW_BG);
        raylib::DrawRectangle(centerX + size/2 - 3, centerY - size/2 + 5, 2, 2, raylib::RED);
    } else if (strcmp(iconType, "cut") == 0) {
        // Scissors icon
        raylib::DrawCircle(centerX - 3, centerY - 3, 2, color);
        raylib::DrawCircle(centerX - 3, centerY + 3, 2, color);
        raylib::DrawLineEx({static_cast<float>(centerX - 3), static_cast<float>(centerY - 3)},
                          {static_cast<float>(centerX + size/2 - 2), static_cast<float>(centerY)}, 
                          static_cast<float>(lineThickness + 1), color);
        raylib::DrawLineEx({static_cast<float>(centerX - 3), static_cast<float>(centerY + 3)},
                          {static_cast<float>(centerX + size/2 - 2), static_cast<float>(centerY)}, 
                          static_cast<float>(lineThickness + 1), color);
    } else if (strcmp(iconType, "copy") == 0) {
        // Two overlapping documents
        raylib::DrawRectangle(centerX - size/2, centerY - size/2, size - 3, size - 3, theme::WINDOW_BG);
        raylib::DrawRectangleLines(centerX - size/2, centerY - size/2, size - 3, size - 3, color);
        raylib::DrawRectangle(centerX - size/2 + 3, centerY - size/2 + 3, size - 3, size - 3, theme::WINDOW_BG);
        raylib::DrawRectangleLines(centerX - size/2 + 3, centerY - size/2 + 3, size - 3, size - 3, color);
    } else if (strcmp(iconType, "paste") == 0) {
        // Clipboard icon
        raylib::DrawRectangle(centerX - size/2, centerY - size/2 + 3, size, size - 3, color);
        raylib::DrawRectangle(centerX - 2, centerY - size/2, 4, 4, color);
        raylib::DrawRectangle(centerX - size/2 + 2, centerY - size/2 + 5, size - 4, size - 8, theme::WINDOW_BG);
    } else if (strcmp(iconType, "undo") == 0) {
        // Curved arrow pointing left
        int arcRadius = size/3;
        raylib::DrawRing({static_cast<float>(centerX + 2), static_cast<float>(centerY)}, 
                        arcRadius - lineThickness, arcRadius, 90, 270, 16, color);
        raylib::DrawTriangle(
            {static_cast<float>(centerX - size/2 + 3), static_cast<float>(centerY - 2)},
            {static_cast<float>(centerX - size/2 + 3), static_cast<float>(centerY + 2)},
            {static_cast<float>(centerX - size/2 - 1), static_cast<float>(centerY)},
            color
        );
    } else if (strcmp(iconType, "redo") == 0) {
        // Curved arrow pointing right
        int arcRadius = size/3;
        raylib::DrawRing({static_cast<float>(centerX - 2), static_cast<float>(centerY)}, 
                        arcRadius - lineThickness, arcRadius, 270, 450, 16, color);
        raylib::DrawTriangle(
            {static_cast<float>(centerX + size/2 - 3), static_cast<float>(centerY - 2)},
            {static_cast<float>(centerX + size/2 - 3), static_cast<float>(centerY + 2)},
            {static_cast<float>(centerX + size/2 + 1), static_cast<float>(centerY)},
            color
        );
    } else if (strcmp(iconType, "B") == 0 || strcmp(iconType, "I") == 0 || strcmp(iconType, "U") == 0) {
        // Text formatting: just draw the letter
        int fontSize = 13;
        int textWidth = theme::MeasureUIText(iconType, fontSize);
        int textX = centerX - textWidth / 2;
        int textY = centerY - fontSize / 2;
        if (strcmp(iconType, "B") == 0) {
            // Draw bold by drawing twice with offset
            theme::DrawUIText(iconType, textX, textY, fontSize, color);
            theme::DrawUIText(iconType, textX + 1, textY, fontSize, color);
        } else {
            theme::DrawUIText(iconType, textX, textY, fontSize, color);
            if (strcmp(iconType, "U") == 0) {
                // Add underline
                raylib::DrawLineEx(
                    {static_cast<float>(centerX - size/2 + 2), static_cast<float>(centerY + size/2 - 2)},
                    {static_cast<float>(centerX + size/2 - 2), static_cast<float>(centerY + size/2 - 2)},
                    static_cast<float>(lineThickness), color
                );
            }
        }
    } else if (strcmp(iconType, "L") == 0 || strcmp(iconType, "C") == 0 || 
               strcmp(iconType, "R") == 0 || strcmp(iconType, "J") == 0) {
        // Alignment icons: lines representing text
        int lineLen = size;
        int spacing = 3;
        if (strcmp(iconType, "L") == 0) {
            // Left align
            for (int i = 0; i < 3; i++) {
                int len = (i == 1) ? lineLen - 3 : lineLen;
                raylib::DrawLineEx(
                    {static_cast<float>(centerX - size/2), static_cast<float>(centerY - 4 + i * spacing)},
                    {static_cast<float>(centerX - size/2 + len), static_cast<float>(centerY - 4 + i * spacing)},
                    static_cast<float>(lineThickness), color
                );
            }
        } else if (strcmp(iconType, "C") == 0) {
            // Center align
            for (int i = 0; i < 3; i++) {
                int len = (i == 1) ? lineLen - 4 : lineLen - 2;
                raylib::DrawLineEx(
                    {static_cast<float>(centerX - len/2), static_cast<float>(centerY - 4 + i * spacing)},
                    {static_cast<float>(centerX + len/2), static_cast<float>(centerY - 4 + i * spacing)},
                    static_cast<float>(lineThickness), color
                );
            }
        } else if (strcmp(iconType, "R") == 0) {
            // Right align
            for (int i = 0; i < 3; i++) {
                int len = (i == 1) ? lineLen - 3 : lineLen;
                raylib::DrawLineEx(
                    {static_cast<float>(centerX + size/2 - len), static_cast<float>(centerY - 4 + i * spacing)},
                    {static_cast<float>(centerX + size/2), static_cast<float>(centerY - 4 + i * spacing)},
                    static_cast<float>(lineThickness), color
                );
            }
        } else if (strcmp(iconType, "J") == 0) {
            // Justify
            for (int i = 0; i < 3; i++) {
                raylib::DrawLineEx(
                    {static_cast<float>(centerX - size/2), static_cast<float>(centerY - 4 + i * spacing)},
                    {static_cast<float>(centerX + size/2), static_cast<float>(centerY - 4 + i * spacing)},
                    static_cast<float>(lineThickness), color
                );
            }
        }
    } else {
        // Fallback: draw text label
        int fontSize = 10;
        int textWidth = theme::MeasureUIText(iconType, fontSize);
        int textX = centerX - textWidth / 2;
        int textY = centerY - fontSize / 2;
        theme::DrawUIText(iconType, textX, textY, fontSize, color);
    }
}

bool DrawToolbarButton(raylib::Rectangle rect, const char* icon, bool enabled, bool pressed) {
    bool clicked = false;
    ButtonState state = enabled ? ButtonState::Normal : ButtonState::Disabled;

    raylib::Vector2 mousePos = input::getMousePosition();
    bool hover = raylib::CheckCollisionPointRec(mousePos, rect);
    bool pressing = hover && IsMouseButtonDown(raylib::MOUSE_LEFT_BUTTON);

    if (enabled) {
        if (pressing || pressed) {
            state = ButtonState::Pressed;
        } else if (hover) {
            state = ButtonState::Hover;
            if (IsMouseButtonReleased(raylib::MOUSE_LEFT_BUTTON)) {
                clicked = true;
            }
        }
    }

    // Draw button background with better visual distinction
    raylib::Color bgColor = theme::BUTTON_FACE;
    if (state == ButtonState::Hover && !pressed) bgColor = theme::TOOLBAR_HOVER_BG;
    if (state == ButtonState::Pressed || pressed) bgColor = theme::TOOLBAR_PRESSED_BG;
    
    raylib::DrawRectangleRec(rect, bgColor);

    // Draw 3D border based on state (more pronounced)
    if (state == ButtonState::Pressed || pressed) {
        DrawSunkenBorder(rect, theme::layout::scaleInt(2));
    } else if (state == ButtonState::Hover || state == ButtonState::Normal) {
        DrawRaisedBorder(rect, theme::layout::scaleInt(2));
    }

    // Draw icon
    int centerX = static_cast<int>(rect.x + rect.width / 2);
    int centerY = static_cast<int>(rect.y + rect.height / 2);

    // Offset icon when pressed
    if (state == ButtonState::Pressed || pressed) {
        centerX += theme::layout::scaleInt(1);
        centerY += theme::layout::scaleInt(1);
    }

    raylib::Color iconColor = enabled ? theme::BUTTON_TEXT : theme::MENU_DISABLED;
    DrawToolbarIcon(icon, centerX, centerY, iconColor, enabled);

    return clicked;
}

void DrawToolbarSeparator(int x, int y, int height) {
    int scaledHeight = theme::layout::scaleInt(height);
    int scaledY = theme::layout::scaleInt(y);
    int scaledX = theme::layout::scaleInt(x);
    
    // Draw a vertical separator line with 3D effect
    raylib::DrawLine(scaledX, scaledY + 2, scaledX, scaledY + scaledHeight - 2, theme::BORDER_DARK);
    raylib::DrawLine(scaledX + 1, scaledY + 2, scaledX + 1, scaledY + scaledHeight - 2, theme::BORDER_LIGHT);
}

bool DrawDropdownButton(raylib::Rectangle rect, const char* label, bool open, bool enabled) {
    bool clicked = false;
    ButtonState state = enabled ? ButtonState::Normal : ButtonState::Disabled;

    raylib::Vector2 mousePos = input::getMousePosition();
    bool hover = raylib::CheckCollisionPointRec(mousePos, rect);

    if (enabled) {
        if (open) {
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

    // Draw 3D border
    if (state == ButtonState::Pressed || open) {
        DrawSunkenBorder(rect, theme::layout::scaleInt(2));
    } else {
        DrawRaisedBorder(rect, theme::layout::scaleInt(2));
    }

    // Draw label text (left-aligned)
    int fontSize = 12;
    int textX = static_cast<int>(rect.x) + theme::layout::scaleInt(4);
    int textY = static_cast<int>(rect.y + (rect.height - theme::layout::scaleInt(fontSize)) / 2);
    
    if (state == ButtonState::Pressed || open) {
        textX += theme::layout::scaleInt(1);
        textY += theme::layout::scaleInt(1);
    }
    
    raylib::Color textColor = enabled ? theme::TEXT_COLOR : theme::MENU_DISABLED;
    theme::DrawUIText(label, textX, textY, fontSize, textColor);

    // Draw dropdown arrow (small triangle on the right)
    int arrowX = static_cast<int>(rect.x + rect.width) - theme::layout::scaleInt(14);
    int arrowY = static_cast<int>(rect.y + rect.height / 2);
    
    if (state == ButtonState::Pressed || open) {
        arrowX += theme::layout::scaleInt(1);
        arrowY += theme::layout::scaleInt(1);
    }
    
    // Draw downward pointing triangle
    raylib::DrawTriangle(
        {static_cast<float>(arrowX), static_cast<float>(arrowY - 2)},
        {static_cast<float>(arrowX + 6), static_cast<float>(arrowY - 2)},
        {static_cast<float>(arrowX + 3), static_cast<float>(arrowY + 2)},
        textColor
    );

    return clicked;
}

int DrawDropdownList(raylib::Rectangle rect, const std::vector<std::string>& items, int /*hoveredIndex*/) {
    int selectedItem = -1;
    
    // Draw dropdown background
    raylib::DrawRectangleRec(rect, theme::WINDOW_BG);
    DrawSunkenBorder(rect, theme::layout::scaleInt(2));

    // Draw items
    raylib::Vector2 mousePos = input::getMousePosition();
    int itemHeight = theme::layout::scaleInt(20);
    int fontSize = 12;
    
    for (std::size_t i = 0; i < items.size(); ++i) {
        int itemY = static_cast<int>(rect.y) + theme::layout::scaleInt(2) + static_cast<int>(i) * itemHeight;
        int itemX = static_cast<int>(rect.x) + theme::layout::scaleInt(4);
        
        raylib::Rectangle itemRect = {
            rect.x + theme::layout::scale(2),
            static_cast<float>(itemY),
            rect.width - theme::layout::scale(4),
            static_cast<float>(itemHeight)
        };
        
        bool hover = raylib::CheckCollisionPointRec(mousePos, itemRect);
        
        if (hover) {
            raylib::DrawRectangleRec(itemRect, theme::MENU_HOVER);
            theme::DrawUIText(items[i].c_str(), itemX, itemY + theme::layout::scaleInt(3), 
                            fontSize, theme::MENU_TEXT_HOVER);
            
            if (IsMouseButtonReleased(raylib::MOUSE_LEFT_BUTTON)) {
                selectedItem = static_cast<int>(i);
            }
        } else {
            theme::DrawUIText(items[i].c_str(), itemX, itemY + theme::layout::scaleInt(3), 
                            fontSize, theme::TEXT_COLOR);
        }
    }

    return selectedItem;
}

}  // namespace win95
