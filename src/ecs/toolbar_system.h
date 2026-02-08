#pragma once

#include "../../vendor/afterhours/src/core/system.h"
#include "../rl.h"
#include "../ui/theme.h"
#include "../ui/ah_win95_widgets.h"
#include "../ui/ah_modal_input.h"
#include "../ui/input.h"
#include "../ui/ui_context.h"
#include "../editor/document_io.h"
#include "components.h"
#include "component_helpers.h"

namespace ecs {

using afterhours::Entity;
using afterhours::ui::UIContext;
using afterhours::ui::imm::ComponentConfig;
using afterhours::ui::imm::div;
using afterhours::ui::imm::button;
using afterhours::ui::imm::mk;
using afterhours::ui::pixels;
using afterhours::ui::percent;
using afterhours::ui::FlexDirection;
using afterhours::ui::AlignItems;
using afterhours::ui::JustifyContent;
using afterhours::ui::ComponentSize;
using afterhours::ui::Padding;
using afterhours::ui::Margin;

// Toolbar Render System - renders the standard toolbar and formatting toolbar
// Uses UIContext for Afterhours widgets, manually queries for other components
struct ToolbarRenderSystem : afterhours::System<UIContext<InputAction>> {
    
    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float) override {
        // Find toolbar entities
        auto toolbarEntities = afterhours::EntityQuery({.force_merge = true})
                                  .whereHasComponent<ToolbarComponent>()
                                  .gen();
        if (toolbarEntities.empty()) return;
        
        auto& toolbar = toolbarEntities[0].get().get<ToolbarComponent>();
        
        // Find document entities
        auto docEntities = afterhours::EntityQuery({.force_merge = true})
                              .whereHasComponent<DocumentComponent>()
                              .gen();
        if (docEntities.empty()) return;
        
        auto& doc = docEntities[0].get().get<DocumentComponent>();
        
        // Find layout entities
        auto layoutEntities = afterhours::EntityQuery({.force_merge = true})
                                 .whereHasComponent<LayoutComponent>()
                                 .gen();
        if (layoutEntities.empty()) return;
        
        auto& layout = layoutEntities[0].get().get<LayoutComponent>();
        
        // Skip rendering toolbars in focus mode
        if (layout.focusMode) {
            return;
        }
        
        float screenWidth = static_cast<float>(layout.screenWidth);
        
        // Calculate toolbar positions
        float toolbarY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + theme::layout::MENU_BAR_HEIGHT);
        float formattingBarY = toolbarY + theme::layout::scale(theme::layout::TOOLBAR_HEIGHT);
        float toolbarHeight = theme::layout::scale(theme::layout::TOOLBAR_HEIGHT);
        float formattingBarHeight = theme::layout::scale(theme::layout::FORMATTING_BAR_HEIGHT);
        
        // Get the UI root entity for Afterhours components
        Entity& uiRoot = ui_imm::getUIRootEntity();
        
        // Button dimensions
        float buttonSize = theme::layout::scale(theme::layout::TOOLBAR_BUTTON_SIZE);
        float buttonPadding = theme::layout::scale(theme::layout::TOOLBAR_BUTTON_PADDING);
        
        // === Standard Toolbar Background ===
        auto stdToolbar = div(ctx, mk(uiRoot, 1000),
            ComponentConfig{}
                .with_size(ComponentSize{pixels(screenWidth), pixels(toolbarHeight)})
                .with_absolute_position()
                .with_translate(0, toolbarY)
                .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                .with_border(ui_imm::win95_colors::BORDER_LIGHT, 1.0f)
                .with_roundness(0.0f)
                .with_flex_direction(FlexDirection::Row)
                .with_align_items(AlignItems::Center)
                .with_padding(Padding{.top = pixels(buttonPadding), .right = pixels(buttonPadding), 
                                     .bottom = pixels(buttonPadding), .left = pixels(buttonPadding)})
                .with_debug_name("std_toolbar"));
        
        int btnId = 1;
        
