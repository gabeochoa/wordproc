#pragma once

#include "../../vendor/afterhours/src/core/system.h"
#include "../rl.h"
#include "../ui/theme.h"
#include "../ui/win95_widgets.h"
#include "../ui/input.h"
#include "../ui/ui_context.h"
#include "../util/drawing.h"
#include "../editor/document_io.h"
#include "components.h"
#include "component_helpers.h"

namespace ecs {

// Toolbar Render System - renders the standard toolbar and formatting toolbar
struct ToolbarRenderSystem : afterhours::System<ToolbarComponent, LayoutComponent, DocumentComponent> {
    
    void for_each_with(const afterhours::Entity& /*entity*/,
                       const ToolbarComponent& constToolbar,
                       const LayoutComponent& layout,
                       const DocumentComponent& constDoc,
                       const float) const override {
        // Cast away const to allow modifications (needed for UI interactions)
        auto& toolbar = const_cast<ToolbarComponent&>(constToolbar);
        auto& doc = const_cast<DocumentComponent&>(constDoc);
        
        // Skip rendering toolbars in focus mode
        if (layout.focusMode) {
            return;
        }
        
        int screenWidth = layout.screenWidth;
        
        // Calculate toolbar positions
        float toolbarY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + theme::layout::MENU_BAR_HEIGHT);
        float formattingBarY = toolbarY + theme::layout::scale(theme::layout::TOOLBAR_HEIGHT);
        
        // Draw standard toolbar background
        raylib::Rectangle toolbarRect = {
            0, toolbarY,
            static_cast<float>(screenWidth),
            theme::layout::scale(theme::layout::TOOLBAR_HEIGHT)
        };
        raylib::DrawRectangleRec(toolbarRect, theme::TOOLBAR_BG);
        util::drawRaisedBorder(toolbarRect);
        
        // Draw formatting toolbar background
        raylib::Rectangle formattingBarRect = {
            0, formattingBarY,
            static_cast<float>(screenWidth),
            theme::layout::scale(theme::layout::FORMATTING_BAR_HEIGHT)
        };
        raylib::DrawRectangleRec(formattingBarRect, theme::TOOLBAR_BG);
        util::drawRaisedBorder(formattingBarRect);
        
        // === Standard Toolbar Buttons ===
        int buttonSize = theme::layout::scaleInt(theme::layout::TOOLBAR_BUTTON_SIZE);
        int buttonPadding = theme::layout::scaleInt(theme::layout::TOOLBAR_BUTTON_PADDING);
        int x = buttonPadding;
        int y = static_cast<int>(toolbarY) + buttonPadding;
        
