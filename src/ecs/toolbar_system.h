#pragma once

#include <cmath>

#include "../../vendor/afterhours/src/core/system.h"
#include "../rl.h"
#include "../ui/theme.h"
#include "../ui/tooltip_math.h"
#include "../ui/ah_win95_widgets.h"
#include "../ui/ah_modal_input.h"
#include "../ui/input.h"
#include "../ui/ui_context.h"
#include "components.h"
#include "component_helpers.h"
#include "toolbar_icons.h"
#include "editor_entity_cache.h"
#include "document_commands.h"
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
inline ComponentConfig absToolbarButton(float x, float y, float size, bool enabled = true, bool pressed = false, bool hovered = false,
                                        ToolbarIcon icon = ToolbarIcon::None) {
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

    // Hover bg, only when the caller says hovered (the caller derives
    // `hovered` from the same rect the framework uses, and forces it
    // false while a menu is open — so a modal menu owns all hover and
    // the framework never paints a toolbar hover bg under it).
    if (enabled && hovered) {
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

    // The icon is drawn by afterhours during the render pass (on_draw_fg), so it
    // layers with everything else and no longer has to be skipped whenever a menu
    // or dropdown is open (feature requests #1 and #9).
    if (icon != ToolbarIcon::None) {
        config.with_on_draw_fg([icon, enabled](RectangleType r) {
            drawToolbarIcon(icon, r.x, r.y, std::min(r.width, r.height), enabled);
        });
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
        .with_cursor(afterhours::ui::CursorType::Pointer)
        // The ▼ is drawn rather than typed: the UI font has no U+25BC glyph, and
        // the " v" suffix it used to wear was a placeholder. on_draw_fg puts it in
        // the render pass, so it no longer needs the overlay's popup-overlap dance
        // (feature request #4).
        .with_on_draw_fg([](RectangleType r) {
            drawDropdownTriangle(r.x + r.width - theme::layout::scale(10),
                                 r.y + r.height / 2.0f, theme::BUTTON_TEXT);
        });
}

// Toolbar Render System - renders the standard toolbar and formatting toolbar
// Uses absolute positioning for each element (translate doesn't propagate to flex children)
struct ToolbarRenderSystem : afterhours::System<UIContext<InputAction>> {
    EditorEntityCache cache_;
    // Tooltip hover-delay state: which button is hovered and for how long.
    // A tooltip shows only after ~0.5s of steady hover on the same button.
    float tooltipHoverTime_ = 0.0f;
    float tooltipHoverX_ = -1.0f;
    float tooltipHoverY_ = -1.0f;
    std::string tooltipHoverText_;

    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float dt) override {
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
        
        // === Print Preview toolbar ===
        // ONE compact row (toolbarHeight, ~28px like the other
        // toolbars), replacing BOTH toolbars in preview mode: Print,
        // Prev/Next Page, "Page X of Y", Zoom dropdown, Close Print
        // Preview. The old bar was toolbarHeight +
        // formattingBarHeight (~56px, double height) because it
        // replaced both toolbars' height; the Formatting toolbar and
        // the ruler are gone in preview (component_helpers reclaims
        // their height), so the bar is one row now. Editing is
        // disabled (TextInputSystem gates on previewMode); these
        // buttons plus scroll and zoom are the only live preview
        // controls. Returns early: no standard/formatting toolbar.
        if (layout.previewMode) {
            float previewBarHeight = toolbarHeight;
            div(ctx, mk(uiRoot, 1000),
                ComponentConfig{}
                    .with_size(ComponentSize{pixels(screenWidth), pixels(previewBarHeight)})
                    .with_absolute_position()
                    .with_translate(0, toolbarY)
                    .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                    .with_bevel(afterhours::ui::BevelStyle::Raised,
                                ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
                    .with_roundness(0.0f)
                    .with_debug_name("preview_toolbar"));

            float pvX = buttonPadding;
            float pvY = toolbarY + buttonPadding;
            int pvId = 1100;
            // Beveled Win95 button (same visual language as
            // win95ToolbarButtonStyle: raised bevel, BUTTON_FACE),
            // absolute-positioned like the Standard toolbar buttons.
            auto previewButton = [&](float x, float w, const std::string& label,
                                     const std::string& debug, bool enabled,
                                     ToolbarIcon icon = ToolbarIcon::None) {
                auto cfg = ComponentConfig{}
                    .with_size(ComponentSize{pixels(w), pixels(buttonSize)})
                    .with_absolute_position()
                    .with_translate(x, pvY)
                    .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                    .with_custom_text_color(enabled ? ui_imm::win95_colors::TEXT
                                                    : ui_imm::win95_colors::TEXT_DISABLED)
                    .with_bevel(afterhours::ui::BevelStyle::Raised,
                                ui_imm::win95_colors::BORDER_LIGHT,
                                ui_imm::win95_colors::BORDER_DARK, 1.0f)
                    .with_roundness(0.0f)
                    .with_alignment(afterhours::ui::TextAlignment::Center)
                    .with_justify_content(JustifyContent::Center)
                    .with_align_items(AlignItems::Center)
                    .with_cursor(afterhours::ui::CursorType::Pointer)
                    .with_label(label)
                    .with_debug_name(debug);
                if (!enabled) cfg.disabled = true;
                if (icon != ToolbarIcon::None) {
                    cfg.with_on_draw_fg([icon, enabled](RectangleType r) {
                        drawToolbarIcon(icon, r.x, r.y,
                                        std::min(r.width, r.height), enabled);
                    });
                }
                return button(ctx, mk(uiRoot, pvId++), cfg);
            };

            // Print (same action as File > Print... / Ctrl+P)
            if (previewButton(pvX, buttonSize, "", "btn_preview_print", true,
                              ToolbarIcon::Print)) {
                if (cache_.dialogs) cmd::requestPrint(doc, *cache_.dialogs);
            }
            pvX += buttonSize + buttonPadding;

            drawEtchedSeparator(ctx, uiRoot, pvId, pvX + sepPadding / 2, pvY, buttonSize - 4);
            pvId += 2;
            pvX += sepWidth + sepPadding;

            // Prev / Next Page (disabled at the first / last page):
            // short labels, not "Prev Page" / "Next Page".
            if (previewButton(pvX, theme::layout::scale(48), "Prev",
                              "btn_preview_prev", layout.previewPage > 0)) {
                layout::previewPrevPage(layout);
            }
            pvX += theme::layout::scale(48) + buttonPadding;
            if (previewButton(pvX, theme::layout::scale(48), "Next",
                              "btn_preview_next",
                              layout.previewPage < layout.previewPageCount - 1)) {
                layout::previewNextPage(layout);
            }
            pvX += theme::layout::scale(48) + buttonPadding;

            drawEtchedSeparator(ctx, uiRoot, pvId, pvX + sepPadding / 2, pvY, buttonSize - 4);
            pvId += 2;
            pvX += sepWidth + sepPadding;

            // "Page X of Y" (page count is the scroll_math estimate)
            std::string pageLabel = "Page " +
                std::to_string(layout.previewPage + 1) + " of " +
                std::to_string(layout.previewPageCount);
            test_input::register_visible_text(pageLabel);
            div(ctx, mk(uiRoot, pvId++),
                ComponentConfig{}
                    .with_label(pageLabel)
                    .with_size(ComponentSize{pixels(theme::layout::scale(110)), pixels(buttonSize)})
                    .with_absolute_position()
                    .with_translate(pvX, pvY)
                    .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                    .with_custom_text_color(ui_imm::win95_colors::TEXT)
                    .with_alignment(afterhours::ui::TextAlignment::Center)
                    .with_roundness(0.0f)
                    .with_debug_name("preview_page_label"));
            pvX += theme::layout::scale(110) + buttonPadding;

            drawEtchedSeparator(ctx, uiRoot, pvId, pvX + sepPadding / 2, pvY, buttonSize - 4);
            pvId += 2;
            pvX += sepWidth + sepPadding;

            // Zoom dropdown: shows the EFFECTIVE zoom % (fit *
            // user zoom); choices are effective targets and set the
            // user zoom so fit * user == target, then recompute the
            // preview pagination estimate (zoom changes lines per page).
            float zoomBtnX = pvX;
            float zoomBtnW = theme::layout::scale(76);
            std::string zoomLabel =
                std::to_string(static_cast<int>(std::lround(
                    layout::effectiveZoom(layout) * 100.0f))) + "%";
            test_input::register_visible_text(zoomLabel);
            bool zoomOpen = cache_.toolbar &&
                            cache_.toolbar->previewZoomDropdownOpen;
            if (button(ctx, mk(uiRoot, pvId++),
                absDropdownButton(zoomBtnX, pvY, zoomBtnW, buttonSize, zoomOpen)
                    .with_label(zoomLabel)
                    .with_debug_name("preview_zoom_dropdown"))) {
                if (cache_.toolbar) {
                    cache_.toolbar->previewZoomDropdownOpen = !zoomOpen;
                }
            }
            pvX += zoomBtnW + buttonPadding;

            if (zoomOpen) {
                const int zoomChoices[] = {50, 75, 100, 150, 200};
                float listY = pvY + buttonSize + 2.0f;
                div(ctx, mk(uiRoot, pvId++),
                    ah_win95::win95DropdownListStyle(zoomBtnW, 5)
                        .with_translate(zoomBtnX, listY)
                        .with_render_layer(50)
                        .with_debug_name("preview_zoom_list"));
                for (int i = 0; i < 5; ++i) {
                    int pct = zoomChoices[i];
                    bool selected = static_cast<int>(std::lround(
                        layout::effectiveZoom(layout) * 100.0f)) == pct;
                    float itemY = listY + 2.0f + static_cast<float>(i) *
                                                 theme::layout::scale(20);
                    if (button(ctx, mk(uiRoot, pvId++),
                        ComponentConfig{}
                            .with_label(std::to_string(pct) + "%")
                            .with_size(ComponentSize{pixels(zoomBtnW - 4.0f),
                                                     pixels(theme::layout::scale(18))})
                            .with_absolute_position()
                            .with_translate(zoomBtnX + 2.0f, itemY)
                            .with_custom_background(selected
                                ? ui_imm::win95_colors::HIGHLIGHT
                                : ui_imm::win95_colors::TEXT_AREA)
                            .with_custom_text_color(selected
                                ? ui_imm::win95_colors::TEXT_WHITE
                                : ui_imm::win95_colors::TEXT)
                            .with_roundness(0.0f)
                            .with_render_layer(51)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_debug_name("preview_zoom_item_" +
                                            std::to_string(pct)))) {
                        layout::setEffectiveZoom(
                            layout, static_cast<float>(pct) / 100.0f);
                        if (cache_.toolbar) {
                            cache_.toolbar->previewZoomDropdownOpen = false;
                        }
                        // The dropdown item dies now; a stale focus
                        // id on it recycles to the File header (see
                        // MenuUISystem dispatch).
                        ctx.set_focus(ctx.ROOT);
                        layout::updateLayout(layout, layout.screenWidth,
                                             layout.screenHeight);
                        layout::refreshPreviewPagination(
                            layout, doc.buffer.lineCount(),
                            doc.buffer.textStyle().fontSize);
                    }
                }
            }

            drawEtchedSeparator(ctx, uiRoot, pvId, pvX + sepPadding / 2, pvY, buttonSize - 4);
            pvId += 2;
            pvX += sepWidth + sepPadding;

            // Close (Esc does the same; mode + pre-preview zoom restored)
            if (previewButton(pvX, theme::layout::scale(64),
                              "Close", "btn_preview_close", true)) {
                cmd::closePrintPreview(layout);
                // The preview toolbar dies with preview; clear
                // focus so its Close button's stale id cannot
                // recycle onto the File header.
                ctx.set_focus(ctx.ROOT);
            }

            // Close the zoom dropdown on a click outside it (same
            // rule as the formatting-bar dropdowns).
            if (zoomOpen && cache_.toolbar && ctx.mouse.just_pressed) {
                float mx = ctx.mouse.pos.x;
                float my = ctx.mouse.pos.y;
                float listY = pvY + buttonSize + 2.0f;
                float listH = 5.0f * theme::layout::scale(20) +
                              theme::layout::scale(4);
                bool insideButton = mx >= zoomBtnX && mx <= zoomBtnX + zoomBtnW &&
                                    my >= pvY && my <= pvY + buttonSize;
                bool insideList = mx >= zoomBtnX && mx <= zoomBtnX + zoomBtnW &&
                                  my >= listY && my <= listY + listH;
                if (!insideButton && !insideList) {
                    cache_.toolbar->previewZoomDropdownOpen = false;
                }
            }
            return;
        }

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
        
        // A menu is open: the menu is modal. No toolbar hover, bevel, or
        // tooltip until it closes (clicks are already gated by the
        // menu_dropdown input gate on render_layer).
        bool menuOpen = false;
        if (cache_.menu) {
            for (const auto& m : cache_.menu->menus) {
                if (m.open) { menuOpen = true; break; }
            }
            if (cache_.menu->activeMenuIndex >= 0) menuOpen = true;
        }

        // Mouse hover helper: use direct position check instead of ctx.is_hot/was_hot
        // (those compare entity IDs, but we only have MK IDs here)
        auto isMouseOver = [&](float bx, float by) -> bool {
            if (menuOpen) return false;
            return afterhours::ui::is_mouse_inside(
                ctx.mouse.pos, {bx, by, buttonSize, buttonSize});
        };
        auto trackTooltip = [&](float bx, float by, const std::string& tip) {
            if (isMouseOver(bx, by)) {
                tooltips.push_back({bx, by + buttonSize + 2.0f, tip});
            }
        };
        
        // === File Operations ===
        int newBtnId = btnId++;
        if (button(ctx, mk(uiRoot, newBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::New).with_label("").with_debug_name("btn_new"))) {
            cmd::newDocument(doc);
        }
        trackTooltip(curX, btnY, "New (Ctrl+N)");
        curX += buttonSize + buttonPadding;
        
        int openBtnId = btnId++;
        if (button(ctx, mk(uiRoot, openBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::Open).with_label("").with_debug_name("btn_open"))) {
            // Open document (would trigger file dialog)
        }
        trackTooltip(curX, btnY, "Open (Ctrl+O)");
        curX += buttonSize + buttonPadding;
        
        int saveBtnId = btnId++;
        if (button(ctx, mk(uiRoot, saveBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::Save).with_label("").with_debug_name("btn_save"))) {
            cmd::saveDocument(doc, layout, *cache_.menu);
        }
        trackTooltip(curX, btnY, "Save (Ctrl+S)");
        curX += buttonSize + buttonPadding;
        
        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Print (File > Print... / Ctrl+P)
        int printBtnId = btnId++;
        if (button(ctx, mk(uiRoot, printBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::Print).with_label("").with_debug_name("btn_print"))) {
            if (cache_.dialogs) cmd::requestPrint(doc, *cache_.dialogs);
        }
        trackTooltip(curX, btnY, "Print (Ctrl+P)");
        curX += buttonSize + buttonPadding;

        // Print Preview (File > Print Preview) — Word 95 Standard order:
        // Print, Print Preview, Spelling, then Cut/Copy/Paste.
        int previewBtnId = btnId++;
        if (button(ctx, mk(uiRoot, previewBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::PrintPreview).with_label("").with_debug_name("btn_print_preview"))) {
            cmd::openPrintPreview(doc, layout);
            // Entering preview replaces this toolbar; clear focus
            // so this button's stale id cannot recycle onto File.
            ctx.set_focus(ctx.ROOT);
        }
        trackTooltip(curX, btnY, "Print Preview");
        curX += buttonSize + buttonPadding;

        // Spelling (Tools > Spelling..., F7)
        int spellingBtnId = btnId++;
        if (button(ctx, mk(uiRoot, spellingBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::Spelling).with_label("").with_debug_name("btn_spelling"))) {
            if (cache_.dialogs) cmd::openSpellingDialog(doc, *cache_.dialogs);
        }
        trackTooltip(curX, btnY, "Spelling (F7)");
        curX += buttonSize + buttonPadding;

        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;
        
        // Cut/Copy/Paste
        bool hasSelection = doc.buffer.hasSelection();
        
        int cutBtnId = btnId++;
        if (button(ctx, mk(uiRoot, cutBtnId),
            absToolbarButton(curX, btnY, buttonSize, hasSelection, false, isMouseOver(curX, btnY), ToolbarIcon::Cut).with_label("").with_debug_name("btn_cut"))) {
            if (hasSelection) {
                doc.isDirty = true;
            }
        }
        trackTooltip(curX, btnY, "Cut (Ctrl+X)");
        curX += buttonSize + buttonPadding;
        
        int copyBtnId = btnId++;
        if (button(ctx, mk(uiRoot, copyBtnId),
            absToolbarButton(curX, btnY, buttonSize, hasSelection, false, isMouseOver(curX, btnY), ToolbarIcon::Copy).with_label("").with_debug_name("btn_copy"))) {
            // Copy operation
        }
        trackTooltip(curX, btnY, "Copy (Ctrl+C)");
        curX += buttonSize + buttonPadding;
        
        int pasteBtnId = btnId++;
        if (button(ctx, mk(uiRoot, pasteBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::Paste).with_label("").with_debug_name("btn_paste"))) {
            // Paste operation
        }
        trackTooltip(curX, btnY, "Paste (Ctrl+V)");
        curX += buttonSize + buttonPadding;

        // Format Painter: copy the current style and arm; the next
        // click in the text applies it and disarms (MouseSelection-
        // System). Sunken while armed. Word 95 Standard order:
        // Cut/Copy/Paste, Format Painter, then Undo/Redo.
        int painterBtnId = btnId++;
        if (button(ctx, mk(uiRoot, painterBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, toolbar.formatPainter.armed, isMouseOver(curX, btnY), ToolbarIcon::FormatPainter).with_label("").with_debug_name("btn_format_painter"))) {
            cmd::toggleFormatPainter(doc, toolbar);
        }
        trackTooltip(curX, btnY, "Format Painter");
        curX += buttonSize + buttonPadding;

        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;

        // Undo/Redo
        bool canUndo = doc.buffer.canUndo();
        int undoBtnId = btnId++;
        if (button(ctx, mk(uiRoot, undoBtnId),
            absToolbarButton(curX, btnY, buttonSize, canUndo, false, isMouseOver(curX, btnY), ToolbarIcon::Undo).with_label("").with_debug_name("btn_undo"))) {
            cmd::undo(doc);
        }
        trackTooltip(curX, btnY, "Undo (Ctrl+Z)");
        curX += buttonSize + buttonPadding;

        bool canRedo = doc.buffer.canRedo();
        int redoBtnId = btnId++;
        if (button(ctx, mk(uiRoot, redoBtnId),
            absToolbarButton(curX, btnY, buttonSize, canRedo, false, isMouseOver(curX, btnY), ToolbarIcon::Redo).with_label("").with_debug_name("btn_redo"))) {
            cmd::redo(doc);
        }
        trackTooltip(curX, btnY, "Redo (Ctrl+Y)");
        curX += buttonSize + buttonPadding;

        // Separator (etched)
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;

        // Show/Hide ¶: toggles paragraph marks in the text pass
        // (¶ glyph at each line end). Sunken while on.
        int pilcrowBtnId = btnId++;
        if (button(ctx, mk(uiRoot, pilcrowBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, layout.showParagraphMarks, isMouseOver(curX, btnY), ToolbarIcon::ShowParagraph).with_label("").with_debug_name("btn_show_paragraph"))) {
            layout.showParagraphMarks = !layout.showParagraphMarks;
        }
        trackTooltip(curX, btnY, "Show/Hide Paragraph Marks");
        curX += buttonSize + buttonPadding;

        // Zoom control immediately after the last Standard group
        // (¶), NOT pinned to the window's right edge: dropdown
        // showing the current zoom %, wired to zoomLevel.
        float zoomBtnW = theme::layout::scale(76);
        float zoomBtnX = curX;
        float zoomBtnY = btnY;
        curX += zoomBtnW + buttonPadding;
        std::string zoomLabel =
            std::to_string(static_cast<int>(std::lround(
                layout.zoomLevel * 100.0f))) + "%";
        test_input::register_visible_text(zoomLabel);
        if (button(ctx, mk(uiRoot, btnId++),
            absDropdownButton(zoomBtnX, zoomBtnY, zoomBtnW, theme::layout::scale(22), toolbar.zoomDropdownOpen)
                .with_label(zoomLabel)
                .with_debug_name("dropdown_zoom"))) {
            toolbar.zoomDropdownOpen = !toolbar.zoomDropdownOpen;
            toolbar.styleDropdownOpen = false;
            toolbar.fontDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;
        }
        if (toolbar.zoomDropdownOpen) {
            const int zoomChoices[] = {50, 75, 100, 125, 150, 200};
            float zoomListY = zoomBtnY + buttonSize + 2.0f;
            div(ctx, mk(uiRoot, 3300),
                ah_win95::win95DropdownListStyle(zoomBtnW, 6)
                    .with_translate(zoomBtnX, zoomListY)
                    .with_render_layer(50)
                    .with_debug_name("dropdown_zoom_list"));
            for (int i = 0; i < 6; ++i) {
                int pct = zoomChoices[i];
                bool selected = static_cast<int>(std::lround(
                    layout.zoomLevel * 100.0f)) == pct;
                float itemY = zoomListY + 2.0f + static_cast<float>(i) *
                                                 theme::layout::scale(20);
                if (button(ctx, mk(uiRoot, 3301 + i),
                    ComponentConfig{}
                        .with_label(std::to_string(pct) + "%")
                        .with_size(ComponentSize{pixels(zoomBtnW - 4.0f), pixels(theme::layout::scale(18))})
                        .with_absolute_position()
                        .with_translate(zoomBtnX + 2.0f, itemY)
                        .with_custom_background(selected
                            ? ui_imm::win95_colors::HIGHLIGHT
                            : ui_imm::win95_colors::TEXT_AREA)
                        .with_custom_text_color(selected
                            ? ui_imm::win95_colors::TEXT_WHITE
                            : ui_imm::win95_colors::TEXT)
                        .with_roundness(0.0f)
                        .with_render_layer(51)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_debug_name("zoom_item_" + std::to_string(pct)))) {
                    layout.zoomLevel = static_cast<float>(pct) / 100.0f;
                    toolbar.zoomDropdownOpen = false;
                    // Dropdown item dies; stale focus recycles to File.
                    ctx.set_focus(ctx.ROOT);
                }
            }
        }

        // Separator (etched) before the Insert group.
        drawEtchedSeparator(ctx, uiRoot, btnId, curX + sepPadding / 2, btnY, buttonSize - 4);
        btnId += 2;
        curX += sepWidth + sepPadding;

        // === Insert group (Word order: Hyperlink, Image, Table) ===
        // All three are wired, no dead buttons: Hyperlink and Image
        // run the same cmd:: commands as Insert > Hyperlink... /
        // Image...; Table opens the Table menu dropdown via the
        // existing menu-open mechanism (MenuUISystem renders the
        // dropdown for whichever menu has open=true).
        int hyperlinkBtnId = btnId++;
        if (button(ctx, mk(uiRoot, hyperlinkBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::Hyperlink).with_label("").with_debug_name("btn_insert_hyperlink"))) {
            cmd::insertHyperlink(doc);
        }
        trackTooltip(curX, btnY, "Insert Hyperlink (Ctrl+K)");
        curX += buttonSize + buttonPadding;

        int imageBtnId = btnId++;
        if (button(ctx, mk(uiRoot, imageBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::Image).with_label("").with_debug_name("btn_insert_image"))) {
            cmd::insertImagePlaceholder(doc);
        }
        trackTooltip(curX, btnY, "Insert Image");
        curX += buttonSize + buttonPadding;

        int tableBtnId = btnId++;
        if (button(ctx, mk(uiRoot, tableBtnId),
            absToolbarButton(curX, btnY, buttonSize, true, false, isMouseOver(curX, btnY), ToolbarIcon::InsertTable).with_label("").with_debug_name("btn_insert_table"))) {
            // Toggle the Table menu dropdown (same mechanism as
            // clicking the Table header / E2E menu_open Table).
            if (cache_.menu) {
                int tableIdx = -1;
                bool wasOpen = false;
                for (std::size_t m = 0; m < cache_.menu->menus.size(); ++m) {
                    if (cache_.menu->menus[m].label == "Table") {
                        tableIdx = static_cast<int>(m);
                        wasOpen = cache_.menu->menus[m].open;
                    }
                    cache_.menu->menus[m].open = false;
                }
                if (tableIdx >= 0 && !wasOpen) {
                    cache_.menu->menus[static_cast<std::size_t>(tableIdx)].open = true;
                    cache_.menu->activeMenuIndex = tableIdx;
                } else {
                    cache_.menu->activeMenuIndex = -1;
                }
            }
        }
        trackTooltip(curX, btnY, "Insert Table");
        curX += buttonSize + buttonPadding;

        // === Outlining Toolbar (replaces the Formatting toolbar) ===
        // In Outline mode the Formatting toolbar's controls (style /
        // font / size / B / I / U / alignment) do not apply to an
        // outline editor, so this bar occupies the same chrome slot
        // (formattingBarY, formattingBarHeight) with the Outlining
        // controls: Promote, Demote, Move Up, Move Down,
        // Expand/Collapse, and a Show Level dropdown. The
        // Formatting toolbar is not rendered in Outline mode (the
        // else below), and its dropdowns are closed when this bar
        // draws, so no formatting dropdown can leak into Outline.
        if (layout.showOutline) {
            toolbar.styleDropdownOpen = false;
            toolbar.fontDropdownOpen = false;
            toolbar.fontSizeDropdownOpen = false;

            div(ctx, mk(uiRoot, 2000),
                ComponentConfig{}
                    .with_size(ComponentSize{pixels(screenWidth), pixels(formattingBarHeight)})
                    .with_absolute_position()
                    .with_translate(0, formattingBarY)
                    .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                    .with_bevel(afterhours::ui::BevelStyle::Raised,
                                ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
                    .with_roundness(0.0f)
                    .with_debug_name("outline_toolbar"));

            float outlineX = buttonPadding;
            float outlineBtnY = formattingBarY + buttonPadding;
            float outlineBtnH = theme::layout::scale(22);
            int outlineBtnId = 2300;
            auto outlineButton = [&](float x, float w, const std::string& label,
                                     const std::string& debug) {
                return button(ctx, mk(uiRoot, outlineBtnId++),
                    ComponentConfig{}
                        .with_size(ComponentSize{pixels(w), pixels(outlineBtnH)})
                        .with_absolute_position()
                        .with_translate(x, outlineBtnY)
                        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_bevel(afterhours::ui::BevelStyle::Raised,
                                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
                        .with_roundness(0.0f)
                        .with_alignment(afterhours::ui::TextAlignment::Center)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_cursor(afterhours::ui::CursorType::Pointer)
                        .with_label(label)
                        .with_debug_name(debug));
            };
            auto outlineSeparator = [&](float x) {
                drawEtchedSeparator(ctx, uiRoot, outlineBtnId, x, outlineBtnY, outlineBtnH - 4);
                outlineBtnId += 2;
            };
            auto outlineFlash = [&](const std::string& msg) {
                if (cache_.status) {
                    status::flash(*cache_.status, msg,
                                  afterhours::graphics::get_time());
                }
            };

            test_input::register_visible_text("Outlining");
            test_input::register_visible_text("Promote <=");
            if (outlineButton(outlineX, theme::layout::scale(95), "Promote <=",
                              "outline_promote")) {
                cmd::promoteOutlineHeading(doc);
            }
            trackTooltip(outlineX, outlineBtnY, "Promote (demote the other way)");
            outlineX += theme::layout::scale(95) + buttonPadding;

            test_input::register_visible_text("Demote =>");
            if (outlineButton(outlineX, theme::layout::scale(95), "Demote =>",
                              "outline_demote")) {
                cmd::demoteOutlineHeading(doc);
            }
            trackTooltip(outlineX, outlineBtnY, "Demote (promote the other way)");
            outlineX += theme::layout::scale(95) + buttonPadding;

            outlineSeparator(outlineX + sepPadding / 2);
            outlineX += sepWidth + sepPadding;

            test_input::register_visible_text("Move Up");
            if (outlineButton(outlineX, theme::layout::scale(78), "Move Up",
                              "outline_move_up")) {
                cmd::moveOutlineBlock(doc, layout, true);
            }
            trackTooltip(outlineX, outlineBtnY, "Move Up (swap with previous sibling block)");
            outlineX += theme::layout::scale(78) + buttonPadding;

            test_input::register_visible_text("Move Down");
            if (outlineButton(outlineX, theme::layout::scale(88), "Move Down",
                              "outline_move_down")) {
                cmd::moveOutlineBlock(doc, layout, false);
            }
            trackTooltip(outlineX, outlineBtnY, "Move Down (swap with next sibling block)");
            outlineX += theme::layout::scale(88) + buttonPadding;

            outlineSeparator(outlineX + sepPadding / 2);
            outlineX += sepWidth + sepPadding;

            // Expand / Collapse for the heading at the caret:
            // label follows the heading's state; the fixed
            // "Expand/Collapse" text is registered for E2E too.
            bool caretCollapsed = cmd::isOutlineCollapsedAtCaret(doc, layout);
            test_input::register_visible_text("Expand/Collapse");
            test_input::register_visible_text(caretCollapsed ? "Expand" : "Collapse");
            if (outlineButton(outlineX, theme::layout::scale(88),
                              caretCollapsed ? "Expand" : "Collapse",
                              "outline_expand_collapse")) {
                bool wasCollapsed = caretCollapsed;
                if (cmd::toggleOutlineCollapse(doc, layout)) {
                    outlineFlash(wasCollapsed ? "Expanded" : "Collapsed");
                }
            }
            trackTooltip(outlineX, outlineBtnY, "Expand/Collapse heading at caret");
            outlineX += theme::layout::scale(88) + buttonPadding;

            outlineSeparator(outlineX + sepPadding / 2);
            outlineX += sepWidth + sepPadding;

            // Show Level dropdown: Level 1..6 or All Levels.
            std::string showLevelLabel =
                layout.outlineShowLevel == 0
                    ? "Show Level: All Levels"
                    : "Show Level: Level " +
                          std::to_string(layout.outlineShowLevel);
            test_input::register_visible_text("Show Level:");
            test_input::register_visible_text(showLevelLabel);
            float showLevelBtnX = outlineX;
            float showLevelBtnW = theme::layout::scale(190);
            bool showLevelOpen = toolbar.outlineShowLevelDropdownOpen;
            if (button(ctx, mk(uiRoot, outlineBtnId++),
                absDropdownButton(showLevelBtnX, outlineBtnY, showLevelBtnW,
                                  outlineBtnH, showLevelOpen)
                    .with_label(showLevelLabel)
                    .with_debug_name("outline_show_level"))) {
                toolbar.outlineShowLevelDropdownOpen = !showLevelOpen;
            }
            outlineX += showLevelBtnW + buttonPadding;

            if (toolbar.outlineShowLevelDropdownOpen) {
                float listY = formattingBarY + outlineBtnH + buttonPadding;
                div(ctx, mk(uiRoot, 2400),
                    ah_win95::win95DropdownListStyle(showLevelBtnW, 7)
                        .with_translate(showLevelBtnX, listY)
                        .with_render_layer(50)
                        .with_debug_name("outline_show_level_list"));
                for (int choice = 0; choice < 7; ++choice) {
                    int level = choice == 6 ? 0 : choice + 1;
                    std::string itemLabel =
                        level == 0 ? "All Levels"
                                   : "Level " + std::to_string(level);
                    bool selected = layout.outlineShowLevel == level;
                    test_input::register_visible_text(itemLabel);
                    float itemY = listY + 2.0f +
                                  static_cast<float>(choice) *
                                      theme::layout::scale(20);
                    if (button(ctx, mk(uiRoot, 2401 + choice),
                        ComponentConfig{}
                            .with_label(itemLabel)
                            .with_size(ComponentSize{pixels(showLevelBtnW - 4.0f),
                                                     pixels(theme::layout::scale(18))})
                            .with_absolute_position()
                            .with_translate(showLevelBtnX + 2.0f, itemY)
                            .with_custom_background(selected ? ui_imm::win95_colors::HIGHLIGHT : ui_imm::win95_colors::TEXT_AREA)
                            .with_custom_text_color(selected ? ui_imm::win95_colors::TEXT_WHITE : ui_imm::win95_colors::TEXT)
                            .with_roundness(0.0f)
                            .with_render_layer(51)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_debug_name("outline_show_level_" +
                                            (level == 0 ? std::string("all")
                                                        : std::to_string(level))))) {
                        layout.outlineShowLevel = level;
                        toolbar.outlineShowLevelDropdownOpen = false;
                        ctx.set_focus(ctx.ROOT);
                        outlineFlash(level == 0
                                         ? "Show All Levels"
                                         : "Show Level " + std::to_string(level));
                    }
                }
                // Click outside the button / list closes the
                // dropdown (same pattern as the formatting
                // dropdown close below).
                if (ctx.mouse.just_pressed) {
                    float mx = ctx.mouse.pos.x;
                    float my = ctx.mouse.pos.y;
                    bool inside = mx >= showLevelBtnX &&
                                  mx <= showLevelBtnX + showLevelBtnW &&
                                  my >= outlineBtnY &&
                                  my <= outlineBtnY + outlineBtnH;
                    float listBottom = listY + 7.0f * theme::layout::scale(20) +
                                       theme::layout::scale(4);
                    if (mx >= showLevelBtnX && mx <= showLevelBtnX + showLevelBtnW &&
                        my >= listY && my <= listBottom) {
                        inside = true;
                    }
                    if (!inside) {
                        toolbar.outlineShowLevelDropdownOpen = false;
                    }
                }
            }
        }

        // === Formatting Toolbar Background ===
        if (!layout.showOutline) {
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
                    ctx.set_focus(ctx.ROOT);
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
                    ctx.set_focus(ctx.ROOT);
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
                    ctx.set_focus(ctx.ROOT);
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
            cmd::toggleBold(doc);
        }
        trackTooltip(fmtX, fmtBtnY, "Bold (Ctrl+B)");
        fmtX += buttonSize + buttonPadding;
        
        int italicBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, italicBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.italicActive, isMouseOver(fmtX, fmtBtnY))
                .with_label("I")
                .with_debug_name("btn_italic"))) {
            cmd::toggleItalic(doc);
        }
        trackTooltip(fmtX, fmtBtnY, "Italic (Ctrl+I)");
        fmtX += buttonSize + buttonPadding;
        
        int underlineBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, underlineBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.underlineActive, isMouseOver(fmtX, fmtBtnY))
                .with_label("U")
                .with_debug_name("btn_underline"))) {
            cmd::toggleUnderline(doc);
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
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 0, isMouseOver(fmtX, fmtBtnY), ToolbarIcon::AlignLeft)
                .with_label("")
                .with_debug_name("btn_align_left"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Left);
            toolbar.currentAlignment = 0;
            doc.isDirty = true;
        }
        trackTooltip(fmtX, fmtBtnY, "Align Left (Ctrl+L)");
        fmtX += buttonSize + buttonPadding;
        
        int alignCBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, alignCBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 1, isMouseOver(fmtX, fmtBtnY), ToolbarIcon::AlignCenter)
                .with_label("")
                .with_debug_name("btn_align_center"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Center);
            toolbar.currentAlignment = 1;
            doc.isDirty = true;
        }
        trackTooltip(fmtX, fmtBtnY, "Align Center (Ctrl+E)");
        fmtX += buttonSize + buttonPadding;
        
        int alignRBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, alignRBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 2, isMouseOver(fmtX, fmtBtnY), ToolbarIcon::AlignRight)
                .with_label("")
                .with_debug_name("btn_align_right"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Right);
            toolbar.currentAlignment = 2;
            doc.isDirty = true;
        }
        trackTooltip(fmtX, fmtBtnY, "Align Right (Ctrl+R)");
        fmtX += buttonSize + buttonPadding;
        
        int alignJBtnId = fmtBtnId++;
        if (button(ctx, mk(uiRoot, alignJBtnId),
            absToolbarButton(fmtX, fmtBtnY, buttonSize, true, toolbar.currentAlignment == 3, isMouseOver(fmtX, fmtBtnY), ToolbarIcon::AlignJustify)
                .with_label("")
                .with_debug_name("btn_align_justify"))) {
            doc.buffer.setCurrentAlignment(TextAlignment::Justify);
            toolbar.currentAlignment = 3;
            doc.isDirty = true;
        }
        trackTooltip(fmtX, fmtBtnY, "Justify (Ctrl+J)");
        }  // if (!layout.showOutline): the Formatting toolbar

        // === Draw Tooltips ===
        // One tooltip at a time, after ~0.5s of steady hover on the same
        // button. Win95 style: pale yellow #FFFFE1, 1px black border,
        // 4px horizontal / 2px vertical padding.
        if (tooltips.empty()) {
            tooltipHoverTime_ = 0.0f;
            tooltipHoverX_ = -1.0f;
            tooltipHoverY_ = -1.0f;
            tooltipHoverText_.clear();
        } else {
            const auto& tip = tooltips.front();
            if (tip.x == tooltipHoverX_ && tip.y == tooltipHoverY_ &&
                tip.text == tooltipHoverText_) {
                tooltipHoverTime_ += dt;
            } else {
                tooltipHoverX_ = tip.x;
                tooltipHoverY_ = tip.y;
                tooltipHoverText_ = tip.text;
                tooltipHoverTime_ = 0.0f;
            }
            if (tooltip_math::delayElapsed(tooltipHoverTime_)) {
                float tipFontSize = 11.0f;
                float tipW = tooltip_math::tooltipWidth(static_cast<float>(
                    theme::MeasureUIText(tip.text.c_str(), static_cast<int>(tipFontSize))));
                float tipH = tooltip_math::tooltipHeight(tipFontSize);
                div(ctx, mk(uiRoot, 4000),
                    ComponentConfig{}
                        .with_label(tip.text)
                        .with_size(ComponentSize{pixels(tipW), pixels(tipH)})
                        .with_absolute_position()
                        .with_translate(tip.x, tip.y)
                        .with_custom_background(toAhColor(theme::TOOLTIP_BG))
                        .with_custom_text_color(toAhColor(theme::TOOLTIP_TEXT))
                        .with_roundness(0.0f)
                        .with_render_layer(20)
                        .with_bevel(afterhours::ui::BevelStyle::Raised,
                                    toAhColor(theme::TOOLTIP_BORDER),
                                    toAhColor(theme::TOOLTIP_BORDER), 1.0f)
                        .with_alignment(afterhours::ui::TextAlignment::Center)
                        .with_debug_name("tooltip"));
            }
        }
        
        // === Close dropdowns on click outside ===
        // fmtBtnY lives in the Formatting toolbar block (not
        // rendered in Outline mode); the close code below needs
        // the same value, so define it here in the outer scope.
        float fmtBtnY = formattingBarY + buttonPadding;
        // If mouse was pressed and no dropdown button is hovered, close all dropdowns
        if (ctx.mouse.just_pressed && (toolbar.styleDropdownOpen || toolbar.fontDropdownOpen || toolbar.fontSizeDropdownOpen || toolbar.zoomDropdownOpen)) {
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
            // Standard toolbar Zoom dropdown button + list (right end).
            if (toolbar.zoomDropdownOpen) {
                if (mx >= zoomBtnX && mx <= zoomBtnX + zoomBtnW &&
                    my >= zoomBtnY && my <= zoomBtnY + theme::layout::scale(22)) {
                    insideDropdown = true;
                }
                float zoomListY = zoomBtnY + buttonSize + 2.0f;
                float zoomListBottom = zoomListY + 6.0f * theme::layout::scale(20) +
                                       theme::layout::scale(4);
                if (mx >= zoomBtnX && mx <= zoomBtnX + zoomBtnW &&
                    my >= zoomListY && my <= zoomListBottom) {
                    insideDropdown = true;
                }
            }
            if (!insideDropdown) {
                toolbar.styleDropdownOpen = false;
                toolbar.fontDropdownOpen = false;
                toolbar.fontSizeDropdownOpen = false;
                toolbar.zoomDropdownOpen = false;
            }
        }
    }
};

}  // namespace ecs
