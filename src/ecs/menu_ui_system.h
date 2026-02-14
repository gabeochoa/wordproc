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
#include "../input_mapping.h"  // For InputAction enum
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
    
    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float) override {
        // Find entities with MenuComponent
        auto menuEntities = afterhours::EntityQuery({.force_merge = true})
                               .whereHasComponent<MenuComponent>()
                               .gen();
        if (menuEntities.empty()) return;
        
        MenuComponent& menu = menuEntities[0].get().get<MenuComponent>();

        // Skip rendering menus in focus mode
        auto layoutEntities = afterhours::EntityQuery({.force_merge = true})
                                 .whereHasComponent<LayoutComponent>()
                                 .gen();
        if (!layoutEntities.empty()) {
            auto& layout = layoutEntities[0].get().get<LayoutComponent>();
            if (layout.focusMode) {
                return;
            }
        }

        // Refresh menus if recent file count changed
        const auto& recentFiles = Settings::get().get_recent_files();
        if (static_cast<int>(recentFiles.size()) != menu.recentFilesCount) {
            menu.menus = menu_setup::createMenuBar(recentFiles);
            menu.recentFilesCount = static_cast<int>(recentFiles.size());

            auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                   .whereHasComponent<DocumentComponent>()
                                   .gen();
            if (!docEntities.empty()) {
                auto& doc = docEntities[0].get().get<DocumentComponent>();
                if (doc.trackChangesEnabled &&
                    menu.menus.size() > 1 &&
                    menu.menus[1].items.size() > 3) {
                    menu.menus[1].items[3].mark = win95::MenuMark::Checkmark;
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

        // Get screen width from layout
        float screenWidth = 800.0f;
        {
            auto layEntities = afterhours::EntityQuery({.force_merge = true})
                                  .whereHasComponent<LayoutComponent>()
                                  .gen();
            if (!layEntities.empty()) {
                screenWidth = static_cast<float>(layEntities[0].get().get<LayoutComponent>().screenWidth);
            }
        }
        
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
        
        // Track X position for header buttons
        float headerX = theme::layout::scale(4.0f);
        float headerY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT);
        
        // Track whether any header was clicked or hovered this frame
        bool headerInteracted = false;
        
        // Render each menu header button and handle clicks + hover-to-switch
        for (size_t menuIdx = 0; menuIdx < menu.menus.size(); ++menuIdx) {
            auto& menuDef = menu.menus[menuIdx];
            bool isOpen = menuDef.open;
            
            // Calculate button width using actual text measurement (matching original Win95 DrawMenuBar)
            int menuFontSize = 16;
            float buttonWidth = static_cast<float>(theme::MeasureUIText(menuDef.label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
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
                    .with_roundness(0.0f)
                    .with_bevel(highlighted ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
                                toAhColor(theme::BORDER_LIGHT), toAhColor(theme::BORDER_DARK), 1.0f)
                    .with_render_layer(1));

            // Handle header click: toggle this menu, close others
            if (headerResult) {
                for (size_t j = 0; j < menu.menus.size(); ++j) {
                    menu.menus[j].open = (j == menuIdx) ? !isOpen : false;
                }
                menu.activeMenuIndex = isOpen ? -1 : static_cast<int>(menuIdx);
                headerInteracted = true;
            }
            
            // Handle hover-to-switch: when any menu is open and we hover a different header
            if (anyMenuOpen && !isOpen && mouseOverHeader) {
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
                dropdownX += static_cast<float>(theme::MeasureUIText(menu.menus[i].label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
            }
            float dropdownY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + 
                                                  theme::layout::MENU_BAR_HEIGHT);
            
            // Compute dropdown height
            float dropdownHeight = 0;
            for (const auto& item : menuDef.items) {
                dropdownHeight += item.separator ? theme::layout::scale(8.0f) : theme::layout::scale(20.0f);
            }
            
            // Compute max width from content
            float maxWidth = 150.0f;
            for (const auto& item : menuDef.items) {
                float labelWidth = static_cast<float>(item.label.length() * 7);
                float shortcutWidth = static_cast<float>(item.shortcut.length() * 7);
                float totalWidth = labelWidth + shortcutWidth + 50.0f;
                if (totalWidth > maxWidth) maxWidth = totalWidth;
            }
            
            // Dropdown container background
            div(ctx, mk(entity, 100 + static_cast<int>(menuIdx)),
                ComponentConfig{}
                    .with_debug_name("dropdown_" + menuDef.label)
                    .with_size(ComponentSize{pixels(maxWidth), pixels(dropdownHeight + 4.0f)})
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
                    // Etched separator: dark line on top, light line below (Win95 style)
                    int sepBaseId = 10000 + static_cast<int>(menuIdx) * 100 + static_cast<int>(itemIdx);
                    div(ctx, mk(entity, sepBaseId),
                        ComponentConfig{}
                            .with_debug_name("separator_dark")
                            .with_size(ComponentSize{pixels(maxWidth - 8.0f), pixels(1.0f)})
                            .with_absolute_position()
                            .with_translate(dropdownX + 4.0f, itemY + 3.0f)
                            .with_custom_background(toAhColor(theme::BORDER_DARK))
                            .with_roundness(0.0f)
                            .with_render_layer(51));
                    div(ctx, mk(entity, sepBaseId + 50),
                        ComponentConfig{}
                            .with_debug_name("separator_light")
                            .with_size(ComponentSize{pixels(maxWidth - 8.0f), pixels(1.0f)})
                            .with_absolute_position()
                            .with_translate(dropdownX + 4.0f, itemY + 4.0f)
                            .with_custom_background(toAhColor(theme::BORDER_LIGHT))
                            .with_roundness(0.0f)
                            .with_render_layer(51));
                    itemY += theme::layout::scale(8.0f);
                } else {
                    // Build full label: mark + label + padded shortcut
                    std::string fullLabel;
                    
                    // Prepend mark character if present
                    if (item.mark != win95::MenuMark::None) {
                        switch (item.mark) {
                            case win95::MenuMark::Checkmark: fullLabel = "\xE2\x9C\x93 "; break;
                            case win95::MenuMark::Radio:     fullLabel = "\xE2\x80\xA2 "; break;
                            case win95::MenuMark::Dash:      fullLabel = "- "; break;
                            case win95::MenuMark::None: break;
                            default: break;
                        }
                    } else {
                        fullLabel = "  ";  // Reserve space for mark column
                    }
                    
                    fullLabel += item.label;
                    
                    if (!item.shortcut.empty()) {
                        size_t currentLen = fullLabel.length();
                        size_t targetLen = 24;
                        if (currentLen < targetLen) {
                            fullLabel += std::string(targetLen - currentLen, ' ');
                        }
                        fullLabel += item.shortcut;
                    }
                    
                    // Register menu item label for E2E tests
                    test_input::register_visible_text(item.label);
                    
                    int itemId = 20000 + static_cast<int>(menuIdx) * 100 + static_cast<int>(itemIdx);
                    float itemHeight = theme::layout::scale(20.0f);
                    
                    // Determine hover highlight using direct mouse position check
                    // (was_hot uses entity IDs, not MK IDs, so it would never match)
                    bool hovered = afterhours::ui::is_mouse_inside(
                        ctx.mouse.pos, {dropdownX + 2.0f, itemY, maxWidth - 4.0f, itemHeight}) && item.enabled;
                    
                    auto itemResult = button(ctx, mk(entity, itemId),
                        ComponentConfig{}
                            .with_debug_name("item_" + item.label)
                            .with_label(fullLabel)
                            .with_size(ComponentSize{pixels(maxWidth - 4.0f), pixels(itemHeight)})
                            .with_absolute_position()
                            .with_translate(dropdownX + 2.0f, itemY)
                            .with_custom_background(hovered ? toAhColor(theme::MENU_HOVER) : toAhColor(theme::MENU_BG))
                            .with_custom_text_color(
                                !item.enabled ? toAhColor(theme::MENU_DISABLED) :
                                hovered ? toAhColor(theme::MENU_TEXT_HOVER) : toAhColor(theme::MENU_TEXT))
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_justify_content(afterhours::ui::JustifyContent::Center)
                            .with_roundness(0.0f)
                            .with_render_layer(51));
                    
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
        
        // Close menus on click outside (if no header or item was interacted with)
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
                        float bw = static_cast<float>(theme::MeasureUIText(menu.menus[i].label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
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
                        // Compute dropdown rect (same as rendering logic above)
                        float dx = theme::layout::scale(4.0f);
                        for (size_t i = 0; i < menuIdx; ++i) {
                            dx += static_cast<float>(theme::MeasureUIText(menu.menus[i].label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
                        }
                        float dy = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + theme::layout::MENU_BAR_HEIGHT);
                        float dh = 0;
                        for (const auto& item : menu.menus[menuIdx].items) {
                            dh += item.separator ? theme::layout::scale(8.0f) : theme::layout::scale(20.0f);
                        }
                        float dw = 150.0f;
                        for (const auto& item : menu.menus[menuIdx].items) {
                            float tw = static_cast<float>(item.label.length() * 7 + item.shortcut.length() * 7) + 50.0f;
                            if (tw > dw) dw = tw;
                        }
                        if (afterhours::ui::is_mouse_inside(ctx.mouse.pos, {dx, dy, dw, dh + 4.0f})) {
                            clickInMenu = true;
                            break;
                        }
                    }
                }
                
                if (!clickInMenu) {
                    for (auto& m : menu.menus) { m.open = false; }
                    menu.activeMenuIndex = -1;
                }
            }
        }
        
        // ============================================================
        // Modal Dialogs (using afterhours modal.h)
        // ============================================================
        
        // About dialog
        if (menu.showAboutDialog) {
            constexpr int ABOUT_MODAL_ID = 50000;
            afterhours::modal::info(ctx, mk(entity, ABOUT_MODAL_ID),
                menu.showAboutDialog,
                "About Wordproc",
                "Wordproc v0.1\n\nA Windows 95 style word processor\nbuilt with Afterhours.",
                "OK");
        }
        
        // Word Count dialog - uses larger modal to fit 5 lines of stats
        if (menu.showWordCountDialog) {
            // Need to get document stats - query for DocumentComponent
            auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                  .whereHasComponent<DocumentComponent>()
                                  .gen();
            if (!docEntities.empty()) {
                auto& doc = docEntities[0].get().get<DocumentComponent>();
                TextStats stats = doc.buffer.stats();
                std::string msg = std::format(
                    "Words: {}\nCharacters: {}\nLines: {}\nParagraphs: {}\nSentences: {}",
                    stats.words, stats.characters, stats.lines, stats.paragraphs,
                    stats.sentences);
                
                constexpr int WORDCOUNT_MODAL_ID = 50001;
                // Use custom modal with larger height for 5 lines of text
                auto result = afterhours::modal(ctx, mk(entity, WORDCOUNT_MODAL_ID),
                    menu.showWordCountDialog,
                    afterhours::ModalConfig{}
                        .with_size(afterhours::ui::h720(350), afterhours::ui::h720(220))
                        .with_title("Word Count")
                        .with_show_close_button(false));
                
                if (result) {
                    using namespace afterhours::ui;
                    using namespace afterhours::ui::imm;
                    
                    // Message text
                    div(ctx, mk(result.ent(), 0),
                        ComponentConfig{}
                            .with_label(msg)
                            .with_size(ComponentSize{percent(1.0f), children()})
                            .with_padding(Spacing::md));
                    
                    // OK button row
                    auto button_row = div(ctx, mk(result.ent(), 1),
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(44)})
                            .with_flex_direction(FlexDirection::Row)
                            .with_justify_content(JustifyContent::Center)
                            .with_align_items(AlignItems::Center));
                    
                    if (button_row) {
                        if (button(ctx, mk(button_row.ent(), 0),
                            ComponentConfig{}
                                .with_label("OK")
                                .with_size(ComponentSize{h720(100), h720(32)}))) {
                            menu.showWordCountDialog = false;
                        }
                    }
                }
            }
        }
        
        // Comment input dialog
        if (menu.showCommentDialog) {
            constexpr int COMMENT_MODAL_ID = 50002;
            auto result = afterhours::modal(ctx, mk(entity, COMMENT_MODAL_ID),
                menu.showCommentDialog,
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
                    menu.commentInputStr,
                    ComponentConfig{}
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_margin(Margin{.right = DefaultSpacing::small()})
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - add comment
                    auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                          .whereHasComponent<DocumentComponent>()
                                          .gen();
                    if (!docEntities.empty() && !menu.commentInputStr.empty()) {
                        auto& doc = docEntities[0].get().get<DocumentComponent>();
                        Comment comment;
                        comment.startOffset = menu.pendingCommentStart;
                        comment.endOffset = menu.pendingCommentEnd;
                        comment.author = "User";
                        comment.text = menu.commentInputStr;
                        comment.createdAt = std::time(nullptr);
                        doc.comments.push_back(comment);
                        toast_notify::success("Comment added");
                    }
                    menu.commentInputStr.clear();
                    menu.showCommentDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    menu.commentInputStr.clear();
                    menu.showCommentDialog = false;
                }
            }
        }
        
        // Template input dialog
        if (menu.showTemplateDialog) {
            constexpr int TEMPLATE_MODAL_ID = 50003;
            auto result = afterhours::modal(ctx, mk(entity, TEMPLATE_MODAL_ID),
                menu.showTemplateDialog,
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
                    menu.templateInputStr,
                    ComponentConfig{}
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_margin(Margin{.right = DefaultSpacing::small()})
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - load template
                    auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                          .whereHasComponent<DocumentComponent>()
                                          .gen();
                    if (!docEntities.empty() && !menu.templateInputStr.empty()) {
                        auto& doc = docEntities[0].get().get<DocumentComponent>();
                        std::string name = menu.templateInputStr;
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
                    menu.templateInputStr.clear();
                    menu.showTemplateDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    menu.templateInputStr.clear();
                    menu.showTemplateDialog = false;
                }
            }
        }
        
        // Tab Width input dialog
        if (menu.showTabWidthDialog) {
            constexpr int TABWIDTH_MODAL_ID = 50004;
            auto result = afterhours::modal(ctx, mk(entity, TABWIDTH_MODAL_ID),
                menu.showTabWidthDialog,
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
                    menu.tabWidthInputStr,
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_margin(Margin{.right = DefaultSpacing::small()})
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - set tab width
                    auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                          .whereHasComponent<DocumentComponent>()
                                          .gen();
                    if (!docEntities.empty() && !menu.tabWidthInputStr.empty()) {
                        auto& doc = docEntities[0].get().get<DocumentComponent>();
                        int width = std::atoi(menu.tabWidthInputStr.c_str());
                        if (width >= 1 && width <= 16) {
                            doc.docSettings.tabWidth = width;
                            toast_notify::success("Tab width set to " + std::to_string(width));
                        } else {
                            toast_notify::error("Tab width must be 1-16");
                        }
                    }
                    menu.tabWidthInputStr.clear();
                    menu.showTabWidthDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    menu.tabWidthInputStr.clear();
                    menu.showTabWidthDialog = false;
                }
            }
        }
        
        // UI Settings dialog
        if (menu.showSettingsDialog) {
            constexpr int SETTINGS_MODAL_ID = 50005;
            auto result = afterhours::modal(ctx, mk(entity, SETTINGS_MODAL_ID),
                menu.showSettingsDialog,
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
                    menu.uiScaleInputStr,
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("OK")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_background(Theme::Usage::Primary)
                        .with_margin(Margin{.right = DefaultSpacing::small()})
                        .with_render_layer(CONTENT_LAYER))) {
                    // Handle OK - set UI scale
                    if (!menu.uiScaleInputStr.empty()) {
                        int percentage = std::atoi(menu.uiScaleInputStr.c_str());
                        if (percentage >= 50 && percentage <= 200) {
                            float scale = static_cast<float>(percentage) / 100.0f;
                            Settings::get().set_ui_scale(scale);
                            toast_notify::success("UI scale set to " + std::to_string(percentage) + "%");
                        } else {
                            toast_notify::error("UI scale must be 50-200%");
                        }
                    }
                    menu.uiScaleInputStr.clear();
                    menu.showSettingsDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 1),
                    ComponentConfig{}
                        .with_label("Cancel")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_margin(Margin{.right = DefaultSpacing::small()})
                        .with_render_layer(CONTENT_LAYER))) {
                    menu.uiScaleInputStr.clear();
                    menu.showSettingsDialog = false;
                }
                
                if (button(ctx, mk(buttonRow.ent(), 2),
                    ComponentConfig{}
                        .with_label("Reset")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    Settings::get().set_ui_scale(1.0f);
                    menu.uiScaleInputStr = "100";
                    toast_notify::success("UI scale reset to 100%");
                }
            }
        }
        
        // Go To Bookmark dialog
        if (menu.showBookmarkListDialog) {
            constexpr int BOOKMARK_LIST_MODAL_ID = 50007;
            auto result = afterhours::modal(ctx, mk(entity, BOOKMARK_LIST_MODAL_ID),
                menu.showBookmarkListDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(400), afterhours::ui::h720(300))
                    .with_title("Go To Bookmark"));
            
            if (result) {
                using namespace afterhours::ui;
                using namespace afterhours::ui::imm;
                constexpr int CONTENT_LAYER = 1001;
                
                // Query for DocumentComponent
                auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                       .whereHasComponent<DocumentComponent>()
                                       .gen();
                if (docEntities.empty()) {
                    menu.showBookmarkListDialog = false;
                    return;
                }
                auto& docComp = docEntities[0].get().get<DocumentComponent>();
                
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
                            .with_render_layer(CONTENT_LAYER));
                    
                    int idx = 0;
                    for (const auto& bookmark : bookmarks) {
                        if (button(ctx, mk(listContainer.ent(), 1000 + idx),
                            ComponentConfig{}
                                .with_label(bookmark.name)
                                .with_size(ComponentSize{percent(1.0f), h720(32)})
                                .with_background(Theme::Usage::Surface)
                                .with_margin(Margin{.bottom = DefaultSpacing::tiny()})
                                .with_render_layer(CONTENT_LAYER))) {
                            // Jump to bookmark
                            if (docComp.buffer.goToBookmark(bookmark.name)) {
                                toast_notify::success("Jumped to: " + bookmark.name);
                            } else {
                                toast_notify::error("Bookmark not found");
                            }
                            menu.showBookmarkListDialog = false;
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (button(ctx, mk(buttonRow.ent(), 0),
                    ComponentConfig{}
                        .with_label("Close")
                        .with_size(ComponentSize{h720(80), h720(32)})
                        .with_render_layer(CONTENT_LAYER))) {
                    menu.showBookmarkListDialog = false;
                }
            }
        }
        
        // Help Window (Keyboard Shortcuts)
        if (menu.showHelpWindow) {
            constexpr int HELP_MODAL_ID = 50008;
            auto result = afterhours::modal(ctx, mk(entity, HELP_MODAL_ID),
                menu.showHelpWindow,
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
                        .with_padding(Spacing::sm)
                        .with_render_layer(CONTENT_LAYER));
                if (scrollContainer) {
                    scrollContainer.ent().addComponent<HasScrollView>();
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (buttonRow) {
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ComponentConfig{}
                            .with_label("OK")
                            .with_size(ComponentSize{h720(100), h720(32)})
                            .with_render_layer(CONTENT_LAYER))) {
                        menu.showHelpWindow = false;
                    }
                }
            }
        }
        
        // Find/Replace Dialog
        if (menu.showFindDialog) {
            constexpr int FIND_MODAL_ID = 50009;
            std::string title = menu.findReplaceMode ? "Find and Replace" : "Find";
            auto result = afterhours::modal(ctx, mk(entity, FIND_MODAL_ID),
                menu.showFindDialog,
                afterhours::ModalConfig{}
                    .with_size(afterhours::ui::h720(450), menu.findReplaceMode ? afterhours::ui::h720(280) : afterhours::ui::h720(220))
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
                    menu.findInputStr,
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(32)})
                        .with_background(Theme::Usage::Surface)
                        .with_render_layer(CONTENT_LAYER));
                
                // Replace label and input (only in replace mode)
                if (menu.findReplaceMode) {
                    div(ctx, mk(result.ent(), 2),
                        ComponentConfig{}
                            .with_label("Replace with:")
                            .with_size(ComponentSize{percent(1.0f), h720(24)})
                            .with_margin(Margin{.top = DefaultSpacing::small()})
                            .with_render_layer(CONTENT_LAYER));
                    
                    afterhours::text_input::text_input(ctx, mk(result.ent(), 3),
                        menu.replaceInputStr,
                        ComponentConfig{}
                            .with_size(ComponentSize{percent(1.0f), h720(32)})
                            .with_background(Theme::Usage::Surface)
                            .with_render_layer(CONTENT_LAYER));
                }
                
                // Options checkboxes
                auto optionsRow = div(ctx, mk(result.ent(), 4),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.0f), h720(30)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (optionsRow) {
                    // Case sensitive checkbox
                    checkbox(ctx, mk(optionsRow.ent(), 0),
                        menu.findOptions.caseSensitive,
                        ComponentConfig{}
                            .with_label("Case sensitive")
                            .with_size(ComponentSize{h720(150), h720(24)})
                            .with_render_layer(CONTENT_LAYER));
                    
                    // Whole word checkbox
                    checkbox(ctx, mk(optionsRow.ent(), 1),
                        menu.findOptions.wholeWord,
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
                        .with_margin(Margin{.top = DefaultSpacing::medium()})
                        .with_render_layer(CONTENT_LAYER));
                
                if (buttonRow) {
                    // Find Next button
                    if (button(ctx, mk(buttonRow.ent(), 0),
                        ComponentConfig{}
                            .with_label("Find Next")
                            .with_size(ComponentSize{h720(100), h720(32)})
                            .with_background(Theme::Usage::Primary)
                            .with_margin(Margin{.right = DefaultSpacing::small()})
                            .with_render_layer(CONTENT_LAYER))) {
                        // Get document and perform find
                        auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                              .whereHasComponent<DocumentComponent>()
                                              .gen();
                        if (!docEntities.empty() && !menu.findInputStr.empty()) {
                            auto& doc = docEntities[0].get().get<DocumentComponent>();
                            menu.lastSearchTerm = menu.findInputStr;
                            FindResult findResult = doc.buffer.findNext(menu.lastSearchTerm, menu.findOptions);
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
                    if (menu.findReplaceMode) {
                        if (button(ctx, mk(buttonRow.ent(), 1),
                            ComponentConfig{}
                                .with_label("Replace")
                                .with_size(ComponentSize{h720(100), h720(32)})
                                .with_margin(Margin{.right = DefaultSpacing::small()})
                                .with_render_layer(CONTENT_LAYER))) {
                            // Get document and perform replace
                            auto docEntities = afterhours::EntityQuery({.force_merge = true})
                                                  .whereHasComponent<DocumentComponent>()
                                                  .gen();
                            if (!docEntities.empty() && !menu.findInputStr.empty()) {
                                auto& doc = docEntities[0].get().get<DocumentComponent>();
                                menu.lastSearchTerm = menu.findInputStr;
                                menu.replaceTerm = menu.replaceInputStr;
                                
                                // Check if current selection matches the search term
                                if (doc.buffer.hasSelection()) {
                                    std::string selected = doc.buffer.getSelectedText();
                                    bool matches = menu.findOptions.caseSensitive ? 
                                        (selected == menu.lastSearchTerm) :
                                        ([&]() {
                                            std::string selLower = selected;
                                            std::string termLower = menu.lastSearchTerm;
                                            for (auto& c : selLower) c = static_cast<char>(std::tolower(c));
                                            for (auto& c : termLower) c = static_cast<char>(std::tolower(c));
                                            return selLower == termLower;
                                        })();
                                    
                                    if (matches) {
                                        doc.buffer.deleteSelection();
                                        doc.buffer.insertText(menu.replaceTerm);
                                        doc.isDirty = true;
                                        toast_notify::success("Replaced");
                                        
                                        // Find next occurrence
                                        FindResult findResult = doc.buffer.findNext(menu.lastSearchTerm, menu.findOptions);
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
                            .with_render_layer(CONTENT_LAYER))) {
                        menu.findInputStr.clear();
                        menu.replaceInputStr.clear();
                        menu.showFindDialog = false;
                    }
                }
            }
        }
        
        // Restore the global theme so subsequent systems (toolbar, etc.) use the correct theme
        ctx.theme = savedTheme;
    }
};

}  // namespace ecs