        // File operations
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "new", true)) {
            // New document
            doc.buffer.setText("");
            doc.filePath.clear();
            doc.isDirty = false;
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "open", true)) {
            // Open document (would trigger file dialog)
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "save", true)) {
            // Save document
            if (!doc.filePath.empty()) {
                auto result = saveDocumentEx(doc.buffer, doc.docSettings, doc.filePath);
                if (result.success) {
                    doc.isDirty = false;
                }
            }
        }
        x += buttonSize + buttonPadding;
        
        // Separator
        win95::DrawToolbarSeparator(x, y, buttonSize);
        x += theme::layout::scaleInt(6);
        
        // Print
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "print", true)) {
            // Print (not implemented)
        }
        x += buttonSize + buttonPadding;
        
        // Separator
        win95::DrawToolbarSeparator(x, y, buttonSize);
        x += theme::layout::scaleInt(6);
        
        // Cut/Copy/Paste
        bool hasSelection = doc.buffer.hasSelection();
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "cut", hasSelection)) {
            if (hasSelection) {
                // Cut operation (would need clipboard integration)
                doc.isDirty = true;
            }
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "copy", hasSelection)) {
            // Copy operation
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "paste", true)) {
            // Paste operation (would need clipboard)
        }
        x += buttonSize + buttonPadding;
        
        // Separator
        win95::DrawToolbarSeparator(x, y, buttonSize);
        x += theme::layout::scaleInt(6);
        
        // Undo/Redo
        bool canUndo = doc.buffer.canUndo();
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "undo", canUndo)) {
            if (canUndo) {
                doc.buffer.undo();
                doc.isDirty = true;
            }
        }
        x += buttonSize + buttonPadding;
        
        bool canRedo = doc.buffer.canRedo();
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "redo", canRedo)) {
            if (canRedo) {
                doc.buffer.redo();
                doc.isDirty = true;
            }
        }
        x += buttonSize + buttonPadding;
        
        // === Formatting Toolbar ===
        x = buttonPadding;
        y = static_cast<int>(formattingBarY) + buttonPadding;
        
        // Style dropdown
        int dropdownWidth = theme::layout::scaleInt(120);
        int dropdownHeight = theme::layout::scaleInt(22);
        
        if (win95::DrawDropdownButton({static_cast<float>(x), static_cast<float>(y), 
                                       static_cast<float>(dropdownWidth), static_cast<float>(dropdownHeight)}, 
                                      toolbar.currentStyle.c_str(), 
                                      toolbar.styleDropdownOpen, true)) {
            toolbar.styleDropdownOpen = !toolbar.styleDropdownOpen;
            toolbar.fontDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;
        }
        
        // Draw style dropdown list if open
        if (toolbar.styleDropdownOpen) {
            int listHeight = static_cast<int>(toolbar.styles.size()) * theme::layout::scaleInt(20) + theme::layout::scaleInt(4);
            raylib::Rectangle listRect = {
                static_cast<float>(x),
                static_cast<float>(y + dropdownHeight),
                static_cast<float>(dropdownWidth),
                static_cast<float>(listHeight)
            };
            
            int selected = win95::DrawDropdownList(listRect, toolbar.styles);
            if (selected >= 0) {
                toolbar.currentStyle = toolbar.styles[selected];
                toolbar.styleDropdownOpen = false;
                // Apply style to current selection
            }
        }
        
        x += dropdownWidth + buttonPadding * 2;
        
        // Font dropdown
        dropdownWidth = theme::layout::scaleInt(140);
        if (win95::DrawDropdownButton({static_cast<float>(x), static_cast<float>(y), 
                                       static_cast<float>(dropdownWidth), static_cast<float>(dropdownHeight)}, 
                                      toolbar.currentFont.c_str(), 
                                      toolbar.fontDropdownOpen, true)) {
            toolbar.fontDropdownOpen = !toolbar.fontDropdownOpen;
            toolbar.styleDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;
        }
        
        // Draw font dropdown list if open
        if (toolbar.fontDropdownOpen) {
            int listHeight = static_cast<int>(toolbar.fonts.size()) * theme::layout::scaleInt(20) + theme::layout::scaleInt(4);
            raylib::Rectangle listRect = {
                static_cast<float>(x),
                static_cast<float>(y + dropdownHeight),
                static_cast<float>(dropdownWidth),
                static_cast<float>(listHeight)
            };
            
            int selected = win95::DrawDropdownList(listRect, toolbar.fonts);
            if (selected >= 0) {
                toolbar.currentFont = toolbar.fonts[selected];
                toolbar.fontDropdownOpen = false;
                // Apply font to document
                TextStyle style = doc.buffer.textStyle();
                style.font = toolbar.currentFont;
                doc.buffer.setTextStyle(style);
                doc.isDirty = true;
            }
        }
        
        x += dropdownWidth + buttonPadding * 2;
        
        // Font size dropdown
        dropdownWidth = theme::layout::scaleInt(50);
        std::string fontSizeStr = std::to_string(toolbar.currentFontSize);
        if (win95::DrawDropdownButton({static_cast<float>(x), static_cast<float>(y), 
                                       static_cast<float>(dropdownWidth), static_cast<float>(dropdownHeight)}, 
                                      fontSizeStr.c_str(), 
                                      toolbar.fontSizeDropdownOpen, true)) {
            toolbar.fontSizeDropdownOpen = !toolbar.fontSizeDropdownOpen;
            toolbar.styleDropdownOpen = false;
            toolbar.fontDropdownOpen = false;
        }
        
        // Draw font size dropdown list if open
        if (toolbar.fontSizeDropdownOpen) {
            std::vector<std::string> fontSizeStrs;
            for (int size : toolbar.fontSizes) {
                fontSizeStrs.push_back(std::to_string(size));
            }
            
            int listHeight = static_cast<int>(fontSizeStrs.size()) * theme::layout::scaleInt(20) + theme::layout::scaleInt(4);
            raylib::Rectangle listRect = {
                static_cast<float>(x),
                static_cast<float>(y + dropdownHeight),
                static_cast<float>(dropdownWidth),
                static_cast<float>(listHeight)
            };
            
            int selected = win95::DrawDropdownList(listRect, fontSizeStrs);
            if (selected >= 0) {
                toolbar.currentFontSize = toolbar.fontSizes[selected];
                toolbar.fontSizeDropdownOpen = false;
                // Apply font size to document
                TextStyle style = doc.buffer.textStyle();
                style.fontSize = toolbar.currentFontSize;
                doc.buffer.setTextStyle(style);
                doc.isDirty = true;
            }
        }
        
        x += dropdownWidth + buttonPadding * 2;
        
        // Separator
        win95::DrawToolbarSeparator(x, y, dropdownHeight);
        x += theme::layout::scaleInt(6);
        
        // Formatting buttons (Bold, Italic, Underline)
        buttonSize = theme::layout::scaleInt(theme::layout::TOOLBAR_BUTTON_SIZE);
        y = static_cast<int>(formattingBarY) + buttonPadding;
        
        // Update toolbar button states based on current text style
        TextStyle currentStyle = doc.buffer.textStyle();
        toolbar.boldActive = currentStyle.bold;
        toolbar.italicActive = currentStyle.italic;
        toolbar.underlineActive = currentStyle.underline;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "B", true, toolbar.boldActive)) {
            TextStyle style = doc.buffer.textStyle();
            style.bold = !style.bold;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "I", true, toolbar.italicActive)) {
            TextStyle style = doc.buffer.textStyle();
            style.italic = !style.italic;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "U", true, toolbar.underlineActive)) {
            TextStyle style = doc.buffer.textStyle();
            style.underline = !style.underline;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        x += buttonSize + buttonPadding;
        
        // Separator
        win95::DrawToolbarSeparator(x, y, buttonSize);
        x += theme::layout::scaleInt(6);
        
        // Alignment buttons
        TextAlignment currentAlign = doc.buffer.currentAlignment();
        toolbar.currentAlignment = static_cast<int>(currentAlign);
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "L", true, toolbar.currentAlignment == 0)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Left);
            toolbar.currentAlignment = 0;
            doc.isDirty = true;
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "C", true, toolbar.currentAlignment == 1)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Center);
            toolbar.currentAlignment = 1;
            doc.isDirty = true;
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "R", true, toolbar.currentAlignment == 2)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Right);
            toolbar.currentAlignment = 2;
            doc.isDirty = true;
        }
        x += buttonSize + buttonPadding;
        
        if (win95::DrawToolbarButton({static_cast<float>(x), static_cast<float>(y), 
                                      static_cast<float>(buttonSize), static_cast<float>(buttonSize)}, 
                                     "J", true, toolbar.currentAlignment == 3)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Justify);
            toolbar.currentAlignment = 3;
            doc.isDirty = true;
        }
        
        // Close dropdowns on click outside
        if (IsMouseButtonPressed(raylib::MOUSE_LEFT_BUTTON)) {
            raylib::Vector2 mousePos = input::getMousePosition();
            
            // For simplicity, close all dropdowns on any click outside the toolbar area
            if (mousePos.y < formattingBarY || mousePos.y > formattingBarY + theme::layout::scale(theme::layout::FORMATTING_BAR_HEIGHT) + 200) {
                toolbar.styleDropdownOpen = false;
                toolbar.fontDropdownOpen = false;
                toolbar.fontSizeDropdownOpen = false;
            }
        }
    }
};

}  // namespace ecs

