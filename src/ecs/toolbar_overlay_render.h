#pragma once

// Toolbar Overlay Render System
// Draws dropdown triangles AFTER the afterhours UI render pass.
// Icons moved into the render pass itself (see toolbar_icons.h + on_draw_fg).

#include "../../vendor/afterhours/src/core/system.h"
#include <afterhours/src/drawing_helpers.h>
#include "../rl.h"
#include "../ui/theme.h"
#include "components.h"
#include "toolbar_icons.h"

namespace ecs {

// Render system that draws dropdown triangles after the afterhours UI render
// systems. Raw draw calls land on top of everything, so anything drawn here has
// to be skipped while a popup is open -- which is exactly why icons moved into
// the render pass.
struct ToolbarOverlayRenderSystem : afterhours::System<ToolbarComponent> {
    
    void for_each_with(const Entity& /*entity*/, const ToolbarComponent& toolbar, float) const override {
        // Skip in focus mode
        auto layoutEntities = afterhours::EntityQuery({.force_merge = true})
                                 .whereHasComponent<LayoutComponent>()
                                 .gen();
        if (!layoutEntities.empty() && layoutEntities[0].get().get<LayoutComponent>().focusMode) {
            return;
        }
        
        // Compute popup overlay rects so we can skip icons that overlap
        // (raw draw calls run after afterhours render, so they'd appear on top of popups)
        struct PopupRect { float x, y, w, h; };
        std::vector<PopupRect> popupRects;
        
        auto menuEntities = afterhours::EntityQuery({.force_merge = true})
                               .whereHasComponent<MenuComponent>()
                               .gen();
        if (!menuEntities.empty()) {
            const auto& menu = menuEntities[0].get().get<MenuComponent>();
            int menuFontSize = 14;
            for (size_t menuIdx = 0; menuIdx < menu.menus.size(); ++menuIdx) {
                if (!menu.menus[menuIdx].open) continue;
                
                // Compute dropdown bounds (mirroring menu_ui_system.h logic)
                float dropdownX = theme::layout::scale(4.0f);
                for (size_t i = 0; i < menuIdx; ++i) {
                    dropdownX += static_cast<float>(theme::MeasureUIText(menu.menus[i].label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
                }
                float dropdownY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + 
                                                      theme::layout::MENU_BAR_HEIGHT);
                float dropdownHeight = 0;
                float maxWidth = 150.0f;
                for (const auto& item : menu.menus[menuIdx].items) {
                    dropdownHeight += item.separator ? theme::layout::scale(8.0f) : theme::layout::scale(20.0f);
                    float totalWidth = static_cast<float>(item.label.length() * 7 + item.shortcut.length() * 7 + 50);
                    if (totalWidth > maxWidth) maxWidth = totalWidth;
                }
                popupRects.push_back({dropdownX, dropdownY, maxWidth, dropdownHeight + 4.0f});
            }
        }
        
        // Add toolbar dropdown rects
        if (toolbar.styleDropdownOpen || toolbar.fontDropdownOpen || toolbar.fontSizeDropdownOpen) {
            float formattingBarY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + 
                                                       theme::layout::MENU_BAR_HEIGHT +
                                                       theme::layout::TOOLBAR_HEIGHT);
            float dropdownHeight = theme::layout::scale(22);
            float buttonPadding = theme::layout::scale(theme::layout::TOOLBAR_BUTTON_PADDING);
            float fmtX = buttonPadding;
            float styleW = theme::layout::scale(120);
            float fontW = theme::layout::scale(140);
            float fontSizeW = theme::layout::scale(50);
            
            if (toolbar.styleDropdownOpen) {
                float listH = static_cast<float>(toolbar.styles.size()) * theme::layout::scale(20) + theme::layout::scale(4);
                popupRects.push_back({fmtX, formattingBarY + dropdownHeight + buttonPadding, styleW, listH});
            }
            float fontX = fmtX + styleW + buttonPadding * 2;
            if (toolbar.fontDropdownOpen) {
                float listH = static_cast<float>(toolbar.fonts.size()) * theme::layout::scale(20) + theme::layout::scale(4);
                popupRects.push_back({fontX, formattingBarY + dropdownHeight + buttonPadding, fontW, listH});
            }
            float fontSizeX = fontX + fontW + buttonPadding * 2;
            if (toolbar.fontSizeDropdownOpen) {
                float listH = static_cast<float>(toolbar.fontSizes.size()) * theme::layout::scale(20) + theme::layout::scale(4);
                popupRects.push_back({fontSizeX, formattingBarY + dropdownHeight + buttonPadding, fontSizeW, listH});
            }
        }
        
        // Helper: check if an icon rect overlaps any popup
        auto overlapsPopup = [&](float ix, float iy, float iw, float ih) -> bool {
            for (const auto& p : popupRects) {
                if (ix < p.x + p.w && ix + iw > p.x && iy < p.y + p.h && iy + ih > p.y)
                    return true;
            }
            return false;
        };
        
        // Draw dropdown triangles that don't overlap with any open popup
        for (const auto& tri : toolbar.dropdownTriangles) {
            if (!overlapsPopup(tri.x - 4, tri.y - 4, 8, 8)) {
                drawDropdownTriangle(tri.x, tri.y, theme::BUTTON_TEXT);
            }
        }
    }
};

}  // namespace ecs
