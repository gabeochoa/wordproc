#pragma once

// Menu UI System using Afterhours immediate-mode UI
// No direct Raylib calls - everything goes through Afterhours

#include <afterhours/ah.h>
#include <afterhours/src/plugins/ui.h>

#include "components.h"
#include "../input_mapping.h"  // For InputAction enum
#include "../testing/visible_text_registry.h"  // For E2E testing
#include "../ui/theme.h"
#include "../ui/ui_context.h"  // For ui_imm::getUIRootEntity()

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

// Menu UI colors (Win95 style)
namespace menu_colors {
constexpr afterhours::Color MENU_BG = {192, 192, 192, 255};
constexpr afterhours::Color MENU_HIGHLIGHT = {0, 0, 128, 255};
constexpr afterhours::Color TEXT_DARK = {0, 0, 0, 255};
constexpr afterhours::Color TEXT_LIGHT = {255, 255, 255, 255};
constexpr afterhours::Color TEXT_DISABLED = {128, 128, 128, 255};
constexpr afterhours::Color SEPARATOR = {128, 128, 128, 255};
}  // namespace menu_colors

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
        
        // Get the UI root entity for parenting UI elements
        Entity& entity = ui_imm::getUIRootEntity();
        
        // Set up Win95 theme
        Theme theme;
        theme.font = menu_colors::TEXT_LIGHT;
        theme.darkfont = menu_colors::TEXT_DARK;
        theme.font_muted = menu_colors::TEXT_DISABLED;
        theme.background = menu_colors::MENU_BG;
        theme.surface = menu_colors::MENU_BG;
        theme.primary = menu_colors::MENU_HIGHLIGHT;
        theme.secondary = menu_colors::MENU_BG;
        theme.accent = menu_colors::MENU_HIGHLIGHT;
        theme.roundness = 0.0f;
        theme.segments = 0;
        ctx.theme = theme;

        int screenWidth = 800;  // TODO: get from layout component
        
        // Create menu bar container
        auto menuBarContainer = div(ctx, mk(entity, 0),
            ComponentConfig{}
                .with_debug_name("menu_bar_container")
                .with_size(ComponentSize{pixels(static_cast<float>(screenWidth)), 
                                        pixels(static_cast<float>(theme::layout::MENU_BAR_HEIGHT))})
                .with_absolute_position()
                .with_translate(0.0f, static_cast<float>(theme::layout::TITLE_BAR_HEIGHT))
                .with_flex_direction(FlexDirection::Row)
                .with_custom_background(menu_colors::MENU_BG));

        Entity& menuBar = menuBarContainer.ent();
        (void)menuBar;  // Menu bar container used for background only
        
        // Track X position for header buttons
        float headerX = 4.0f;
        float headerY = static_cast<float>(theme::layout::TITLE_BAR_HEIGHT);
        
        // Render each menu header button (absolute positioned to avoid layout issues)
        for (size_t menuIdx = 0; menuIdx < menu.menus.size(); ++menuIdx) {
            auto& menuItem = menu.menus[menuIdx];
            bool isOpen = menuItem.open;
            
            // Calculate button width based on label
            float buttonWidth = static_cast<float>(menuItem.label.length() * 8 + 16);
            
            // Register menu label for E2E tests
            test_input::registerVisibleText(menuItem.label);
            
            // Each header button is absolute-positioned to avoid parent layout dependency
            auto headerBtn = button(ctx, mk(entity, 500 + static_cast<int>(menuIdx)),
                ComponentConfig{}
                    .with_debug_name("menu_header_" + menuItem.label)
                    .with_label(menuItem.label)
                    .with_size(ComponentSize{pixels(buttonWidth), 
                                            pixels(static_cast<float>(theme::layout::MENU_BAR_HEIGHT))})
                    .with_absolute_position()
                    .with_translate(headerX, headerY)
                    .with_custom_background(isOpen ? menu_colors::MENU_HIGHLIGHT : menu_colors::MENU_BG)
                    .with_custom_text_color(isOpen ? menu_colors::TEXT_LIGHT : menu_colors::TEXT_DARK)
                    .with_render_layer(1));
            
            // Update X position for next header
            headerX += buttonWidth;
            
            // Handle header click - toggle menu open state
            if (headerBtn) {
                // Close all other menus
                for (size_t j = 0; j < menu.menus.size(); ++j) {
                    if (j != menuIdx) {
                        menu.menus[j].open = false;
                    }
                }
                menuItem.open = !menuItem.open;
            }
            
            // If any menu is open and we hover this header, switch to it
            bool anyMenuOpen = false;
            for (const auto& m : menu.menus) {
                if (m.open) anyMenuOpen = true;
            }
            
            if (anyMenuOpen && !menuItem.open && ctx.is_hot(headerBtn.ent().id)) {
                for (auto& m : menu.menus) m.open = false;
                menuItem.open = true;
            }
        }
        
        // Render dropdown for open menu
        for (size_t menuIdx = 0; menuIdx < menu.menus.size(); ++menuIdx) {
            auto& menuItem = menu.menus[menuIdx];
            if (!menuItem.open) continue;
            
            // Calculate dropdown position
            float dropdownX = 4.0f;
            for (size_t i = 0; i < menuIdx; ++i) {
                dropdownX += static_cast<float>(menu.menus[i].label.length() * 8 + 16);
            }
            float dropdownY = static_cast<float>(theme::layout::TITLE_BAR_HEIGHT + 
                                                  theme::layout::MENU_BAR_HEIGHT);
            
            // Count non-separator items for height calculation
            float dropdownHeight = 0;
            for (const auto& item : menuItem.items) {
                dropdownHeight += item.separator ? 8.0f : 20.0f;
            }
            
            // Calculate max width based on content
            float maxWidth = 150.0f;
            for (const auto& item : menuItem.items) {
                float labelWidth = static_cast<float>(item.label.length() * 7);
                float shortcutWidth = static_cast<float>(item.shortcut.length() * 7);
                float totalWidth = labelWidth + shortcutWidth + 40.0f;
                if (totalWidth > maxWidth) maxWidth = totalWidth;
            }
            
            // Create dropdown container
            auto dropdownContainer = div(ctx, mk(entity, 100 + static_cast<int>(menuIdx)),
                ComponentConfig{}
                    .with_debug_name("dropdown_" + menuItem.label)
                    .with_size(ComponentSize{pixels(maxWidth), pixels(dropdownHeight)})
                    .with_absolute_position()
                    .with_translate(dropdownX, dropdownY)
                    .with_flex_direction(FlexDirection::Column)
                    .with_custom_background(menu_colors::MENU_BG)
                    .with_render_layer(10));  // Render on top
            
            Entity& dropdown = dropdownContainer.ent();
            (void)dropdown;  // Dropdown container used for background only
            
            // Track Y position for menu items (each item individually absolute-positioned)
            float itemY = dropdownY;
            
            // Render menu items - each individually absolute-positioned to avoid layout issues
            for (size_t itemIdx = 0; itemIdx < menuItem.items.size(); ++itemIdx) {
                const auto& item = menuItem.items[itemIdx];
                
                if (item.separator) {
                    // Draw separator as a thin div - absolute positioned
                    div(ctx, mk(entity, 1000 + static_cast<int>(menuIdx) * 100 + static_cast<int>(itemIdx)),
                        ComponentConfig{}
                            .with_debug_name("separator")
                            .with_size(ComponentSize{pixels(maxWidth), pixels(8.0f)})
                            .with_absolute_position()
                            .with_translate(dropdownX, itemY)
                            .with_custom_background(menu_colors::SEPARATOR)
                            .with_render_layer(11));
                    itemY += 8.0f;
                } else {
                    // Build label with shortcut combined (padded for alignment)
                    std::string fullLabel = item.label;
                    if (!item.shortcut.empty()) {
                        // Pad label to align shortcuts on the right
                        size_t labelLen = item.label.length();
                        size_t targetLen = 20;  // Approximate width for menu item text
                        if (labelLen < targetLen) {
                            fullLabel += std::string(targetLen - labelLen, ' ');
                        }
                        fullLabel += item.shortcut;
                    }
                    
                    // Register menu item label for E2E tests (when dropdown is open)
                    test_input::registerVisibleText(item.label);
                    
                    // Menu item button - absolute positioned with explicit coordinates
                    auto itemBtn = button(ctx, mk(entity, 2000 + static_cast<int>(menuIdx) * 100 + static_cast<int>(itemIdx)),
                        ComponentConfig{}
                            .with_debug_name("item_" + item.label)
                            .with_label(fullLabel)
                            .with_size(ComponentSize{pixels(maxWidth), pixels(20.0f)})
                            .with_absolute_position()
                            .with_translate(dropdownX, itemY)
                            .with_custom_background(menu_colors::MENU_BG)
                            .with_custom_text_color(item.enabled ? menu_colors::TEXT_DARK 
                                                                  : menu_colors::TEXT_DISABLED)
                            .with_alignment(afterhours::ui::TextAlignment::Left)
                            .with_render_layer(11));
                    
                    itemY += 20.0f;
                    
                    // Handle item click
                    if (itemBtn && item.enabled) {
                        // Store the result for the render system to process
                        menu.lastClickedResult = static_cast<int>(menuIdx * 100 + itemIdx);
                        
                        // Close menu
                        menuItem.open = false;
                    }
                }
            }
        }
        
        // Handle click outside menus to close them
        bool anyOpen = false;
        for (const auto& m : menu.menus) {
            if (m.open) anyOpen = true;
        }
        
        if (anyOpen && ctx.pressed(InputAction::WidgetPress)) {
            // Check if click was on menu bar or dropdown
            // For now, just close menus on any click that wasn't handled by a button
            // This is simplified - a full implementation would track bounds
        }
    }
};

}  // namespace ecs

