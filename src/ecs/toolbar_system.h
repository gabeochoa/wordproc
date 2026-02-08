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

// Helper to convert raylib::Color to afterhours::Color
inline afterhours::Color rlToAh(const raylib::Color& c) { return {c.r, c.g, c.b, c.a}; }

// Helper: create an absolute-positioned toolbar button at (x, y) with given size
// Matches original Win95 DrawToolbarButton: raised 3D border, hover/pressed states
inline ComponentConfig absToolbarButton(float x, float y, float size, bool enabled = true, bool pressed = false) {
    // Match original colors: BUTTON_FACE normal, TOOLBAR_PRESSED_BG pressed
    afterhours::Color bg = pressed ? rlToAh(theme::TOOLBAR_PRESSED_BG) : rlToAh(theme::BUTTON_FACE);
    afterhours::Color textColor = enabled ? rlToAh(theme::BUTTON_TEXT) : rlToAh(theme::MENU_DISABLED);

    auto config = ComponentConfig{}
        .with_size(ComponentSize{pixels(size), pixels(size)})
        .with_absolute_position()
        .with_translate(x, y)
        .with_roundness(0.0f)
        .with_custom_background(bg)
        .with_custom_text_color(textColor)
        // Win95 3D bevel border (raised normal, sunken when pressed)
        .with_bevel(pressed ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 2.0f)
        .with_alignment(afterhours::ui::TextAlignment::Center);

    if (!enabled) {
        config.disabled = true;
    }

    return config;
}

// Helper: draw an etched separator (dark|light line pair) at (x, y)
// Uses two entity IDs: baseId and baseId+1
inline void drawEtchedSeparator(afterhours::ui::UIContext<InputAction>& ctx,
                                 afterhours::Entity& uiRoot, int baseId,
                                 float x, float y, float height) {
    // Dark line (left)
    div(ctx, mk(uiRoot, baseId),
        ComponentConfig{}
            .with_size(ComponentSize{pixels(1), pixels(height)})
            .with_absolute_position()
            .with_translate(x, y)
            .with_custom_background(ui_imm::win95_colors::BORDER_DARK)
            .with_roundness(0.0f));
    // Light line (right)
    div(ctx, mk(uiRoot, baseId + 1),
        ComponentConfig{}
            .with_size(ComponentSize{pixels(1), pixels(height)})
            .with_absolute_position()
            .with_translate(x + 1, y)
            .with_custom_background(ui_imm::win95_colors::BORDER_LIGHT)
            .with_roundness(0.0f));
}

