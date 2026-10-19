#pragma once

// Menu UI System using Afterhours immediate-mode UI

#include <afterhours/ah.h>
#include <afterhours/src/plugins/ui.h>
#include <afterhours/src/plugins/modal.h>
#include <afterhours/src/plugins/ui/text_input/text_input.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "components.h"
#include "../editor/comments.h"
#include "toolbar_icons.h"
#include "editor_entity_cache.h"
#include "scrollbar_system.h"  // anyModalDialogOpen for the no-UI focus guard
#include "menu_actions.h"
#include "../input_mapping.h"  // For InputAction enum
#include "../ui/ah_win95_widgets.h"
#include "../ui/menu_geometry.h"
#include "../ui/input.h"       // For input::isKeyPressed
// test_input:: available via rl.h -> external.h
#include "../ui/theme.h"
#include "../ui/ui_context.h"  // For ui_imm::getUIRootEntity()
#include "../ui/menu_setup.h"
#include "../settings.h"

namespace ecs {

using afterhours::Entity;
using afterhours::System;
using afterhours::ui::UIContext;
using afterhours::ui::Theme;
using afterhours::ui::imm::ComponentConfig;
using afterhours::ui::ComponentSize;
using afterhours::ui::FlexDirection;
using afterhours::ui::imm::div;
using afterhours::ui::imm::button;
using afterhours::ui::imm::mk;
using afterhours::ui::pixels;
using afterhours::ui::percent;

inline afterhours::Color toAhColor(const afterhours::Color& color) {
    return {color.r, color.g, color.b, color.a};
}

// Menu UI System - runs during update phase to handle menu interactions
// Queries only for UIContext singleton, then manually finds MenuComponent entities
struct MenuUISystem : System<UIContext<InputAction>> {
    EditorEntityCache cache_;
    
    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float) override {
        cache_.resolve();
        if (!cache_.resolved()) return;

        MenuComponent& menu = *cache_.menu;
        DialogState& dialogs = *cache_.dialogs;
        auto& layout = *cache_.layout;
        auto& doc = *cache_.doc;
        ToolbarComponent& toolbar = *cache_.toolbar;

        // Confirmations to the transient status-bar flash, not a
        // floating toast. Errors keep toast_notify::error.
        auto flash = [&](const std::string& msg, double duration = 3.0) {
            if (cache_.status) {
                status::flash(*cache_.status, msg,
                              afterhours::graphics::get_time(), duration);
            }
        };

        // Click shields are rebuilt every frame below (comment cards,
        // chip popup). Clear first so a stale rect never eats a click
        // after its owner stopped rendering (e.g. Focus mode).
        dialogs.clickShields.clear();

        if (layout.focusMode) return;

        // Refresh menus if recent file count changed, or if the open
        // document changed (the Window menu lists "1 <current file>").
        const auto& recentFiles = Settings::get().get_recent_files();
        std::string expectedWindowLabel =
            menu_setup::windowDocLabel(doc.filePath);
        if (static_cast<int>(recentFiles.size()) != menu.recentFilesCount ||
            menu.windowDocLabel != expectedWindowLabel) {
            menu.menus = menu_setup::createMenuBar(recentFiles, doc.filePath);
            menu.recentFilesCount = static_cast<int>(recentFiles.size());
            menu.windowDocLabel = expectedWindowLabel;

            if (doc.trackChangesEnabled &&
                menu.menus.size() > 1 &&
                menu.menus[1].items.size() > 3) {
                menu.menus[1].items[3].mark = win95::MenuMark::Checkmark;
            }
        }

        // Keep the View > Normal / Page Layout / Ruler / Outline
        // checkmarks in sync: any menu rebuild resets marks to the
        // createMenuBar defaults. Normal / Page Layout are a radio
        // pair (Normal checked exactly when isNormalView; Page
        // Layout checked when Paged — in Outline neither is
        // checked, Outline is).
        for (auto& m : menu.menus) {
            if (m.label != "View") continue;
            for (auto& item : m.items) {
                if (item.label == "Normal") {
                    item.mark = layout::isNormalView(layout)
                                    ? win95::MenuMark::Checkmark
                                    : win95::MenuMark::None;
                }
                if (item.label == "Page Layout") {
                    item.mark = layout.pageMode == PageMode::Paged
                                    ? win95::MenuMark::Checkmark
                                    : win95::MenuMark::None;
                }
                if (item.label == "Ruler") {
                    item.mark = layout.showRuler ? win95::MenuMark::Checkmark
                                                 : win95::MenuMark::None;
                }
                if (item.label == "Outline") {
                    item.mark = layout.showOutline ? win95::MenuMark::Checkmark
                                                   : win95::MenuMark::None;
                }
                if (item.label == "Header and Footer") {
                    item.mark = layout.headerEditTarget !=
                                        HeaderEditTarget::None
                                    ? win95::MenuMark::Checkmark
                                    : win95::MenuMark::None;
                }
                if (item.label == "Standard Toolbar") {
                    item.mark = layout.showStandardToolbar ? win95::MenuMark::Checkmark
                                                           : win95::MenuMark::None;
                }
                if (item.label == "Formatting Toolbar") {
                    item.mark = layout.showFormattingToolbar ? win95::MenuMark::Checkmark
                                                             : win95::MenuMark::None;
                }
            }
        }
        
        // Get the UI root entity for parenting UI elements
        Entity& entity = ui_imm::getUIRootEntity();
        
        // Save the global theme so we can restore it after menu rendering
        Theme savedTheme = ctx.theme;
        
        // Set up Win95 menu theme (only for this system's components)
        Theme menuTheme;
        menuTheme.font = toAhColor(theme::MENU_TEXT_HOVER);
        menuTheme.darkfont = toAhColor(theme::MENU_TEXT);
        menuTheme.font_muted = toAhColor(theme::MENU_DISABLED);
        menuTheme.background = toAhColor(theme::MENU_BG);
        menuTheme.surface = toAhColor(theme::MENU_BG);
        menuTheme.primary = toAhColor(theme::MENU_HOVER);
        menuTheme.secondary = toAhColor(theme::MENU_BG);
        menuTheme.accent = toAhColor(theme::MENU_HOVER);
        menuTheme.roundness = 0.0f;
        menuTheme.segments = 0;
        ctx.theme = menuTheme;

        float screenWidth = static_cast<float>(layout.screenWidth);
        
        // Create menu bar container (background + raised border via afterhours)
        // Menu bar container (background)
        div(ctx, mk(entity, 0),
            ComponentConfig{}
                .with_debug_name("menu_bar_container")
                .with_size(ComponentSize{pixels(screenWidth), 
                                        pixels(theme::layout::scale(theme::layout::MENU_BAR_HEIGHT))})
                .with_absolute_position()
                .with_translate(0.0f, theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT))
                .with_flex_direction(FlexDirection::Row)
                .with_custom_background(toAhColor(theme::WINDOW_BG))
                .with_bevel(afterhours::ui::BevelStyle::Raised,
                            toAhColor(theme::BORDER_LIGHT), toAhColor(theme::BORDER_DARK), 1.0f)
                .with_roundness(0.0f));

        // Check if any menu is currently open (for hover-to-switch logic)
        bool anyMenuOpen = false;
        for (const auto& m : menu.menus) {
            if (m.open) { anyMenuOpen = true; break; }
        }
        
        // When a dropdown is open, add an input gate to block clicks on
        // lower-layer entities (e.g. the editor area). Without this,
        // HandleClicks processes entities in creation order and a layer-0
        // editor entity can steal 'active' before the layer-51 dropdown item.
        if (anyMenuOpen) {
            ctx.add_input_gate("menu_dropdown", [](afterhours::EntityID id) {
                auto opt = afterhours::ui::UICollectionHolder::getEntityForID(id);
                if (!opt.has_value()) return false;
                auto& e = opt.asE();
                if (!e.has<afterhours::ui::UIComponent>()) return false;
                return e.get<afterhours::ui::UIComponent>().render_layer >= 1;
            });
        } else {
            ctx.remove_input_gate("menu_dropdown");
        }
        
