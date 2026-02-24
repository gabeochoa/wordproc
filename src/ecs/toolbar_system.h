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
#include "editor_entity_cache.h"
#include "../external.h"

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

// Color conversion helper (identity when using same type, needed for backend abstraction)
inline afterhours::Color rlToAh(const afterhours::Color& c) { return c; }

// Helper: create an absolute-positioned toolbar button at (x, y) with given size
// Win95 Office-style: flat by default, raised border on hover, sunken when pressed
inline ComponentConfig absToolbarButton(float x, float y, float size, bool enabled = true, bool pressed = false, bool hovered = false) {
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
        .with_alignment(afterhours::ui::TextAlignment::Center)
        .with_justify_content(JustifyContent::Center)
        .with_align_items(AlignItems::Center)
        .with_cursor(afterhours::ui::CursorType::Pointer);

    // Hover bg: subtle highlight so buttons feel interactive even before bevel kicks in
    if (enabled) {
        config.with_custom_hover_bg(afterhours::Color{220, 220, 220, 255});
    }

    // Win95 Office-style hover: flat when idle, raised on hover, sunken when pressed
    if (pressed) {
        config.with_bevel(afterhours::ui::BevelStyle::Sunken,
                          ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 2.0f);
    } else if (hovered && enabled) {
        config.with_bevel(afterhours::ui::BevelStyle::Raised,
                          ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f);
    }
    // else: flat (no bevel) -- Win95 Office toolbar convention

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
    div(ctx, mk(uiRoot, baseId),
        ComponentConfig{}
            .with_size(ComponentSize{pixels(2), pixels(height)})
            .with_absolute_position()
            .with_translate(x, y)
            .with_custom_background(afterhours::Color{0, 0, 0, 0})
            .with_border_left(ui_imm::win95_colors::BORDER_DARK)
            .with_border_right(ui_imm::win95_colors::BORDER_LIGHT)
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
        .with_alignment(afterhours::ui::TextAlignment::Left)
        .with_text_overflow(afterhours::ui::TextOverflow::Ellipsis)
        .with_justify_content(JustifyContent::Center)
        .with_align_items(AlignItems::FlexStart)
        .with_cursor(afterhours::ui::CursorType::Pointer);
}

// Toolbar Render System - renders the standard toolbar and formatting toolbar
// Uses absolute positioning for each element (translate doesn't propagate to flex children)
struct ToolbarRenderSystem : afterhours::System<UIContext<InputAction>> {
    EditorEntityCache cache_;
    
    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float) override {
        cache_.resolve();
        if (!cache_.resolved()) return;
        
        auto& toolbar = *cache_.toolbar;
        auto& doc = *cache_.doc;
        auto& layout = *cache_.layout;
        
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
        
        // Clear overlay data for this frame
        toolbar.iconOverlays.clear();
        toolbar.dropdownTriangles.clear();
        
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
        
        // Tooltip helper: track which button is hovered and for how long
        // We'll draw tooltips at the end after all buttons
        struct TooltipInfo { float x; float y; std::string text; };
        std::vector<TooltipInfo> tooltips;
        
        // Mouse hover helper: use direct position check instead of ctx.is_hot/was_hot
        // (those compare entity IDs, but we only have MK IDs here)
        auto isMouseOver = [&](float bx, float by) -> bool {
            return afterhours::ui::is_mouse_inside(
                ctx.mouse.pos, {bx, by, buttonSize, buttonSize});
        };
        auto trackTooltip = [&](float bx, float by, const std::string& tip) {
            if (isMouseOver(bx, by)) {
                tooltips.push_back({bx, by + buttonSize + 2.0f, tip});
            }
        };
        
        // Icon overlay helper: record button position + icon type for post-render drawing
        auto trackIcon = [&](float bx, float by, ToolbarIcon icon, bool enabled = true) {
            toolbar.iconOverlays.push_back({bx, by, buttonSize, buttonSize, icon, enabled});
        };

        // === File Operations ===
        int newBtnId = btnId++;
        if (button(ctx, mk(uiRoot, newBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_new"))) {
            doc.buffer.setText("");
            doc.filePath.clear();
            doc.isDirty = false;
        }
        trackIcon(curX, btnY, ToolbarIcon::New);
        trackTooltip(curX, btnY, "New (Ctrl+N)");
        curX += buttonSize + buttonPadding;
        
        int openBtnId = btnId++;
        if (button(ctx, mk(uiRoot, openBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_open"))) {
            // Open document (would trigger file dialog)
        }
        trackIcon(curX, btnY, ToolbarIcon::Open);
        trackTooltip(curX, btnY, "Open (Ctrl+O)");
        curX += buttonSize + buttonPadding;
        
        int saveBtnId = btnId++;
        if (button(ctx, mk(uiRoot, saveBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_save"))) {
            if (!doc.filePath.empty()) {
                auto result = saveDocumentEx(doc.buffer, doc.docSettings, doc.filePath);
                if (result.success) {
                    doc.isDirty = false;
                }
            }
        }
        trackIcon(curX, btnY, ToolbarIcon::Save);
        trackTooltip(curX, btnY, "Save (Ctrl+S)");
        curX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Print
        int printBtnId = btnId++;
        if (button(ctx, mk(uiRoot, printBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_print"))) {
            // Print (not implemented)
        }
        trackIcon(curX, btnY, ToolbarIcon::Print);
        trackTooltip(curX, btnY, "Print (Ctrl+P)");
        curX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Cut/Copy/Paste
        bool hasSelection = doc.buffer.hasSelection();
        
        int cutBtnId = btnId++;
        if (button(ctx, mk(uiRoot, cutBtnId),
            absToolbarButton(curX, btnY, buttonSize, hasSelection, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_cut"))) {
            if (hasSelection) {
                doc.isDirty = true;
            }
        }
        trackIcon(curX, btnY, ToolbarIcon::Cut, hasSelection);
        trackTooltip(curX, btnY, "Cut (Ctrl+X)");
        curX += buttonSize + buttonPadding;
        
        int copyBtnId = btnId++;
        if (button(ctx, mk(uiRoot, copyBtnId),
            absToolbarButton(curX, btnY, buttonSize, hasSelection, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_copy"))) {
            // Copy operation
        }
        trackIcon(curX, btnY, ToolbarIcon::Copy, hasSelection);
        trackTooltip(curX, btnY, "Copy (Ctrl+C)");
        curX += buttonSize + buttonPadding;
        
        int pasteBtnId = btnId++;
        if (button(ctx, mk(uiRoot, pasteBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_paste"))) {
            // Paste operation
        }
        trackIcon(curX, btnY, ToolbarIcon::Paste);
        trackTooltip(curX, btnY, "Paste (Ctrl+V)");
        curX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Undo/Redo
        bool canUndo = doc.buffer.canUndo();
        int undoBtnId = btnId++;
        if (button(ctx, mk(uiRoot, undoBtnId),
            absToolbarButton(curX, btnY, buttonSize, canUndo, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_undo"))) {
            if (canUndo) {
                doc.buffer.undo();
                doc.isDirty = true;
            }
        }
        trackIcon(curX, btnY, ToolbarIcon::Undo, canUndo);
        trackTooltip(curX, btnY, "Undo (Ctrl+Z)");
        curX += buttonSize + buttonPadding;
        
        bool canRedo = doc.buffer.canRedo();
        int redoBtnId = btnId++;
        if (button(ctx, mk(uiRoot, redoBtnId),
            absToolbarButton(curX, btnY, buttonSize, canRedo, false, isMouseOver(curX, btnY)).with_label("").with_debug_name("btn_redo"))) {
            if (canRedo) {
                doc.buffer.redo();
                doc.isDirty = true;
            }
        }
        trackIcon(curX, btnY, ToolbarIcon::Redo, canRedo);
        trackTooltip(curX, btnY, "Redo (Ctrl+Y)");
        
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
        std::string styleLabel = toolbar.currentStyle;
        test_input::register_visible_text(styleLabel);
        
        int styleDropBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, styleDropBtnId),
            absDropdownButton(fmtX, fmtBtnY, styleDropdownWidth, dropdownHeight, toolbar.styleDropdownOpen)
                .with_label(styleLabel)
                .with_debug_name("dropdown_style"))) {
            toolbar.styleDropdownOpen = !toolbar.styleDropdownOpen;
            toolbar.fontDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;
        }
        // Track dropdown triangle position (right edge of button, vertically centered)
        toolbar.dropdownTriangles.push_back({fmtX + styleDropdownWidth - theme::layout::scale(10), fmtBtnY + dropdownHeight / 2.0f});
        
        // Style dropdown list
        if (toolbar.styleDropdownOpen && !toolbar.styles.empty()) {
            div(ctx, mk(uiRoot, 3000),
                ah_win95::win95DropdownListStyle(styleDropdownWidth, static_cast<int>(toolbar.styles.size()))
                    .with_translate(fmtX, formattingBarY + dropdownHeight + buttonPadding)
                    .with_render_layer(50)
                    .with_debug_name("dropdown_style_list"));
            
            for (size_t i = 0; i < toolbar.styles.size(); ++i) {
                bool isSelected = (toolbar.styles[i] == toolbar.currentStyle);
                test_input::register_visible_text(toolbar.styles[i]);
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
                        .with_render_layer(51)
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
        std::string fontLabel = toolbar.currentFont;
        test_input::register_visible_text(fontLabel);
        
        int fontDropBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, fontDropBtnId),
            absDropdownButton(fmtX, fmtBtnY, fontDropdownWidth, dropdownHeight, toolbar.fontDropdownOpen)
                .with_label(fontLabel)
                .with_debug_name("dropdown_font"))) {
            toolbar.fontDropdownOpen = !toolbar.fontDropdownOpen;
            toolbar.styleDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;
        }
        toolbar.dropdownTriangles.push_back({fmtX + fontDropdownWidth - theme::layout::scale(10), fmtBtnY + dropdownHeight / 2.0f});
        
        // Font dropdown list
        if (toolbar.fontDropdownOpen && !toolbar.fonts.empty()) {
            div(ctx, mk(uiRoot, 3100),
                ah_win95::win95DropdownListStyle(fontDropdownWidth, static_cast<int>(toolbar.fonts.size()))
                    .with_translate(fontDropdownX, formattingBarY + dropdownHeight + buttonPadding)
                    .with_render_layer(50)
                    .with_debug_name("dropdown_font_list"));
            
            for (size_t i = 0; i < toolbar.fonts.size(); ++i) {
                bool isSelected = (toolbar.fonts[i] == toolbar.currentFont);
                test_input::register_visible_text(toolbar.fonts[i]);
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
                        .with_render_layer(51)
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
        std::string fontSizeLabel = std::to_string(toolbar.currentFontSize);
        test_input::register_visible_text(fontSizeLabel);
        
        int fontSizeDropBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, fontSizeDropBtnId),
            absDropdownButton(fmtX, fmtBtnY, fontSizeDropdownWidth, dropdownHeight, toolbar.fontSizeDropdownOpen)
                .with_label(fontSizeLabel)
                .with_debug_name("dropdown_fontsize"))) {
            toolbar.fontSizeDropdownOpen = !toolbar.fontSizeDropdownOpen;
            toolbar.styleDropdownOpen = false;
            toolbar.fontDropdownOpen = false;
        }
        toolbar.dropdownTriangles.push_back({fmtX + fontSizeDropdownWidth - theme::layout::scale(10), fmtBtnY + dropdownHeight / 2.0f});
        
        // Font size dropdown list
        if (toolbar.fontSizeDropdownOpen && !toolbar.fontSizes.empty()) {
            div(ctx, mk(uiRoot, 3200),
                ah_win95::win95DropdownListStyle(fontSizeDropdownWidth, static_cast<int>(toolbar.fontSizes.size()))
                    .with_translate(fontSizeDropdownX, formattingBarY + dropdownHeight + buttonPadding)
                    .with_render_layer(50)
                    .with_debug_name("dropdown_fontsize_list"));
            
            for (size_t i = 0; i < toolbar.fontSizes.size(); ++i) {
                bool isSelected = (toolbar.fontSizes[i] == toolbar.currentFontSize);
                std::string sizeStr = std::to_string(toolbar.fontSizes[i]);
                test_input::register_visible_text(sizeStr);
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
                        .with_render_layer(51)
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
        
        // Register formatting button labels for E2E testing
        test_input::register_visible_text("B");
        test_input::register_visible_text("I");
        test_input::register_visible_text("U");

        int boldBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, boldBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.boldActive, isMouseOver(fmtX, fmtBtnY))
                .with_label("B")
                .with_debug_name("btn_bold"))) {
            TextStyle style = doc.buffer.textStyle();
            style.bold = !style.bold;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        trackTooltip(fmtX, fmtBtnY, "Bold (Ctrl+B)");
        fmtX += buttonSize + buttonPadding;
        
        int italicBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, italicBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.italicActive, isMouseOver(fmtX, fmtBtnY))
                .with_label("I")
                .with_debug_name("btn_italic"))) {
            TextStyle style = doc.buffer.textStyle();
            style.italic = !style.italic;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        trackTooltip(fmtX, fmtBtnY, "Italic (Ctrl+I)");
        fmtX += buttonSize + buttonPadding;
        
        int underlineBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, underlineBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.underlineActive, isMouseOver(fmtX, fmtBtnY))
                .with_label("U")
                .with_debug_name("btn_underline"))) {
            TextStyle style = doc.buffer.textStyle();
            style.underline = !style.underline;
            doc.buffer.setTextStyle(style);
            doc.isDirty = true;
        }
        trackTooltip(fmtX, fmtBtnY, "Underline (Ctrl+U)");
        fmtX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, fmtBtnId, fmtX + sepPadding / 2, fmtBtnY, buttonSize - 4);
        fmtBtnId += 2;
        fmtX += sepWidth + sepPadding;
        
        // === Alignment Buttons ===
        toolbar.currentAlignment = static_cast<int>(doc.buffer.currentAlignment());
        
        int alignLBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, alignLBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 0, isMouseOver(fmtX, fmtBtnY))
                .with_label("")
                .with_debug_name("btn_align_left"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Left);
            toolbar.currentAlignment = 0;
            doc.isDirty = true;
        }
        trackIcon(fmtX, fmtBtnY, ToolbarIcon::AlignLeft);
        trackTooltip(fmtX, fmtBtnY, "Align Left (Ctrl+L)");
        fmtX += buttonSize + buttonPadding;
        
        int alignCBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, alignCBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 1, isMouseOver(fmtX, fmtBtnY))
                .with_label("")
                .with_debug_name("btn_align_center"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Center);
            toolbar.currentAlignment = 1;
            doc.isDirty = true;
        }
        trackIcon(fmtX, fmtBtnY, ToolbarIcon::AlignCenter);
        trackTooltip(fmtX, fmtBtnY, "Align Center (Ctrl+E)");
        fmtX += buttonSize + buttonPadding;
        
        int alignRBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, alignRBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 2, isMouseOver(fmtX, fmtBtnY))
                .with_label("")
                .with_debug_name("btn_align_right"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Right);
            toolbar.currentAlignment = 2;
            doc.isDirty = true;
        }
        trackIcon(fmtX, fmtBtnY, ToolbarIcon::AlignRight);
        trackTooltip(fmtX, fmtBtnY, "Align Right (Ctrl+R)");
        fmtX += buttonSize + buttonPadding;
        
        int alignJBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, alignJBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 3, isMouseOver(fmtX, fmtBtnY))
                .with_label("")
                .with_debug_name("btn_align_justify"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Justify);
            toolbar.currentAlignment = 3;
            doc.isDirty = true;
        }
        trackIcon(fmtX, fmtBtnY, ToolbarIcon::AlignJustify);
        trackTooltip(fmtX, fmtBtnY, "Justify (Ctrl+J)");

        // === Draw Tooltips ===
        // Render any tooltip for hovered buttons (drawn last so they appear on top)
        for (const auto& tip : tooltips) {
            float tipFontSize = 11.0f;
            float tipW = theme::MeasureUIText(tip.text.c_str(), static_cast<int>(tipFontSize)) + 8.0f;
            float tipH = tipFontSize + 6.0f;
            // Yellow background tooltip with black text and thin border
            div(ctx, mk(uiRoot, 4000),
                ComponentConfig{}
                    .with_label(tip.text)
                    .with_size(ComponentSize{pixels(tipW), pixels(tipH)})
                    .with_absolute_position()
                    .with_translate(tip.x, tip.y)
                    .with_custom_background(afterhours::Color{255, 255, 225, 255})
                    .with_custom_text_color(afterhours::Color{0, 0, 0, 255})
                    .with_roundness(0.0f)
                    .with_render_layer(20)
                    .with_bevel(afterhours::ui::BevelStyle::Raised,
                                afterhours::Color{0, 0, 0, 255}, afterhours::Color{0, 0, 0, 255}, 1.0f)
                    .with_alignment(afterhours::ui::TextAlignment::Center)
                    .with_debug_name("tooltip"));
        }
        
        // === Close dropdowns on click outside ===
        // If mouse was pressed and no dropdown button is hovered, close all dropdowns
        if (ctx.mouse.just_pressed && (toolbar.styleDropdownOpen || toolbar.fontDropdownOpen || toolbar.fontSizeDropdownOpen)) {
            float mx = ctx.mouse.pos.x;
            float my = ctx.mouse.pos.y;
            // Check if click is inside any dropdown button or dropdown list
            bool insideDropdown = false;
            // Style dropdown button: (buttonPadding, fmtBtnY, styleDropdownWidth, dropdownHeight)
            float sdx = buttonPadding, sdy = fmtBtnY, sdw = theme::layout::scale(120), sdh = theme::layout::scale(22);
            if (mx >= sdx && mx <= sdx + sdw && my >= sdy && my <= sdy + sdh) insideDropdown = true;
            // Font dropdown button
            float fdx = sdx + sdw + buttonPadding * 2, fdy = fmtBtnY, fdw = theme::layout::scale(140), fdh = theme::layout::scale(22);
            if (mx >= fdx && mx <= fdx + fdw && my >= fdy && my <= fdy + fdh) insideDropdown = true;
            // Font size dropdown button
            float fsdx = fdx + fdw + buttonPadding * 2, fsdy = fmtBtnY, fsdw = theme::layout::scale(50), fsdh = theme::layout::scale(22);
            if (mx >= fsdx && mx <= fsdx + fsdw && my >= fsdy && my <= fsdy + fsdh) insideDropdown = true;
            // Dropdown lists (below the formatting bar)
            float listY = formattingBarY + theme::layout::scale(22) + buttonPadding;
            float listBottom = listY + 400; // generous height to cover any open list
            if (my >= listY && my <= listBottom) {
                if (toolbar.styleDropdownOpen && mx >= sdx && mx <= sdx + sdw) insideDropdown = true;
                if (toolbar.fontDropdownOpen && mx >= fdx && mx <= fdx + fdw) insideDropdown = true;
                if (toolbar.fontSizeDropdownOpen && mx >= fsdx && mx <= fsdx + fsdw) insideDropdown = true;
            }
            if (!insideDropdown) {
                toolbar.styleDropdownOpen = false;
                toolbar.fontDropdownOpen = false;
                toolbar.fontSizeDropdownOpen = false;
            }
        }
    }
};

}  // namespace ecs
