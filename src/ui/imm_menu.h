#pragma once

// Immediate-mode menu system using Afterhours UI primitives
// Win95-styled menus built on top of the Afterhours UI framework

#include <functional>
#include <string>
#include <vector>

#include "../../vendor/afterhours/src/plugins/ui.h"
#include "../rl.h"
#include "theme.h"
#include "ui_context.h"

namespace imm_menu {

using namespace afterhours;
using namespace afterhours::ui;

// Menu item structure (same as win95::MenuItem for compatibility)
struct MenuItem {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    bool separator = false;
    std::function<void()> action;
};

// Menu structure (same as win95::Menu for compatibility)
struct Menu {
    std::string label;
    std::vector<MenuItem> items;
    bool open = false;
    int hoverIndex = -1;
};

// Menu bar state component
struct MenuBarState : BaseComponent {
    std::vector<Menu> menus;
    int activeMenuIndex = -1;
    int lastClickedResult = -1;  // Encoded as menuIndex * 100 + itemIndex

    void reset() { lastClickedResult = -1; }

    int consumeResult() {
        int result = lastClickedResult;
        lastClickedResult = -1;
        return result;
    }
};

// Create Win95-style component config for menu bar item
inline ComponentConfig menuBarItemConfig(const std::string& label,
                                         bool isActive) {
    auto config = ComponentConfig{}
                      .with_label(label)
                      .with_size(ComponentSize{
                          children(), pixels(theme::layout::MENU_BAR_HEIGHT)})
                      .with_padding(Padding{.left = DefaultSpacing::small(),
                                            .right = DefaultSpacing::small()});

    if (isActive) {
        config.with_background(Theme::Usage::Primary);  // Blue highlight
    } else {
        config.with_background(Theme::Usage::Secondary);  // Gray background
    }

    return config;
}

// Create Win95-style dropdown item config
inline ComponentConfig dropdownItemConfig(const std::string& label,
                                          const std::string& shortcut,
                                          bool isHovered, bool enabled) {
    std::string fullLabel = label;
    if (!shortcut.empty()) {
        // Pad with spaces to push shortcut to the right
        fullLabel += "        " + shortcut;
    }

    auto config = ComponentConfig{}
                      .with_label(fullLabel)
                      .with_size(ComponentSize{percent(1.0f), pixels(20)})
                      .with_padding(Padding{.left = DefaultSpacing::small(),
                                            .right = DefaultSpacing::small()});

    if (!enabled) {
        config.disabled = true;
    }

    if (isHovered && enabled) {
        config.with_background(Theme::Usage::Primary);
    } else {
        config.with_background(Theme::Usage::Secondary);
    }

    return config;
}

// Render menu bar using Afterhours immediate-mode UI
// Returns the encoded result (menuIndex * 100 + itemIndex) or -1 if no action
template<typename UIContextT>
int renderMenuBar(UIContextT& ctx, Entity& parent, MenuBarState& state) {
    // Create the menu bar container
    auto menuBarResult =
        imm::div(ctx, mk(parent),
                 ComponentConfig{}
                     .with_debug_name("menu_bar")
                     .with_size(ComponentSize{
                         percent(1.0f), pixels(theme::layout::MENU_BAR_HEIGHT)})
                     .with_flex_direction(FlexDirection::Row)
                     .with_justify_content(JustifyContent::FlexStart)
                     .with_background(Theme::Usage::Secondary));

    Entity& menuBar = menuBarResult.ent();
    int result = -1;

    // Render each menu header
    for (size_t menuIdx = 0; menuIdx < state.menus.size(); ++menuIdx) {
        Menu& menu = state.menus[menuIdx];
        bool isActive = (state.activeMenuIndex == static_cast<int>(menuIdx));

        // Menu header button
        auto headerConfig =
            menuBarItemConfig(menu.label, isActive || menu.open);
        auto headerResult = imm::button(
            ctx, mk(menuBar, static_cast<int>(menuIdx)), headerConfig);

        // Handle menu header click
        if (headerResult) {
            if (menu.open) {
                // Close this menu
                menu.open = false;
                state.activeMenuIndex = -1;
            } else {
                // Close all menus and open this one
                for (auto& m : state.menus) m.open = false;
                menu.open = true;
                state.activeMenuIndex = static_cast<int>(menuIdx);
            }
        }

        // If any menu is open and we hover this header, switch to it
        if (state.activeMenuIndex >= 0 && !menu.open) {
            if (headerResult.ent().has<ui::UIComponent>()) {
                auto& uiComp = headerResult.ent().get<ui::UIComponent>();
                if (ctx.is_hot(headerResult.ent().id)) {
                    for (auto& m : state.menus) m.open = false;
                    menu.open = true;
                    state.activeMenuIndex = static_cast<int>(menuIdx);
                }
            }
        }
    }

    // Render dropdown for open menu
    for (size_t menuIdx = 0; menuIdx < state.menus.size(); ++menuIdx) {
        Menu& menu = state.menus[menuIdx];
        if (!menu.open) continue;

        // Calculate dropdown height
        float dropdownHeight = static_cast<float>(menu.items.size()) * 20.0f;

        // Create dropdown container (positioned below menu bar)
        // Note: In a full implementation, this would use absolute positioning
        // For now, we render it as part of the flow and rely on the render
        // system
        auto dropdownResult = imm::div(
            ctx, mk(parent, 1000 + static_cast<int>(menuIdx)),
            ComponentConfig{}
                .with_debug_name("dropdown_" + std::to_string(menuIdx))
                .with_size(ComponentSize{pixels(150), pixels(dropdownHeight)})
                .with_flex_direction(FlexDirection::Column)
                .with_background(Theme::Usage::Secondary));

        Entity& dropdown = dropdownResult.ent();

        // Render menu items
        for (size_t itemIdx = 0; itemIdx < menu.items.size(); ++itemIdx) {
            const MenuItem& item = menu.items[itemIdx];

            if (item.separator) {
                // Render separator
                imm::separator(ctx, mk(dropdown, static_cast<int>(itemIdx)));
            } else {
                bool isHovered = (menu.hoverIndex == static_cast<int>(itemIdx));
                auto itemConfig = dropdownItemConfig(item.label, item.shortcut,
                                                     isHovered, item.enabled);

                auto itemResult = imm::button(
                    ctx, mk(dropdown, static_cast<int>(itemIdx)), itemConfig);

                // Update hover state
                if (itemResult.ent().has<ui::UIComponent>()) {
                    if (ctx.is_hot(itemResult.ent().id) && item.enabled) {
                        menu.hoverIndex = static_cast<int>(itemIdx);
                    }
                }

                // Handle item click
                if (itemResult && item.enabled) {
                    result = static_cast<int>(menuIdx * 100 + itemIdx);

                    // Call action callback if present
                    if (item.action) {
                        item.action();
                    }

                    // Close menu after selection
                    menu.open = false;
                    menu.hoverIndex = -1;
                    state.activeMenuIndex = -1;
                }
            }
        }
    }

    return result;
}

// Helper to check if click was outside all menus (for closing)
inline bool clickedOutsideMenus(const input::MousePosition& mousePos,
                                const MenuBarState& state, float menuBarY,
                                float menuBarHeight) {
    // Check if click is in menu bar area
    if (mousePos.y >= menuBarY && mousePos.y < menuBarY + menuBarHeight) {
        return false;
    }

    // Check if click is in any dropdown area
    // (This is simplified - a full implementation would track dropdown bounds)
    for (const auto& menu : state.menus) {
        if (menu.open) {
            // Approximate dropdown bounds
            float dropdownHeight =
                static_cast<float>(menu.items.size()) * 20.0f;
            if (mousePos.y >= menuBarY + menuBarHeight &&
                mousePos.y < menuBarY + menuBarHeight + dropdownHeight) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace imm_menu