        // Track X position for header buttons
        float headerX = theme::layout::scale(4.0f);
        float headerY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT);
        
        // Track whether any header was clicked or hovered this frame
        bool headerInteracted = false;
        
        // Render each menu header button and handle clicks + hover-to-switch
        for (size_t menuIdx = 0; menuIdx < menu.menus.size(); ++menuIdx) {
            auto& menuDef = menu.menus[menuIdx];
            bool isOpen = menuDef.open;
            
            // Header width = label + 7px padding per side (Word95),
            // via menu_geometry so click rects and dropdowns agree.
            int menuFontSize = 16;
            float headerPadding = theme::layout::scale(menu_geometry::HEADER_PADDING_X);
            float buttonWidth = menu_geometry::headerWidth(
                static_cast<float>(theme::MeasureUIText(menuDef.label.c_str(), menuFontSize)),
                headerPadding);
            float menuBarHeight = theme::layout::scale(theme::layout::MENU_BAR_HEIGHT);
            
            // Register menu label for E2E tests
            test_input::register_visible_text(menuDef.label);
            
            int headerId = 500 + static_cast<int>(menuIdx);
            
            // Determine highlight: open OR mouse hovering while another menu is open
            // Use direct mouse position check (was_hot uses entity IDs, not MK IDs)
            bool mouseOverHeader = afterhours::ui::is_mouse_inside(
                ctx.mouse.pos, {headerX, headerY, buttonWidth, menuBarHeight});
            bool highlighted = isOpen;
            if (!isOpen && anyMenuOpen && mouseOverHeader) {
                highlighted = true;
            }
            
            auto headerResult = button(ctx, mk(entity, headerId),
                ComponentConfig{}
                    .with_debug_name("menu_header_" + menuDef.label)
                    .with_label(menuDef.label)
                    .with_size(ComponentSize{pixels(buttonWidth),
                                            pixels(menuBarHeight)})
                    .with_absolute_position()
                    .with_translate(headerX, headerY)
                    // Win95 menu titles: flat at rest; the OPEN title
                    // (and hover-switch target) is the navy selection
                    // with white text — see real Word 95 captures (the
                    // open Insert menu title is navy/white).
                    .with_custom_background(highlighted ? toAhColor(theme::MENU_HOVER) : toAhColor(theme::MENU_BG))
                    .with_custom_text_color(highlighted ? toAhColor(theme::MENU_TEXT_HOVER) : toAhColor(theme::MENU_TEXT))
                    // Label centered in its hot area (Word 95): width is
                    // text + 2*padding, so centering makes both insets
                    // exactly headerPadding. NOTE: with_padding on a label
                    // button is a no-op in afterhours (use text_inset /
                    // alignment) — the old Left+padding left the label
                    // hard against the left bevel.
                    .with_alignment(afterhours::ui::TextAlignment::Center)
                    .with_justify_content(afterhours::ui::JustifyContent::Center)
                    .with_align_items(afterhours::ui::AlignItems::Center)
                    .with_roundness(0.0f)
                    .with_bevel(isOpen ? afterhours::ui::BevelStyle::Sunken
                                       : afterhours::ui::BevelStyle::None,
                                toAhColor(theme::BORDER_LIGHT), toAhColor(theme::BORDER_DARK), 1.0f)
                    .with_cursor(afterhours::ui::CursorType::Pointer)
                    .with_render_layer(1));

            // Mnemonic underline: Word95 underlines the menu's hotkey
            // letter (the first character) beneath the label.
            if (!menuDef.label.empty()) {
                std::string firstChar = menuDef.label.substr(0, 1);
                float underlineW = static_cast<float>(
                    theme::MeasureUIText(firstChar.c_str(), menuFontSize));
                div(ctx, mk(entity, 600 + static_cast<int>(menuIdx)),
                    ComponentConfig{}
                        .with_debug_name("menu_mnemonic_" + menuDef.label)
                        .with_size(ComponentSize{pixels(underlineW), pixels(1.0f)})
                        .with_absolute_position()
                        .with_translate(headerX + headerPadding,
                                        headerY + menuBarHeight - theme::layout::scale(5.0f))
                        .with_custom_background(highlighted ? toAhColor(theme::MENU_TEXT_HOVER)
                                                            : toAhColor(theme::MENU_TEXT))
                        .with_roundness(0.0f)
                        .with_render_layer(1));
            }

            // Handle header click: toggle this menu, close others
            if (headerResult) {
                for (size_t j = 0; j < menu.menus.size(); ++j) {
                    menu.menus[j].open = (j == menuIdx) ? !isOpen : false;
                }
                menu.activeMenuIndex = isOpen ? -1 : static_cast<int>(menuIdx);
                headerInteracted = true;
            }
            
            // Handle hover-to-switch: when any menu is open and we hover a different header
            // with the mouse UP. While the button is down, the click handler
            // owns switching: without this, hover-switch opens the menu on
            // the press frame and the click toggle closes it again on the
            // release frame (button() reports the held-down state), so
            // clicking another menu title ended with all menus closed.
            if (anyMenuOpen && !isOpen && mouseOverHeader && !ctx.mouse.left_down) {
                for (size_t j = 0; j < menu.menus.size(); ++j) {
                    menu.menus[j].open = (j == menuIdx);
                }
                menu.activeMenuIndex = static_cast<int>(menuIdx);
                headerInteracted = true;
            }
            
            headerX += buttonWidth;
        }
        
        // Render dropdown for whichever menu is open
        bool itemInteracted = false;
        for (size_t menuIdx = 0; menuIdx < menu.menus.size(); ++menuIdx) {
            auto& menuDef = menu.menus[menuIdx];
            if (!menuDef.open) continue;
            
            // Calculate dropdown position (matching menu header widths)
            int menuFontSize = 16;
            float dropdownX = theme::layout::scale(4.0f);
            for (size_t i = 0; i < menuIdx; ++i) {
                dropdownX += menu_geometry::headerWidth(
                    static_cast<float>(theme::MeasureUIText(menu.menus[i].label.c_str(), menuFontSize)),
                    theme::layout::scale(menu_geometry::HEADER_PADDING_X));
            }
            float dropdownY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + 
                                                  theme::layout::MENU_BAR_HEIGHT);
            
            // Compute dropdown height
            float dropdownHeight = 0;
            for (const auto& item : menuDef.items) {
                dropdownHeight += item.separator ? theme::layout::scale(8.0f) : theme::layout::scale(20.0f);
            }
            
            // Compute max width from content (display labels, so an
            // ellipsized recent path measures as displayed), capped so
            // long content can't stretch the dropdown across the screen.
            float maxWidth = 150.0f;
            for (const auto& item : menuDef.items) {
                const std::string& showLabel =
                    item.displayLabel.empty() ? item.label : item.displayLabel;
                float labelWidth = static_cast<float>(showLabel.length() * 7);
                float shortcutWidth = static_cast<float>(item.shortcut.length() * 7);
                float totalWidth = labelWidth + shortcutWidth + 50.0f;
                if (totalWidth > maxWidth) maxWidth = totalWidth;
            }
            if (maxWidth > menu_geometry::MAX_DROPDOWN_WIDTH) {
                maxWidth = menu_geometry::MAX_DROPDOWN_WIDTH;
            }
            
            // Dropdown container background
            div(ctx, mk(entity, 100 + static_cast<int>(menuIdx)),
                ComponentConfig{}
                    .with_debug_name("dropdown_" + menuDef.label)
                    .with_size(ComponentSize{pixels(maxWidth), pixels(dropdownHeight + 4.0f)})
                    .with_hard_shadow(ui_imm::WIN95_POPUP_SHADOW_OFFSET,
                                      ui_imm::WIN95_POPUP_SHADOW_OFFSET,
                                      ui_imm::WIN95_POPUP_SHADOW_COLOR)
                    .with_absolute_position()
                    .with_translate(dropdownX, dropdownY)
                    .with_custom_background(toAhColor(theme::MENU_BG))
                    .with_bevel(afterhours::ui::BevelStyle::Raised,
                                toAhColor(theme::BORDER_LIGHT), toAhColor(theme::BORDER_DARK), 1.0f)
                    .with_roundness(0.0f)
                    .with_render_layer(50));
            
            // Render each menu item
            float itemY = dropdownY + 2.0f;
            
            for (size_t itemIdx = 0; itemIdx < menuDef.items.size(); ++itemIdx) {
                const auto& item = menuDef.items[itemIdx];
                
                if (item.separator) {
                    int sepBaseId = 10000 + static_cast<int>(menuIdx) * 100 + static_cast<int>(itemIdx);
                    div(ctx, mk(entity, sepBaseId),
                        ComponentConfig{}
                            .with_debug_name("separator_etched")
                            .with_size(ComponentSize{pixels(maxWidth - 8.0f), pixels(2.0f)})
                            .with_absolute_position()
                            .with_translate(dropdownX + 4.0f, itemY + 3.0f)
                            .with_custom_background(afterhours::Color{0, 0, 0, 0})
                            .with_border_top(toAhColor(theme::BORDER_DARK))
                            .with_border_bottom(toAhColor(theme::BORDER_LIGHT))
                            .with_roundness(0.0f)
                            .with_render_layer(51));
                    itemY += theme::layout::scale(8.0f);
                } else {
                    // Build mark prefix
                    std::string markPrefix;
                    if (item.mark != win95::MenuMark::None) {
                        switch (item.mark) {
                            // Checkmark is U+221A √, NOT U+2713 ✓:
                            // the UI font (Roboto-Regular) has no
                            // U+2713/U+2714 glyph (verified from its
                            // cmap), so a ✓ prefix rendered as blank
                            // and every checked item (View > Ruler,
                            // Window's "1 <doc>") looked unchecked.
                            // Roboto does have U+221A (glyph 391),
                            // which reads as a checkmark at menu size.
                            case win95::MenuMark::Checkmark: markPrefix = "\xE2\x88\x9A "; break;
                            case win95::MenuMark::Radio:     markPrefix = "\xE2\x80\xA2 "; break;
                            case win95::MenuMark::Dash:      markPrefix = "- "; break;
                            case win95::MenuMark::None: break;
                            default: break;
                        }
                    } else {
                        markPrefix = "  ";  // Reserve space for mark column
                    }
                    
                    // Display label (ellipsized for recent paths).
                    const std::string& showLabel =
                        item.displayLabel.empty() ? item.label : item.displayLabel;

                    // Register menu item label for E2E tests
                    test_input::register_visible_text(showLabel);
                    
                    int itemId = 20000 + static_cast<int>(menuIdx) * 100 + static_cast<int>(itemIdx);
                    float itemHeight = theme::layout::scale(20.0f);
                    
                    // Determine hover highlight using direct mouse position check
                    bool hovered = afterhours::ui::is_mouse_inside(
                        ctx.mouse.pos, {dropdownX + 2.0f, itemY, maxWidth - 4.0f, itemHeight}) && item.enabled;
                    
                    auto textColor = !item.enabled ? toAhColor(theme::MENU_DISABLED) :
                                     hovered ? toAhColor(theme::MENU_TEXT_HOVER) : toAhColor(theme::MENU_TEXT);
                    
                    // Button with left-aligned label. The button sits
                    // ITEM_BUTTON_EDGE inside the dropdown; text_inset
                    // makes up the rest so the label starts exactly
                    // ITEM_PADDING_X (8px) from the dropdown edge.
                    auto itemResult = button(ctx, mk(entity, itemId),
                        ComponentConfig{}
                            .with_debug_name("item_" + item.label)
                            .with_label(markPrefix + showLabel)
                            .with_size(ComponentSize{pixels(maxWidth - 4.0f), pixels(itemHeight)})
                            .with_absolute_position()
                            .with_translate(dropdownX + 2.0f, itemY)
                            .with_custom_background(hovered ? toAhColor(theme::MENU_HOVER) : toAhColor(theme::MENU_BG))
                            .with_custom_text_color(textColor)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_justify_content(afterhours::ui::JustifyContent::Center)
                            .with_text_inset(menu_geometry::ITEM_PADDING_X -
                                                 menu_geometry::ITEM_BUTTON_EDGE,
                                             0.0f)
                            .with_roundness(0.0f)
                            .with_cursor(afterhours::ui::CursorType::Pointer)
                            .with_custom_hover_bg(toAhColor(theme::MENU_HOVER))
                            .with_render_layer(51));

                    // Shortcut label: right-aligned, right edge exactly
                    // ITEM_PADDING_X (8px) from the dropdown edge —
                    // symmetric with the label's left inset.
                    if (!item.shortcut.empty()) {
                        float shortcutWidth = static_cast<float>(
                            theme::MeasureUIText(item.shortcut.c_str(), 14) + 2);
                        float shortcutX = menu_geometry::dropdownShortcutRight(
                                              dropdownX, maxWidth) -
                                          shortcutWidth;
                        div(ctx, mk(entity, 30000 + static_cast<int>(menuIdx) * 100 + static_cast<int>(itemIdx)),
                            ComponentConfig{}
                                .with_label(item.shortcut)
                                .with_size(ComponentSize{pixels(shortcutWidth), pixels(itemHeight)})
                                .with_absolute_position()
                                .with_translate(shortcutX, itemY)
                                .with_background(afterhours::ui::Theme::Usage::None)
                                .with_custom_text_color(textColor)
                                .with_alignment(afterhours::ui::TextAlignment::Right)
                                .with_justify_content(afterhours::ui::JustifyContent::Center)
                                .with_roundness(0.0f)
                                .with_render_layer(51)
                                .with_skip_tabbing(true));
                    }
                    
                    // Handle item click: dispatch action via lastClickedResult
                    if (itemResult && item.enabled) {
                        menu.lastClickedResult = static_cast<int>(menuIdx * 100 + itemIdx);
                        // Call item action callback if set
                        if (item.action) {
                            item.action();
                        }
                        // Close all menus
                        for (auto& m : menu.menus) { m.open = false; }
                        menu.activeMenuIndex = -1;
                        itemInteracted = true;
                    }
                    
                    itemY += itemHeight;
                }
            }
        }
        
        // Esc closes an open menu (modal-menu rule (c)).
        if (anyMenuOpen && input::isKeyPressed(afterhours::keys::ESCAPE)) {
            for (auto& m : menu.menus) { m.open = false; }
            menu.activeMenuIndex = -1;
        }

        // Esc exits Print Preview (only when no menu was open this
        // frame: an Esc that just closed a menu must not also exit
        // preview). Restores the pre-preview page mode.
        if (!anyMenuOpen && layout.previewMode &&
            input::isKeyPressed(afterhours::keys::ESCAPE)) {
            cmd::closePrintPreview(layout);
            // Preview toolbar dies with preview (stale focus id).
            ctx.set_focus(ctx.ROOT);
        }

        // Close menus on click outside and consume stray clicks.
        // Use direct mouse position checks against known rects instead of was_hot(),
        // because was_hot() compares entity IDs but we only have MK IDs here.
        if (anyMenuOpen && !headerInteracted && !itemInteracted) {
            if (ctx.mouse.just_pressed) {
                bool clickInMenu = false;
                
                // Check header button rects
                {
                    float hx = theme::layout::scale(4.0f);
                    float hy = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT);
                    int menuFontSize = 16;
                    for (size_t i = 0; i < menu.menus.size(); ++i) {
                        float bw = menu_geometry::headerWidth(
                            static_cast<float>(theme::MeasureUIText(menu.menus[i].label.c_str(), menuFontSize)),
                            theme::layout::scale(menu_geometry::HEADER_PADDING_X));
                        float bh = theme::layout::scale(theme::layout::MENU_BAR_HEIGHT);
                        if (afterhours::ui::is_mouse_inside(ctx.mouse.pos, {hx, hy, bw, bh})) {
                            clickInMenu = true;
                            break;
                        }
                        hx += bw;
                    }
                }
                
                // Check dropdown rects
                if (!clickInMenu) {
                    int menuFontSize = 16;
                    for (size_t menuIdx = 0; menuIdx < menu.menus.size(); ++menuIdx) {
                        if (!menu.menus[menuIdx].open) continue;
                        float dx = theme::layout::scale(4.0f);
                        for (size_t i = 0; i < menuIdx; ++i) {
                            dx += menu_geometry::headerWidth(
                                static_cast<float>(theme::MeasureUIText(menu.menus[i].label.c_str(), menuFontSize)),
                                theme::layout::scale(menu_geometry::HEADER_PADDING_X));
                        }
                        float dy = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + theme::layout::MENU_BAR_HEIGHT);
                        float dh = 0;
                        for (const auto& item : menu.menus[menuIdx].items) {
                            dh += item.separator ? theme::layout::scale(8.0f) : theme::layout::scale(20.0f);
                        }
                        float dw = 150.0f;
                        for (const auto& item : menu.menus[menuIdx].items) {
                            const std::string& showLabel =
                                item.displayLabel.empty() ? item.label : item.displayLabel;
                            float tw = static_cast<float>(showLabel.length() * 7 + item.shortcut.length() * 7) + 50.0f;
                            if (tw > dw) dw = tw;
                        }
                        if (dw > menu_geometry::MAX_DROPDOWN_WIDTH) {
                            dw = menu_geometry::MAX_DROPDOWN_WIDTH;
                        }
                        if (afterhours::ui::is_mouse_inside(ctx.mouse.pos, {dx, dy, dw, dh + 4.0f})) {
                            clickInMenu = true;
                            break;
                        }
                    }
                }
                
                if (!clickInMenu) {
                    // Click was outside all menu areas: close menus and consume
                    // the click so elements underneath don't receive it.
                    for (auto& m : menu.menus) { m.open = false; }
                    menu.activeMenuIndex = -1;
                    ctx.mouse.just_pressed = false;
                    ctx.mouse.just_released = false;
                }
                // Clicks INSIDE the menu area are left alone so HandleClicks
                // can process them for the dropdown item buttons.
            }
        }
        
        // ============================================================
        // Modal Dialogs (using afterhours modal.h)
        // ============================================================
        
        // About dialog
        if (dialogs.showAboutDialog) {
            constexpr int ABOUT_MODAL_ID = 50000;
            afterhours::modal::info(ctx, mk(entity, ABOUT_MODAL_ID),
                dialogs.showAboutDialog,
                "About Wordproc",
                "Wordproc v0.1\n\nA Windows 95 style word processor\nbuilt with Afterhours.",
                "OK");
        }

        // Save-changes prompt on quit (Win95: Yes / No / Cancel).
        if (dialogs.showQuitPrompt) {
            constexpr int QUIT_MODAL_ID = 50002;
            auto quitResult = afterhours::modal(ctx, mk(entity, QUIT_MODAL_ID),
                dialogs.showQuitPrompt,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(340), afterhours::ui::h720(150),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (quitResult) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(quitResult.ent(), 900),
                                              "Wordproc", CONTENT_LAYER);

                std::string docName = doc.filePath.empty()
                    ? "Untitled"
                    : doc.filePath.substr(doc.filePath.find_last_of("/\\") + 1);
                div(ctx, mk(quitResult.ent(), 901),
                    ComponentConfig{}
                        .with_label("Save changes to " + docName + "?")
                        .with_size(ComponentSize{percent(1.0f), h720(40)})
                        .with_alignment(afterhours::ui::TextAlignment::Center)
                        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_render_layer(CONTENT_LAYER));

                auto quitButtonRow = div(ctx, mk(quitResult.ent(), 902),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));

                if (quitButtonRow) {
                    auto quitBtn = [&](int id, const char* label, const char* debug) {
                        return button(ctx, mk(quitButtonRow.ent(), id),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name(debug)
                                .with_label(label)
                                .with_render_layer(CONTENT_LAYER));
                    };
                    if (quitBtn(0, "Yes", "quit_prompt_yes")) {
                        dialogs.showQuitPrompt = false;
                        if (doc.filePath.empty()) {
                            // No path yet: Save As first, quit once it saves.
                            menu.pendingDialog = MenuComponent::PendingDialog::SaveAs;
                            dialogs.quitAfterSave = true;
                        } else if (cmd::saveDocument(doc, layout, menu)) {
                            dialogs.quitRequested = true;
                        }
                    }
                    if (quitBtn(1, "No", "quit_prompt_no")) {
                        dialogs.showQuitPrompt = false;
                        dialogs.quitRequested = true;
                    }
                    if (quitBtn(2, "Cancel", "quit_prompt_cancel")) {
                        dialogs.showQuitPrompt = false;
                    }
                }
            }
        }
        
        // Word Count dialog - Win95 shell: rows of label + right-aligned value
        if (dialogs.showWordCountDialog) {
            {
                TextStats stats = doc.buffer.stats();
                std::size_t linesPerPage =
                    layout.linesPerPage > 0
                        ? static_cast<std::size_t>(layout.linesPerPage)
                        : 50;
                std::size_t pages =
                    (stats.lines + linesPerPage - 1) / linesPerPage;
                if (pages < 1) pages = 1;

                constexpr int WORDCOUNT_MODAL_ID = 50001;
                auto result = afterhours::modal(ctx, mk(entity, WORDCOUNT_MODAL_ID),
                    dialogs.showWordCountDialog,
                    ah_win95::win95DialogConfig(
                        afterhours::ui::h720(300), afterhours::ui::h720(210),
                        static_cast<float>(layout.screenWidth),
                        static_cast<float>(layout.screenHeight)));

                if (result) {
                    using namespace afterhours::ui;
                    using namespace afterhours::ui::imm;
                    constexpr int CONTENT_LAYER = 1001;

                    ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                                  "Word Count", CONTENT_LAYER);

                    int rowId = 0;
                    auto statRow = [&](const std::string& label,
                                       const std::string& value) {
                        auto row = div(ctx, mk(result.ent(), 10 + rowId++),
                            ComponentConfig{}
                                .with_size(ComponentSize{percent(1.0f), h720(22)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_justify_content(JustifyContent::SpaceBetween)
                                .with_align_items(AlignItems::Center)
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_render_layer(CONTENT_LAYER));
                        if (row) {
                            div(ctx, mk(row.ent(), 0),
                                ComponentConfig{}
                                    .with_label(label)
                                    .with_size(ComponentSize{percent(0.6f), h720(22)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_render_layer(CONTENT_LAYER));
                            div(ctx, mk(row.ent(), 1),
                                ComponentConfig{}
                                    .with_label(value)
                                    .with_size(ComponentSize{percent(0.4f), h720(22)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Right)
                                    .with_render_layer(CONTENT_LAYER));
                        }
                    };

                    statRow("Pages:", std::to_string(pages));
                    statRow("Words:", std::to_string(stats.words));
                    statRow("Characters:", std::to_string(stats.characters));
                    statRow("Lines:", std::to_string(stats.lines));
                    statRow("Paragraphs:", std::to_string(stats.paragraphs));
                    statRow("Sentences:", std::to_string(stats.sentences));

                    // OK button row: compact 90px button at the bottom
                    // right (OK is the default button: dark outline).
                    auto button_row = div(ctx, mk(result.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(44)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_justify_content(JustifyContent::FlexEnd)
                            .with_align_items(AlignItems::Center)
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));

                    if (button_row) {
                        if (button(ctx, mk(button_row.ent(), 0),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("word_count_ok")
                                .with_label("OK")
                                .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.showWordCountDialog = false;
                        }
                    }
                }
            }
        }
        
        // Spelling dialog (Tools > Spelling..., F7) - Win95 shell:
        // left column (Not in dictionary field, Suggestions list,
        // Change to field), right column of gray buttons.
        if (dialogs.showSpellingDialog) {
            constexpr int SPELLING_MODAL_ID = 50010;
            auto result = afterhours::modal(ctx, mk(entity, SPELLING_MODAL_ID),
                dialogs.showSpellingDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(430), afterhours::ui::h720(300),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Spelling", CONTENT_LAYER);

                if (dialogs.spellNoErrors) {
                    div(ctx, mk(result.ent(), 901),
                        ComponentConfig{}
                            .with_label("The spell check is complete.")
                            .with_size(ComponentSize{percent(1.0f), h720(24)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_render_layer(CONTENT_LAYER));
                    auto doneRow = div(ctx, mk(result.ent(), 902),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(44)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_justify_content(JustifyContent::Center)
                            .with_align_items(AlignItems::Center)
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (doneRow) {
                        if (button(ctx, mk(doneRow.ent(), 0),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("spell_close")
                                .with_label("Close")
                                .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.showSpellingDialog = false;
                        }
                    }
                } else {
                    // Body row: left column + right button column
                    auto bodyRow = div(ctx, mk(result.ent(), 903),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), children()})
                            .with_flex_direction(FlexDirection::Row)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (bodyRow) {
                    auto leftCol = div(ctx, mk(bodyRow.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(302), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(4))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (leftCol) {
                    // "Not in dictionary:" label + sunken white field
                    div(ctx, mk(leftCol.ent(), 0),
                        ComponentConfig{}
                            .with_label("Not in dictionary:")
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));

                    div(ctx, mk(leftCol.ent(), 1),
                        ah_win95::win95SunkenStyle()
                            .with_label(dialogs.spellCurrentWord)
                            .with_size(ComponentSize{percent(1.0f), h720(26)})
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_padding(Padding{.left = pixels(4)})
                            .with_render_layer(CONTENT_LAYER));

                    // Suggestions list (click one to use it as the replacement)
                    div(ctx, mk(leftCol.ent(), 2),
                        ComponentConfig{}
                            .with_label("Suggestions:")
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));

                    float suggHeight = 22.0f * static_cast<float>(
                        std::max<std::size_t>(dialogs.spellSuggestions.size(), 1)) + 4.0f;
                    auto suggList = div(ctx, mk(leftCol.ent(), 3),
                        ah_win95::win95SunkenStyle()
                            .with_size(ComponentSize{percent(1.0f), h720(suggHeight)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                            .with_render_layer(CONTENT_LAYER));

                    if (suggList) {
                        if (dialogs.spellSuggestions.empty()) {
                            div(ctx, mk(suggList.ent(), 0),
                                ComponentConfig{}
                                    .with_label("(no suggestions)")
                                    .with_size(ComponentSize{percent(1.0f), h720(22)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_padding(Padding{.left = pixels(4)})
                                    .with_render_layer(CONTENT_LAYER));
                        } else {
                            for (std::size_t i = 0; i < dialogs.spellSuggestions.size(); ++i) {
                                const std::string& suggestion = dialogs.spellSuggestions[i];
                                bool selected =
                                    dialogs.spellReplacementStr == suggestion;
                                if (button(ctx, mk(suggList.ent(), 100 + static_cast<int>(i)),
                                    ComponentConfig{}
                                        .with_debug_name("spell_suggestion_" + suggestion)
                                        .with_label(suggestion)
                                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_padding(Padding{.left = pixels(4)})
                                        .with_cursor(afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    dialogs.spellReplacementStr = suggestion;
                                }
                            }
                        }
                    }

                    // "Change to:" field under the Suggestions list (Word 95)
                    div(ctx, mk(leftCol.ent(), 4),
                        ComponentConfig{}
                            .with_label("Change to:")
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));

                    afterhours::text_input::text_input(ctx, mk(leftCol.ent(), 5),
                        dialogs.spellReplacementStr,
                        ah_win95::win95SunkenStyle()
                            .with_debug_name("spell_change_to_input")
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_render_layer(CONTENT_LAYER));
                    }  // leftCol

                    // Right column: vertical stack of compact 90px
                    // gray buttons. Ignore is the default button
                    // (dark outline).
                    auto rightCol = div(ctx, mk(bodyRow.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(100), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(6))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (rightCol) {
                        if (button(ctx, mk(rightCol.ent(), 0),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("spell_ignore")
                                .with_label("Ignore")
                                .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                                .with_render_layer(CONTENT_LAYER))) {
                            cmd::spellIgnore(doc, dialogs);
                        }
                        if (button(ctx, mk(rightCol.ent(), 1),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("spell_ignore_all")
                                .with_label("Ignore All")
                                .with_render_layer(CONTENT_LAYER))) {
                            cmd::spellIgnoreAll(doc, dialogs);
                        }
                        if (button(ctx, mk(rightCol.ent(), 2),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("spell_add")
                                .with_label("Add")
                                .with_render_layer(CONTENT_LAYER))) {
                            std::string added = dialogs.spellCurrentWord;
                            cmd::spellAddToDictionary(doc, dialogs);
                            flash("Added to dictionary: " + added);
                        }
                        if (button(ctx, mk(rightCol.ent(), 3),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("spell_change")
                                .with_label("Change")
                                .with_render_layer(CONTENT_LAYER))) {
                            if (dialogs.spellReplacementStr.empty()) {
                                toast_notify::error("Enter a replacement word");
                            } else {
                                cmd::spellChange(doc, dialogs);
                            }
                        }
                        if (button(ctx, mk(rightCol.ent(), 4),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("spell_change_all")
                                .with_label("Change All")
                                .with_render_layer(CONTENT_LAYER))) {
                            if (dialogs.spellReplacementStr.empty()) {
                                toast_notify::error("Enter a replacement word");
                            } else {
                                cmd::spellChangeAll(doc, dialogs);
                            }
                        }
                        if (button(ctx, mk(rightCol.ent(), 5),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("spell_close")
                                .with_label("Close")
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.showSpellingDialog = false;
                        }
                    }  // rightCol
                    }  // bodyRow
                }  // else (errors remain)
            }
        }

        // Grammar dialog (Tools > Grammar...) - mirrors the Spelling
        // dialog: left column (In context field, Issue field,
        // Suggestion field), right column of gray buttons
        // (Ignore Once, Ignore All, Change, Close). One issue at a
        // time at grammarIndex; the enum name (e.g. REPEATED_WORD) is
        // never shown — Issue shows the human message, with the rule
        // display name as fallback. The clean state keeps this same
        // field layout with "No grammar issues found." in the
        // fields and Ignore/Change disabled (Close stays enabled):
        // a separate complete screen let a stale issue's buttons
        // survive a re-scan, and its Change button vanished instead
        // of disabling, so a click on a stale issue stayed possible.
        if (dialogs.showGrammarDialog) {
            constexpr int GRAMMAR_MODAL_ID = 50012;
            // Height fits the content: title(20) + body(~176) +
            // panel padding(8) = ~204, so 215, in both the issue
            // and the clean state (same field layout). The old
            // fixed 300 left ~95px of empty bottom in the issue view.
            constexpr float GRAMMAR_HEIGHT = 215.0f;
            auto result = afterhours::modal(ctx, mk(entity, GRAMMAR_MODAL_ID),
                dialogs.showGrammarDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(430), afterhours::ui::h720(GRAMMAR_HEIGHT),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Grammar", CONTENT_LAYER);

                const GrammarError* current =
                    cmd::grammarCurrentError(dialogs);
                {
                    // Body row: left column + right button column
                    auto bodyRow = div(ctx, mk(result.ent(), 903),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), children()})
                            .with_flex_direction(FlexDirection::Row)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (bodyRow) {
                    // Copy before any button: the button handlers
                    // rebuild grammarErrors (refresh), which would
                    // dangle `current`. Declared in bodyRow scope so
                    // the right column can use it.
                    bool hasIssue = current != nullptr;
                    bool canIgnore = hasIssue;
                    bool canChange =
                        hasIssue && !current->suggestion.empty();
                    auto leftCol = div(ctx, mk(bodyRow.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(302), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(4))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (leftCol) {
                    // Clean state: the fields read "No grammar
                    // issues found." with no highlight span, and
                    // the buttons below are disabled — a fixed
                    // issue is never left on display, actionable.
                    static const char* CLEAN_TEXT = "No grammar issues found.";
                    GrammarContext grammarCtx;
                    std::string sentence;
                    std::string issue;
                    std::string suggestion;
                    if (hasIssue) {
                        grammarCtx =
                            grammarSentenceContextWithHighlight(
                                doc.buffer.getText(), *current);
                        sentence = grammarCtx.sentence;
                        issue = cmd::grammarIssueLabel(*current);
                        suggestion = current->suggestion.empty()
                            ? "(no suggestion)"
                            : current->suggestion;
                    } else {
                        sentence = CLEAN_TEXT;
                        issue = CLEAN_TEXT;
                        suggestion = CLEAN_TEXT;
                    }
                    test_input::register_visible_text(sentence);
                    test_input::register_visible_text(issue);
                    if (hasIssue) {
                        test_input::register_visible_text(
                            current->suggestion);
                    }

                    auto fieldLabel = [&](int id, const std::string& label) {
                        div(ctx, mk(leftCol.ent(), id),
                            ComponentConfig{}
                                .with_label(label)
                                .with_size(ComponentSize{percent(1.0f), h720(20)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(ui_imm::win95_colors::TEXT)
                                .with_alignment(afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                    };
                    auto sunkenField = [&](int id, const std::string& text,
                                           const std::string& debug, float h) {
                        div(ctx, mk(leftCol.ent(), id),
                            ah_win95::win95SunkenStyle()
                                .with_debug_name(debug)
                                .with_label(text)
                                .with_size(ComponentSize{percent(1.0f), h720(h)})
                                .with_alignment(afterhours::ui::TextAlignment::Left)
                                .with_padding(Padding{.left = pixels(4)})
                                .with_render_layer(CONTENT_LAYER));
                    };

                    // "In context:" — the sentence as three spans:
                    // before / offending phrase / after. The offending
                    // span is navy bg + white text (HIGHLIGHT /
                    // TEXT_WHITE), the rest white bg + black text, so
                    // the exact phrase (e.g. the doubled "the the")
                    // is visually marked. Each span sizes to its own
                    // text (children() width), like the stepper value
                    // labels; the row wraps at span boundaries.
                    fieldLabel(0, "In context:");
                    {
                        auto contextRow = div(ctx, mk(leftCol.ent(), 1),
                            ah_win95::win95SunkenStyle()
                                .with_debug_name("grammar_context")
                                .with_size(ComponentSize{percent(1.0f), h720(44.0f)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_wrap()
                                .with_align_items(AlignItems::Center)
                                .with_padding(Padding{.left = pixels(4)})
                                .with_render_layer(CONTENT_LAYER));
                        if (contextRow) {
                            auto span = [&](int id, const std::string& text,
                                            afterhours::Color bg,
                                            afterhours::Color fg,
                                            const std::string& debug) {
                                if (text.empty()) return;
                                div(ctx, mk(contextRow.ent(), id),
                                    ComponentConfig{}
                                        .with_debug_name(debug)
                                        .with_label(text)
                                        // Text-height bg (children() tall),
                                        // not full-field height: a rounded
                                        // full-height div overflowed the
                                        // sunken field and theme roundness
                                        // made it a blob.
                                        .with_size(ComponentSize{children(), children()})
                                        .with_roundness(0.0f)
                                        .with_padding(Padding::all(pixels(0)))
                                        .with_margin(Margin{.top = pixels(0), .right = pixels(0), .bottom = pixels(0), .left = pixels(0)})
                                        .with_custom_background(bg)
                                        .with_custom_text_color(fg)
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_render_layer(CONTENT_LAYER));
                            };
                            std::string before = sentence.substr(
                                0, grammarCtx.highlightStart);
                            std::string marked = sentence.substr(
                                grammarCtx.highlightStart,
                                grammarCtx.highlightLength);
                            std::string after = sentence.substr(
                                grammarCtx.highlightStart +
                                grammarCtx.highlightLength);
                            span(0, before, ui_imm::win95_colors::TEXT_AREA,
                                 ui_imm::win95_colors::TEXT,
                                 "grammar_context_before");
                            span(1, marked, ui_imm::win95_colors::HIGHLIGHT,
                                 ui_imm::win95_colors::TEXT_WHITE,
                                 "grammar_context_highlight");
                            span(2, after, ui_imm::win95_colors::TEXT_AREA,
                                 ui_imm::win95_colors::TEXT,
                                 "grammar_context_after");
                        }
                    }
                    // Human wording for the issue (never the enum name).
                    fieldLabel(2, "Issue:");
                    sunkenField(3, issue, "grammar_issue", 26.0f);
                    // Suggestion / message field.
                    fieldLabel(4, "Suggestion:");
                    sunkenField(5, suggestion, "grammar_suggestion", 26.0f);
                    }  // leftCol

                    // Right column: vertical stack of compact 90px
                    // gray buttons. Ignore Once is the default button
                    // (dark outline). Change is disabled with no
                    // concrete suggestion; in the clean state
                    // Ignore/Change are all disabled, so no stale
                    // issue stays actionable — only Close works.
                    auto rightCol = div(ctx, mk(bodyRow.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(100), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(6))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (rightCol) {
                        if (button(ctx, mk(rightCol.ent(), 0),
                            ah_win95::win95DialogButtonStyle(canIgnore)
                                .with_debug_name("grammar_ignore_once")
                                .with_label("Ignore Once")
                                .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                                .with_render_layer(CONTENT_LAYER))) {
                            if (canIgnore) {
                                cmd::grammarIgnoreOnce(doc, dialogs);
                            }
                        }
                        if (button(ctx, mk(rightCol.ent(), 1),
                            ah_win95::win95DialogButtonStyle(canIgnore)
                                .with_debug_name("grammar_ignore_all")
                                .with_label("Ignore All")
                                .with_render_layer(CONTENT_LAYER))) {
                            if (canIgnore) {
                                cmd::grammarIgnoreAll(doc, dialogs);
                            }
                        }
                        if (button(ctx, mk(rightCol.ent(), 2),
                            ah_win95::win95DialogButtonStyle(canChange)
                                .with_debug_name("grammar_change")
                                .with_label("Change")
                                .with_render_layer(CONTENT_LAYER))) {
                            if (canChange) cmd::grammarChange(doc, dialogs);
                        }
                        if (button(ctx, mk(rightCol.ent(), 3),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("grammar_close")
                                .with_label("Close")
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.showGrammarDialog = false;
                        }
                    }  // rightCol
                    }  // bodyRow
                }  // field layout (issue or clean state)
            }
        }

        // Thesaurus dialog (Tools > Thesaurus..., Shift+F7) - Win95
        // shell, Spelling pattern: left column (current word field,
        // synonyms sunken list, Look Up field), right column of gray
        // buttons (Look Up, Replace, Close). Clicking a synonym
        // selects it (highlight); Replace swaps the word at the
        // caret for the selected synonym through the command layer
        // (one undo step). A word with no entry shows
        // "(no thesaurus entry)" in the list and disables Replace.
        if (dialogs.showThesaurusDialog) {
            constexpr int THESAURUS_MODAL_ID = 50013;
            auto result = afterhours::modal(ctx, mk(entity, THESAURUS_MODAL_ID),
                dialogs.showThesaurusDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(430), afterhours::ui::h720(300),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Thesaurus", CONTENT_LAYER);

                test_input::register_visible_text(dialogs.thesaurusCurrentWord);
                for (const auto& syn : dialogs.thesaurusSynonyms) {
                    test_input::register_visible_text(syn);
                }
                if (dialogs.thesaurusSynonyms.empty()) {
                    test_input::register_visible_text("(no thesaurus entry)");
                }

                auto bodyRow = div(ctx, mk(result.ent(), 903),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), children()})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (bodyRow) {
                auto leftCol = div(ctx, mk(bodyRow.ent(), 0),
                    ComponentConfig{}
                        .with_size(ComponentSize{w1280(302), children()})
                        .with_flex_direction(FlexDirection::Column)
                        .with_gap(pixels(4))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (leftCol) {
                    div(ctx, mk(leftCol.ent(), 0),
                        ComponentConfig{}
                            .with_label("Current word:")
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                    div(ctx, mk(leftCol.ent(), 1),
                        ah_win95::win95SunkenStyle()
                            .with_label(dialogs.thesaurusCurrentWord)
                            .with_size(ComponentSize{percent(1.0f), h720(26)})
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_padding(Padding{.left = pixels(4)})
                            .with_render_layer(CONTENT_LAYER));
                    div(ctx, mk(leftCol.ent(), 2),
                        ComponentConfig{}
                            .with_label("Synonyms:")
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                    float synHeight = 22.0f * static_cast<float>(
                        std::max<std::size_t>(dialogs.thesaurusSynonyms.size(), 1)) + 4.0f;
                    auto synList = div(ctx, mk(leftCol.ent(), 3),
                        ah_win95::win95SunkenStyle()
                            .with_size(ComponentSize{percent(1.0f), h720(synHeight)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                            .with_render_layer(CONTENT_LAYER));
                    if (synList) {
                        if (dialogs.thesaurusSynonyms.empty()) {
                            div(ctx, mk(synList.ent(), 0),
                                ComponentConfig{}
                                    .with_label("(no thesaurus entry)")
                                    .with_size(ComponentSize{percent(1.0f), h720(22)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_padding(Padding{.left = pixels(4)})
                                    .with_render_layer(CONTENT_LAYER));
                        } else {
                            for (std::size_t i = 0; i < dialogs.thesaurusSynonyms.size(); ++i) {
                                const std::string& syn = dialogs.thesaurusSynonyms[i];
                                bool selected =
                                    dialogs.thesaurusSelected == static_cast<int>(i);
                                if (button(ctx, mk(synList.ent(), 100 + static_cast<int>(i)),
                                    ComponentConfig{}
                                        .with_debug_name("thesaurus_synonym_" + syn)
                                        .with_label(syn)
                                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_padding(Padding{.left = pixels(4)})
                                        .with_cursor(afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    dialogs.thesaurusSelected = static_cast<int>(i);
                                }
                            }
                        }
                    }
                    div(ctx, mk(leftCol.ent(), 4),
                        ComponentConfig{}
                            .with_label("Look Up:")
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                    afterhours::text_input::text_input(ctx, mk(leftCol.ent(), 5),
                        dialogs.thesaurusLookupStr,
                        ah_win95::win95SunkenStyle()
                            .with_debug_name("thesaurus_lookup_input")
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_render_layer(CONTENT_LAYER));
                }  // leftCol

                auto rightCol = div(ctx, mk(bodyRow.ent(), 1),
                    ComponentConfig{}
                        .with_size(ComponentSize{w1280(100), children()})
                        .with_flex_direction(FlexDirection::Column)
                        .with_gap(pixels(6))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (rightCol) {
                    if (button(ctx, mk(rightCol.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("thesaurus_lookup")
                            .with_label("Look Up")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        cmd::thesaurusLookup(doc, dialogs);
                    }
                    bool canReplace = dialogs.thesaurusHasWord &&
                                      dialogs.thesaurusSelected >= 0 &&
                                      static_cast<std::size_t>(dialogs.thesaurusSelected) <
                                          dialogs.thesaurusSynonyms.size();
                    if (button(ctx, mk(rightCol.ent(), 1),
                        ah_win95::win95DialogButtonStyle(canReplace)
                            .with_debug_name("thesaurus_replace")
                            .with_label("Replace")
                            .with_render_layer(CONTENT_LAYER))) {
                        if (canReplace) cmd::thesaurusReplace(doc, dialogs);
                    }
                    if (button(ctx, mk(rightCol.ent(), 2),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("thesaurus_close")
                            .with_label("Close")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showThesaurusDialog = false;
                    }
                }  // rightCol
                }  // bodyRow
            }
        }

        // Table Formula dialog (Table > Formula...): one Formula
        // field (prefilled =SUM(ABOVE), editable), OK evaluates
        // against the caret cell and writes the result into it
        // (one undo step); a failed evaluation leaves the dialog
        // open with the error in red below the field.
        if (dialogs.showTableFormulaDialog) {
            constexpr int FORMULA_MODAL_ID = 50014;
            auto result = afterhours::modal(ctx, mk(entity, FORMULA_MODAL_ID),
                dialogs.showTableFormulaDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(380), afterhours::ui::h720(180),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Formula", CONTENT_LAYER);
                test_input::register_visible_text(dialogs.tableFormulaStr);
                if (!dialogs.tableFormulaError.empty()) {
                    test_input::register_visible_text(dialogs.tableFormulaError);
                }

                div(ctx, mk(result.ent(), 901),
                    ComponentConfig{}
                        .with_label("Formula:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));
                afterhours::text_input::text_input(ctx, mk(result.ent(), 902),
                    dialogs.tableFormulaStr,
                    ah_win95::win95SunkenStyle()
                        .with_debug_name("table_formula_input")
                        .with_size(ComponentSize{percent(1.0f), h720(28)})
                        .with_render_layer(CONTENT_LAYER));
                if (!dialogs.tableFormulaError.empty()) {
                    div(ctx, mk(result.ent(), 903),
                        ComponentConfig{}
                            .with_label(dialogs.tableFormulaError)
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(
                                ui_imm::win95_colors::ERROR_COLOR)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                }
                auto buttonRow = div(ctx, mk(result.ent(), 904),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::FlexEnd)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("table_formula_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        cmd::applyTableFormula(doc, dialogs);
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("table_formula_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showTableFormulaDialog = false;
                        dialogs.tableFormulaError.clear();
                    }
                }
            }
        }

        // AutoCorrect dialog (Tools > AutoCorrect...) - genuine
        // Word 95 layout: 3 option checkboxes, a scrollable sunken
        // Replace/With list, editable Replace:/With: fields with
        // Add/Delete, and OK/Cancel at the bottom right. Toggles bind
        // directly to the AutoCorrect settings, so OK/Cancel just
        // dismiss; Add/Delete apply immediately to the session-only
        // user entries.
        if (dialogs.showAutoCorrectDialog) {
            constexpr int AUTOCORRECT_MODAL_ID = 50011;
            auto result = afterhours::modal(ctx, mk(entity, AUTOCORRECT_MODAL_ID),
                dialogs.showAutoCorrectDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(400), afterhours::ui::h720(400),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "AutoCorrect", CONTENT_LAYER);

                // Built-ins + session user entries, sorted by Replace.
                const auto entries = doc.autoCorrect.entries();
                // A stale selection (e.g. after Delete) clears itself.
                if (dialogs.autoCorrectSelected >=
                    static_cast<int>(entries.size())) {
                    dialogs.autoCorrectSelected = -1;
                }
                const bool selectedIsUser =
                    dialogs.autoCorrectSelected >= 0 &&
                    doc.autoCorrect.isUserEntry(
                        entries[static_cast<std::size_t>(
                            dialogs.autoCorrectSelected)]
                            .first);
                auto hasText = [](const std::string& text) {
                    return text.find_first_not_of(" \t\r\n") !=
                           std::string::npos;
                };
                const bool canAdd =
                    hasText(dialogs.autoCorrectReplaceStr) &&
                    hasText(dialogs.autoCorrectWithStr);

                // Small native checkboxes immediately left of each label.
                ah_win95::win95Checkbox(
                    ctx, mk(result.ent(), 0),
                    doc.autoCorrect.settings.replaceTextAsYouType,
                    "autocorrect_replace", "Replace text as you type",
                    CONTENT_LAYER);
                ah_win95::win95Checkbox(
                    ctx, mk(result.ent(), 1),
                    doc.autoCorrect.settings.capitalizeFirstLetterOfSentence,
                    "autocorrect_capitalize",
                    "Capitalize first letter of sentences", CONTENT_LAYER);
                ah_win95::win95Checkbox(
                    ctx, mk(result.ent(), 2),
                    doc.autoCorrect.settings.correctTwoInitialCapitals,
                    "autocorrect_two_initials",
                    "Correct TWo INitial CApitals", CONTENT_LAYER);

                // Sunken list: one Replace | With pair per row.
                // The header stays fixed; the rows scroll (Overflow::Auto).
                // Clicking a row selects it and fills the Replace:/With:
                // fields; the selected row is navy/white.
                auto listBox = div(ctx, mk(result.ent(), 10),
                    ah_win95::win95SunkenStyle()
                        .with_size(ComponentSize{percent(1.0f), h720(186)})
                        .with_flex_direction(FlexDirection::Column)
                        .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                        .with_margin(Margin{.top = pixels(4)})
                        .with_render_layer(CONTENT_LAYER));
                if (listBox) {
                    auto headerRow = div(ctx, mk(listBox.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_custom_background(
                                ui_imm::win95_colors::TEXT_AREA)
                            .with_render_layer(CONTENT_LAYER));
                    if (headerRow) {
                        for (int col = 0; col < 2; ++col) {
                            div(ctx, mk(headerRow.ent(), col),
                                ComponentConfig{}
                                    .with_label(col == 0 ? "Replace" : "With")
                                    .with_size(ComponentSize{percent(0.5f), h720(20)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_font_weight(afterhours::colors::FontWeight::Bold)
                                    .with_font_size(h720(14))
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_padding(Padding{.left = pixels(4)})
                                    .with_render_layer(CONTENT_LAYER));
                        }
                    }
                    auto scrollArea = div(ctx, mk(listBox.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(160)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_overflow(afterhours::ui::Overflow::Auto)
                            .with_custom_background(
                                ui_imm::win95_colors::TEXT_AREA)
                            .with_render_layer(CONTENT_LAYER));
                    if (scrollArea) {
                        for (std::size_t i = 0; i < entries.size(); ++i) {
                            const bool selected =
                                dialogs.autoCorrectSelected ==
                                static_cast<int>(i);
                            auto entryRow = button(
                                ctx, mk(scrollArea.ent(), 100 + static_cast<int>(i)),
                                ComponentConfig{}
                                    .with_debug_name("autocorrect_entry_" +
                                                    entries[i].first)
                                    .with_label("")
                                    .with_size(ComponentSize{percent(1.0f), h720(20)})
                                    .with_flex_direction(FlexDirection::Row)
                                    .with_roundness(0.0f)
                                    // No theme button padding: a 20px row
                                    // with 32px of default vertical padding
                                    // leaves no content area for the cells.
                                    .with_padding(Padding::all(pixels(0)))
                                    .with_custom_background(selected
                                        ? ui_imm::win95_colors::HIGHLIGHT
                                        : ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(selected
                                        ? ui_imm::win95_colors::TEXT_WHITE
                                        : ui_imm::win95_colors::TEXT)
                                    .with_cursor(afterhours::ui::CursorType::Pointer)
                                    .with_render_layer(CONTENT_LAYER));
                            if (entryRow) {
                                dialogs.autoCorrectSelected =
                                    static_cast<int>(i);
                                dialogs.autoCorrectReplaceStr = entries[i].first;
                                dialogs.autoCorrectWithStr = entries[i].second;
                            }
                            for (int col = 0; col < 2; ++col) {
                                div(ctx, mk(entryRow.ent(), col),
                                    ComponentConfig{}
                                        .with_label(col == 0 ? entries[i].first
                                                            : entries[i].second)
                                        .with_size(ComponentSize{percent(0.5f), h720(20)})
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_font_size(h720(14))
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_padding(Padding{.left = pixels(4)})
                                        .with_render_layer(CONTENT_LAYER));
                            }
                        }
                    }
                }

                // Editable Replace:/With: fields (left) + Add/Delete
                // (right). Add appends a custom entry that autocorrects;
                // Delete removes the selected custom entry and is
                // disabled for built-ins (and with no selection).
                auto fieldsRow = div(ctx, mk(result.ent(), 11),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), children()})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_margin(Margin{.top = pixels(8)})
                        .with_render_layer(CONTENT_LAYER));
                if (fieldsRow) {
                    auto fieldsCol = div(ctx, mk(fieldsRow.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(278), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(6))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (fieldsCol) {
                        auto replaceRow = div(ctx, mk(fieldsCol.ent(), 0),
                            ComponentConfig{}
                                .with_size(ComponentSize{percent(1.0f), h720(28)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_align_items(AlignItems::Center)
                                .with_gap(pixels(6))
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_render_layer(CONTENT_LAYER));
                        if (replaceRow) {
                            div(ctx, mk(replaceRow.ent(), 0),
                                ComponentConfig{}
                                    .with_label("Replace:")
                                    .with_size(ComponentSize{w1280(64), h720(20)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_render_layer(CONTENT_LAYER));
                            afterhours::text_input::text_input(
                                ctx, mk(replaceRow.ent(), 1),
                                dialogs.autoCorrectReplaceStr,
                                ah_win95::win95SunkenStyle()
                                    .with_debug_name("autocorrect_replace_input")
                                    .with_size(ComponentSize{w1280(208), h720(28)})
                                    .with_render_layer(CONTENT_LAYER));
                        }
                        auto withRow = div(ctx, mk(fieldsCol.ent(), 1),
                            ComponentConfig{}
                                .with_size(ComponentSize{percent(1.0f), h720(28)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_align_items(AlignItems::Center)
                                .with_gap(pixels(6))
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_render_layer(CONTENT_LAYER));
                        if (withRow) {
                            div(ctx, mk(withRow.ent(), 0),
                                ComponentConfig{}
                                    .with_label("With:")
                                    .with_size(ComponentSize{w1280(64), h720(20)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_render_layer(CONTENT_LAYER));
                            afterhours::text_input::text_input(
                                ctx, mk(withRow.ent(), 1),
                                dialogs.autoCorrectWithStr,
                                ah_win95::win95SunkenStyle()
                                    .with_debug_name("autocorrect_with_input")
                                    .with_size(ComponentSize{w1280(208), h720(28)})
                                    .with_render_layer(CONTENT_LAYER));
                        }
                    }
                    auto addDeleteCol = div(ctx, mk(fieldsRow.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(90), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(6))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (addDeleteCol) {
                        if (button(ctx, mk(addDeleteCol.ent(), 0),
                            ah_win95::win95DialogButtonStyle(canAdd)
                                .with_debug_name("autocorrect_add")
                                .with_label("Add")
                                .with_render_layer(CONTENT_LAYER))) {
                            doc.autoCorrect.addUserEntry(
                                dialogs.autoCorrectReplaceStr,
                                dialogs.autoCorrectWithStr);
                            // Select the entry just added.
                            const auto updated = doc.autoCorrect.entries();
                            for (std::size_t i = 0; i < updated.size(); ++i) {
                                std::string key = updated[i].first;
                                std::string wanted =
                                    dialogs.autoCorrectReplaceStr;
                                auto lower = [](std::string s) {
                                    for (char& ch : s) {
                                        ch = static_cast<char>(std::tolower(
                                            static_cast<unsigned char>(ch)));
                                    }
                                    return s;
                                };
                                if (lower(key) == lower(wanted)) {
                                    dialogs.autoCorrectSelected =
                                        static_cast<int>(i);
                                    break;
                                }
                            }
                        }
                        if (button(ctx, mk(addDeleteCol.ent(), 1),
                            ah_win95::win95DialogButtonStyle(selectedIsUser)
                                .with_debug_name("autocorrect_delete")
                                .with_label("Delete")
                                .with_render_layer(CONTENT_LAYER))) {
                            if (selectedIsUser) {
                                doc.autoCorrect.removeUserEntry(
                                    entries[static_cast<std::size_t>(
                                        dialogs.autoCorrectSelected)]
                                        .first);
                                dialogs.autoCorrectSelected = -1;
                                dialogs.autoCorrectReplaceStr.clear();
                                dialogs.autoCorrectWithStr.clear();
                            }
                        }
                    }
                }

                // OK/Cancel: compact 90px buttons at the bottom right.
                // OK is the default button (dark outline).
                auto buttonRow = div(ctx, mk(result.ent(), 3),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::FlexEnd)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_margin(Margin{.top = pixels(4)})
                        .with_render_layer(CONTENT_LAYER));

                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("autocorrect_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showAutoCorrectDialog = false;
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("autocorrect_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showAutoCorrectDialog = false;
                    }
                }
            }
        }

        // Options dialog (Tools > Options...) - Win95 shell with
        // live checkboxes. Each checkbox binds directly to its
        // setting, so toggling flips the state immediately; OK
        // just dismisses.
        if (dialogs.showOptionsDialog) {
            constexpr int OPTIONS_MODAL_ID = 50013;
            auto result = afterhours::modal(ctx, mk(entity, OPTIONS_MODAL_ID),
                dialogs.showOptionsDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(400), afterhours::ui::h720(220),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Options", CONTENT_LAYER);

                test_input::register_visible_text("Show text boundaries");
                test_input::register_visible_text("Spell check as you type");
                test_input::register_visible_text("AutoCorrect as you type");
                ah_win95::win95Checkbox(
                    ctx, mk(result.ent(), 0),
                    doc.docSettings.showTextBoundaries,
                    "options_text_boundaries", "Show text boundaries",
                    CONTENT_LAYER);
                ah_win95::win95Checkbox(
                    ctx, mk(result.ent(), 1),
                    doc.docSettings.spellCheckAsYouType,
                    "options_spell_as_you_type", "Spell check as you type",
                    CONTENT_LAYER);
                ah_win95::win95Checkbox(
                    ctx, mk(result.ent(), 2),
                    doc.autoCorrect.settings.replaceTextAsYouType,
                    "options_autocorrect_as_you_type",
                    "AutoCorrect as you type", CONTENT_LAYER);

                auto buttonRow = div(ctx, mk(result.ent(), 3),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::FlexEnd)
                        .with_align_items(AlignItems::Center)
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_margin(Margin{.top = pixels(4)})
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("options_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showOptionsDialog = false;
                    }
                }
            }
        }

        // Mail Merge Helper (Tools > Mail Merge...) - UNREACHABLE
        // while the menu item is disabled (enabled=false, no dispatch
        // in menu_actions.h): kept, not deleted, so re-enabling it
        // later is a menu/dispatch change, not a dialog rewrite.
        // Was a parity stub in a Win95 shell; no merge engine exists.
        if (dialogs.showMailMergeDialog) {
            constexpr int MAILMERGE_MODAL_ID = 50014;
            auto result = afterhours::modal(ctx, mk(entity, MAILMERGE_MODAL_ID),
                dialogs.showMailMergeDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(400), afterhours::ui::h720(220),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Mail Merge Helper",
                                              CONTENT_LAYER);

                test_input::register_visible_text("Mail Merge Helper");
                test_input::register_visible_text(
                    "A data source is required");
                div(ctx, mk(result.ent(), 901),
                    ComponentConfig{}
                        .with_label("A data source is required before "
                                    "you can merge.")
                        .with_size(ComponentSize{percent(1.0f), h720(24)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_render_layer(CONTENT_LAYER));
                div(ctx, mk(result.ent(), 902),
                    ComponentConfig{}
                        .with_label("Create a main document, then get data "
                                    "to begin a merge.")
                        .with_size(ComponentSize{percent(1.0f), h720(24)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_render_layer(CONTENT_LAYER));

                auto buttonRow = div(ctx, mk(result.ent(), 903),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_margin(Margin{.top = pixels(4)})
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    // Disabled: no merge engine, no data source.
                    button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle(false)
                            .with_debug_name("mail_merge_create")
                            .with_label("Create")
                            .with_render_layer(CONTENT_LAYER));
                    button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle(false)
                            .with_debug_name("mail_merge_get_data")
                            .with_label("Get Data")
                            .with_render_layer(CONTENT_LAYER));
                    if (button(ctx, mk(buttonRow.ent(), 2),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("mail_merge_close")
                            .with_label("Close")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showMailMergeDialog = false;
                    }
                }
            }
        }

        // Comment input dialog - Win95 shell: square BUTTON_FACE
        // panel, navy caption, no backdrop dim, sunken field,
        // raised 90px buttons.
        if (dialogs.showCommentDialog) {
            constexpr int COMMENT_MODAL_ID = 50002;
            auto result = afterhours::modal(ctx, mk(entity, COMMENT_MODAL_ID),
                dialogs.showCommentDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(360), afterhours::ui::h720(130),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Add Comment", CONTENT_LAYER);

                // Prompt label
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("Comment:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                // Text input
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.commentInputStr,
                    ah_win95::win95SunkenStyle()
                        .with_debug_name("comment_input")
                        .with_size(ComponentSize{percent(1.0f), h720(28)})
                        .with_render_layer(CONTENT_LAYER));

                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = pixels(4)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));

                if (button(ctx, mk(buttonRow.ent(), 0),
                    ah_win95::win95DialogButtonStyle()
                        .with_debug_name("comment_ok")
                        .with_label("OK")
                        .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - add comment
                    if (!dialogs.commentInputStr.empty()) {
                        Comment comment;
                        comment.startOffset = dialogs.pendingCommentStart;
                        comment.endOffset = dialogs.pendingCommentEnd;
                        comment.author = "Gabe";
                        comment.text = dialogs.commentInputStr;
                        comment.createdAt = std::time(nullptr);
                        doc.comments.push_back(comment);
                        // Confirmation to the status-bar flash, not
                        // a floating toast.
                        if (cache_.status)
                            status::flash(*cache_.status, "Comment added",
                                          afterhours::graphics::get_time());
                    }
                    dialogs.commentInputStr.clear();
                    dialogs.showCommentDialog = false;
                }

                if (button(ctx, mk(buttonRow.ent(), 1),
                    ah_win95::win95DialogButtonStyle()
                        .with_debug_name("comment_cancel")
                        .with_label("Cancel")
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.commentInputStr.clear();
                    dialogs.showCommentDialog = false;
                }
            }
        }
        
        // New from Template dialog (File > New from Template...) -
        // Win95 shell: sunken list of templates, OK opens the
        // selected template as a NEW Untitled document (filePath
        // cleared, dirty). Real Word's New dialog has tabs +
        // template icons; a clean categorized list (name + one-line
        // description) is acceptable parity here, not a debug
        // picker: the list is compact (height fits the entries,
        // ~6 rows max) and each row reads "Name — description".
        if (dialogs.showTemplateDialog) {
            constexpr int TEMPLATE_MODAL_ID = 50003;
            auto result = afterhours::modal(ctx, mk(entity, TEMPLATE_MODAL_ID),
                dialogs.showTemplateDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(460), afterhours::ui::h720(200),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "New from Template", CONTENT_LAYER);

                test_input::register_visible_text("Templates:");
                div(ctx, mk(result.ent(), 901),
                    ComponentConfig{}
                        .with_label("Templates:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                float listHeight = 22.0f * static_cast<float>(
                    std::max<std::size_t>(dialogs.templateNames.size(), 1)) +
                    4.0f;
                auto templateList = div(ctx, mk(result.ent(), 902),
                    ah_win95::win95SunkenStyle()
                        .with_size(ComponentSize{percent(1.0f), h720(listHeight)})
                        .with_flex_direction(FlexDirection::Column)
                        .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                        .with_render_layer(CONTENT_LAYER));
                if (templateList) {
                    if (dialogs.templateNames.empty()) {
                        div(ctx, mk(templateList.ent(), 0),
                            ComponentConfig{}
                                .with_label("(no templates found)")
                                .with_size(ComponentSize{percent(1.0f), h720(22)})
                                .with_custom_background(
                                    ui_imm::win95_colors::TEXT_AREA)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(afterhours::ui::TextAlignment::Left)
                                .with_padding(Padding{.left = pixels(4)})
                                .with_render_layer(CONTENT_LAYER));
                    }
                    for (std::size_t i = 0; i < dialogs.templateNames.size(); ++i) {
                        const std::string& name = dialogs.templateNames[i];
                        bool selected =
                            static_cast<int>(i) == dialogs.templateSelected;
                        // Row label is "Name — description"; register
                        // both parts so E2E can expect/click either.
                        const std::string desc =
                            templates::templateDescriptionFor(name);
                        const std::string label =
                            desc.empty() ? name : name + " — " + desc;
                        test_input::register_visible_text(name);
                        if (!desc.empty()) {
                            test_input::register_visible_text(desc);
                        }
                        // Debug names are single tokens (E2E click_ui):
                        // spaces become underscores.
                        std::string debugName = "template_item_" + name;
                        for (char& c : debugName) {
                            if (c == ' ') c = '_';
                        }
                        if (button(ctx, mk(templateList.ent(), 100 + static_cast<int>(i)),
                            ComponentConfig{}
                                .with_debug_name(debugName)
                                .with_label(label)
                                .with_size(ComponentSize{percent(1.0f), h720(22)})
                                // Padding on a label is a no-op in
                                // afterhours; text_inset is what
                                // actually insets the text. Without
                                // it the selected row's text sat on
                                // the list's left edge (clipped).
                                // Vertical inset stays 2: the row is
                                // only ~20px tall and a 14px font
                                // needs >12px of usable height.
                                .with_text_inset(8.0f, 2.0f)
                                .with_roundness(0.0f)
                                .with_custom_background(selected
                                    ? ui_imm::win95_colors::HIGHLIGHT
                                    : ui_imm::win95_colors::TEXT_AREA)
                                .with_custom_text_color(selected
                                    ? ui_imm::win95_colors::TEXT_WHITE
                                    : ui_imm::win95_colors::TEXT)
                                .with_alignment(afterhours::ui::TextAlignment::Left)
                                .with_padding(Padding{.left = pixels(4)})
                                .with_cursor(afterhours::ui::CursorType::Pointer)
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.templateSelected = static_cast<int>(i);
                        }
                    }
                }

                auto buttonRow = div(ctx, mk(result.ent(), 903),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("template_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        if (dialogs.templateSelected >= 0 &&
                            dialogs.templateSelected < static_cast<int>(
                                dialogs.templateNames.size())) {
                            cmd::openTemplateAsNew(
                                doc, menu,
                                dialogs.templateNames[static_cast<std::size_t>(
                                    dialogs.templateSelected)]);
                        }
                        dialogs.showTemplateDialog = false;
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("template_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showTemplateDialog = false;
                    }
                }
            }
        }

        // Date and Time dialog (Insert > Date and Time...) - Win95
        // shell: label + sunken list of formats computed from the
        // current local time at open, "Update automatically"
        // checkbox, OK/Cancel. "Update automatically" inserts the
        // same static text: no field engine exists, so there is no
        // live-updating field to insert (see cmd::insertDateTime).
        if (dialogs.showDateTimeDialog) {
            constexpr int DATETIME_MODAL_ID = 50015;
            auto result = afterhours::modal(ctx, mk(entity, DATETIME_MODAL_ID),
                dialogs.showDateTimeDialog,
                // Height fits content: title 20 + label 20 + list
                // 136 (6 x 22 + 4 padding) + checkbox 22 + buttons
                // 44 + panel padding 8 = 250, +12 breathing room.
                // The old fixed 300 left a large empty gray band.
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(420), afterhours::ui::h720(262),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Date and Time", CONTENT_LAYER);

                test_input::register_visible_text("Available formats:");
                div(ctx, mk(result.ent(), 901),
                    ComponentConfig{}
                        .with_label("Available formats:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                float listHeight = 22.0f * static_cast<float>(
                    std::max<std::size_t>(dialogs.dateTimeFormats.size(), 1)) +
                    4.0f;
                auto formatList = div(ctx, mk(result.ent(), 902),
                    ah_win95::win95SunkenStyle()
                        .with_size(ComponentSize{percent(1.0f), h720(listHeight)})
                        .with_flex_direction(FlexDirection::Column)
                        .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                        .with_render_layer(CONTENT_LAYER));
                if (formatList) {
                    for (std::size_t i = 0; i < dialogs.dateTimeFormats.size(); ++i) {
                        const std::string& format = dialogs.dateTimeFormats[i];
                        bool selected =
                            static_cast<int>(i) == dialogs.dateTimeSelected;
                        test_input::register_visible_text(format);
                        if (button(ctx, mk(formatList.ent(), 100 + static_cast<int>(i)),
                            ComponentConfig{}
                                .with_debug_name("date_time_format_" +
                                                std::to_string(i))
                                .with_label(format)
                                .with_size(ComponentSize{percent(1.0f), h720(22)})
                                .with_roundness(0.0f)
                                .with_custom_background(selected
                                    ? ui_imm::win95_colors::HIGHLIGHT
                                    : ui_imm::win95_colors::TEXT_AREA)
                                .with_custom_text_color(selected
                                    ? ui_imm::win95_colors::TEXT_WHITE
                                    : ui_imm::win95_colors::TEXT)
                                .with_alignment(afterhours::ui::TextAlignment::Left)
                                .with_padding(Padding{.left = pixels(4)})
                                .with_cursor(afterhours::ui::CursorType::Pointer)
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.dateTimeSelected = static_cast<int>(i);
                        }
                    }
                }

                test_input::register_visible_text("Update automatically");
                ah_win95::win95Checkbox(
                    ctx, mk(result.ent(), 903),
                    dialogs.dateTimeUpdateAutomatically,
                    "date_time_update_auto", "Update automatically",
                    CONTENT_LAYER);

                auto buttonRow = div(ctx, mk(result.ent(), 904),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("date_time_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        if (dialogs.dateTimeSelected >= 0 &&
                            dialogs.dateTimeSelected < static_cast<int>(
                                dialogs.dateTimeFormats.size())) {
                            cmd::insertDateTime(
                                doc,
                                dialogs.dateTimeFormats[static_cast<std::size_t>(
                                    dialogs.dateTimeSelected)]);
                        }
                        dialogs.showDateTimeDialog = false;
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("date_time_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showDateTimeDialog = false;
                    }
                }
            }
        }

        // Font dialog (Format > Font...) - Word 95 layout: Font /
        // Font style / Size lists, each with a pending-value field
        // above it (Word's editable fields; here read-only except
        // Size, which is the custom-size edit), Effects
        // checkboxes, Underline-style + Font-color dropdowns
        // (the Font color dropdown is the ONLY color control --
        // the old swatch row is gone), and a Preview box. The
        // font list comes from the toolbar fonts vector; OK
        // applies the selection via buffer.setTextStyle, the same
        // path the toolbar uses. ONE pending state: every
        // list/dropdown click writes
        // dialogs.fontDialogSelection, and the fields + Preview
        // read only that state, so they can never disagree with
        // the lists (the old dialog had no fields, so the toolbar
        // labels were the only "field" and they showed the old
        // style while the lists showed the pending one).
        if (dialogs.showFontDialog) {
            constexpr int FONT_MODAL_ID = 50016;
            // Dialog height fits content: title 20 + lists row
            // (label 20 + field 26 + list 196 = 242) + Effects
            // (label 20 + row 48 = 68) + dropdown row 28 +
            // dropdown lists (underline 48 / color 92, when open)
            // + Preview (label 20 + box) + buttons 44 + panel
            // padding 8. The old fixed 500 clipped the Size
            // list's lower entries (8 rows x 22 + padding + the
            // text_input field's extra layout height > the old
            // 176 box), so the list boxes carry 16px of slack
            // over the 180 the rows nominally need.
            const int pendingPreviewSize =
                dialogs.fontDialogInitialized
                    ? dialogs.fontDialogSelection.size
                    : doc.buffer.textStyle().fontSize;
            // Preview box grows with the scaled preview font
            // (font_dialog::previewFontPx) so a large pending size
            // is never clipped by a box sized for the old size.
            const int previewPx = font_dialog::previewFontPx(
                pendingPreviewSize, Settings::get().get_ui_scale());
            const float previewBoxH = static_cast<float>(
                std::clamp(previewPx + 32, 48, 120));
            const float fontDialogH =
                20.0f + 242.0f + 68.0f + 28.0f + 20.0f +
                previewBoxH + 44.0f + 8.0f +
                (dialogs.fontDialogUnderlineDropdownOpen ? 48.0f : 0.0f) +
                (dialogs.fontDialogColorDropdownOpen ? 92.0f : 0.0f);
            auto result = afterhours::modal(ctx, mk(entity, FONT_MODAL_ID),
                dialogs.showFontDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(540),
                    afterhours::ui::h720(fontDialogH),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Font", CONTENT_LAYER);

                if (!dialogs.fontDialogInitialized) {
                    dialogs.fontDialogSelection =
                        font_dialog::fontDialogSelectionFromStyle(
                            doc.buffer.textStyle(), toolbar.fonts);
                    dialogs.fontDialogCustomSize = std::to_string(
                        dialogs.fontDialogSelection.size);
                    dialogs.fontDialogInitialized = true;
                }
                auto& sel = dialogs.fontDialogSelection;
                const auto colors = font_dialog::fontDialogColors();
                // A typed custom size is pending too: sync it into
                // sel every frame (guarded so clearing the field
                // while typing does not zero the size).
                {
                    int typed = std::atoi(dialogs.fontDialogCustomSize.c_str());
                    if (typed > 0) sel.size = typed;
                }

                auto listLabel = [&](auto& parentEnt, int id,
                                     const std::string& label) {
                    div(ctx, mk(parentEnt, id),
                        ComponentConfig{}
                            .with_label(label)
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                };
                // Pending-value field above a list: sunken, like the
                // list itself. The value is the ONE pending state,
                // so a list click updates this text on the next
                // frame. Registered as "X field: value" for E2E.
                auto pendingField = [&](auto& parentEnt, int id,
                                        const std::string& fieldLabel,
                                        const std::string& value) {
                    test_input::register_visible_text(fieldLabel + ": " +
                                                      value);
                    div(ctx, mk(parentEnt, id),
                        ah_win95::win95SunkenStyle()
                            .with_label(value)
                            .with_size(ComponentSize{percent(1.0f), h720(26)})
                            .with_text_inset(8.0f, 2.0f)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                };
                test_input::register_visible_text("Font:");
                test_input::register_visible_text("Font style:");
                test_input::register_visible_text("Size:");

                // Three list columns: Font | Font style | Size.
                auto listsRow = div(ctx, mk(result.ent(), 901),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(242)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (listsRow) {
                    // Font list.
                    auto fontCol = div(ctx, mk(listsRow.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(0.38f), h720(242)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (fontCol) {
                        listLabel(fontCol.ent(), 0, "Font:");
                        {
                            const std::string& pendingFont =
                                sel.fontIndex >= 0 &&
                                sel.fontIndex < static_cast<int>(toolbar.fonts.size())
                                    ? toolbar.fonts[static_cast<std::size_t>(sel.fontIndex)]
                                    : toolbar.currentFont;
                            pendingField(fontCol.ent(), 10, "Font field",
                                         pendingFont);
                        }
                        auto fontList = div(ctx, mk(fontCol.ent(), 1),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{percent(1.0f), h720(196)})
                                .with_flex_direction(FlexDirection::Column)
                                .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (fontList) {
                            for (std::size_t i = 0; i < toolbar.fonts.size(); ++i) {
                                const std::string& name = toolbar.fonts[i];
                                bool selected =
                                    static_cast<int>(i) == sel.fontIndex;
                                test_input::register_visible_text(name);
                                if (button(ctx, mk(fontList.ent(), 100 + static_cast<int>(i)),
                                    ComponentConfig{}
                                        .with_debug_name("font_item_" + name)
                                        .with_label(name)
                                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_padding(Padding{.left = pixels(4)})
                                        .with_cursor(afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    sel.fontIndex = static_cast<int>(i);
                                }
                            }
                        }
                    }
                    // Font style list: Regular / Bold / Italic / Bold Italic.
                    static const char* kStyleNames[4] = {
                        "Regular", "Bold", "Italic", "Bold Italic"};
                    static const char* kStyleDebug[4] = {
                        "font_style_Regular", "font_style_Bold",
                        "font_style_Italic", "font_style_BoldItalic"};
                    auto styleCol = div(ctx, mk(listsRow.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(0.28f), h720(242)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (styleCol) {
                        listLabel(styleCol.ent(), 2, "Font style:");
                        pendingField(styleCol.ent(), 11, "Style field",
                                     sel.styleIndex >= 0 && sel.styleIndex < 4
                                         ? kStyleNames[sel.styleIndex]
                                         : "Regular");
                        auto styleList = div(ctx, mk(styleCol.ent(), 3),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{percent(1.0f), h720(196)})
                                .with_flex_direction(FlexDirection::Column)
                                .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (styleList) {
                            for (int i = 0; i < 4; ++i) {
                                bool selected = i == sel.styleIndex;
                                test_input::register_visible_text(kStyleNames[i]);
                                if (button(ctx, mk(styleList.ent(), 200 + i),
                                    ComponentConfig{}
                                        .with_debug_name(kStyleDebug[i])
                                        .with_label(kStyleNames[i])
                                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_padding(Padding{.left = pixels(4)})
                                        .with_cursor(afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    sel.styleIndex = i;
                                }
                            }
                        }
                    }
                    // Size list: ONE vertical chronological column
                    // (8, 9, 10, ... 72 top to bottom), scrolling
                    // when the 16 entries exceed the box -- the old
                    // two-column split (8|20, 9|22, ...) read as two
                    // unrelated lists. The scroll container is the
                    // same Overflow::Auto pattern as the AutoCorrect
                    // and bookmark lists.
                    auto sizeCol = div(ctx, mk(listsRow.ent(), 2),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(0.34f), h720(242)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (sizeCol) {
                        listLabel(sizeCol.ent(), 4, "Size:");
                        // Size field: Word's editable field above the
                        // list — this IS the custom-size edit (the
                        // old separate "Custom size:" row below the
                        // lists is gone). Typing syncs into sel
                        // every frame; a list pick writes the field.
                        test_input::register_visible_text(
                            "Size field: " + std::to_string(sel.size));
                        afterhours::text_input::text_input(
                            ctx, mk(sizeCol.ent(), 6),
                            dialogs.fontDialogCustomSize,
                            ah_win95::win95SunkenStyle()
                                .with_debug_name("font_custom_size")
                                .with_size(ComponentSize{percent(1.0f),
                                                         h720(26)})
                                .with_render_layer(CONTENT_LAYER));
                        auto sizeList = div(ctx, mk(sizeCol.ent(), 5),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{percent(1.0f), h720(196)})
                                .with_flex_direction(FlexDirection::Column)
                                .with_overflow(afterhours::ui::Overflow::Auto)
                                .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (sizeList) {
                            for (std::size_t i = 0; i < toolbar.fontSizes.size(); ++i) {
                                int size = toolbar.fontSizes[i];
                                bool selected = size == sel.size;
                                std::string label = std::to_string(size);
                                test_input::register_visible_text(label);
                                if (button(ctx, mk(sizeList.ent(), 400 + static_cast<int>(i)),
                                    ComponentConfig{}
                                        .with_debug_name("font_size_" + label)
                                        .with_label(label)
                                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_padding(Padding{.left = pixels(4)})
                                        .with_cursor(afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    sel.size = size;
                                    dialogs.fontDialogCustomSize = label;
                                }
                            }
                        }
                    }
                }

                // (No separate Custom-size row: the Size field above
                // the Size list is the custom-size edit.)

                // Effects checkboxes, two columns of two.
                test_input::register_visible_text("Effects:");
                div(ctx, mk(result.ent(), 907),
                    ComponentConfig{}
                        .with_label("Effects:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));
                auto effectsRow = div(ctx, mk(result.ent(), 908),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(48)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (effectsRow) {
                    auto effectsLeft = div(ctx, mk(effectsRow.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(0.5f), h720(48)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (effectsLeft) {
                        ah_win95::win95Checkbox(
                            ctx, mk(effectsLeft.ent(), 0), sel.underline,
                            "font_underline", "Underline", CONTENT_LAYER);
                        ah_win95::win95Checkbox(
                            ctx, mk(effectsLeft.ent(), 1), sel.strikethrough,
                            "font_strikethrough", "Strikethrough",
                            CONTENT_LAYER);
                    }
                    auto effectsRight = div(ctx, mk(effectsRow.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(0.5f), h720(48)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (effectsRight) {
                        ah_win95::win95Checkbox(
                            ctx, mk(effectsRight.ent(), 0), sel.superscript,
                            "font_superscript", "Superscript", CONTENT_LAYER);
                        ah_win95::win95Checkbox(
                            ctx, mk(effectsRight.ent(), 1), sel.subscript,
                            "font_subscript", "Subscript", CONTENT_LAYER);
                    }
                }

                // Underline style + Font color dropdowns (the Word
                // controls): the Effects Underline checkbox and the
                // Font color dropdown are the same state through
                // sel.underline / sel.colorIndex, so both views
                // stay in sync (there is no swatch row). The lists expand in-flow
                // below the button row (the dialog height grows by
                // their height, computed before the modal call), so
                // they never clip or overlay other controls. The
                // triangle is drawn, not typed: the UI font has no
                // U+25BC glyph (same as the toolbar dropdowns).
                {
                    auto dropdownBtn = [&](auto& parentEnt, int id,
                                           const std::string& debugName,
                                           const std::string& value,
                                           bool open) {
                        return button(
                            ctx, mk(parentEnt, id),
                            ah_win95::win95DropdownButtonStyle(
                                120.0f, 26.0f, open)
                                .with_debug_name(debugName)
                                .with_label(value)
                                .with_text_inset(8.0f, 2.0f)
                                .with_on_draw_fg([](RectangleType r) {
                                    drawDropdownTriangle(
                                        r.x + r.width -
                                            theme::layout::scale(10),
                                        r.y + r.height / 2.0f,
                                        theme::BUTTON_TEXT);
                                })
                                .with_render_layer(CONTENT_LAYER));
                    };
                    auto dropRow = div(ctx, mk(result.ent(), 914),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (dropRow) {
                        auto underlineHalf = div(ctx, mk(dropRow.ent(), 0),
                            ComponentConfig{}
                                .with_size(ComponentSize{percent(0.5f), h720(28)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_align_items(AlignItems::Center)
                                .with_gap(pixels(6))
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_render_layer(CONTENT_LAYER));
                        if (underlineHalf) {
                            test_input::register_visible_text("Underline style:");
                            div(ctx, mk(underlineHalf.ent(), 0),
                                ComponentConfig{}
                                    .with_label("Underline style:")
                                    .with_size(ComponentSize{children(), h720(20)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_render_layer(CONTENT_LAYER));
                            test_input::register_visible_text(
                                "Underline style: " +
                                std::string(sel.underline ? "Single" : "None"));
                            if (dropdownBtn(underlineHalf.ent(), 1,
                                            "font_underline_dropdown",
                                            sel.underline ? "Single" : "None",
                                            dialogs.fontDialogUnderlineDropdownOpen)) {
                                dialogs.fontDialogUnderlineDropdownOpen =
                                    !dialogs.fontDialogUnderlineDropdownOpen;
                                dialogs.fontDialogColorDropdownOpen = false;
                            }
                        }
                        auto colorHalf = div(ctx, mk(dropRow.ent(), 1),
                            ComponentConfig{}
                                .with_size(ComponentSize{percent(0.5f), h720(28)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_align_items(AlignItems::Center)
                                .with_gap(pixels(6))
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_render_layer(CONTENT_LAYER));
                        if (colorHalf) {
                            test_input::register_visible_text("Font color:");
                            div(ctx, mk(colorHalf.ent(), 0),
                                ComponentConfig{}
                                    .with_label("Font color:")
                                    .with_size(ComponentSize{children(), h720(20)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_render_layer(CONTENT_LAYER));
                            const std::string colorName =
                                sel.colorIndex >= 0 &&
                                sel.colorIndex < static_cast<int>(colors.size())
                                    ? colors[static_cast<std::size_t>(sel.colorIndex)].first
                                    : "";
                            test_input::register_visible_text("Font color: " + colorName);
                            if (dropdownBtn(colorHalf.ent(), 1,
                                            "font_color_dropdown", colorName,
                                            dialogs.fontDialogColorDropdownOpen)) {
                                dialogs.fontDialogColorDropdownOpen =
                                    !dialogs.fontDialogColorDropdownOpen;
                                dialogs.fontDialogUnderlineDropdownOpen = false;
                            }
                        }
                    }
                    if (dialogs.fontDialogUnderlineDropdownOpen) {
                        auto uList = div(ctx, mk(result.ent(), 915),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{percent(1.0f), h720(48)})
                                .with_flex_direction(FlexDirection::Column)
                                .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (uList) {
                            static const char* kUnderlineNames[2] = {"None", "Single"};
                            for (int i = 0; i < 2; ++i) {
                                bool selected = (sel.underline == (i == 1));
                                test_input::register_visible_text(kUnderlineNames[i]);
                                if (button(ctx, mk(uList.ent(), 700 + i),
                                    ComponentConfig{}
                                        .with_debug_name(
                                            std::string("font_underline_") +
                                            kUnderlineNames[i])
                                        .with_label(kUnderlineNames[i])
                                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(afterhours::ui::TextAlignment::Left)
                                        .with_text_inset(8.0f, 2.0f)
                                        .with_cursor(afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    sel.underline = (i == 1);
                                    dialogs.fontDialogUnderlineDropdownOpen = false;
                                }
                            }
                        }
                    }
                    if (dialogs.fontDialogColorDropdownOpen) {
                        auto cList = div(ctx, mk(result.ent(), 916),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{percent(1.0f), h720(92)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (cList) {
                            for (int half = 0; half < 2; ++half) {
                                auto halfCol = div(ctx, mk(cList.ent(), 800 + half),
                                    ComponentConfig{}
                                        .with_size(ComponentSize{percent(0.5f), children()})
                                        .with_flex_direction(FlexDirection::Column)
                                        .with_custom_background(
                                            ui_imm::win95_colors::TEXT_AREA)
                                        .with_render_layer(CONTENT_LAYER));
                                if (!halfCol) continue;
                                std::size_t begin =
                                    static_cast<std::size_t>(half) *
                                    (colors.size() + 1) / 2;
                                std::size_t end =
                                    static_cast<std::size_t>(half + 1) *
                                    (colors.size() + 1) / 2;
                                for (std::size_t i = begin;
                                     i < end && i < colors.size(); ++i) {
                                    bool selected =
                                        static_cast<int>(i) == sel.colorIndex;
                                    test_input::register_visible_text(colors[i].first);
                                    if (button(ctx, mk(halfCol.ent(), 900 + static_cast<int>(i)),
                                        ComponentConfig{}
                                            // _option_ to not collide with
                                            // the swatch debug names
                                            // (font_color_<name>) below.
                                            .with_debug_name("font_color_option_" + colors[i].first)
                                            .with_label(colors[i].first)
                                            .with_size(ComponentSize{percent(1.0f), h720(22)})
                                            .with_roundness(0.0f)
                                            .with_custom_background(selected
                                                ? ui_imm::win95_colors::HIGHLIGHT
                                                : ui_imm::win95_colors::TEXT_AREA)
                                            .with_custom_text_color(selected
                                                ? ui_imm::win95_colors::TEXT_WHITE
                                                : ui_imm::win95_colors::TEXT)
                                            .with_alignment(afterhours::ui::TextAlignment::Left)
                                            .with_text_inset(8.0f, 2.0f)
                                            .with_cursor(afterhours::ui::CursorType::Pointer)
                                            .with_render_layer(CONTENT_LAYER))) {
                                        sel.colorIndex = static_cast<int>(i);
                                        dialogs.fontDialogColorDropdownOpen = false;
                                    }
                                }
                            }
                        }
                    }
                }

                // No color swatch row: the Font color dropdown above
                // is the only color control.

                // Preview box: "AaBbYyZz 123" in the PENDING font /
                // size / bold / underline / strikethrough / color
                // (the same sel the fields read above, so a list
                // click updates the preview on the next frame).
                // The family is requested by name, the way the
                // toolbar font dropdown names it. Gaegu-Bold (and
                // Gaegu, same file) ARE loaded faces now (preload),
                // so a Gaegu-Bold preview renders in Gaegu-Bold; a
                // name with no loaded face (Times New Roman /
                // Arial / Courier New are toolbar names, not
                // loaded faces) falls back to the default face, so
                // the preview is exact for size/bold/color and
                // best-effort for those families. A bold weight
                // resolves to "<family>@bold" and falls back to
                // the family when that variant was never loaded
                // (Gaegu-Bold has no bold variant), so the preview
                // still uses that family. Size is the scaled
                // preview size (font_dialog::previewFontPx), not
                // the raw point size. Underline / strikethrough
                // are drawn by on_draw_fg: the UI label has no
                // decoration support. Italic has no slanted UI
                // face (no italic font is shipped, and the UI font
                // system has no shear), so italic is recorded in
                // the pending state and applied on OK but the
                // preview glyphs stay upright -- the one effect
                // the preview cannot draw.
                test_input::register_visible_text("Preview:");
                div(ctx, mk(result.ent(), 911),
                    ComponentConfig{}
                        .with_label("Preview:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));
                {
                    std::string previewFont =
                        sel.fontIndex >= 0 &&
                        sel.fontIndex < static_cast<int>(toolbar.fonts.size())
                        ? toolbar.fonts[static_cast<std::size_t>(sel.fontIndex)]
                        : toolbar.currentFont;
                    bool previewBold =
                        sel.styleIndex == 1 || sel.styleIndex == 3;
                    bool previewItalic =
                        sel.styleIndex == 2 || sel.styleIndex == 3;
                    TextColor previewColor =
                        sel.colorIndex >= 0 &&
                        sel.colorIndex < static_cast<int>(colors.size())
                        ? colors[static_cast<std::size_t>(sel.colorIndex)].second
                        : TextColors::Black;
                    test_input::register_visible_text("AaBbYyZz 123");
                    // Test-visible preview state for E2E (checked
                    // before OK): the pending family + size + the
                    // computed preview pixel size (font_preview_size
                    // in the runner reads the same value).
                    test_input::register_visible_text(
                        "Preview font: " + previewFont + ", size " +
                        std::to_string(sel.size));
                    test_input::register_visible_text(
                        "Preview size: " + std::to_string(previewPx) +
                        "px");
                    test_input::register_visible_text(
                        std::string("Preview style: ") +
                        (previewBold ? "Bold " : "") +
                        (previewItalic ? "Italic " : "") +
                        (sel.underline ? "Underline " : "") +
                        (sel.strikethrough ? "Strikethrough" : ""));
                    // Width of the preview string in the preview
                    // face, for centring the underline /
                    // strikethrough rules in on_draw_fg. Measured
                    // in the face the renderer will use
                    // (FontManager::find_font falls back to the
                    // default face for unloaded names, exactly as
                    // the label renderer does), so the rules span
                    // the drawn glyphs.
                    float previewTextW = 0.0f;
                    if (auto* fontMgr = afterhours::EntityHelper::
                            get_singleton_cmp<
                                afterhours::ui::FontManager>()) {
                        auto face = fontMgr->find_font(previewFont);
                        previewTextW = afterhours::measure_text(
                            face, "AaBbYyZz 123",
                            static_cast<float>(previewPx), 1.0f).x;
                    }
                    const afterhours::Color previewAhColor{
                        previewColor.r, previewColor.g,
                        previewColor.b, previewColor.a};
                    const bool previewUnderline = sel.underline;
                    const bool previewStrike = sel.strikethrough;
                    const float previewPxF = static_cast<float>(previewPx);
                    div(ctx, mk(result.ent(), 912),
                        ah_win95::win95SunkenStyle()
                            .with_label("AaBbYyZz 123")
                            .with_size(ComponentSize{percent(1.0f),
                                                     h720(previewBoxH)})
                            .with_font(previewFont, pixels(previewPxF))
                            .with_font_weight(previewBold
                                ? afterhours::colors::FontWeight::Bold
                                : afterhours::colors::FontWeight::Regular)
                            .with_custom_text_color(previewAhColor)
                            .with_alignment(afterhours::ui::TextAlignment::Center)
                            .with_on_draw_fg(
                                [previewTextW, previewAhColor,
                                 previewUnderline, previewStrike,
                                 previewPxF](RectangleType r) {
                                    if (!previewUnderline && !previewStrike) {
                                        return;
                                    }
                                    float x1 = r.x + (r.width - previewTextW) / 2.0f;
                                    float x2 = x1 + previewTextW;
                                    if (previewUnderline) {
                                        int uy = static_cast<int>(
                                            r.y + r.height / 2.0f +
                                            previewPxF * 0.35f);
                                        afterhours::draw_line(
                                            static_cast<int>(x1), uy,
                                            static_cast<int>(x2), uy,
                                            previewAhColor);
                                    }
                                    if (previewStrike) {
                                        int sy = static_cast<int>(
                                            r.y + r.height / 2.0f);
                                        afterhours::draw_line(
                                            static_cast<int>(x1), sy,
                                            static_cast<int>(x2), sy,
                                            previewAhColor);
                                    }
                                })
                            .with_render_layer(CONTENT_LAYER));
                }

                auto buttonRow = div(ctx, mk(result.ent(), 913),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("font_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        int custom = std::atoi(dialogs.fontDialogCustomSize.c_str());
                        if (custom > 0) sel.size = custom;
                        TextStyle style = font_dialog::fontDialogApplyStyle(
                            doc.buffer.textStyle(), sel, toolbar.fonts);
                        doc.buffer.setTextStyle(style);
                        toolbar.currentFont = style.font;
                        toolbar.currentFontSize = style.fontSize;
                        doc.isDirty = true;
                        dialogs.showFontDialog = false;
                        dialogs.fontDialogInitialized = false;
                        dialogs.fontDialogUnderlineDropdownOpen = false;
                        dialogs.fontDialogColorDropdownOpen = false;
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("font_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showFontDialog = false;
                        dialogs.fontDialogInitialized = false;
                        dialogs.fontDialogUnderlineDropdownOpen = false;
                        dialogs.fontDialogColorDropdownOpen = false;
                    }
                }
            }
        }

        // Columns dialog (Format > Columns...) - Win95 shell:
        // preset buttons (One / Two / Three / Four), "Equal column
        // width" fixed on (the columns are always equal width),
        // spacing field in inches (default 0.5), OK/Cancel.
        if (dialogs.showColumnsDialog) {
            constexpr int COLUMNS_MODAL_ID = 50017;
            auto result = afterhours::modal(ctx, mk(entity, COLUMNS_MODAL_ID),
                dialogs.showColumnsDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(420), afterhours::ui::h720(300),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Columns", CONTENT_LAYER);

                test_input::register_visible_text("Presets:");
                div(ctx, mk(result.ent(), 901),
                    ComponentConfig{}
                        .with_label("Presets:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                static const char* kPresetNames[4] = {
                    "One", "Two", "Three", "Four"};
                auto presetRow = div(ctx, mk(result.ent(), 902),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(32)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (presetRow) {
                    for (int i = 0; i < 4; ++i) {
                        bool selected = i + 1 == dialogs.columnsSelected;
                        test_input::register_visible_text(kPresetNames[i]);
                        if (button(ctx, mk(presetRow.ent(), 100 + i),
                            ComponentConfig{}
                                .with_debug_name("columns_preset_" +
                                                std::to_string(i + 1))
                                .with_label(kPresetNames[i])
                                .with_size(ComponentSize{w1280(70), h720(28)})
                                .with_roundness(0.0f)
                                .with_custom_background(selected
                                    ? ui_imm::win95_colors::HIGHLIGHT
                                    : ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(selected
                                    ? ui_imm::win95_colors::TEXT_WHITE
                                    : ui_imm::win95_colors::TEXT)
                                .with_border(ui_imm::win95_colors::TEXT,
                                             selected ? 2.0f : 1.0f)
                                .with_cursor(afterhours::ui::CursorType::Pointer)
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.columnsSelected = i + 1;
                        }
                    }
                }

                // Equal column width is fixed on: the layout math
                // always splits the text width equally, so this is
                // a label, not a toggle.
                test_input::register_visible_text(
                    "Equal column width");
                div(ctx, mk(result.ent(), 903),
                    ComponentConfig{}
                        .with_label("[X] Equal column width")
                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                test_input::register_visible_text("Spacing (inches):");
                div(ctx, mk(result.ent(), 904),
                    ComponentConfig{}
                        .with_label("Spacing (inches):")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));
                afterhours::text_input::text_input(ctx, mk(result.ent(), 905),
                    dialogs.columnsSpacingStr,
                    ah_win95::win95SunkenStyle()
                        .with_debug_name("columns_spacing_input")
                        .with_size(ComponentSize{w1280(80), h720(28)})
                        .with_render_layer(CONTENT_LAYER));

                auto buttonRow = div(ctx, mk(result.ent(), 906),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("columns_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        float inches = std::atof(
                            dialogs.columnsSpacingStr.c_str());
                        if (inches < 0.0f) inches = 0.0f;
                        if (inches > 2.0f) inches = 2.0f;
                        doc.docSettings.pageSettings.columnCount =
                            std::clamp(dialogs.columnsSelected, 1, 4);
                        doc.docSettings.pageSettings.columnSpacing =
                            inches * 72.0f;
                        doc.isDirty = true;
                        dialogs.showColumnsDialog = false;
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("columns_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showColumnsDialog = false;
                    }
                }
            }
        }

        // Paragraph dialog (Format > Paragraph...) - Win95 shell,
        // two tabs: "Indents and Spacing" (Alignment dropdown,
        // Indentation Left / Special + By / Right, Spacing Before /
        // After, Line spacing dropdown, Tabs... button) and "Line
        // and Page Breaks" ("Page break before" + "Keep with next"
        // checkboxes). No Default-tab-stops field: per-paragraph
        // tab stops do not exist, so Tabs... opens the existing
        // Tab Width dialog for the document-wide default.
        // OK applies to the caret paragraph, or every paragraph in
        // the selection, as ONE undo step (commitParagraphFormat);
        // an invalid / cleared field keeps its seeded value, like
        // Page Setup margins. Cancel / Tabs... discard edits.
        // OK builds each row's format from that row's stored
        // format (next = old) so it never wipes fields this
        // dialog does not edit (borders/shading).
        if (dialogs.showParagraphDialog) {
            constexpr int PARAGRAPH_MODAL_ID = 50019;
            float paragraphDialogH =
                (dialogs.paragraphTab == 0 ? 370.0f : 180.0f) +
                (dialogs.paragraphAlignmentDropdownOpen ? 88.0f : 0.0f) +
                (dialogs.paragraphSpecialDropdownOpen ? 66.0f : 0.0f) +
                (dialogs.paragraphLineSpacingDropdownOpen ? 66.0f : 0.0f);
            auto result = afterhours::modal(ctx, mk(entity, PARAGRAPH_MODAL_ID),
                dialogs.showParagraphDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(460),
                    afterhours::ui::h720(paragraphDialogH),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Paragraph", CONTENT_LAYER);

                auto fieldRow = [&](int id, const char* label,
                                    const char* debugName,
                                    std::string& value) {
                    auto row = div(ctx, mk(result.ent(), id),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (!row) return;
                    test_input::register_visible_text(label);
                    div(ctx, mk(row.ent(), 0),
                        ComponentConfig{}
                            .with_label(label)
                            .with_size(ComponentSize{w1280(150), h720(24)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                    afterhours::text_input::text_input(
                        ctx, mk(row.ent(), 1), value,
                        ah_win95::win95SunkenStyle()
                            .with_debug_name(debugName)
                            .with_size(ComponentSize{w1280(80), h720(24)})
                            .with_render_layer(CONTENT_LAYER));
                };

                auto groupLabel = [&](int id, const char* label) {
                    test_input::register_visible_text(label);
                    div(ctx, mk(result.ent(), id),
                        ComponentConfig{}
                            .with_label(label)
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                };

                // Dropdown row + in-flow option list, the same
                // pattern as the Font dialog dropdowns: the button
                // shows the pending choice, the list expands
                // in-flow below (the dialog height grows by its
                // height). Opening one dropdown closes the other
                // two, so at most one list is open.
                auto dropdownRow = [&](int rowId, int listId,
                                       const char* label,
                                       const char* currentName,
                                       bool& openFlag, bool& otherOpen1,
                                       bool& otherOpen2,
                                       const char* const* names, int count,
                                       const char* const* debugNames,
                                       int& index,
                                       const char* dropdownDebug) {
                    auto row = div(ctx, mk(result.ent(), rowId),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        test_input::register_visible_text(label);
                        div(ctx, mk(row.ent(), 0),
                            ComponentConfig{}
                                .with_label(label)
                                .with_size(ComponentSize{w1280(150), h720(24)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                        test_input::register_visible_text(
                            std::string(label) + " " + currentName);
                        if (button(ctx, mk(row.ent(), 1),
                            ah_win95::win95DropdownButtonStyle(
                                120.0f, 26.0f, openFlag)
                                .with_debug_name(dropdownDebug)
                                .with_label(currentName)
                                .with_text_inset(8.0f, 2.0f)
                                .with_on_draw_fg([](RectangleType r) {
                                    drawDropdownTriangle(
                                        r.x + r.width -
                                            theme::layout::scale(10),
                                        r.y + r.height / 2.0f,
                                        theme::BUTTON_TEXT);
                                })
                                .with_render_layer(CONTENT_LAYER))) {
                            openFlag = !openFlag;
                            otherOpen1 = false;
                            otherOpen2 = false;
                        }
                    }
                    if (openFlag) {
                        auto list = div(ctx, mk(result.ent(), listId),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{
                                    percent(1.0f),
                                    h720(static_cast<float>(count * 22))})
                                .with_flex_direction(FlexDirection::Column)
                                .with_padding(Padding{.top = pixels(2),
                                                     .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (list) {
                            for (int i = 0; i < count; ++i) {
                                bool selected = i == index;
                                test_input::register_visible_text(names[i]);
                                if (button(ctx, mk(list.ent(), 700 + i),
                                    ComponentConfig{}
                                        .with_debug_name(debugNames[i])
                                        .with_label(names[i])
                                        .with_size(ComponentSize{percent(1.0f),
                                                                h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(
                                            afterhours::ui::TextAlignment::Left)
                                        .with_text_inset(8.0f, 2.0f)
                                        .with_cursor(
                                            afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    index = i;
                                    openFlag = false;
                                }
                            }
                        }
                    }
                };

                // Tab strip: two tabs, the selected one highlighted.
                {
                    auto strip = div(ctx, mk(result.ent(), 901),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (strip) {
                        static const char* kTabLabels[2] = {
                            "Indents and Spacing", "Line and Page Breaks"};
                        static const char* kTabDebug[2] = {
                            "paragraph_tab_indents", "paragraph_tab_breaks"};
                        static const float kTabWidths[2] = {140.0f,
                                                             150.0f};
                        for (int i = 0; i < 2; ++i) {
                            bool selected = dialogs.paragraphTab == i;
                            test_input::register_visible_text(kTabLabels[i]);
                            if (button(ctx, mk(strip.ent(), i),
                                ah_win95::win95DialogButtonStyle()
                                    .with_debug_name(kTabDebug[i])
                                    .with_size(ComponentSize{
                                        w1280(kTabWidths[i]), h720(24)})
                                    .with_label(kTabLabels[i])
                                    .with_custom_background(selected
                                        ? ui_imm::win95_colors::HIGHLIGHT
                                        : ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(selected
                                        ? ui_imm::win95_colors::TEXT_WHITE
                                        : ui_imm::win95_colors::TEXT)
                                    .with_render_layer(CONTENT_LAYER))) {
                                dialogs.paragraphTab = i;
                                dialogs.paragraphLineSpacingDropdownOpen =
                                    false;
                                dialogs.paragraphAlignmentDropdownOpen =
                                    false;
                                dialogs.paragraphSpecialDropdownOpen =
                                    false;
                            }
                        }
                    }
                }

                if (dialogs.paragraphTab == 0) {
                // Alignment dropdown (Tab 1, like Word's Alignment
                // dropdown at the top of Indents and Spacing).
                {
                    static const char* kAlignDebug[4] = {
                        "paragraph_alignment_left",
                        "paragraph_alignment_centered",
                        "paragraph_alignment_right",
                        "paragraph_alignment_justified"};
                    const char* currentName =
                        paragraph_dialog::kAlignmentNames
                            [static_cast<std::size_t>(std::clamp(
                                dialogs.paragraphAlignmentIndex, 0, 3))];
                    dropdownRow(902, 903, "Alignment:", currentName,
                                dialogs.paragraphAlignmentDropdownOpen,
                                dialogs.paragraphSpecialDropdownOpen,
                                dialogs.paragraphLineSpacingDropdownOpen,
                                paragraph_dialog::kAlignmentNames, 4,
                                kAlignDebug,
                                dialogs.paragraphAlignmentIndex,
                                "paragraph_alignment_dropdown");
                }

                groupLabel(904, "Indentation:");
                fieldRow(905, "Left (inches):", "paragraph_left_input",
                         dialogs.paragraphLeftStr);
                // Special + By: Word's Special dropdown (None /
                // First line / Hanging) and the By amount driving
                // firstLineIndent (+By for First line, -By for
                // Hanging, 0 for None). The dropdown shares the
                // row with the By input, so it is bespoke rather
                // than the dropdownRow helper.
                {
                    auto row = div(ctx, mk(result.ent(), 906),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        test_input::register_visible_text("Special:");
                        div(ctx, mk(row.ent(), 0),
                            ComponentConfig{}
                                .with_label("Special:")
                                .with_size(ComponentSize{w1280(150), h720(24)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                        const char* currentName =
                            paragraph_dialog::kSpecialNames
                                [static_cast<std::size_t>(std::clamp(
                                    dialogs.paragraphSpecialIndex, 0, 2))];
                        test_input::register_visible_text(
                            std::string("Special: ") + currentName);
                        if (button(ctx, mk(row.ent(), 1),
                            ah_win95::win95DropdownButtonStyle(
                                120.0f, 26.0f,
                                dialogs.paragraphSpecialDropdownOpen)
                                .with_debug_name(
                                    "paragraph_special_dropdown")
                                .with_label(currentName)
                                .with_text_inset(8.0f, 2.0f)
                                .with_on_draw_fg([](RectangleType r) {
                                    drawDropdownTriangle(
                                        r.x + r.width -
                                            theme::layout::scale(10),
                                        r.y + r.height / 2.0f,
                                        theme::BUTTON_TEXT);
                                })
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.paragraphSpecialDropdownOpen =
                                !dialogs.paragraphSpecialDropdownOpen;
                            dialogs.paragraphAlignmentDropdownOpen = false;
                            dialogs.paragraphLineSpacingDropdownOpen = false;
                        }
                        test_input::register_visible_text("By:");
                        div(ctx, mk(row.ent(), 2),
                            ComponentConfig{}
                                .with_label("By:")
                                .with_size(ComponentSize{w1280(32), h720(24)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                        afterhours::text_input::text_input(
                            ctx, mk(row.ent(), 3),
                            dialogs.paragraphByStr,
                            ah_win95::win95SunkenStyle()
                                .with_debug_name("paragraph_by_input")
                                .with_size(ComponentSize{w1280(80), h720(24)})
                                .with_render_layer(CONTENT_LAYER));
                    }
                    if (dialogs.paragraphSpecialDropdownOpen) {
                        auto list = div(ctx, mk(result.ent(), 907),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{percent(1.0f),
                                                         h720(66)})
                                .with_flex_direction(FlexDirection::Column)
                                .with_padding(Padding{.top = pixels(2),
                                                     .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (list) {
                            static const char* kSpecialDebug[3] = {
                                "paragraph_special_none",
                                "paragraph_special_firstline",
                                "paragraph_special_hanging"};
                            for (int i = 0; i < 3; ++i) {
                                bool selected =
                                    i == dialogs.paragraphSpecialIndex;
                                test_input::register_visible_text(
                                    paragraph_dialog::kSpecialNames[i]);
                                if (button(ctx, mk(list.ent(), 700 + i),
                                    ComponentConfig{}
                                        .with_debug_name(kSpecialDebug[i])
                                        .with_label(
                                            paragraph_dialog::kSpecialNames[i])
                                        .with_size(ComponentSize{percent(1.0f),
                                                                h720(22)})
                                        .with_roundness(0.0f)
                                        .with_custom_background(selected
                                            ? ui_imm::win95_colors::HIGHLIGHT
                                            : ui_imm::win95_colors::TEXT_AREA)
                                        .with_custom_text_color(selected
                                            ? ui_imm::win95_colors::TEXT_WHITE
                                            : ui_imm::win95_colors::TEXT)
                                        .with_alignment(
                                            afterhours::ui::TextAlignment::Left)
                                        .with_text_inset(8.0f, 2.0f)
                                        .with_cursor(
                                            afterhours::ui::CursorType::Pointer)
                                        .with_render_layer(CONTENT_LAYER))) {
                                    dialogs.paragraphSpecialIndex = i;
                                    dialogs.paragraphSpecialDropdownOpen =
                                        false;
                                }
                            }
                        }
                    }
                }
                fieldRow(908, "Right (inches):", "paragraph_right_input",
                         dialogs.paragraphRightStr);

                groupLabel(909, "Spacing:");
                fieldRow(910, "Before (pt):", "paragraph_before_input",
                         dialogs.paragraphBeforeStr);
                fieldRow(911, "After (pt):", "paragraph_after_input",
                         dialogs.paragraphAfterStr);

                // Line spacing dropdown: button shows the pending
                // choice; the list expands in-flow below (the dialog
                // height grows by its height), same pattern as the
                // Font dialog dropdowns.
                {
                    auto row = div(ctx, mk(result.ent(), 912),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        test_input::register_visible_text("Line spacing:");
                        div(ctx, mk(row.ent(), 0),
                            ComponentConfig{}
                                .with_label("Line spacing:")
                                .with_size(ComponentSize{w1280(150), h720(24)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                        const char* currentName =
                            paragraph_dialog::kLineSpacingNames
                                [static_cast<std::size_t>(std::clamp(
                                    dialogs.paragraphLineSpacingIndex, 0,
                                    2))];
                        test_input::register_visible_text(
                            std::string("Line spacing: ") + currentName);
                        if (button(ctx, mk(row.ent(), 1),
                            ah_win95::win95DropdownButtonStyle(
                                120.0f, 26.0f,
                                dialogs.paragraphLineSpacingDropdownOpen)
                                .with_debug_name(
                                    "paragraph_linespacing_dropdown")
                                .with_label(currentName)
                                .with_text_inset(8.0f, 2.0f)
                                .with_on_draw_fg([](RectangleType r) {
                                    drawDropdownTriangle(
                                        r.x + r.width -
                                            theme::layout::scale(10),
                                        r.y + r.height / 2.0f,
                                        theme::BUTTON_TEXT);
                                })
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.paragraphLineSpacingDropdownOpen =
                                !dialogs.paragraphLineSpacingDropdownOpen;
                            dialogs.paragraphAlignmentDropdownOpen = false;
                            dialogs.paragraphSpecialDropdownOpen = false;
                        }
                    }
                }
                if (dialogs.paragraphLineSpacingDropdownOpen) {
                    auto list = div(ctx, mk(result.ent(), 913),
                        ah_win95::win95SunkenStyle()
                            .with_size(ComponentSize{percent(1.0f), h720(66)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_padding(Padding{.top = pixels(2),
                                                 .bottom = pixels(2)})
                            .with_render_layer(CONTENT_LAYER));
                    if (list) {
                        static const char* kOptionDebug[3] = {
                            "paragraph_linespacing_single",
                            "paragraph_linespacing_1_5",
                            "paragraph_linespacing_double"};
                        for (int i = 0; i < 3; ++i) {
                            bool selected =
                                i == dialogs.paragraphLineSpacingIndex;
                            test_input::register_visible_text(
                                paragraph_dialog::kLineSpacingNames[i]);
                            if (button(ctx, mk(list.ent(), 700 + i),
                                ComponentConfig{}
                                    .with_debug_name(kOptionDebug[i])
                                    .with_label(
                                        paragraph_dialog::kLineSpacingNames[i])
                                    .with_size(ComponentSize{percent(1.0f),
                                                            h720(22)})
                                    .with_roundness(0.0f)
                                    .with_custom_background(selected
                                        ? ui_imm::win95_colors::HIGHLIGHT
                                        : ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(selected
                                        ? ui_imm::win95_colors::TEXT_WHITE
                                        : ui_imm::win95_colors::TEXT)
                                    .with_alignment(
                                        afterhours::ui::TextAlignment::Left)
                                    .with_text_inset(8.0f, 2.0f)
                                    .with_cursor(
                                        afterhours::ui::CursorType::Pointer)
                                    .with_render_layer(CONTENT_LAYER))) {
                                dialogs.paragraphLineSpacingIndex = i;
                                dialogs.paragraphLineSpacingDropdownOpen =
                                    false;
                            }
                        }
                    }
                }

                // Tabs... button: no Default-tab-stops field on
                // this tab (per-paragraph tab stops do not exist).
                // Opens the existing Tab Width dialog flow for the
                // document-wide default; this dialog closes,
                // discarding its unsaved edits, like Cancel.
                {
                    auto row = div(ctx, mk(result.ent(), 914),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        test_input::register_visible_text("Tabs...");
                        if (button(ctx, mk(row.ent(), 0),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("paragraph_tabs_button")
                                .with_label("Tabs...")
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.tabWidthInputStr =
                                std::to_string(doc.docSettings.tabWidth);
                            dialogs.showParagraphDialog = false;
                            dialogs.paragraphLineSpacingDropdownOpen =
                                false;
                            dialogs.paragraphAlignmentDropdownOpen = false;
                            dialogs.paragraphSpecialDropdownOpen = false;
                            dialogs.showTabWidthDialog = true;
                        }
                        test_input::register_visible_text(
                            "Tab stops are document-wide.");
                        div(ctx, mk(row.ent(), 1),
                            ComponentConfig{}
                                .with_label("Tab stops are document-wide.")
                                .with_size(ComponentSize{w1280(260), h720(24)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                    }
                }
                }  // paragraphTab == 0

                // Tab 2: "Line and Page Breaks" — two checkboxes.
                // Line spacing stays on Tab 1, like Word.
                if (dialogs.paragraphTab == 1) {
                    auto checkboxRow = [&](int id, const char* label,
                                           const char* debugName,
                                           bool& value) {
                        test_input::register_visible_text(
                            std::string(value ? "[X] " : "[ ] ") + label);
                        if (button(ctx, mk(result.ent(), id),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name(debugName)
                                .with_size(ComponentSize{w1280(170),
                                                         h720(24)})
                                .with_label(std::string(value ? "[X] "
                                                             : "[ ] ") +
                                            label)
                                .with_render_layer(CONTENT_LAYER))) {
                            value = !value;
                        }
                    };
                    checkboxRow(920, "Page break before",
                                "paragraph_pagebreak_checkbox",
                                dialogs.paragraphPageBreakBefore);
                    checkboxRow(921, "Keep with next",
                                "paragraph_keepnext_checkbox",
                                dialogs.paragraphKeepWithNext);
                }

                auto buttonRow = div(ctx, mk(result.ent(), 930),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("paragraph_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        ParagraphFormat fallback =
                            doc.buffer.lineParagraphFormat(
                                doc.buffer.caret().row);
                        // Parsed values, once: invalid / cleared
                        // fields keep their seeded (fallback)
                        // values, like Page Setup margins. By
                        // falls back to the seeded |firstLine|, or
                        // the 0.5" default when that is 0.
                        int leftIndent =
                            paragraph_dialog::inchesToStored(
                                paragraph_dialog::parseInches(
                                    dialogs.paragraphLeftStr,
                                    static_cast<double>(
                                        fallback.indents.left) /
                                        paragraph_dialog::POINTS_PER_INCH));
                        double byFallback =
                            fallback.indents.firstLine != 0
                                ? static_cast<double>(std::abs(
                                      fallback.indents.firstLine)) /
                                      paragraph_dialog::POINTS_PER_INCH
                                : paragraph_dialog::kDefaultByInches;
                        int firstLineIndent =
                            paragraph_dialog::firstLineIndentFor(
                                dialogs.paragraphSpecialIndex,
                                paragraph_dialog::parseInches(
                                    dialogs.paragraphByStr, byFallback));
                        // First line never left of the margin, like
                        // the ruler drag.
                        if (firstLineIndent < -leftIndent)
                            firstLineIndent = -leftIndent;
                        int rightIndent =
                            paragraph_dialog::inchesToStored(
                                paragraph_dialog::parseInches(
                                    dialogs.paragraphRightStr,
                                    static_cast<double>(
                                        fallback.indents.right) /
                                        paragraph_dialog::POINTS_PER_INCH));
                        int spaceBefore =
                            paragraph_dialog::parsePoints(
                                dialogs.paragraphBeforeStr,
                                fallback.spaceBefore);
                        int spaceAfter =
                            paragraph_dialog::parsePoints(
                                dialogs.paragraphAfterStr,
                                fallback.spaceAfter);
                        float lineSpacing =
                            paragraph_dialog::lineSpacingForIndex(
                                dialogs.paragraphLineSpacingIndex);
                        ::TextAlignment alignment =
                            paragraph_dialog::alignmentForIndex(
                                dialogs.paragraphAlignmentIndex);
                        std::size_t firstRow = doc.buffer.caret().row;
                        std::size_t lastRow = firstRow;
                        if (doc.buffer.hasSelection()) {
                            firstRow = std::min(
                                doc.buffer.selectionStart().row,
                                doc.buffer.selectionEnd().row);
                            lastRow = std::max(
                                doc.buffer.selectionStart().row,
                                doc.buffer.selectionEnd().row);
                        }
                        if (lastRow >= doc.buffer.lineCount() &&
                            doc.buffer.lineCount() > 0)
                            lastRow = doc.buffer.lineCount() - 1;
                        std::vector<ParagraphFormatChange> changes;
                        for (std::size_t row = firstRow; row <= lastRow;
                             ++row) {
                            ParagraphFormat old =
                                doc.buffer.lineParagraphFormat(row);
                            ParagraphFormat next = old;
                            next.indents.left = leftIndent;
                            next.indents.firstLine = firstLineIndent;
                            next.indents.right = rightIndent;
                            next.spaceBefore = spaceBefore;
                            next.spaceAfter = spaceAfter;
                            next.lineSpacing = lineSpacing;
                            next.alignment = alignment;
                            next.pageBreakBefore =
                                dialogs.paragraphPageBreakBefore;
                            next.keepWithNext =
                                dialogs.paragraphKeepWithNext;
                            doc.buffer.setLineParagraphFormat(row, next);
                            changes.push_back({row, old, next});
                        }
                        doc.buffer.commitParagraphFormat(
                            std::move(changes));
                        doc.isDirty = true;
                        dialogs.showParagraphDialog = false;
                        dialogs.paragraphLineSpacingDropdownOpen = false;
                        dialogs.paragraphAlignmentDropdownOpen = false;
                        dialogs.paragraphSpecialDropdownOpen = false;
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("paragraph_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showParagraphDialog = false;
                        dialogs.paragraphLineSpacingDropdownOpen = false;
                        dialogs.paragraphAlignmentDropdownOpen = false;
                        dialogs.paragraphSpecialDropdownOpen = false;
                    }
                }
            }
        }

        // Borders and Shading dialog (Format > Borders and Shading...)
        // - Win95 shell: Setting (None / Box / Custom), per-edge
        // toggles, Style (None / Single), shading swatches (None +
        // highlight palette). OK applies to the caret paragraph, or
        // every paragraph in the selection, as ONE undo step
        // (commitParagraphFormat); it only touches the border /
        // shading fields, preserving each paragraph's indents,
        // spacing, and line spacing. Cancel discards edits.
        if (dialogs.showBordersShadingDialog) {
            constexpr int BORDERS_MODAL_ID = 50021;
            auto result = afterhours::modal(ctx, mk(entity, BORDERS_MODAL_ID),
                dialogs.showBordersShadingDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(460),
                    afterhours::ui::h720(340),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Borders and Shading",
                                              CONTENT_LAYER);

                auto groupLabel = [&](int id, const char* label) {
                    test_input::register_visible_text(label);
                    div(ctx, mk(result.ent(), id),
                        ComponentConfig{}
                            .with_label(label)
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                };

                // Setting: None clears edges + style, Box sets all
                // four edges + Single style, Custom keeps the
                // working copy for the edge toggles below.
                groupLabel(901, "Setting:");
                {
                    auto row = div(ctx, mk(result.ent(), 902),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(30)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        bool noneSelected =
                            dialogs.bordersStyle ==
                                borders_shading::kBorderStyleNone &&
                            dialogs.bordersEdges == 0;
                        bool boxSelected =
                            dialogs.bordersStyle ==
                                borders_shading::kBorderStyleSingle &&
                            dialogs.bordersEdges ==
                                borders_shading::kEdgeAll;
                        bool customSelected = !noneSelected && !boxSelected;
                        static const char* kSettingLabels[3] = {"None", "Box",
                                                               "Custom"};
                        static const char* kSettingDebug[3] = {
                            "borders_setting_none", "borders_setting_box",
                            "borders_setting_custom"};
                        bool selected[3] = {noneSelected, boxSelected,
                                            customSelected};
                        for (int i = 0; i < 3; ++i) {
                            test_input::register_visible_text(
                                kSettingLabels[i]);
                            if (button(ctx, mk(row.ent(), i),
                                ah_win95::win95DialogButtonStyle()
                                    .with_debug_name(kSettingDebug[i])
                                    .with_label(kSettingLabels[i])
                                    .with_custom_background(selected[i]
                                        ? ui_imm::win95_colors::HIGHLIGHT
                                        : ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(selected[i]
                                        ? ui_imm::win95_colors::TEXT_WHITE
                                        : ui_imm::win95_colors::TEXT)
                                    .with_render_layer(CONTENT_LAYER))) {
                                if (i == 0) {
                                    dialogs.bordersEdges = 0;
                                    dialogs.bordersStyle =
                                        borders_shading::kBorderStyleNone;
                                } else if (i == 1) {
                                    dialogs.bordersEdges =
                                        borders_shading::kEdgeAll;
                                    dialogs.bordersStyle =
                                        borders_shading::kBorderStyleSingle;
                                }
                            }
                        }
                    }
                }

                // Edges: per-edge toggles. Turning an edge on while
                // the style is None also picks Single, so the toggle
                // visibly takes effect.
                groupLabel(903, "Edges:");
                {
                    auto row = div(ctx, mk(result.ent(), 904),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(30)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        static const char* kEdgeLabels[4] = {"Top", "Bottom",
                                                             "Left", "Right"};
                        static const int kEdgeBits[4] = {
                            borders_shading::kEdgeTop,
                            borders_shading::kEdgeBottom,
                            borders_shading::kEdgeLeft,
                            borders_shading::kEdgeRight};
                        static const char* kEdgeDebug[4] = {
                            "borders_edge_top", "borders_edge_bottom",
                            "borders_edge_left", "borders_edge_right"};
                        for (int i = 0; i < 4; ++i) {
                            bool on = (dialogs.bordersEdges &
                                       kEdgeBits[i]) != 0;
                            std::string label =
                                std::string(on ? "[X] " : "[ ] ") +
                                kEdgeLabels[i];
                            test_input::register_visible_text(label);
                            if (button(ctx, mk(row.ent(), i),
                                ah_win95::win95DialogButtonStyle()
                                    .with_debug_name(kEdgeDebug[i])
                                    .with_label(label)
                                    .with_render_layer(CONTENT_LAYER))) {
                                dialogs.bordersEdges ^= kEdgeBits[i];
                                if ((dialogs.bordersEdges &
                                     kEdgeBits[i]) != 0 &&
                                    dialogs.bordersStyle ==
                                        borders_shading::kBorderStyleNone) {
                                    dialogs.bordersStyle =
                                        borders_shading::kBorderStyleSingle;
                                }
                            }
                        }
                    }
                }

                // Style: None draws no border even with edges set;
                // Single draws one 1px line on the chosen edges.
                groupLabel(905, "Style:");
                {
                    auto row = div(ctx, mk(result.ent(), 906),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(30)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        static const char* kStyleLabels[2] = {"None",
                                                              "Single"};
                        static const char* kStyleDebug[2] = {
                            "borders_style_none", "borders_style_single"};
                        static const int kStyleValues[2] = {
                            borders_shading::kBorderStyleNone,
                            borders_shading::kBorderStyleSingle};
                        for (int i = 0; i < 2; ++i) {
                            bool selected =
                                dialogs.bordersStyle == kStyleValues[i];
                            test_input::register_visible_text(
                                kStyleLabels[i]);
                            if (button(ctx, mk(row.ent(), i),
                                ah_win95::win95DialogButtonStyle()
                                    .with_debug_name(kStyleDebug[i])
                                    .with_label(kStyleLabels[i])
                                    .with_custom_background(selected
                                        ? ui_imm::win95_colors::HIGHLIGHT
                                        : ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(selected
                                        ? ui_imm::win95_colors::TEXT_WHITE
                                        : ui_imm::win95_colors::TEXT)
                                    .with_render_layer(CONTENT_LAYER))) {
                                dialogs.bordersStyle = kStyleValues[i];
                            }
                        }
                    }
                }

                // Shading: one swatch per palette entry (None +
                // highlight palette). The None swatch is white with
                // a gray frame; the current choice is named below
                // the row so None is unambiguous.
                groupLabel(907, "Shading:");
                {
                    auto row = div(ctx, mk(result.ent(), 908),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(30)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(6))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (row) {
                        for (int i = 0; i < borders_shading::kShadingCount;
                             ++i) {
                            TextColor swatch =
                                borders_shading::shadingColorForIndex(i);
                            bool selected =
                                dialogs.bordersShadingIndex == i;
                            std::string debugName =
                                std::string("borders_shading_") +
                                borders_shading::kShadingNames[i];
                            for (char& ch : debugName) {
                                if (ch >= 'A' && ch <= 'Z')
                                    ch = static_cast<char>(ch - 'A' + 'a');
                            }
                            test_input::register_visible_text(
                                borders_shading::kShadingNames[i]);
                            if (button(ctx, mk(row.ent(), i),
                                ComponentConfig{}
                                    .with_debug_name(debugName)
                                    .with_label("")
                                    .with_size(ComponentSize{w1280(32),
                                                             h720(24)})
                                    .with_custom_background(afterhours::Color{
                                        swatch.r, swatch.g, swatch.b,
                                        swatch.a})
                                    .with_border(
                                        selected
                                            ? ui_imm::win95_colors::TEXT
                                            : afterhours::Color{128, 128, 128,
                                                               255},
                                        selected ? 2.0f : 1.0f)
                                    .with_roundness(0.0f)
                                    .with_cursor(afterhours::ui::CursorType::
                                                   Pointer)
                                    .with_render_layer(CONTENT_LAYER))) {
                                dialogs.bordersShadingIndex = i;
                            }
                        }
                    }
                }
                {
                    std::string current =
                        std::string("Shading: ") +
                        borders_shading::kShadingNames
                            [static_cast<std::size_t>(
                                borders_shading::clampShadingIndex(
                                    dialogs.bordersShadingIndex))];
                    test_input::register_visible_text(current);
                    div(ctx, mk(result.ent(), 909),
                        ComponentConfig{}
                            .with_label(current)
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));
                }

                auto buttonRow = div(ctx, mk(result.ent(), 910),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("borders_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        std::size_t firstRow = doc.buffer.caret().row;
                        std::size_t lastRow = firstRow;
                        if (doc.buffer.hasSelection()) {
                            firstRow = std::min(
                                doc.buffer.selectionStart().row,
                                doc.buffer.selectionEnd().row);
                            lastRow = std::max(
                                doc.buffer.selectionStart().row,
                                doc.buffer.selectionEnd().row);
                        }
                        if (lastRow >= doc.buffer.lineCount() &&
                            doc.buffer.lineCount() > 0)
                            lastRow = doc.buffer.lineCount() - 1;
                        std::vector<ParagraphFormatChange> changes;
                        for (std::size_t row = firstRow; row <= lastRow;
                             ++row) {
                            ParagraphFormat old =
                                doc.buffer.lineParagraphFormat(row);
                            ParagraphFormat next = old;
                            next.borderEdges = dialogs.bordersEdges;
                            next.borderStyle = dialogs.bordersStyle;
                            next.shadingColorIndex =
                                dialogs.bordersShadingIndex;
                            doc.buffer.setLineParagraphFormat(row, next);
                            changes.push_back({row, old, next});
                        }
                        doc.buffer.commitParagraphFormat(
                            std::move(changes));
                        doc.isDirty = true;
                        dialogs.showBordersShadingDialog = false;
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("borders_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showBordersShadingDialog = false;
                    }
                }
            }
        }

        // Page Setup dialog (File > Page Setup...) - Win95 shell:
        // paper-size presets, Portrait/Landscape, four margin
        // fields in inches. OK writes the whole block to
        // docSettings.pageSettings and pushes it to the layout
        // (cmd::applyPageSettings), so the paged renderer, ruler,
        // hit-test, and Print Preview pagination all consume the
        // same values. Cancel leaves the document untouched. A
        // cleared/unknowable margin field keeps its current value.
        if (dialogs.showPageSetup) {
            constexpr int PAGE_SETUP_MODAL_ID = 50018;
            auto result = afterhours::modal(ctx, mk(entity, PAGE_SETUP_MODAL_ID),
                dialogs.showPageSetup,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(500), afterhours::ui::h720(400),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Page Setup", CONTENT_LAYER);

                test_input::register_visible_text("Paper size:");
                div(ctx, mk(result.ent(), 901),
                    ComponentConfig{}
                        .with_label("Paper size:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                struct PageSizeOption {
                    PageSize size;
                    const char* name;
                    const char* debug;
                };
                static const PageSizeOption kPageSizes[8] = {
                    {PageSize::Letter, "Letter", "pagesetup_size_letter"},
                    {PageSize::Legal, "Legal", "pagesetup_size_legal"},
                    {PageSize::Tabloid, "Tabloid", "pagesetup_size_tabloid"},
                    {PageSize::A4, "A4", "pagesetup_size_a4"},
                    {PageSize::A5, "A5", "pagesetup_size_a5"},
                    {PageSize::B5, "B5", "pagesetup_size_b5"},
                    {PageSize::Executive, "Executive",
                     "pagesetup_size_executive"},
                    {PageSize::Custom, "Custom", "pagesetup_size_custom"},
                };
                for (int row = 0; row < 2; ++row) {
                    auto sizeRow = div(ctx, mk(result.ent(), 902 + row),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(32)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (sizeRow) {
                        for (int i = 0; i < 4; ++i) {
                            const auto& opt = kPageSizes[row * 4 + i];
                            bool selected =
                                dialogs.selectedPageSize == opt.size;
                            test_input::register_visible_text(opt.name);
                            if (button(ctx, mk(sizeRow.ent(), 100 + i),
                                ComponentConfig{}
                                    .with_debug_name(opt.debug)
                                    .with_label(opt.name)
                                    .with_size(ComponentSize{w1280(96), h720(28)})
                                    .with_roundness(0.0f)
                                    .with_custom_background(selected
                                        ? ui_imm::win95_colors::HIGHLIGHT
                                        : ui_imm::win95_colors::BUTTON_FACE)
                                    .with_custom_text_color(selected
                                        ? ui_imm::win95_colors::TEXT_WHITE
                                        : ui_imm::win95_colors::TEXT)
                                    .with_border(ui_imm::win95_colors::TEXT,
                                                 selected ? 2.0f : 1.0f)
                                    .with_cursor(afterhours::ui::CursorType::Pointer)
                                    .with_render_layer(CONTENT_LAYER))) {
                                dialogs.selectedPageSize = opt.size;
                            }
                        }
                    }
                }

                test_input::register_visible_text("Orientation:");
                div(ctx, mk(result.ent(), 904),
                    ComponentConfig{}
                        .with_label("Orientation:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                auto orientRow = div(ctx, mk(result.ent(), 905),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(32)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (orientRow) {
                    static const struct {
                        PageOrientation orient;
                        const char* name;
                        const char* debug;
                    } kOrients[2] = {
                        {PageOrientation::Portrait, "Portrait",
                         "pagesetup_orientation_portrait"},
                        {PageOrientation::Landscape, "Landscape",
                         "pagesetup_orientation_landscape"},
                    };
                    for (int i = 0; i < 2; ++i) {
                        bool selected =
                            dialogs.selectedOrientation == kOrients[i].orient;
                        test_input::register_visible_text(kOrients[i].name);
                        if (button(ctx, mk(orientRow.ent(), 100 + i),
                            ComponentConfig{}
                                .with_debug_name(kOrients[i].debug)
                                .with_label(kOrients[i].name)
                                .with_size(ComponentSize{w1280(96), h720(28)})
                                .with_roundness(0.0f)
                                .with_custom_background(selected
                                    ? ui_imm::win95_colors::HIGHLIGHT
                                    : ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(selected
                                    ? ui_imm::win95_colors::TEXT_WHITE
                                    : ui_imm::win95_colors::TEXT)
                                .with_border(ui_imm::win95_colors::TEXT,
                                             selected ? 2.0f : 1.0f)
                                .with_cursor(afterhours::ui::CursorType::Pointer)
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.selectedOrientation = kOrients[i].orient;
                        }
                    }
                }

                test_input::register_visible_text("Margins (inches):");
                div(ctx, mk(result.ent(), 906),
                    ComponentConfig{}
                        .with_label("Margins (inches):")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                struct MarginField {
                    const char* label;
                    const char* debug;
                    std::string* value;
                };
                MarginField marginFields[4] = {
                    {"Top:", "pagesetup_margin_top_input",
                     &dialogs.pageSetupTopStr},
                    {"Bottom:", "pagesetup_margin_bottom_input",
                     &dialogs.pageSetupBottomStr},
                    {"Left:", "pagesetup_margin_left_input",
                     &dialogs.pageSetupLeftStr},
                    {"Right:", "pagesetup_margin_right_input",
                     &dialogs.pageSetupRightStr},
                };
                for (int i = 0; i < 4; ++i) {
                    auto marginRow = div(ctx, mk(result.ent(), 907 + i),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_align_items(AlignItems::Center)
                            .with_gap(pixels(8))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (marginRow) {
                        test_input::register_visible_text(
                            marginFields[i].label);
                        div(ctx, mk(marginRow.ent(), 0),
                            ComponentConfig{}
                                .with_label(marginFields[i].label)
                                .with_size(ComponentSize{w1280(70), h720(24)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                        afterhours::text_input::text_input(
                            ctx, mk(marginRow.ent(), 1),
                            *marginFields[i].value,
                            ah_win95::win95SunkenStyle()
                                .with_debug_name(marginFields[i].debug)
                                .with_size(ComponentSize{w1280(80), h720(24)})
                                .with_render_layer(CONTENT_LAYER));
                    }
                }

                auto buttonRow = div(ctx, mk(result.ent(), 911),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("pagesetup_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        auto parseMarginInches = [](const std::string& text,
                                                    float fallbackPoints) {
                            if (text.empty()) return fallbackPoints;
                            char* end = nullptr;
                            double inches =
                                std::strtod(text.c_str(), &end);
                            // No digits, NaN, or absurd: keep current.
                            if (end == text.c_str() || inches != inches ||
                                inches > 1000.0 || inches < -1000.0) {
                                return fallbackPoints;
                            }
                            return std::clamp(
                                       static_cast<float>(inches), 0.0f,
                                       4.0f) *
                                   72.0f;
                        };
                        PageSettings& ps = doc.docSettings.pageSettings;
                        // Custom keeps its current dims (no width/height
                        // fields in this dialog); a preset re-derives
                        // them from the size + orientation below.
                        if (dialogs.selectedPageSize != PageSize::Custom) {
                            ps.applyPageSize(dialogs.selectedPageSize);
                        } else {
                            ps.size = PageSize::Custom;
                        }
                        ps.setOrientation(dialogs.selectedOrientation);
                        ps.marginTop = parseMarginInches(
                            dialogs.pageSetupTopStr, ps.marginTop);
                        ps.marginBottom = parseMarginInches(
                            dialogs.pageSetupBottomStr, ps.marginBottom);
                        ps.marginLeft = parseMarginInches(
                            dialogs.pageSetupLeftStr, ps.marginLeft);
                        ps.marginRight = parseMarginInches(
                            dialogs.pageSetupRightStr, ps.marginRight);
                        // Legacy uniform field mirrors the left margin.
                        ps.pageMargin = ps.marginLeft;
                        doc.isDirty = true;
                        dialogs.showPageSetup = false;
                        cmd::applyPageSettings(doc, layout);
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("pagesetup_cancel")
                            .with_label("Cancel")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showPageSetup = false;
                    }
                }
            }
        }

        // Tab Width input dialog - Win95 shell: square BUTTON_FACE
        // panel, navy caption, no backdrop dim, sunken field,
        // raised 90px buttons.
        if (dialogs.showTabWidthDialog) {
            constexpr int TABWIDTH_MODAL_ID = 50004;
            auto result = afterhours::modal(ctx, mk(entity, TABWIDTH_MODAL_ID),
                dialogs.showTabWidthDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(320), afterhours::ui::h720(130),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Tab Width", CONTENT_LAYER);

                // Prompt label: the unit is characters — one tab
                // expands to this many space characters.
                test_input::register_visible_text(
                    "Tab width in characters (1-16):");
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("Tab width in characters (1-16):")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                // Text input
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.tabWidthInputStr,
                    ah_win95::win95SunkenStyle()
                        .with_debug_name("tabwidth_input")
                        .with_size(ComponentSize{w1280(100), h720(28)})
                        .with_render_layer(CONTENT_LAYER));

                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = pixels(4)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));

                if (button(ctx, mk(buttonRow.ent(), 0),
                    ah_win95::win95DialogButtonStyle()
                        .with_debug_name("tabwidth_ok")
                        .with_label("OK")
                        .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - set tab width
                    if (!dialogs.tabWidthInputStr.empty()) {
                        int width = std::atoi(dialogs.tabWidthInputStr.c_str());
                        if (width >= 1 && width <= 16) {
                            doc.docSettings.tabWidth = width;
                            flash("Tab width set to " + std::to_string(width));
                        } else {
                            toast_notify::error("Tab width must be 1-16");
                        }
                    }
                    dialogs.tabWidthInputStr.clear();
                    dialogs.showTabWidthDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ah_win95::win95DialogButtonStyle()
                        .with_debug_name("tabwidth_cancel")
                        .with_label("Cancel")
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.tabWidthInputStr.clear();
                    dialogs.showTabWidthDialog = false;
                }
            }
        }
        
        // UI Settings dialog - Win95 shell: square BUTTON_FACE
        // panel, navy caption, no backdrop dim, sunken field,
        // raised 90px buttons.
        if (dialogs.showSettingsDialog) {
            constexpr int SETTINGS_MODAL_ID = 50005;
            auto result = afterhours::modal(ctx, mk(entity, SETTINGS_MODAL_ID),
                dialogs.showSettingsDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(380), afterhours::ui::h720(130),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "UI Settings", CONTENT_LAYER);

                // Prompt label
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("UI Scale (50% - 200%):")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                // Text input
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.uiScaleInputStr,
                    ah_win95::win95SunkenStyle()
                        .with_debug_name("ui_scale_input")
                        .with_size(ComponentSize{w1280(100), h720(28)})
                        .with_render_layer(CONTENT_LAYER));

                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = pixels(4)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));

                if (button(ctx, mk(buttonRow.ent(), 0),
                    ah_win95::win95DialogButtonStyle()
                        .with_debug_name("settings_ok")
                        .with_label("OK")
                        .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - set UI scale
                    if (!dialogs.uiScaleInputStr.empty()) {
                        int percentage = std::atoi(dialogs.uiScaleInputStr.c_str());
                        if (percentage >= 50 && percentage <= 200) {
                            float scale = static_cast<float>(percentage) / 100.0f;
                            Settings::get().set_ui_scale(scale);
                            flash("UI scale set to " + std::to_string(percentage) + "%");
                        } else {
                            toast_notify::error("UI scale must be 50-200%");
                        }
                    }
                    dialogs.uiScaleInputStr.clear();
                    dialogs.showSettingsDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ah_win95::win95DialogButtonStyle()
                        .with_debug_name("settings_cancel")
                        .with_label("Cancel")
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.uiScaleInputStr.clear();
                    dialogs.showSettingsDialog = false;
                }

                if (button(ctx, mk(buttonRow.ent(), 2),
                    ah_win95::win95DialogButtonStyle()
                        .with_debug_name("settings_reset")
                        .with_label("Reset")
                        .with_render_layer(CONTENT_LAYER))) {
                    Settings::get().set_ui_scale(1.0f);
                    dialogs.uiScaleInputStr = "100";
                    flash("UI scale reset to 100%");
                }
            }
        }
        
        // Go To dialog (Batch 9, Word 95): ONE dialog for
        // Page / Line / Bookmark — a "Go to what:" listbox,
        // a number/name field, Go To + Close. Bookmark
        // jumping (the old Go To Bookmark list) lives here:
        // the Bookmark target lists the document's bookmark
        // names under the field; clicking a name fills the
        // field, Go To jumps (and flashes when the name does
        // not exist). The dialog stays open after a jump so
        // the user can jump again; Close / Esc dismisses it.
        // Win95 shell (Batch 9 fix round): square BUTTON_FACE
        // panel, navy caption, no backdrop dim, sunken list /
        // field, raised 90px buttons — the Batch 1 dialog
        // shell, not the default modal.
        if (dialogs.showGoToDialog) {
            constexpr int GO_TO_MODAL_ID = 50007;
            // Height fits the content (title 20 + body 122 +
            // panel padding 8 + a little slack = 160); the
            // Bookmark names list grows it, capped at the
            // 3-row list below. A fixed 240 left ~90px of
            // empty panel under the buttons.
            float goToHeight = 160.0f;
            if (dialogs.goToTarget == 2) {
                std::size_t bookmarkRows = std::min<std::size_t>(
                    doc.buffer.bookmarks().size(), 3);
                float namesHeight =
                    bookmarkRows == 0
                        ? 20.0f
                        : static_cast<float>(bookmarkRows * 22 + 4);
                goToHeight += 6.0f + namesHeight;
            }
            auto result = afterhours::modal(ctx, mk(entity, GO_TO_MODAL_ID),
                dialogs.showGoToDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(340),
                    afterhours::ui::h720(goToHeight),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Go To", CONTENT_LAYER);

                // Body row (Word 95 layout): left "Go to what:"
                // list, right field + stacked Go To / Close.
                auto bodyRow = div(ctx, mk(result.ent(), 800),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), children()})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(pixels(8))
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));
                if (bodyRow) {
                    auto leftCol = div(ctx, mk(bodyRow.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(140), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(4))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (leftCol) {
                        // "Go to what:" label + sunken listbox.
                        div(ctx, mk(leftCol.ent(), 0),
                            ComponentConfig{}
                                .with_label("Go to what:")
                                .with_size(ComponentSize{percent(1.0f),
                                                         h720(20)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                        auto targetList = div(ctx, mk(leftCol.ent(), 1),
                            ah_win95::win95SunkenStyle()
                                .with_size(ComponentSize{percent(1.0f),
                                                         h720(76)})
                                .with_flex_direction(FlexDirection::Column)
                                .with_padding(Padding{.top = pixels(2),
                                                      .bottom = pixels(2)})
                                .with_render_layer(CONTENT_LAYER));
                        if (targetList) {
                            static const char* kGoToTargets[3] = {
                                "Page", "Line", "Bookmark"};
                            static const char* kGoToTargetDebug[3] = {
                                "goto_target_page", "goto_target_line",
                                "goto_target_bookmark"};
                            for (int t = 0; t < 3; ++t) {
                                bool selected = dialogs.goToTarget == t;
                                test_input::register_visible_text(
                                    kGoToTargets[t]);
                                if (button(ctx, mk(targetList.ent(), 10 + t),
                                    ComponentConfig{}
                                        .with_debug_name(
                                            kGoToTargetDebug[t])
                                        .with_label(kGoToTargets[t])
                                        .with_size(ComponentSize{
                                            percent(1.0f), h720(24)})
                                        .with_custom_background(
                                            selected
                                                ? ui_imm::win95_colors::
                                                      HIGHLIGHT
                                                : ui_imm::win95_colors::
                                                      TEXT_AREA)
                                        .with_custom_text_color(
                                            selected
                                                ? ui_imm::win95_colors::
                                                      TEXT_WHITE
                                                : ui_imm::win95_colors::TEXT)
                                        .with_alignment(
                                            afterhours::ui::TextAlignment::
                                                Left)
                                        .with_text_inset(4.0f, 0.0f)
                                        .with_roundness(0.0f)
                                        .with_cursor(
                                            afterhours::ui::CursorType::
                                                Pointer)
                                        .with_render_layer(
                                            CONTENT_LAYER))) {
                                    // Switching targets clears
                                    // the field: its meaning
                                    // changes (page vs line vs
                                    // name), so a stale number
                                    // must not carry over and
                                    // jump somewhere odd.
                                    if (dialogs.goToTarget != t) {
                                        dialogs.goToTarget = t;
                                        dialogs.goToInputStr.clear();
                                    }
                                }
                            }
                        }
                    }  // leftCol

                    auto rightCol = div(ctx, mk(bodyRow.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(184), children()})
                            .with_flex_direction(FlexDirection::Column)
                            .with_gap(pixels(6))
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (rightCol) {

                        // Number / name field, label per target.
                        const char* fieldLabel =
                            dialogs.goToTarget == 2
                                ? "Enter bookmark name:"
                                : dialogs.goToTarget == 1
                                      ? "Enter line number:"
                                      : "Enter page number:";
                        div(ctx, mk(rightCol.ent(), 0),
                            ComponentConfig{}
                                .with_label(fieldLabel)
                                .with_size(ComponentSize{percent(1.0f),
                                                         h720(20)})
                                .with_custom_background(
                                    ui_imm::win95_colors::BUTTON_FACE)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_alignment(
                                    afterhours::ui::TextAlignment::Left)
                                .with_render_layer(CONTENT_LAYER));
                        afterhours::text_input::text_input(
                            ctx, mk(rightCol.ent(), 1), dialogs.goToInputStr,
                            ah_win95::win95SunkenStyle()
                                .with_debug_name("goto_input")
                                .with_size(ComponentSize{percent(1.0f),
                                                         h720(28)})
                                .with_render_layer(CONTENT_LAYER));

                        // Bookmark names under the field
                        // (Bookmark target only): clicking one
                        // fills the field. Sunken list, capped at
                        // 3 rows (scrolls past that); the panel
                        // height above grows with this list.
                        if (dialogs.goToTarget == 2) {
                            const auto& bookmarks =
                                doc.buffer.bookmarks();
                            if (bookmarks.empty()) {
                                div(ctx, mk(rightCol.ent(), 2),
                                    ComponentConfig{}
                                        .with_label(
                                            "No bookmarks in document")
                                        .with_size(ComponentSize{
                                            percent(1.0f), h720(20)})
                                        .with_custom_background(
                                            ui_imm::win95_colors::BUTTON_FACE)
                                        .with_custom_text_color(
                                            ui_imm::win95_colors::TEXT)
                                        .with_alignment(
                                            afterhours::ui::TextAlignment::
                                                Left)
                                        .with_render_layer(CONTENT_LAYER));
                            } else {
                                std::size_t visibleRows =
                                    std::min<std::size_t>(bookmarks.size(),
                                                           3);
                                auto bookmarkList =
                                    div(ctx, mk(rightCol.ent(), 2),
                                        ah_win95::win95SunkenStyle()
                                            .with_size(ComponentSize{
                                                percent(1.0f),
                                                h720(static_cast<float>(
                                                    visibleRows * 22 + 4))})
                                            .with_flex_direction(
                                                FlexDirection::Column)
                                            .with_padding(
                                                Padding{.top = pixels(2),
                                                        .bottom = pixels(2)})
                                            .with_overflow(
                                                afterhours::ui::Overflow::
                                                    Auto)
                                            .with_render_layer(
                                                CONTENT_LAYER));
                                if (bookmarkList) {
                                    for (std::size_t bi = 0;
                                         bi < bookmarks.size(); ++bi) {
                                        const std::string& name =
                                            bookmarks[bi].name;
                                        test_input::register_visible_text(
                                            name);
                                        if (button(
                                                ctx,
                                                mk(bookmarkList.ent(),
                                                   1000 + static_cast<int>(
                                                              bi)),
                                                ComponentConfig{}
                                                    .with_debug_name(
                                                        "goto_bookmark_" +
                                                        std::to_string(bi))
                                                    .with_label(name)
                                                    .with_size(
                                                        ComponentSize{
                                                            percent(1.0f),
                                                            h720(22)})
                                                    .with_custom_background(
                                                        ui_imm::win95_colors::
                                                            TEXT_AREA)
                                                    .with_custom_text_color(
                                                        ui_imm::win95_colors::
                                                            TEXT)
                                                    .with_alignment(
                                                        afterhours::ui::
                                                            TextAlignment::
                                                                Left)
                                                    .with_padding(
                                                        Padding{.left =
                                                                    pixels(
                                                                        4)})
                                                    .with_text_overflow(
                                                        afterhours::ui::
                                                            TextOverflow::
                                                                Ellipsis)
                                                    .with_cursor(
                                                        afterhours::ui::
                                                            CursorType::
                                                                Pointer)
                                                    .with_roundness(0.0f)
                                                    .with_render_layer(
                                                        CONTENT_LAYER))) {
                                            dialogs.goToInputStr = name;
                                        }
                                    }
                                }
                            }
                        }

                        // Go To + Close, stacked under the
                        // field (Word 95): raised rectangular
                        // ~90px buttons, Go To the default.
                        if (button(ctx, mk(rightCol.ent(), 3),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("goto_go_button")
                                .with_label("Go To")
                                .with_border(
                                    ui_imm::win95_colors::TEXT, 1.0f)
                                .with_render_layer(CONTENT_LAYER))) {
                            cmd::performGoTo(doc, layout, cache_.scroll,
                                             dialogs);
                        }
                        if (button(ctx, mk(rightCol.ent(), 4),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("goto_close_button")
                                .with_label("Close")
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.showGoToDialog = false;
                        }
                    }  // rightCol
                }  // bodyRow
            }
        }
        
        // Help Window (Keyboard Shortcuts) - Win95 shell: square
        // BUTTON_FACE panel, navy caption, no backdrop dim, sunken
        // scrollable bindings list, raised 90px OK button. ESC
        // still dismisses via the modal's CloseRequest default.
        if (dialogs.showHelpWindow) {
            constexpr int HELP_MODAL_ID = 50008;
            auto result = afterhours::modal(ctx, mk(entity, HELP_MODAL_ID),
                dialogs.showHelpWindow,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(500), afterhours::ui::h720(480),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Keyboard Shortcuts",
                                              CONTENT_LAYER);

                // Get keybindings
                input::ActionMap defaultMap = input::createDefaultActionMap();
                auto bindings = input::getBindingsList(defaultMap);

                // Sunken bindings list (AutoCorrect pattern): the
                // outer box carries the sunken bevel, the inner area
                // scrolls (Overflow::Auto).
                auto listBox = div(ctx, mk(result.ent(), 0),
                    ah_win95::win95SunkenStyle()
                        .with_size(ComponentSize{percent(1.0f), h720(400)})
                        .with_flex_direction(FlexDirection::Column)
                        .with_padding(Padding{.top = pixels(2), .bottom = pixels(2)})
                        .with_render_layer(CONTENT_LAYER));
                if (listBox) {
                    // Header row
                    auto headerRow = div(ctx, mk(listBox.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(24)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_custom_background(
                                ui_imm::win95_colors::TEXT_AREA)
                            .with_render_layer(CONTENT_LAYER));

                    if (headerRow) {
                        auto headerCell = [&](int id, const char* label) {
                            div(ctx, mk(headerRow.ent(), id),
                                ComponentConfig{}
                                    .with_label(label)
                                    .with_size(ComponentSize{percent(0.5f), h720(20)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_font_weight(afterhours::colors::FontWeight::Bold)
                                    .with_font_size(h720(14))
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_padding(Padding{.left = pixels(4)})
                                    .with_render_layer(CONTENT_LAYER));
                        };
                        headerCell(0, "Action");
                        headerCell(1, "Shortcut");
                    }

                    // Bindings list
                    auto scrollArea = div(ctx, mk(listBox.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(372)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_overflow(afterhours::ui::Overflow::Auto)
                            .with_custom_background(
                                ui_imm::win95_colors::TEXT_AREA)
                            .with_render_layer(CONTENT_LAYER));
                    if (scrollArea) {
                        for (size_t i = 0; i < bindings.size(); ++i) {
                            const auto& binding = bindings[i];
                            auto bindingRow = div(ctx, mk(scrollArea.ent(), static_cast<int>(i)),
                                ComponentConfig{}
                                    .with_size(ComponentSize{percent(1.0f), h720(22)})
                                    .with_flex_direction(FlexDirection::Row)
                                    .with_custom_background(
                                        ui_imm::win95_colors::TEXT_AREA)
                                    .with_render_layer(CONTENT_LAYER));

                            if (bindingRow) {
                                auto bindingCell = [&](int id, const std::string& text) {
                                    div(ctx, mk(bindingRow.ent(), id),
                                        ComponentConfig{}
                                            .with_label(text)
                                            .with_size(ComponentSize{percent(0.5f), h720(18)})
                                            .with_custom_background(
                                                ui_imm::win95_colors::TEXT_AREA)
                                            .with_custom_text_color(
                                                ui_imm::win95_colors::TEXT)
                                            .with_alignment(afterhours::ui::TextAlignment::Left)
                                            .with_padding(Padding{.left = pixels(4)})
                                            .with_render_layer(CONTENT_LAYER));
                                };
                                bindingCell(0, binding.actionName);
                                bindingCell(1, binding.bindingStr);
                            }
                        }
                    }
                }

                // OK button at bottom
                auto buttonRow = div(ctx, mk(result.ent(), 1),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = pixels(4)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));

                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("help_ok")
                            .with_label("OK")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showHelpWindow = false;
                    }
                }
            }
        }
        
        // Find/Replace Dialog - Win95 shell: square BUTTON_FACE
        // panel, navy caption, no backdrop dim, sunken fields,
        // raised 90px buttons. Both modes kept: Find, and Replace
        // with the Replace field + options checkboxes.
        if (dialogs.showFindDialog) {
            constexpr int FIND_MODAL_ID = 50009;
            std::string title = dialogs.findReplaceMode ? "Find and Replace" : "Find";
            // Height fits the content: title(20) + Find label(20) +
            // Find field(28) [+ Replace label(20) + top margin(4) +
            // Replace field(28)] + options(22) + top margin(4) +
            // buttons(44) + top margin(4) + panel padding(8).
            float findHeight = dialogs.findReplaceMode ? 202.0f : 150.0f;
            auto result = afterhours::modal(ctx, mk(entity, FIND_MODAL_ID),
                dialogs.showFindDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(450), afterhours::ui::h720(findHeight),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              title, CONTENT_LAYER);

                // Find label and input
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("Find:")
                        .with_size(ComponentSize{percent(1.0f), h720(20)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_custom_text_color(ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_render_layer(CONTENT_LAYER));

                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.findInputStr,
                    ah_win95::win95SunkenStyle()
                        .with_debug_name("find_input")
                        .with_size(ComponentSize{percent(1.0f), h720(28)})
                        .with_render_layer(CONTENT_LAYER));

                // Replace label and input (only in replace mode)
                if (dialogs.findReplaceMode) {
                    div(ctx, mk(result.ent(), 2),
                        ComponentConfig{}
                            .with_label("Replace with:")
                            .with_size(ComponentSize{percent(1.0f), h720(20)})
                            .with_margin(Margin{.top = pixels(4)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_custom_text_color(ui_imm::win95_colors::TEXT)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(CONTENT_LAYER));

                    afterhours::text_input::text_input(ctx, mk(result.ent(), 3),
                        dialogs.replaceInputStr,
                        ah_win95::win95SunkenStyle()
                            .with_debug_name("replace_input")
                            .with_size(ComponentSize{percent(1.0f), h720(28)})
                            .with_render_layer(CONTENT_LAYER));
                }
                
                // Options checkboxes: Win95 sunken box + label
                // (AutoCorrect pattern). Each checkbox lives in a
                // fixed-width wrapper so the pair stays on one row.
                auto optionsRow = div(ctx, mk(result.ent(), 4),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(22)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = pixels(4)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));

                if (optionsRow) {
                    // Case sensitive checkbox
                    auto caseWrap = div(ctx, mk(optionsRow.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(170), h720(22)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (caseWrap) {
                        ah_win95::win95Checkbox(
                            ctx, mk(caseWrap.ent(), 0),
                            dialogs.findOptions.caseSensitive,
                            "find_case_sensitive", "Case sensitive",
                            CONTENT_LAYER);
                    }

                    // Whole word checkbox
                    auto wholeWrap = div(ctx, mk(optionsRow.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{w1280(150), h720(22)})
                            .with_custom_background(
                                ui_imm::win95_colors::BUTTON_FACE)
                            .with_render_layer(CONTENT_LAYER));
                    if (wholeWrap) {
                        ah_win95::win95Checkbox(
                            ctx, mk(wholeWrap.ent(), 0),
                            dialogs.findOptions.wholeWord,
                            "find_whole_word", "Whole word",
                            CONTENT_LAYER);
                    }
                }

                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 5),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = pixels(4)})
                        .with_custom_background(
                            ui_imm::win95_colors::BUTTON_FACE)
                        .with_render_layer(CONTENT_LAYER));

                if (buttonRow) {
                    // Find Next button (the default button: dark outline)
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("find_next")
                            .with_label("Find Next")
                            .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                            .with_render_layer(CONTENT_LAYER))) {
                        // Perform find
                        if (!dialogs.findInputStr.empty()) {
                            dialogs.lastSearchTerm = dialogs.findInputStr;
                            FindResult findResult = doc.buffer.findNext(dialogs.lastSearchTerm, dialogs.findOptions);
                            if (findResult.found) {
                                doc.buffer.setCaret(findResult.start);
                                doc.buffer.setSelectionAnchor(findResult.start);
                                doc.buffer.setCaret(findResult.end);
                                doc.buffer.updateSelectionToCaret();
                                flash("Found", 2.0f);
                            } else {
                                flash("Not found");
                            }
                        }
                    }
                    
                    // Replace button (only in replace mode)
                    if (dialogs.findReplaceMode) {
                        if (button(ctx, mk(buttonRow.ent(), 1),
                            ah_win95::win95DialogButtonStyle()
                                .with_debug_name("find_replace")
                                .with_label("Replace")
                                .with_render_layer(CONTENT_LAYER))) {
                            // Perform replace
                            if (!dialogs.findInputStr.empty()) {
                                dialogs.lastSearchTerm = dialogs.findInputStr;
                                dialogs.replaceTerm = dialogs.replaceInputStr;
                                
                                // Check if current selection matches the search term
                                if (doc.buffer.hasSelection()) {
                                    std::string selected = doc.buffer.getSelectedText();
                                    bool matches = dialogs.findOptions.caseSensitive ? 
                                        (selected == dialogs.lastSearchTerm) :
                                        ([&]() {
                                            std::string selLower = selected;
                                            std::string termLower = dialogs.lastSearchTerm;
                                            for (auto& c : selLower) c = static_cast<char>(std::tolower(c));
                                            for (auto& c : termLower) c = static_cast<char>(std::tolower(c));
                                            return selLower == termLower;
                                        })();
                                    
                                    if (matches) {
                                        doc.buffer.deleteSelection();
                                        doc.buffer.insertText(dialogs.replaceTerm);
                                        doc.isDirty = true;
                                        flash("Replaced");
                                        
                                        // Find next occurrence
                                        FindResult findResult = doc.buffer.findNext(dialogs.lastSearchTerm, dialogs.findOptions);
                                        if (findResult.found) {
                                            doc.buffer.setCaret(findResult.start);
                                            doc.buffer.setSelectionAnchor(findResult.start);
                                            doc.buffer.setCaret(findResult.end);
                                            doc.buffer.updateSelectionToCaret();
                                        }
                                    } else {
                                        flash("Selection doesn't match search term");
                                    }
                                } else {
                                    flash("No selection");
                                }
                            }
                        }
                    }
                    
                    // Close button
                    if (button(ctx, mk(buttonRow.ent(), 2),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("find_close")
                            .with_label("Close")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.findInputStr.clear();
                        dialogs.replaceInputStr.clear();
                        dialogs.showFindDialog = false;
                    }
                }
            }
        }
        
        // Revisions dialog (Tools > Revisions...): one sunken
        // listbox of grouped human rows (`Inserted "abc" — Ln 3`,
        // adjacent contiguous revisions of the same type in one
        // row — see revisions::groupRevisions) with a right button
        // column. Accept / Reject act on the selected GROUP (all
        // its members, one undo step); Accept All / Reject All are
        // wired to the existing Edit-menu commands. Deletes are
        // records only (see revisions.h: no keep-visible deleted
        // text in the buffer), so the dialog is where they are
        // visible; inserts additionally render green-underlined in
        // the text while their text still matches the buffer.
        if (dialogs.showRevisionsDialog) {
            constexpr int REVISIONS_MODAL_ID = 50013;
            auto result = afterhours::modal(ctx, mk(entity, REVISIONS_MODAL_ID),
                dialogs.showRevisionsDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(480), afterhours::ui::h720(300),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Revisions", CONTENT_LAYER);

                const auto revisionGroups =
                    revisions::groupRevisions(doc.revisions);
                auto clampSelection = [&]() {
                    if (revisionGroups.empty()) {
                        dialogs.revisionsSelected = -1;
                    } else if (dialogs.revisionsSelected >=
                               static_cast<int>(revisionGroups.size())) {
                        dialogs.revisionsSelected =
                            static_cast<int>(revisionGroups.size()) - 1;
                    }
                };

                // One sunken listbox + right button column (Spelling
                // / AutoCorrect layout): the rows are ONE continuous
                // white inset list, not separate cards — the
                // container carries the sunken bevel; the selected
                // row is navy/white. Accept / Reject act on the
                // selected GROUP; Accept All / Reject All resolve
                // every record; Close dismisses. Widths: the dialog
                // is 480 wide with 4px panel padding each side, so
                // the content row is 472; the button column is
                // w1280(100) (the 90-wide buttons, Spelling-style)
                // and the list takes the rest: 472 - 8 gap - 100
                // = 364. Widths MUST be w1280 (width axis), not
                // h720: h720 resolves against the width axis at
                // 1280/720 scale, so the old h720(320) list +
                // h720(120) column summed to ~790 in a 472 slot
                // and flex-shrink ran the navy row under the
                // Accept button. Row text sits 5px in from the
                // row edge (text_inset, the chip-popup pattern):
                // the old padding-left did not offset the label,
                // so the sunken bevel clipped the 'I' in
                // 'Inserted'.
                auto contentRow = div(ctx, mk(result.ent(), 1),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(216)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_render_layer(CONTENT_LAYER));
                if (contentRow) {
                    auto listBox = div(ctx, mk(contentRow.ent(), 10),
                        ah_win95::win95SunkenStyle()
                            .with_size(ComponentSize{afterhours::ui::w1280(364),
                                                    h720(216)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_overflow(afterhours::ui::Overflow::Auto)
                            .with_padding(Padding{.top = pixels(2),
                                                  .bottom = pixels(2),
                                                  .left = pixels(2),
                                                  .right = pixels(2)})
                            .with_render_layer(CONTENT_LAYER));
                    if (listBox) {
                        if (revisionGroups.empty()) {
                            div(ctx, mk(listBox.ent(), 0),
                                ComponentConfig{}
                                    .with_label("No revisions to review.")
                                    .with_size(ComponentSize{percent(1.0f),
                                                            h720(24)})
                                    .with_custom_background(
                                        ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(
                                        ui_imm::win95_colors::TEXT)
                                    .with_text_inset(5.0f, 0.0f)
                                    .with_render_layer(CONTENT_LAYER));
                        }
                        for (std::size_t i = 0; i < revisionGroups.size(); ++i) {
                            const revisions::RevisionGroup& group = revisionGroups[i];
                            const bool selected =
                                static_cast<int>(i) == dialogs.revisionsSelected;
                            const std::string rowLabel =
                                revisions::groupLabel(doc.buffer, group);
                            test_input::register_visible_text(rowLabel);
                            if (button(ctx, mk(listBox.ent(), 1000 + static_cast<int>(i)),
                                ComponentConfig{}
                                    .with_debug_name("revision_item_" + std::to_string(i))
                                    .with_label(rowLabel)
                                    .with_size(ComponentSize{percent(1.0f), h720(26)})
                                    .with_custom_background(selected
                                        ? ui_imm::win95_colors::HIGHLIGHT
                                        : ui_imm::win95_colors::TEXT_AREA)
                                    .with_custom_text_color(selected
                                        ? ui_imm::win95_colors::TEXT_WHITE
                                        : ui_imm::win95_colors::TEXT)
                                    .with_alignment(afterhours::ui::TextAlignment::Left)
                                    .with_text_inset(5.0f, 0.0f)
                                    .with_cursor(afterhours::ui::CursorType::Pointer)
                                    .with_render_layer(CONTENT_LAYER))) {
                                dialogs.revisionsSelected = static_cast<int>(i);
                            }
                        }
                    }
                    auto buttonCol = div(ctx, mk(contentRow.ent(), 11),
                        ComponentConfig{}
                            .with_size(ComponentSize{afterhours::ui::w1280(100),
                                                    h720(216)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_align_items(AlignItems::Center)
                            .with_gap(afterhours::ui::pixels(6))
                            .with_render_layer(CONTENT_LAYER));
                    if (buttonCol) {
                        auto dialogBtn = [&](int id, const char* debugName,
                                             const char* label,
                                             auto&& onClick) {
                            if (button(ctx, mk(buttonCol.ent(), id),
                                ah_win95::win95DialogButtonStyle()
                                    .with_debug_name(debugName)
                                    .with_label(label)
                                    .with_render_layer(CONTENT_LAYER))) {
                                onClick();
                                clampSelection();
                            }
                        };
                        dialogBtn(0, "revision_accept", "Accept", [&]() {
                            if (dialogs.revisionsSelected >= 0) {
                                cmd::acceptRevisionGroup(
                                    doc,
                                    static_cast<std::size_t>(dialogs.revisionsSelected));
                            }
                        });
                        dialogBtn(1, "revision_reject", "Reject", [&]() {
                            if (dialogs.revisionsSelected >= 0) {
                                cmd::rejectRevisionGroup(
                                    doc,
                                    static_cast<std::size_t>(dialogs.revisionsSelected));
                            }
                        });
                        dialogBtn(2, "revision_accept_all", "Accept All", [&]() {
                            cmd::acceptAllRevisions(doc);
                            dialogs.revisionsSelected = -1;
                        });
                        dialogBtn(3, "revision_reject_all", "Reject All", [&]() {
                            cmd::rejectAllRevisions(doc);
                            dialogs.revisionsSelected = -1;
                        });
                        dialogBtn(4, "revision_close", "Close", [&]() {
                            dialogs.showRevisionsDialog = false;
                        });
                    }
                }
            }
        }

        // Version History dialog (File > Version History...): one
        // sunken listbox of Save/Save As snapshots newest-first
        // ("Oct 18, 2026, 9:20 PM — 128 words"). Restore replaces
        // the buffer through the command layer (one undo) and
        // marks dirty; the dialog stays open so the result is
        // visible. Footer documents the flow: viewing old text =
        // Restore + Undo.
        if (dialogs.showVersionHistoryDialog) {
            constexpr int VERSION_HISTORY_MODAL_ID = 50014;
            // Height fits content via version_history layout
            // constants (see version_history.h): the list shrinks
            // to its rows and the dialog grows to cover every
            // child, so the restore-warning always sits BELOW
            // the list with an 8px gap — the old 26px-row
            // accounting plus the uncounted 24px button margin
            // left the panel ~20px short and the warning
            // painted over the bottom row.
            const int versionListH = version_history::listHeightForCount(
                dialogs.versionHistorySnapshots.size());
            const int versionModalH =
                version_history::dialogHeightForCount(
                    dialogs.versionHistorySnapshots.size());
            auto result = afterhours::modal(ctx, mk(entity, VERSION_HISTORY_MODAL_ID),
                dialogs.showVersionHistoryDialog,
                ah_win95::win95DialogConfig(
                    afterhours::ui::h720(480),
                    afterhours::ui::h720(versionModalH),
                    static_cast<float>(layout.screenWidth),
                    static_cast<float>(layout.screenHeight)));

            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;

                ah_win95::win95DialogTitleBar(ctx, mk(result.ent(), 900),
                                              "Version History", CONTENT_LAYER);

                auto listBox = div(ctx, mk(result.ent(), 1),
                    ah_win95::win95SunkenStyle()
                        .with_size(ComponentSize{percent(1.0f),
                                                 h720(versionListH)})
                        .with_flex_direction(FlexDirection::Column)
                        .with_overflow(afterhours::ui::Overflow::Auto)
                        .with_padding(Padding{.top = pixels(2),
                                              .bottom = pixels(2),
                                              .left = pixels(2),
                                              .right = pixels(2)})
                        .with_render_layer(CONTENT_LAYER));
                if (listBox) {
                    if (dialogs.versionHistorySnapshots.empty()) {
                        div(ctx, mk(listBox.ent(), 0),
                            ComponentConfig{}
                                .with_label("No saved versions yet. Save the document to create one.")
                                .with_size(ComponentSize{percent(1.0f), h720(24)})
                                .with_custom_background(
                                    ui_imm::win95_colors::TEXT_AREA)
                                .with_custom_text_color(
                                    ui_imm::win95_colors::TEXT)
                                .with_text_inset(5.0f, 0.0f)
                                .with_render_layer(CONTENT_LAYER));
                    }
                    for (std::size_t i = 0; i < dialogs.versionHistorySnapshots.size(); ++i) {
                        const version_history::VersionSnapshot& snap =
                            dialogs.versionHistorySnapshots[i];
                        const bool selected =
                            static_cast<int>(i) == dialogs.versionHistorySelected;
                        const std::string rowLabel =
                            version_history::displayLabel(snap.label,
                                                          snap.words);
                        test_input::register_visible_text(rowLabel);
                        if (button(ctx, mk(listBox.ent(), 1000 + static_cast<int>(i)),
                            ComponentConfig{}
                                .with_debug_name("version_item_" + std::to_string(i))
                                .with_label(rowLabel)
                                .with_size(ComponentSize{percent(1.0f), h720(26)})
                                .with_custom_background(selected
                                    ? ui_imm::win95_colors::HIGHLIGHT
                                    : ui_imm::win95_colors::TEXT_AREA)
                                .with_custom_text_color(selected
                                    ? ui_imm::win95_colors::TEXT_WHITE
                                    : ui_imm::win95_colors::TEXT)
                                .with_alignment(afterhours::ui::TextAlignment::Left)
                                .with_text_inset(5.0f, 0.0f)
                                .with_cursor(afterhours::ui::CursorType::Pointer)
                                .with_render_layer(CONTENT_LAYER))) {
                            dialogs.versionHistorySelected = static_cast<int>(i);
                        }
                    }
                }

                // Restore warning: BELOW the list with an 8px gap
                // (kWarningTopGap, counted in dialogHeightForCount),
                // never overlapping the bottom row.
                div(ctx, mk(result.ent(), 3),
                    ComponentConfig{}
                        .with_label("Restore replaces the document. Undo (Ctrl+Z) returns to the current text.")
                        .with_size(ComponentSize{percent(1.0f), h720(28)})
                        .with_margin(Margin{.top = pixels(8)})
                        .with_text_overflow(afterhours::ui::TextOverflow::Wrap)
                        .with_render_layer(CONTENT_LAYER));

                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("version_restore")
                            .with_label("Restore")
                            .with_render_layer(CONTENT_LAYER))) {
                        if (dialogs.versionHistorySelected >= 0 &&
                            dialogs.versionHistorySelected <
                                static_cast<int>(dialogs.versionHistorySnapshots.size())) {
                            const version_history::VersionSnapshot snap =
                                dialogs.versionHistorySnapshots[
                                    static_cast<std::size_t>(dialogs.versionHistorySelected)];
                            if (!std::filesystem::exists(snap.path)) {
                                toast_notify::error("Snapshot is gone; reopen Version History");
                                dialogs.versionHistorySnapshots =
                                    cmd::versionSnapshots(doc);
                                dialogs.versionHistorySelected =
                                    dialogs.versionHistorySnapshots.empty() ? -1 : 0;
                            } else {
                                cmd::restoreVersion(
                                    doc, version_history::readSnapshot(snap.path));
                                flash("Restored version: " + snap.label);
                            }
                        }
                    }
                    if (button(ctx, mk(buttonRow.ent(), 1),
                        ah_win95::win95DialogButtonStyle()
                            .with_debug_name("version_history_close")
                            .with_label("Close")
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showVersionHistoryDialog = false;
                    }
                }
            }
        }

        // Margin comment cards (Docs comments, Win95 dress): one card
        // per comment, anchored to its anchor row (card top aligns
        // with the anchor row). Paged mode: the card sits fully
        // OUTSIDE the page (page right edge + 8, kept on screen),
        // with a 1px dark leader line from the page edge to the
        // card at the anchor row's centre. Pageless: docked at the
        // right edge of the text area (inside the margin band, left
        // of the scrollbar). Every card carries a 1px dark border
        // + raised bevel frame; Resolve is a small raised button.
        // Cards on the same row stack below each other. The card
        // body is a button (click -> caret to the anchor start);
        // Resolve is a higher-layer button on top, so a click on it
        // fires only Resolve — the UI click resolver picks the
        // single topmost candidate. Cards are UI, not render-system
        // rects, so E2E can click_ui them (comment_card_<i>,
        // comment_resolve_<i>). Split view shows the top pane's
        // cards only; Print Preview shows none (printable content).
        if (!layout.previewMode && cache_.scroll &&
            !doc.comments.empty() && !layout.focusMode) {
            using namespace afterhours::ui;
            using namespace afterhours::ui::imm;
            constexpr float CARD_W = 180.0f;
            constexpr float CARD_H = 72.0f;
            constexpr int CARD_LAYER = 60;
            const auto effective = layout::effectiveTextArea(layout);
            const float ds = layout::displayScale(layout);
            const int cardFontSize =
                ruler_math::scaledFontSize(doc.buffer.textStyle().fontSize,
                                           ds);
            const int cardLineHeight = cardFontSize + 4;
            const std::time_t now = std::time(nullptr);
            std::vector<std::pair<float, float>> placed;
            int cardClicked = -1;
            int resolveClicked = -1;
            for (std::size_t ci = 0; ci < doc.comments.size(); ++ci) {
                const Comment& comment = doc.comments[ci];
                CaretPosition pos =
                    doc.buffer.positionForOffset(comment.startOffset);
                if (pos.row < static_cast<std::size_t>(cache_.scroll->offset)) {
                    continue;
                }
                float cardY = effective.y +
                              static_cast<float>(theme::layout::TEXT_PADDING) +
                              static_cast<float>(pos.row - static_cast<std::size_t>(cache_.scroll->offset)) *
                                  static_cast<float>(cardLineHeight);
                float cardX = 0.0f;
                float pageRight = 0.0f;
                if (layout.pageMode == PageMode::Paged) {
                    // Fully OUTSIDE the page: the card starts at the
                    // page's right edge + gap, not at the text-area
                    // edge (the old x sat inside the page's right
                    // margin band and overlapped the page by ~32px).
                    // Only when the window cannot fit page + gap +
                    // card does it clamp back on screen (overlapping
                    // the page again — the narrow-window fallback).
                    pageRight =
                        layout.pageOffsetX + layout.pageDisplayWidth;
                    cardX = pageRight + 8.0f;
                    const float maxX = static_cast<float>(layout.screenWidth) -
                                       CARD_W - 4.0f;
                    if (cardX > maxX) cardX = maxX;
                } else {
                    cardX = layout.textArea.x + layout.textArea.width -
                            scroll_math::SCROLLBAR_WIDTH - CARD_W - 4.0f;
                }
                if (cardY > layout.textArea.y + layout.textArea.height - CARD_H) {
                    continue;
                }
                // The anchor row's top: the leader line sits at
                // this row's centre when the card is not stacked.
                const float anchorTopY = cardY;
                // Stack cards that would overlap a placed card.
                bool stacked = false;
                for (const auto& prev : placed) {
                    if (cardY < prev.second + CARD_H + 4.0f &&
                        cardY + CARD_H + 4.0f > prev.second) {
                        cardY = prev.second + CARD_H + 4.0f;
                        stacked = true;
                    }
                }
                if (cardY > layout.textArea.y + layout.textArea.height - CARD_H) {
                    continue;
                }
                placed.emplace_back(cardX, cardY);
                dialogs.clickShields.push_back(ClickShieldRect{
                    cardX, cardY, CARD_W, CARD_H, 0});
                const int baseId = 40000 + static_cast<int>(ci) * 10;
                test_input::register_visible_text(comment.author);
                test_input::register_visible_text(comment.text);
                div(ctx, mk(entity, baseId),
                    ComponentConfig{}
                        .with_size(ComponentSize{pixels(CARD_W),
                                                 pixels(CARD_H)})
                        .with_absolute_position()
                        .with_translate(cardX, cardY)
                        .with_custom_background(
                            afterhours::Color{255, 255, 220, 255})
                        .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                        .with_roundness(0.0f)
                        .with_render_layer(CARD_LAYER));
                if (button(ctx, mk(entity, baseId + 1),
                           ComponentConfig{}
                               .with_debug_name("comment_card_" +
                                               std::to_string(ci))
                               .with_label("")
                               .with_size(ComponentSize{pixels(CARD_W),
                                                        pixels(CARD_H)})
                               .with_absolute_position()
                               .with_translate(cardX, cardY)
                               .with_custom_background(
                                   afterhours::Color{255, 255, 220, 255})
                               // 1px dark border + raised bevel frame.
                               // The border used to live only on the
                               // base div below, which this button
                               // (same rect, higher layer) painted
                               // over — so no border ever showed. The
                               // border draws after (over) the bevel
                               // and covers its outer 1px, so the
                               // bevel runs at thickness 2 to leave
                               // its inner highlight visible.
                               .with_border(ui_imm::win95_colors::TEXT,
                                           1.0f)
                               .with_bevel(afterhours::ui::BevelStyle::Raised,
                                           ui_imm::win95_colors::BORDER_LIGHT,
                                           ui_imm::win95_colors::BORDER_DARK,
                                           2.0f)
                               .with_roundness(0.0f)
                               .with_cursor(afterhours::ui::CursorType::Pointer)
                               .with_render_layer(CARD_LAYER + 1))) {
                    cardClicked = static_cast<int>(ci);
                }
                div(ctx, mk(entity, baseId + 2),
                    ComponentConfig{}
                        .with_label(comment.author + " - " +
                                    comments::relativeDateLabel(
                                        comment.createdAt, now))
                        .with_size(ComponentSize{pixels(CARD_W - 8.0f),
                                                 pixels(18.0f)})
                        .with_absolute_position()
                        .with_translate(cardX + 4.0f, cardY + 4.0f)
                        .with_custom_background(
                            afterhours::Color{255, 255, 220, 255})
                        .with_custom_text_color(
                            ui_imm::win95_colors::TEXT)
                        .with_font_weight(
                            afterhours::colors::FontWeight::Bold)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_roundness(0.0f)
                        .with_render_layer(CARD_LAYER + 2));
                div(ctx, mk(entity, baseId + 3),
                    ComponentConfig{}
                        .with_label(comment.text)
                        .with_size(ComponentSize{pixels(CARD_W - 8.0f),
                                                 pixels(22.0f)})
                        .with_absolute_position()
                        .with_translate(cardX + 4.0f, cardY + 24.0f)
                        .with_custom_background(
                            afterhours::Color{255, 255, 220, 255})
                        .with_custom_text_color(
                            ui_imm::win95_colors::TEXT)
                        .with_alignment(afterhours::ui::TextAlignment::Left)
                        .with_text_overflow(
                            afterhours::ui::TextOverflow::Ellipsis)
                        .with_roundness(0.0f)
                        .with_render_layer(CARD_LAYER + 2));
                if (button(ctx, mk(entity, baseId + 4),
                           ComponentConfig{}
                               .with_debug_name("comment_resolve_" +
                                               std::to_string(ci))
                               .with_label("Resolve")
                               .with_size(ComponentSize{pixels(64.0f),
                                                        pixels(20.0f)})
                               .with_absolute_position()
                               .with_translate(cardX + CARD_W - 68.0f,
                                               cardY + CARD_H - 24.0f)
                               .with_custom_background(
                                   ui_imm::win95_colors::BUTTON_FACE)
                               .with_custom_text_color(
                                   ui_imm::win95_colors::TEXT)
                               .with_bevel(afterhours::ui::BevelStyle::Raised,
                                           ui_imm::win95_colors::BORDER_LIGHT,
                                           ui_imm::win95_colors::BORDER_DARK,
                                           1.0f)
                               .with_roundness(0.0f)
                               .with_cursor(afterhours::ui::CursorType::Pointer)
                               .with_render_layer(CARD_LAYER + 3))) {
                    resolveClicked = static_cast<int>(ci);
                }
                // Anchor connector (paged): a 1px dark leader line
                // from the page's right edge to the card, at the
                // anchor row's centre (comments::leaderLine).
                // Unstacked, the card top IS the anchor row top,
                // so the line sits at the anchor row; a stacked
                // card has moved off its anchor row, so the line
                // sits at the card's centre and still touches the
                // card. Skipped when the card was clamped (no gap
                // left to bridge). Drawn last at a topmost layer
                // so no card piece covers it. Pageless keeps its
                // current behaviour (no line): the card is docked
                // inside the text area, not across a page edge.
                if (layout.pageMode == PageMode::Paged) {
                    const float leaderTop = stacked ? cardY : anchorTopY;
                    const comments::LeaderLine leader =
                        comments::leaderLine(
                            pageRight, cardX,
                            leaderTop +
                                static_cast<float>(cardLineHeight) / 2.0f);
                    if (leader.present) {
                        div(ctx, mk(entity, baseId + 5),
                            ComponentConfig{}
                                .with_size(ComponentSize{
                                    pixels(leader.width),
                                    pixels(leader.height)})
                                .with_absolute_position()
                                .with_translate(leader.x, leader.y)
                                .with_custom_background(
                                    ui_imm::win95_colors::TEXT)
                                .with_roundness(0.0f)
                                .with_render_layer(CARD_LAYER + 2));
                    }
                }
            }
            if (resolveClicked >= 0 &&
                resolveClicked < static_cast<int>(doc.comments.size())) {
                doc.comments.erase(doc.comments.begin() + resolveClicked);
                doc.isDirty = true;
            } else if (cardClicked >= 0 &&
                       cardClicked < static_cast<int>(doc.comments.size())) {
                doc.buffer.clearSelection();
                doc.buffer.setCaret(doc.buffer.positionForOffset(
                    doc.comments[static_cast<std::size_t>(cardClicked)]
                        .startOffset));
            }
        }

        // @ smart chip popup (Docs chips, Win95 dress): a small
        // Win95 listbox at the caret listing the filtered items.
        // Arrow/Enter/click selects (TextInputSystem owns Enter,
        // NavigationSystem owns Up/Down); Esc or any outside click
        // dismisses. The popup never renders in Print Preview,
        // Focus mode, or while a modal dialog owns input.
        if (dialogs.showChipPopup) {
            if (!cmd::chipPopupValid(doc, dialogs) ||
                layout.previewMode || layout.focusMode ||
                layout.headerEditTarget != HeaderEditTarget::None ||
                ScrollbarSystem::anyModalDialogOpen(dialogs) ||
                !cache_.scroll) {
                dialogs.showChipPopup = false;
            } else {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                const auto items = chips::popupItems(
                    cmd::chipPopupFilter(doc, dialogs), std::time(nullptr));
                constexpr float POPUP_W = 240.0f;
                constexpr float ITEM_H = 22.0f;
                const float popupH =
                    static_cast<float>(std::max<std::size_t>(items.size(), 1)) *
                        ITEM_H +
                    4.0f;
                const auto effective = layout::effectiveTextArea(layout);
                const float ds = layout::displayScale(layout);
                const int popupFontSize =
                    ruler_math::scaledFontSize(doc.buffer.textStyle().fontSize,
                                               ds);
                const int popupLineHeight = popupFontSize + 4;
                CaretPosition caretPos = doc.buffer.caret();
                std::string line =
                    caretPos.row < doc.buffer.lineCount()
                        ? doc.buffer.lineString(caretPos.row)
                        : "";
                std::string prefix =
                    line.substr(0, std::min(caretPos.column, line.size()));
                float popupX =
                    effective.x +
                    static_cast<float>(theme::layout::TEXT_PADDING) +
                    static_cast<float>(afterhours::graphics::measure_text(
                        prefix.c_str(), popupFontSize));
                if (layout.showLineNumbers) {
                    popupX += layout.lineNumberGutterWidth;
                }
                float caretLineY =
                    effective.y +
                    static_cast<float>(theme::layout::TEXT_PADDING) +
                    static_cast<float>(
                        static_cast<int>(caretPos.row) - cache_.scroll->offset) *
                        static_cast<float>(popupLineHeight);
                float popupY = caretLineY + static_cast<float>(popupLineHeight);
                if (popupX > static_cast<float>(layout.screenWidth) - POPUP_W - 4.0f) {
                    popupX = static_cast<float>(layout.screenWidth) - POPUP_W - 4.0f;
                }
                if (popupX < effective.x) popupX = effective.x;
                const float statusH =
                    theme::layout::scale(theme::layout::STATUS_BAR_HEIGHT);
                if (popupY + popupH >
                    static_cast<float>(layout.screenHeight) - statusH - 4.0f) {
                    popupY = caretLineY - popupH;
                }
                if (popupY < layout.textArea.y) {
                    popupY = layout.textArea.y;
                }
                dialogs.clickShields.push_back(ClickShieldRect{
                    popupX, popupY, POPUP_W, popupH, 1});
                div(ctx, mk(entity, 41000),
                    ComponentConfig{}
                        .with_size(ComponentSize{pixels(POPUP_W),
                                                 pixels(popupH)})
                        .with_absolute_position()
                        .with_translate(popupX, popupY)
                        .with_custom_background(
                            ui_imm::win95_colors::TEXT_AREA)
                        .with_border(ui_imm::win95_colors::TEXT, 1.0f)
                        .with_roundness(0.0f)
                        .with_render_layer(70));
                if (items.empty()) {
                    test_input::register_visible_text("(no matches)");
                    div(ctx, mk(entity, 41001),
                        ComponentConfig{}
                            .with_label("(no matches)")
                            .with_size(ComponentSize{pixels(POPUP_W - 4.0f),
                                                     pixels(ITEM_H)})
                            .with_absolute_position()
                            .with_translate(popupX + 2.0f, popupY + 2.0f)
                            .with_custom_background(
                                ui_imm::win95_colors::TEXT_AREA)
                            .with_custom_text_color(
                                ui_imm::win95_colors::TEXT_DISABLED)
                            .with_alignment(
                                afterhours::ui::TextAlignment::Left)
                            .with_roundness(0.0f)
                            .with_render_layer(71));
                }
                for (std::size_t i = 0; i < items.size(); ++i) {
                    const bool selected =
                        static_cast<int>(i) == dialogs.chipPopupSelected;
                    test_input::register_visible_text(items[i].label);
                    if (button(ctx, mk(entity, 41010 + static_cast<int>(i) * 10),
                               ComponentConfig{}
                                   .with_debug_name("chip_item_" +
                                                   std::to_string(i))
                                   .with_label(items[i].label)
                                   .with_size(ComponentSize{
                                       pixels(POPUP_W - 4.0f),
                                       pixels(ITEM_H)})
                                   .with_absolute_position()
                                   .with_translate(
                                       popupX + 2.0f,
                                       popupY + 2.0f +
                                           static_cast<float>(i) * ITEM_H)
                                   .with_custom_background(
                                       selected
                                           ? ui_imm::win95_colors::HIGHLIGHT
                                           : ui_imm::win95_colors::TEXT_AREA)
                                   .with_custom_text_color(
                                       selected
                                           ? ui_imm::win95_colors::TEXT_WHITE
                                           : ui_imm::win95_colors::TEXT)
                                   .with_alignment(
                                       afterhours::ui::TextAlignment::Left)
                                   .with_text_inset(4.0f, 0.0f)
                                   .with_roundness(0.0f)
                                   .with_cursor(
                                       afterhours::ui::CursorType::Pointer)
                                   .with_render_layer(71))) {
                        dialogs.chipPopupSelected = static_cast<int>(i);
                        cmd::applyChipPopup(doc, dialogs);
                    }
                }
            }
        }

        // Right-click detection for context menu (real input, not E2E)
        if (!dialogs.showContextMenu && !dialogs.showFindDialog &&
            !dialogs.showAboutDialog && !dialogs.showHelpWindow &&
            !dialogs.showCommentDialog && !dialogs.showTemplateDialog &&
            !dialogs.showDateTimeDialog && !dialogs.showFontDialog &&
            !dialogs.showColumnsDialog && !dialogs.showParagraphDialog &&
            !dialogs.showBordersShadingDialog &&
            !dialogs.showSpellingDialog && !dialogs.showGrammarDialog &&
            !dialogs.showAutoCorrectDialog &&
            !dialogs.showWordCountDialog && !dialogs.showTabWidthDialog &&
            !dialogs.showSettingsDialog && !dialogs.showGoToDialog &&
            !dialogs.showSaveAsDialog && !dialogs.showPageSetup &&
            !dialogs.showRevisionsDialog &&
            !dialogs.showVersionHistoryDialog) {
            if (input::isMouseButtonPressed(1)) {
                auto mousePos = input::getMousePosition();
                dialogs.showContextMenu = true;
                dialogs.contextMenuX = mousePos.x;
                dialogs.contextMenuY = mousePos.y;
            }
        }

        // Context menu (right-click). afterhours::ui::imm::context_menu owns the
        // panel, the shortcut gutter, the separator band, screen-edge clamping
        // and dismiss-on-click-outside; we own the items and what they do.
        if (dialogs.showContextMenu) {
            using namespace afterhours::ui;

            constexpr int CTX_BASE_ID = 50020;
            constexpr int CTX_LAYER = 1100;
            constexpr float ITEM_HEIGHT = 22.0f;
            constexpr float MENU_WIDTH = 180.0f;

            // Indices are positional and separators occupy one, so this enum has
            // to stay in lockstep with the item list below.
            enum ContextItem : int {
                CtxCut = 0,
                CtxCopy,
                CtxPaste,
                CtxSeparator,
                CtxSelectAll,
            };

            const std::array<imm::MenuItem, 5> items{
                imm::MenuItem{.label = "Cut", .shortcut = "Ctrl+X"},
                imm::MenuItem{.label = "Copy", .shortcut = "Ctrl+C"},
                imm::MenuItem{.label = "Paste", .shortcut = "Ctrl+V"},
                imm::MenuItem::sep(),
                imm::MenuItem{.label = "Select All", .shortcut = "Ctrl+A"},
            };

            const int chosen = imm::context_menu(
                ctx, mk(entity, CTX_BASE_ID), items,
                Vector2Type{dialogs.contextMenuX, dialogs.contextMenuY},
                dialogs.showContextMenu,
                ComponentConfig{}
                    .with_size(ComponentSize{pixels(MENU_WIDTH), pixels(ITEM_HEIGHT)})
                    .with_render_layer(CTX_LAYER)
                    .with_debug_name("context_menu"));

            switch (chosen) {
                case CtxCut: cmd::cut(doc); break;
                case CtxCopy: cmd::copy(doc); break;
                case CtxPaste: cmd::paste(doc); break;
                case CtxSelectAll: cmd::selectAll(doc); break;
                default: break;
            }

            // Escape is ours: the menu only closes itself on focus loss.
            if (input::isKeyPressed(afterhours::keys::ESCAPE)) {
                dialogs.showContextMenu = false;
            }
        }

        // Dispatch menu actions: consume click result set earlier and run handler
        {
            int menuResult = menu.consumeClickedResult();
            if (menuResult >= 0) {
                handleMenuActionImpl(menuResult, doc, menu, dialogs, layout,
                                     cache_.status, cache_.scroll);
                // Entering Print Preview from a menu (or E2E
                // menu_select, which does not clear
                // activeMenuIndex): no menu may stay open or
                // highlighted in preview.
                if (layout.previewMode) {
                    for (auto& m : menu.menus) m.open = false;
                    menu.activeMenuIndex = -1;
                }
                // The menu just closed, so the dropdown-item entity
                // that owned keyboard focus dies next frame — and
                // its EntityID can be recycled to the File header,
                // painting a pressed-looking focus border on File
                // with no menu open (seen in Print Preview
                // screenshots). Clearing to ROOT alone does not
                // hold — HandleTabbing's try_to_grab re-grabs the
                // File header from ROOT on the next frame — but
                // this dispatch runs on a click frame, where FAKE
                // is poisonous (see the guard below), so it stays
                // ROOT and the idle guard parks FAKE right after.
                // Dialogs grab their own focus.
                ctx.set_focus(ctx.ROOT);
            }
        }

        // F1 to toggle help window
        if (input::isKeyPressed(afterhours::keys::F1)) {
            dialogs.showHelpWindow = !dialogs.showHelpWindow;
            dialogs.helpScrollOffset = 0;
        }

        // No menu and no modal dialog is open, so no UI widget
        // should hold focus: focus is in the document. Park it on
        // FAKE, the afterhours "no widget" sentinel, on idle
        // frames only (mouse up, no press/release edge). Clearing
        // to ROOT does not hold — HandleTabbing's try_to_grab
        // re-grabs the first focusable widget, the File header,
        // from ROOT on the next frame, and with has_interacted
        // set by any click the focus ring is painted on File with
        // no menu open and no hover intended (the framed look in
        // headless screenshots). FAKE is skipped by try_to_grab
        // and draws no ring. FAKE must ONLY be set here, on an
        // idle frame: on a click frame it is poisonous.
        // EndUIContextManager early-returns when focus is FAKE
        // and skips resetting active_id to ROOT on the release
        // frame, so active_id stays stuck on the dismissed widget
        // and ResolveHitTarget hot-locks every later click to that
        // dead widget (a preview Close followed by a toolbar
        // dropdown click never landed). Click-frame clears (menu
        // dispatch, preview Close, dropdown items) stay ROOT for
        // exactly that reason; this guard parks FAKE on the next
        // idle frame, before any screenshot or steady-state
        // render. This also covers the two menu-close paths that
        // never cleared focus at all — Esc closing a menu, and a
        // click outside a menu closing it: both leave no menu or
        // dialog open, so the guard parks focus here. It skips
        // while a menu or dialog is open, so menu/dialog keyboard
        // focus keeps working. Hover/open highlight is menu
        // state, untouched by this.
        {
            bool anyMenuStillOpen = false;
            for (const auto& m : menu.menus) {
                if (m.open) {
                    anyMenuStillOpen = true;
                    break;
                }
            }
            if (!anyMenuStillOpen &&
                !ScrollbarSystem::anyModalDialogOpen(dialogs) &&
                !dialogs.showQuitPrompt &&
                !ctx.mouse.just_pressed && !ctx.mouse.just_released &&
                !ctx.mouse.left_down) {
                ctx.set_focus(ctx.FAKE);
            }
        }

        // Restore the global theme so subsequent systems (toolbar, etc.) use the correct theme
        ctx.theme = savedTheme;
    }
};

}  // namespace ecs