// Helper: create an absolute-positioned dropdown button at (x, y)
inline ComponentConfig absDropdownButton(float x, float y, float width, float height, bool open) {
    afterhours::Color bg = open ? ui_imm::win95_colors::TEXT_AREA : rlToAh(theme::BUTTON_BG);
    return ComponentConfig{}
        .with_size(ComponentSize{pixels(width), pixels(height)})
        .with_absolute_position()
        .with_translate(x, y)
        .with_custom_background(bg)
        .with_custom_text_color(ui_imm::win95_colors::TEXT)
        // Win95 3D bevel: sunken when open, raised when closed
        .with_bevel(open ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
        .with_roundness(0.0f)
        .with_padding(Padding{.top = pixels(2), .right = pixels(4), .bottom = pixels(2), .left = pixels(4)})
        .with_alignment(afterhours::ui::TextAlignment::Left);
}

// Toolbar Render System - renders the standard toolbar and formatting toolbar
// Uses absolute positioning for each element (translate doesn't propagate to flex children)
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
        float sepWidth = theme::layout::scale(theme::layout::TOOLBAR_SEPARATOR_WIDTH);
        float sepPadding = theme::layout::scale(4); // padding around separators
        
        // === Standard Toolbar Background ===
        div(ctx, mk(uiRoot, 1000),
            ComponentConfig{}
                .with_size(ComponentSize{pixels(screenWidth), pixels(toolbarHeight)})
                .with_absolute_position()
                .with_translate(0, toolbarY)
                .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                .with_bevel(afterhours::ui::BevelStyle::Raised,
                            ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
                .with_roundness(0.0f)
                .with_debug_name("std_toolbar"));
        
        // Track X position for horizontal button layout
        float curX = buttonPadding;
        float btnY = toolbarY + buttonPadding;
        int btnId = 1100; // ID range for standard toolbar buttons
        
        // === File Operations ===
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, true).with_label("N").with_debug_name("btn_new"))) {
            doc.buffer.setText("");
            doc.filePath.clear();
            doc.isDirty = false;
        }
        curX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, true).with_label("O").with_debug_name("btn_open"))) {
            // Open document (would trigger file dialog)
        }
        curX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, true).with_label("S").with_debug_name("btn_save"))) {
            if (!doc.filePath.empty()) {
                auto result = saveDocumentEx(doc.buffer, doc.docSettings, doc.filePath);
                if (result.success) {
                    doc.isDirty = false;
                }
            }
        }
        curX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Print
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, true).with_label("P").with_debug_name("btn_print"))) {
            // Print (not implemented)
        }
        curX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Cut/Copy/Paste
        bool hasSelection = doc.buffer.hasSelection();
        
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, hasSelection).with_label("X").with_debug_name("btn_cut"))) {
            if (hasSelection) {
                doc.isDirty = true;
            }
        }
        curX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, hasSelection).with_label("C").with_debug_name("btn_copy"))) {
            // Copy operation
        }
        curX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, true).with_label("V").with_debug_name("btn_paste"))) {
            // Paste operation
        }
        curX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Undo/Redo
        bool canUndo = doc.buffer.canUndo();
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, canUndo).with_label("<").with_debug_name("btn_undo"))) {
            if (canUndo) {
                doc.buffer.undo();
                doc.isDirty = true;
            }
        }
        curX += buttonSize + buttonPadding;
        
        bool canRedo = doc.buffer.canRedo();
        if (button(ctx, mk(uiRoot, btnId++),
            absToolbarButton(curX, btnY, buttonSize, canRedo).with_label(">").with_debug_name("btn_redo"))) {
            if (canRedo) {
                doc.buffer.redo();
                doc.isDirty = true;
            }
        }
        
        // === Formatting Toolbar Background ===
        div(ctx, mk(uiRoot, 2000),
            ComponentConfig{}
                .with_size(ComponentSize{pixels(screenWidth), pixels(formattingBarHeight)})
                .with_absolute_position()
                .with_translate(0, formattingBarY)
                .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                .with_bevel(afterhours::ui::BevelStyle::Raised,
                            ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
                .with_roundness(0.0f)
                .with_debug_name("fmt_toolbar"));
        
        int fmtBtnId = 2100; // ID range for formatting toolbar buttons
        float fmtX = buttonPadding;
        float fmtBtnY = formattingBarY + buttonPadding;
        
        // Dropdown dimensions
        float dropdownHeight = theme::layout::scale(22);
        
        // === Style Dropdown ===
        float styleDropdownWidth = theme::layout::scale(120);
        std::string styleLabel = toolbar.currentStyle + " v";
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absDropdownButton(fmtX, fmtBtnY, styleDropdownWidth, dropdownHeight, toolbar.styleDropdownOpen)
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
                    .with_translate(fmtX, formattingBarY + dropdownHeight + buttonPadding)
                    .with_render_layer(10)
                    .with_debug_name("dropdown_style_list"));
            
            for (size_t i = 0; i < toolbar.styles.size(); ++i) {
                bool isSelected = (toolbar.styles[i] == toolbar.currentStyle);
                // Dropdown items inside the list: use absolute positioning too
                float itemY = formattingBarY + dropdownHeight + buttonPadding + 2.0f + static_cast<float>(i) * theme::layout::scale(20);
                if (button(ctx, mk(uiRoot, 3001 + static_cast<int>(i)),
                    ComponentConfig{}
                        .with_label(toolbar.styles[i])
                        .with_size(ComponentSize{pixels(styleDropdownWidth - 4.0f), pixels(theme::layout::scale(18))})
                        .with_absolute_position()
                        .with_translate(fmtX + 2.0f, itemY)
                        .with_custom_background(isSelected ? ui_imm::win95_colors::HIGHLIGHT : ui_imm::win95_colors::TEXT_AREA)
                        .with_custom_text_color(isSelected ? ui_imm::win95_colors::TEXT_WHITE : ui_imm::win95_colors::TEXT)
                        .with_roundness(0.0f)
                        .with_render_layer(11)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_debug_name("style_item_" + std::to_string(i)))) {
                    toolbar.currentStyle = toolbar.styles[i];
                    toolbar.styleDropdownOpen = false;
                }
            }
        }
        
        fmtX += styleDropdownWidth + buttonPadding * 2;
        
        // === Font Dropdown ===
        float fontDropdownWidth = theme::layout::scale(140);
        float fontDropdownX = fmtX;
        std::string fontLabel = toolbar.currentFont + " v";
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absDropdownButton(fmtX, fmtBtnY, fontDropdownWidth, dropdownHeight, toolbar.fontDropdownOpen)
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
                    .with_render_layer(10)
                    .with_debug_name("dropdown_font_list"));
            
            for (size_t i = 0; i < toolbar.fonts.size(); ++i) {
                bool isSelected = (toolbar.fonts[i] == toolbar.currentFont);
                float itemY = formattingBarY + dropdownHeight + buttonPadding + 2.0f + static_cast<float>(i) * theme::layout::scale(20);
                if (button(ctx, mk(uiRoot, 3101 + static_cast<int>(i)),
                    ComponentConfig{}
                        .with_label(toolbar.fonts[i])
                        .with_size(ComponentSize{pixels(fontDropdownWidth - 4.0f), pixels(theme::layout::scale(18))})
                        .with_absolute_position()
                        .with_translate(fontDropdownX + 2.0f, itemY)
                        .with_custom_background(isSelected ? ui_imm::win95_colors::HIGHLIGHT : ui_imm::win95_colors::TEXT_AREA)
                        .with_custom_text_color(isSelected ? ui_imm::win95_colors::TEXT_WHITE : ui_imm::win95_colors::TEXT)
                        .with_roundness(0.0f)
                        .with_render_layer(11)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
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
        
        fmtX += fontDropdownWidth + buttonPadding * 2;
        
        // === Font Size Dropdown ===
        float fontSizeDropdownWidth = theme::layout::scale(50);
        float fontSizeDropdownX = fmtX;
        std::string fontSizeLabel = std::to_string(toolbar.currentFontSize) + " v";
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absDropdownButton(fmtX, fmtBtnY, fontSizeDropdownWidth, dropdownHeight, toolbar.fontSizeDropdownOpen)
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
                    .with_render_layer(10)
                    .with_debug_name("dropdown_fontsize_list"));
            
            for (size_t i = 0; i < toolbar.fontSizes.size(); ++i) {
                bool isSelected = (toolbar.fontSizes[i] == toolbar.currentFontSize);
                std::string sizeStr = std::to_string(toolbar.fontSizes[i]);
                float itemY = formattingBarY + dropdownHeight + buttonPadding + 2.0f + static_cast<float>(i) * theme::layout::scale(20);
                if (button(ctx, mk(uiRoot, 3201 + static_cast<int>(i)),
                    ComponentConfig{}
                        .with_label(sizeStr)
                        .with_size(ComponentSize{pixels(fontSizeDropdownWidth - 4.0f), pixels(theme::layout::scale(18))})
                        .with_absolute_position()
                        .with_translate(fontSizeDropdownX + 2.0f, itemY)
                        .with_custom_background(isSelected ? ui_imm::win95_colors::HIGHLIGHT : ui_imm::win95_colors::TEXT_AREA)
                        .with_custom_text_color(isSelected ? ui_imm::win95_colors::TEXT_WHITE : ui_imm::win95_colors::TEXT)
                        .with_roundness(0.0f)
                        .with_render_layer(11)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
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
        
        fmtX += fontSizeDropdownWidth + buttonPadding * 2;
        
        // Separator after dropdowns (etched)
        drawEtchedSeparator(ctx, uiRoot, fmtBtnId, fmtX + sepPadding / 2, fmtBtnY, dropdownHeight - 4);
        fmtBtnId += 2;
        fmtX += sepWidth + sepPadding;
        
        // === Formatting Buttons (Bold, Italic, Underline) ===
        TextStyle currentStyle = doc.buffer.textStyle();
        toolbar.boldActive = currentStyle.bold;
        toolbar.italicActive = currentStyle.italic;
        toolbar.underlineActive = currentStyle.underline;
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.boldActive)
                .with_label("B")
                .with_debug_name("btn_bold"))) {
            TextStyle style = doc.buffer.textStyle();
            style.bold = !style.bold;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        fmtX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.italicActive)
                .with_label("I")
                .with_debug_name("btn_italic"))) {
            TextStyle style = doc.buffer.textStyle();
            style.italic = !style.italic;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        fmtX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.underlineActive)
                .with_label("U")
                .with_debug_name("btn_underline"))) {
            TextStyle style = doc.buffer.textStyle();
            style.underline = !style.underline;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        fmtX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, fmtBtnId, fmtX + sepPadding / 2, fmtBtnY, buttonSize - 4);
        fmtBtnId += 2;
        fmtX += sepWidth + sepPadding;
        
        // === Alignment Buttons ===
        toolbar.currentAlignment = static_cast<int>(doc.buffer.currentAlignment());
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 0)
                .with_label("L")
                .with_debug_name("btn_align_left"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Left);
            toolbar.currentAlignment = 0;
            doc.isDirty = true;
        }
        fmtX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 1)
                .with_label("C")
                .with_debug_name("btn_align_center"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Center);
            toolbar.currentAlignment = 1;
            doc.isDirty = true;
        }
        fmtX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 2)
                .with_label("R")
                .with_debug_name("btn_align_right"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Right);
            toolbar.currentAlignment = 2;
            doc.isDirty = true;
        }
        fmtX += buttonSize + buttonPadding;
        
        if (button(ctx, mk(uiRoot, fmtBtnId++),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 3)
                .with_label("J")
                .with_debug_name("btn_align_justify"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Justify);
            toolbar.currentAlignment = 3;
            doc.isDirty = true;
        }
    }
};

}  // namespace ecs
