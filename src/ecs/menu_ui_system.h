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
#include "editor_entity_cache.h"
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

        // Keep the View > Ruler checkmark in sync: any menu rebuild
        // resets marks to the createMenuBar defaults.
        for (auto& m : menu.menus) {
            if (m.label != "View") continue;
            for (auto& item : m.items) {
                if (item.label == "Ruler") {
                    item.mark = layout.showRuler ? win95::MenuMark::Checkmark
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
                    .with_custom_background(highlighted ? toAhColor(theme::MENU_HOVER) : toAhColor(theme::MENU_BG))
                    .with_custom_text_color(highlighted ? toAhColor(theme::MENU_TEXT_HOVER) : toAhColor(theme::MENU_TEXT))
                    .with_alignment(afterhours::ui::TextAlignment::Left)
                    .with_justify_content(afterhours::ui::JustifyContent::FlexStart)
                    .with_align_items(afterhours::ui::AlignItems::Center)
                    .with_padding(afterhours::ui::Padding{.left = pixels(headerPadding),
                                                         .right = pixels(headerPadding)})
                    .with_roundness(0.0f)
                    .with_bevel(highlighted ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
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
                            case win95::MenuMark::Checkmark: markPrefix = "\xE2\x9C\x93 "; break;
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
                            toast_notify::success("Added to dictionary: " + added);
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

        // Comment input dialog
        if (dialogs.showCommentDialog) {
            constexpr int COMMENT_MODAL_ID = 50002;
            auto result = afterhours::modal(ctx, mk(entity, COMMENT_MODAL_ID),
                dialogs.showCommentDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(360), afterhours::ui::h720(180))
                    .with_title("Add Comment"));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                // Prompt label
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("Comment:")
                        .with_size(ComponentSize{percent(1.0f), h720(24)})
                        .with_render_layer(CONTENT_LAYER));
                
                // Text input
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.commentInputStr,
                    ComponentConfig{}
                        .with_debug_name("comment_input")
                        .with_size(ComponentSize{percent(1.0f), h720(32)})
                        .with_background(Theme::Usage::Surface)
                        .with_render_layer(CONTENT_LAYER));
                
                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_cursor(afterhours::ui::CursorType::Pointer)
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - add comment
                    if (!dialogs.commentInputStr.empty()) {
                        Comment comment;
                        comment.startOffset = dialogs.pendingCommentStart;
                        comment.endOffset = dialogs.pendingCommentEnd;
                        comment.author = "User";
                        comment.text = dialogs.commentInputStr;
                        comment.createdAt = std::time(nullptr);
                        doc.comments.push_back(comment);
                        toast_notify::success("Comment added");
                    }
                    dialogs.commentInputStr.clear();
                    dialogs.showCommentDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.commentInputStr.clear();
                    dialogs.showCommentDialog = false;
                }
            }
        }
        
        // Template input dialog
        if (dialogs.showTemplateDialog) {
            constexpr int TEMPLATE_MODAL_ID = 50003;
            auto result = afterhours::modal(ctx, mk(entity, TEMPLATE_MODAL_ID),
                dialogs.showTemplateDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(380), afterhours::ui::h720(180))
                    .with_title("New from Template"));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                // Prompt label
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("Template (letter/memo/report/resume/essay):")
                        .with_size(ComponentSize{percent(1.0f), h720(24)})
                        .with_render_layer(CONTENT_LAYER));
                
                // Text input
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.templateInputStr,
                    ComponentConfig{}
                        .with_debug_name("template_input")
                        .with_size(ComponentSize{percent(1.0f), h720(32)})
                        .with_background(Theme::Usage::Surface)
                        .with_render_layer(CONTENT_LAYER));
                
                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_debug_name("template_ok")
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_cursor(afterhours::ui::CursorType::Pointer)
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - load template
                    if (!dialogs.templateInputStr.empty()) {
                        std::string name = dialogs.templateInputStr;
                        for (auto& ch : name) ch = static_cast<char>(std::tolower(ch));
                        std::filesystem::path templatePath =
                            std::filesystem::current_path() / "resources/templates" /
                            (name + ".txt");
                        if (std::filesystem::exists(templatePath)) {
                            std::ifstream ifs(templatePath);
                            std::stringstream buffer;
                            buffer << ifs.rdbuf();
                            doc.buffer.setText(buffer.str());
                            doc.isDirty = true;
                            toast_notify::success("Template loaded: " + name);
                        } else {
                            toast_notify::error("Template not found: " + name);
                        }
                    }
                    dialogs.templateInputStr.clear();
                    dialogs.showTemplateDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.templateInputStr.clear();
                    dialogs.showTemplateDialog = false;
                }
            }
        }
        
        // Tab Width input dialog
        if (dialogs.showTabWidthDialog) {
            constexpr int TABWIDTH_MODAL_ID = 50004;
            auto result = afterhours::modal(ctx, mk(entity, TABWIDTH_MODAL_ID),
                dialogs.showTabWidthDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(320), afterhours::ui::h720(180))
                    .with_title("Tab Width"));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                // Prompt label
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("Spaces per tab (1-16):")
                        .with_size(ComponentSize{percent(1.0f), h720(24)})
                        .with_render_layer(CONTENT_LAYER));
                
                // Text input
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.tabWidthInputStr,
                    ComponentConfig{}
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Surface)
                        .with_render_layer(CONTENT_LAYER));
                
                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_cursor(afterhours::ui::CursorType::Pointer)
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - set tab width
                    if (!dialogs.tabWidthInputStr.empty()) {
                        int width = std::atoi(dialogs.tabWidthInputStr.c_str());
                        if (width >= 1 && width <= 16) {
                            doc.docSettings.tabWidth = width;
                            toast_notify::success("Tab width set to " + std::to_string(width));
                        } else {
                            toast_notify::error("Tab width must be 1-16");
                        }
                    }
                    dialogs.tabWidthInputStr.clear();
                    dialogs.showTabWidthDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.tabWidthInputStr.clear();
                    dialogs.showTabWidthDialog = false;
                }
            }
        }
        
        // UI Settings dialog
        if (dialogs.showSettingsDialog) {
            constexpr int SETTINGS_MODAL_ID = 50005;
            auto result = afterhours::modal(ctx, mk(entity, SETTINGS_MODAL_ID),
                dialogs.showSettingsDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(380), afterhours::ui::h720(220))
                    .with_title("UI Settings"));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                // Prompt label
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("UI Scale (50% - 200%):")
                        .with_size(ComponentSize{percent(1.0f), h720(24)})
                        .with_render_layer(CONTENT_LAYER));
                
                // Text input
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.uiScaleInputStr,
                    ComponentConfig{}
                        .with_size(ComponentSize{h720(100), h720(32)})
                        .with_background(Theme::Usage::Surface)
                        .with_render_layer(CONTENT_LAYER));
                
                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_cursor(afterhours::ui::CursorType::Pointer)
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - set UI scale
                    if (!dialogs.uiScaleInputStr.empty()) {
                        int percentage = std::atoi(dialogs.uiScaleInputStr.c_str());
                        if (percentage >= 50 && percentage <= 200) {
                            float scale = static_cast<float>(percentage) / 100.0f;
                            Settings::get().set_ui_scale(scale);
                            toast_notify::success("UI scale set to " + std::to_string(percentage) + "%");
                        } else {
                            toast_notify::error("UI scale must be 50-200%");
                        }
                    }
                    dialogs.uiScaleInputStr.clear();
                    dialogs.showSettingsDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_cursor(afterhours::ui::CursorType::Pointer)
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.uiScaleInputStr.clear();
                    dialogs.showSettingsDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 2),
                    ComponentConfig{}
                        .with_label("Reset")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    Settings::get().set_ui_scale(1.0f);
                    dialogs.uiScaleInputStr = "100";
                    toast_notify::success("UI scale reset to 100%");
                }
            }
        }
        
        // Go To Bookmark dialog
        if (dialogs.showBookmarkListDialog) {
            constexpr int BOOKMARK_LIST_MODAL_ID = 50007;
            auto result = afterhours::modal(ctx, mk(entity, BOOKMARK_LIST_MODAL_ID),
                dialogs.showBookmarkListDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(400), afterhours::ui::h720(300))
                    .with_title("Go To Bookmark"));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                auto& docComp = doc;
                const auto& bookmarks = docComp.buffer.bookmarks();
                
                if (bookmarks.empty()) {
                    // No bookmarks message
                    div(ctx, mk(result.ent(), 0),
                        ComponentConfig{}
                            .with_label("No bookmarks in document")
                            .with_size(ComponentSize{percent(1.0f), h720(24)})
                            .with_render_layer(CONTENT_LAYER));
                } else {
                    // List of bookmarks
                    auto listContainer = div(ctx, mk(result.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(200)})
                            .with_flex_direction(FlexDirection::Column)
                            .with_overflow(afterhours::ui::Overflow::Auto)
                            .with_render_layer(CONTENT_LAYER));
                    
                    int idx = 0;
                    for (const auto& bookmark : bookmarks) {
                        if (button(ctx, mk(listContainer.ent(), 1000 + idx),
                            ComponentConfig{}
                                .with_label(bookmark.name)
                                .with_size(ComponentSize{percent(1.0f), h720(32)})
                                .with_background(Theme::Usage::Surface)
                                .with_text_overflow(afterhours::ui::TextOverflow::Ellipsis)
                                .with_cursor(afterhours::ui::CursorType::Pointer)
                                .with_margin(Margin{.bottom = DefaultSpacing::tiny()})
                                .with_render_layer(CONTENT_LAYER))) {
                            // Jump to bookmark
                            if (docComp.buffer.goToBookmark(bookmark.name)) {
                                toast_notify::success("Jumped to: " + bookmark.name);
                            } else {
                                toast_notify::error("Bookmark not found");
                            }
                            dialogs.showBookmarkListDialog = false;
                        }
                        idx++;
                    }
                }
                
                // Close button at bottom
                auto buttonRow = div(ctx, mk(result.ent(), 2),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("Close")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_cursor(afterhours::ui::CursorType::Pointer)
                        .with_render_layer(CONTENT_LAYER))) {
                    dialogs.showBookmarkListDialog = false;
                }
            }
        }
        
        // Help Window (Keyboard Shortcuts)
        if (dialogs.showHelpWindow) {
            constexpr int HELP_MODAL_ID = 50008;
            auto result = afterhours::modal(ctx, mk(entity, HELP_MODAL_ID),
                dialogs.showHelpWindow,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(500), afterhours::ui::h720(500))
                    .with_title("Keyboard Shortcuts")
                    .with_show_close_button(true));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                // Get keybindings
                input::ActionMap defaultMap = input::createDefaultActionMap();
                auto bindings = input::getBindingsList(defaultMap);
                
                // Create scrollable container for keybindings list
                auto scrollContainer = div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(380)})
                        .with_overflow(afterhours::ui::Overflow::Auto)
                        .with_padding(Spacing::sm)
                        .with_render_layer(CONTENT_LAYER));
                if (scrollContainer) {
                    // Header row
                    auto headerRow = div(ctx, mk(scrollContainer.ent(), 0),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(24)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_padding(Spacing::xs)
                            .with_render_layer(CONTENT_LAYER));
                    
                    if (headerRow) {
                        div(ctx, mk(headerRow.ent(), 0),
                            ComponentConfig{}
                                .with_label("Action")
                                .with_size(ComponentSize{h720(200), h720(20)})
                                .with_custom_text_color(toAhColor(afterhours::Color{80, 80, 80, 255}))
                                .with_render_layer(CONTENT_LAYER));
                        
                        div(ctx, mk(headerRow.ent(), 1),
                            ComponentConfig{}
                                .with_label("Shortcut")
                                .with_size(ComponentSize{h720(200), h720(20)})
                                .with_custom_text_color(toAhColor(afterhours::Color{80, 80, 80, 255}))
                                .with_render_layer(CONTENT_LAYER));
                    }
                    
                    // Bindings list
                    for (size_t i = 0; i < bindings.size(); ++i) {
                        const auto& binding = bindings[i];
                        auto bindingRow = div(ctx, mk(scrollContainer.ent(), static_cast<int>(i + 1)),
                            ComponentConfig{}
                                .with_size(ComponentSize{percent(1.0f), h720(22)})
                                .with_flex_direction(FlexDirection::Row)
                                .with_padding(Spacing::xs)
                                .with_render_layer(CONTENT_LAYER));
                        
                        if (bindingRow) {
                            div(ctx, mk(bindingRow.ent(), 0),
                                ComponentConfig{}
                                    .with_label(binding.actionName)
                                    .with_size(ComponentSize{h720(200), h720(18)})
                                    .with_render_layer(CONTENT_LAYER));
                            
                            div(ctx, mk(bindingRow.ent(), 1),
                                ComponentConfig{}
                                    .with_label(binding.bindingStr)
                                    .with_size(ComponentSize{h720(200), h720(18)})
                                    .with_render_layer(CONTENT_LAYER));
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ComponentConfig{}
                            .with_label("OK")
                            .with_size(ComponentSize{h720(100), h720(32)})
                            .with_cursor(afterhours::ui::CursorType::Pointer)
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.showHelpWindow = false;
                    }
                }
            }
        }
        
        // Find/Replace Dialog
        if (dialogs.showFindDialog) {
            constexpr int FIND_MODAL_ID = 50009;
            std::string title = dialogs.findReplaceMode ? "Find and Replace" : "Find";
            auto result = afterhours::modal(ctx, mk(entity, FIND_MODAL_ID),
                dialogs.showFindDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(450), dialogs.findReplaceMode ? afterhours::ui::h720(280) : afterhours::ui::h720(220))
                    .with_title(title)
                    .with_show_close_button(true));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                // Find label and input
                div(ctx, mk(result.ent(), 0),
                    ComponentConfig{}
                        .with_label("Find:")
                        .with_size(ComponentSize{percent(1.0f), h720(24)})
                        .with_render_layer(CONTENT_LAYER));
                
                afterhours::text_input::text_input(ctx, mk(result.ent(), 1),
                    dialogs.findInputStr,
                    ComponentConfig{}
                        .with_debug_name("find_input")
                        .with_size(ComponentSize{percent(1.0f), h720(32)})
                        .with_background(Theme::Usage::Surface)
                        .with_render_layer(CONTENT_LAYER));
                
                // Replace label and input (only in replace mode)
                if (dialogs.findReplaceMode) {
                    div(ctx, mk(result.ent(), 2),
                        ComponentConfig{}
                            .with_label("Replace with:")
                            .with_size(ComponentSize{percent(1.0f), h720(24)})
                            .with_margin(Margin{.top = DefaultSpacing::small()})
                            .with_render_layer(CONTENT_LAYER));
                    
                    afterhours::text_input::text_input(ctx, mk(result.ent(), 3),
                        dialogs.replaceInputStr,
                        ComponentConfig{}
                            .with_debug_name("replace_input")
                            .with_size(ComponentSize{percent(1.0f), h720(32)})
                            .with_background(Theme::Usage::Surface)
                            .with_render_layer(CONTENT_LAYER));
                }
                
                // Options checkboxes
                auto optionsRow = div(ctx, mk(result.ent(), 4),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(30)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (optionsRow) {
                    // Case sensitive checkbox
                    checkbox(ctx, mk(optionsRow.ent(), 0),
                        dialogs.findOptions.caseSensitive,
                        ComponentConfig{}
                            .with_label("Case sensitive")
                            .with_size(ComponentSize{h720(150), h720(24)})
                            .with_render_layer(CONTENT_LAYER));
                    
                    // Whole word checkbox
                    checkbox(ctx, mk(optionsRow.ent(), 1),
                        dialogs.findOptions.wholeWord,
                        ComponentConfig{}
                            .with_label("Whole word")
                            .with_size(ComponentSize{h720(120), h720(24)})
                            .with_margin(Margin{.left = DefaultSpacing::small()})
                            .with_render_layer(CONTENT_LAYER));
                }
                
                // Button row
                auto buttonRow = div(ctx, mk(result.ent(), 5),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(44)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_justify_content(JustifyContent::Center)
                        .with_align_items(AlignItems::Center)
                        .with_gap(afterhours::ui::pixels(8))
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (buttonRow) {
                    // Find Next button
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ComponentConfig{}
                            .with_label("Find Next")
                            .with_size(ComponentSize{h720(100), h720(32)})
                            .with_background(Theme::Usage::Primary)
                            .with_cursor(afterhours::ui::CursorType::Pointer)
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
                                toast_notify::info("Found", 2.0f);
                            } else {
                                toast_notify::warning("Not found");
                            }
                        }
                    }
                    
                    // Replace button (only in replace mode)
                    if (dialogs.findReplaceMode) {
                        if (button(ctx, mk(buttonRow.ent(), 1),
                            ComponentConfig{}
                                .with_label("Replace")
                                .with_size(ComponentSize{h720(100), h720(32)})
                                .with_cursor(afterhours::ui::CursorType::Pointer)
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
                                        toast_notify::success("Replaced");
                                        
                                        // Find next occurrence
                                        FindResult findResult = doc.buffer.findNext(dialogs.lastSearchTerm, dialogs.findOptions);
                                        if (findResult.found) {
                                            doc.buffer.setCaret(findResult.start);
                                            doc.buffer.setSelectionAnchor(findResult.start);
                                            doc.buffer.setCaret(findResult.end);
                                            doc.buffer.updateSelectionToCaret();
                                        }
                                    } else {
                                        toast_notify::warning("Selection doesn't match search term");
                                    }
                                } else {
                                    toast_notify::warning("No selection");
                                }
                            }
                        }
                    }
                    
                    // Close button
                    if (button(ctx, mk(buttonRow.ent(), 2),
                        ComponentConfig{}
                            .with_label("Close")
                            .with_size(ComponentSize{h720(80), h720(32)})
                            .with_cursor(afterhours::ui::CursorType::Pointer)
                            .with_render_layer(CONTENT_LAYER))) {
                        dialogs.findInputStr.clear();
                        dialogs.replaceInputStr.clear();
                        dialogs.showFindDialog = false;
                    }
                }
            }
        }
        
        // Right-click detection for context menu (real input, not E2E)
        if (!dialogs.showContextMenu && !dialogs.showFindDialog &&
            !dialogs.showAboutDialog && !dialogs.showHelpWindow &&
            !dialogs.showCommentDialog && !dialogs.showTemplateDialog &&
            !dialogs.showSpellingDialog && !dialogs.showAutoCorrectDialog &&
            !dialogs.showWordCountDialog && !dialogs.showTabWidthDialog &&
            !dialogs.showSettingsDialog && !dialogs.showBookmarkListDialog &&
            !dialogs.showSaveAsDialog && !dialogs.showPageSetup) {
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
                handleMenuActionImpl(menuResult, doc, menu, dialogs, layout);
            }
        }

        // F1 to toggle help window
        if (input::isKeyPressed(afterhours::keys::F1)) {
            dialogs.showHelpWindow = !dialogs.showHelpWindow;
            dialogs.helpScrollOffset = 0;
        }

        // Restore the global theme so subsequent systems (toolbar, etc.) use the correct theme
        ctx.theme = savedTheme;
    }
};

}  // namespace ecs