        // === File Operations ===
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true).with_label("N").with_debug_name("btn_new"))) {
            doc.buffer.setText("");
            doc.filePath.clear();
            doc.isDirty = false;
        }
        
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true).with_label("O").with_debug_name("btn_open"))) {
            // Open document (would trigger file dialog)
        }
        
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true).with_label("S").with_debug_name("btn_save"))) {
            if (!doc.filePath.empty()) {
                auto result = saveDocumentEx(doc.buffer, doc.docSettings, doc.filePath);
                if (result.success) {
                    doc.isDirty = false;
                }
            }
        }
        
        // Separator
        div(ctx, mk(stdToolbar.ent(), btnId++), ah_win95::win95SeparatorStyle(buttonSize - 4).with_debug_name("sep1"));
        
        // Print
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true).with_label("P").with_debug_name("btn_print"))) {
            // Print (not implemented)
        }
        
        // Separator
        div(ctx, mk(stdToolbar.ent(), btnId++), ah_win95::win95SeparatorStyle(buttonSize - 4).with_debug_name("sep2"));
        
        // Cut/Copy/Paste
        bool hasSelection = doc.buffer.hasSelection();
        
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, hasSelection).with_label("X").with_debug_name("btn_cut"))) {
            if (hasSelection) {
                doc.isDirty = true;
            }
        }
        
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, hasSelection).with_label("C").with_debug_name("btn_copy"))) {
            // Copy operation
        }
        
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true).with_label("V").with_debug_name("btn_paste"))) {
            // Paste operation
        }
        
        // Separator
        div(ctx, mk(stdToolbar.ent(), btnId++), ah_win95::win95SeparatorStyle(buttonSize - 4).with_debug_name("sep3"));
        
        // Undo/Redo
        bool canUndo = doc.buffer.canUndo();
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, canUndo).with_label("<").with_debug_name("btn_undo"))) {
            if (canUndo) {
                doc.buffer.undo();
                doc.isDirty = true;
            }
        }
        
        bool canRedo = doc.buffer.canRedo();
        if (button(ctx, mk(stdToolbar.ent(), btnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, canRedo).with_label(">").with_debug_name("btn_redo"))) {
            if (canRedo) {
                doc.buffer.redo();
                doc.isDirty = true;
            }
        }
        
        // === Formatting Toolbar Background ===
        auto fmtToolbar = div(ctx, mk(uiRoot, 2000),
            ComponentConfig{}
                .with_size(ComponentSize{pixels(screenWidth), pixels(formattingBarHeight)})
                .with_absolute_position()
                .with_translate(0, formattingBarY)
                .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                .with_border(ui_imm::win95_colors::BORDER_LIGHT, 1.0f)
                .with_roundness(0.0f)
                .with_flex_direction(FlexDirection::Row)
                .with_align_items(AlignItems::Center)
                .with_padding(Padding{.top = pixels(buttonPadding), .right = pixels(buttonPadding), 
                                     .bottom = pixels(buttonPadding), .left = pixels(buttonPadding)})
                .with_debug_name("fmt_toolbar"));
        
        int fmtBtnId = 1;
        
        // Dropdown dimensions
        float dropdownHeight = theme::layout::scale(22);
        
        // === Style Dropdown ===
        float styleDropdownWidth = theme::layout::scale(120);
        std::string styleLabel = toolbar.currentStyle + " v";
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95DropdownButtonStyle(styleDropdownWidth, dropdownHeight, toolbar.styleDropdownOpen)
                .with_label(styleLabel)
                .with_debug_name("dropdown_style"))) {
            toolbar.styleDropdownOpen = !toolbar.styleDropdownOpen;
            toolbar.fontDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;
        }
        
        // Style dropdown list
        if (toolbar.styleDropdownOpen && !toolbar.styles.empty()) {
            auto styleList = div(ctx, mk(uiRoot, 3000),
                ah_win95::win95DropdownListStyle(styleDropdownWidth, static_cast<int>(toolbar.styles.size()))
                    .with_translate(buttonPadding, formattingBarY + dropdownHeight + buttonPadding)
                    .with_debug_name("dropdown_style_list"));
            
            for (size_t i = 0; i < toolbar.styles.size(); ++i) {
                bool isSelected = (toolbar.styles[i] == toolbar.currentStyle);
                if (button(ctx, mk(styleList.ent(), static_cast<int>(i)),
                    ah_win95::win95DropdownItemStyle(isSelected)
                        .with_label(toolbar.styles[i])
                        .with_debug_name("style_item_" + std::to_string(i)))) {
                    toolbar.currentStyle = toolbar.styles[i];
                    toolbar.styleDropdownOpen = false;
                }
            }
        }
        
        // === Font Dropdown ===
        float fontDropdownWidth = theme::layout::scale(140);
        float fontDropdownX = buttonPadding + styleDropdownWidth + buttonPadding * 2;
        std::string fontLabel = toolbar.currentFont + " v";
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95DropdownButtonStyle(fontDropdownWidth, dropdownHeight, toolbar.fontDropdownOpen)
                .with_label(fontLabel)
                .with_debug_name("dropdown_font"))) {
            toolbar.fontDropdownOpen = !toolbar.fontDropdownOpen;
            toolbar.styleDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;
        }
        
        // Font dropdown list
        if (toolbar.fontDropdownOpen && !toolbar.fonts.empty()) {
            auto fontList = div(ctx, mk(uiRoot, 3100),
                ah_win95::win95DropdownListStyle(fontDropdownWidth, static_cast<int>(toolbar.fonts.size()))
                    .with_translate(fontDropdownX, formattingBarY + dropdownHeight + buttonPadding)
                    .with_debug_name("dropdown_font_list"));
            
            for (size_t i = 0; i < toolbar.fonts.size(); ++i) {
                bool isSelected = (toolbar.fonts[i] == toolbar.currentFont);
                if (button(ctx, mk(fontList.ent(), static_cast<int>(i)),
                    ah_win95::win95DropdownItemStyle(isSelected)
                        .with_label(toolbar.fonts[i])
                        .with_debug_name("font_item_" + std::to_string(i)))) {
                    toolbar.currentFont = toolbar.fonts[i];
                    toolbar.fontDropdownOpen = false;
                    TextStyle style = doc.buffer.textStyle();
                    style.font = toolbar.currentFont;
                    doc.buffer.setTextStyle(style);
                    doc.isDirty = true;
                }
            }
        }
        
        // === Font Size Dropdown ===
        float fontSizeDropdownWidth = theme::layout::scale(50);
        float fontSizeDropdownX = fontDropdownX + fontDropdownWidth + buttonPadding * 2;
        std::string fontSizeLabel = std::to_string(toolbar.currentFontSize) + " v";
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95DropdownButtonStyle(fontSizeDropdownWidth, dropdownHeight, toolbar.fontSizeDropdownOpen)
                .with_label(fontSizeLabel)
                .with_debug_name("dropdown_fontsize"))) {
            toolbar.fontSizeDropdownOpen = !toolbar.fontSizeDropdownOpen;
            toolbar.styleDropdownOpen = false;
            toolbar.fontDropdownOpen = false;
        }
        
        // Font size dropdown list
        if (toolbar.fontSizeDropdownOpen && !toolbar.fontSizes.empty()) {
            auto sizeList = div(ctx, mk(uiRoot, 3200),
                ah_win95::win95DropdownListStyle(fontSizeDropdownWidth, static_cast<int>(toolbar.fontSizes.size()))
                    .with_translate(fontSizeDropdownX, formattingBarY + dropdownHeight + buttonPadding)
                    .with_debug_name("dropdown_fontsize_list"));
            
            for (size_t i = 0; i < toolbar.fontSizes.size(); ++i) {
                bool isSelected = (toolbar.fontSizes[i] == toolbar.currentFontSize);
                std::string sizeStr = std::to_string(toolbar.fontSizes[i]);
                if (button(ctx, mk(sizeList.ent(), static_cast<int>(i)),
                    ah_win95::win95DropdownItemStyle(isSelected)
                        .with_label(sizeStr)
                        .with_debug_name("size_item_" + std::to_string(i)))) {
                    toolbar.currentFontSize = toolbar.fontSizes[i];
                    toolbar.fontSizeDropdownOpen = false;
                    TextStyle style = doc.buffer.textStyle();
                    style.fontSize = toolbar.currentFontSize;
                    doc.buffer.setTextStyle(style);
                    doc.isDirty = true;
                }
            }
        }
        
        // Separator after dropdowns
        div(ctx, mk(fmtToolbar.ent(), fmtBtnId++), ah_win95::win95SeparatorStyle(dropdownHeight - 4).with_debug_name("sep4"));
        
        // === Formatting Buttons (Bold, Italic, Underline) ===
        TextStyle currentStyle = doc.buffer.textStyle();
        toolbar.boldActive = currentStyle.bold;
        toolbar.italicActive = currentStyle.italic;
        toolbar.underlineActive = currentStyle.underline;
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true, toolbar.boldActive)
                .with_label("B")
                .with_debug_name("btn_bold"))) {
            TextStyle style = doc.buffer.textStyle();
            style.bold = !style.bold;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true, toolbar.italicActive)
                .with_label("I")
                .with_debug_name("btn_italic"))) {
            TextStyle style = doc.buffer.textStyle();
            style.italic = !style.italic;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true, toolbar.underlineActive)
                .with_label("U")
                .with_debug_name("btn_underline"))) {
            TextStyle style = doc.buffer.textStyle();
            style.underline = !style.underline;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        
        // Separator
        div(ctx, mk(fmtToolbar.ent(), fmtBtnId++), ah_win95::win95SeparatorStyle(buttonSize - 4).with_debug_name("sep5"));
        
        // === Alignment Buttons ===
        toolbar.currentAlignment = static_cast<int>(doc.buffer.currentAlignment());
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true, toolbar.currentAlignment == 0)
                .with_label("L")
                .with_debug_name("btn_align_left"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Left);
            toolbar.currentAlignment = 0;
            doc.isDirty = true;
        }
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true, toolbar.currentAlignment == 1)
                .with_label("C")
                .with_debug_name("btn_align_center"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Center);
            toolbar.currentAlignment = 1;
            doc.isDirty = true;
        }
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true, toolbar.currentAlignment == 2)
                .with_label("R")
                .with_debug_name("btn_align_right"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Right);
            toolbar.currentAlignment = 2;
            doc.isDirty = true;
        }
        
        if (button(ctx, mk(fmtToolbar.ent(), fmtBtnId++),
            ah_win95::win95ToolbarButtonStyle(buttonSize, true, toolbar.currentAlignment == 3)
                .with_label("J")
                .with_debug_name("btn_align_justify"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Justify);
            toolbar.currentAlignment = 3;
            doc.isDirty = true;
        }
        
        // Close dropdowns when clicking elsewhere - handled via Afterhours focus system
        // When any non-dropdown button is clicked, dropdowns close automatically
        // because we toggle off other dropdowns when opening one
    }
};

}  // namespace ecs
