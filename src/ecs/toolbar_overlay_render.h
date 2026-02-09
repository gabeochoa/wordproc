#pragma once

// Toolbar Overlay Render System
// Draws toolbar icons and dropdown triangles AFTER afterhours UI render pass.
// This is a workaround for afterhours not supporting bitmap icons or programmatic glyphs.

#include "../../vendor/afterhours/src/core/system.h"
#include <afterhours/src/drawing_helpers.h>
#include "../rl.h"
#include "../ui/theme.h"
#include "components.h"

namespace ecs {

// Draw a simple 16x16 pixel-art icon at (cx, cy) center position
// All icons use simple geometric primitives (lines, rectangles, triangles)
inline void drawToolbarIcon(ToolbarIcon icon, float cx, float cy, float size, bool enabled) {
    raylib::Color fg = enabled ? theme::BUTTON_TEXT : theme::MENU_DISABLED;
    
    // Scale icon to fit within button (leaving 4px padding)
    float pad = size * 0.2f;
    float x0 = cx + pad;
    float y0 = cy + pad;
    float iw = size - pad * 2.0f;  // icon width
    float ih = size - pad * 2.0f;  // icon height
    
    switch (icon) {
        default: break;
        case ToolbarIcon::New: {
            // Blank page with folded corner
            float fold = iw * 0.3f;
            // Page body
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{x0, y0, iw, ih}, fg);
            // Inner white fill
            afterhours::draw_rectangle(
                raylib::Rectangle{x0 + 1, y0 + 1, iw - 2, ih - 2},
                raylib::Color{255, 255, 255, 255});
            // Corner fold (triangle in top-right)
            afterhours::draw_line(static_cast<int>(x0 + iw - fold), static_cast<int>(y0),
                                  static_cast<int>(x0 + iw - fold), static_cast<int>(y0 + fold), fg);
            afterhours::draw_line(static_cast<int>(x0 + iw - fold), static_cast<int>(y0 + fold),
                                  static_cast<int>(x0 + iw), static_cast<int>(y0 + fold), fg);
            break;
        }
        case ToolbarIcon::Open: {
            // Folder shape
            float tabW = iw * 0.4f;
            float tabH = ih * 0.2f;
            // Folder tab (top-left)
            afterhours::draw_rectangle(
                raylib::Rectangle{x0, y0, tabW, tabH},
                raylib::Color{255, 220, 100, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{x0, y0, tabW, tabH}, fg);
            // Folder body
            afterhours::draw_rectangle(
                raylib::Rectangle{x0, y0 + tabH, iw, ih - tabH},
                raylib::Color{255, 220, 100, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{x0, y0 + tabH, iw, ih - tabH}, fg);
            break;
        }
        case ToolbarIcon::Save: {
            // Floppy disk
            // Outer rectangle
            afterhours::draw_rectangle(
                raylib::Rectangle{x0, y0, iw, ih},
                raylib::Color{50, 50, 180, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{x0, y0, iw, ih}, fg);
            // Metal slider (top center)
            float sliderW = iw * 0.5f;
            float sliderH = ih * 0.25f;
            float sliderX = x0 + (iw - sliderW) / 2.0f;
            afterhours::draw_rectangle(
                raylib::Rectangle{sliderX, y0, sliderW, sliderH},
                raylib::Color{192, 192, 192, 255});
            // Label area (bottom)
            float labelH = ih * 0.35f;
            afterhours::draw_rectangle(
                raylib::Rectangle{x0 + 2, y0 + ih - labelH, iw - 4, labelH},
                raylib::Color{240, 240, 240, 255});
            break;
        }
        case ToolbarIcon::Print: {
            // Printer: paper on top, body in middle, tray at bottom
            float bodyH = ih * 0.45f;
            float bodyY = y0 + ih * 0.25f;
            // Paper coming out top
            float paperW = iw * 0.6f;
            float paperX = x0 + (iw - paperW) / 2.0f;
            afterhours::draw_rectangle(
                raylib::Rectangle{paperX, y0, paperW, bodyY - y0 + 2},
                raylib::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{paperX, y0, paperW, bodyY - y0 + 2}, fg);
            // Printer body
            afterhours::draw_rectangle(
                raylib::Rectangle{x0, bodyY, iw, bodyH},
                raylib::Color{192, 192, 192, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{x0, bodyY, iw, bodyH}, fg);
            // Output tray
            float trayW = iw * 0.7f;
            float trayX = x0 + (iw - trayW) / 2.0f;
            afterhours::draw_rectangle(
                raylib::Rectangle{trayX, bodyY + bodyH, trayW, ih - bodyH - ih * 0.25f},
                raylib::Color{192, 192, 192, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{trayX, bodyY + bodyH, trayW, ih - bodyH - ih * 0.25f}, fg);
            break;
        }
        case ToolbarIcon::Cut: {
            // Scissors: two crossing lines with circles at handles
            float midX = x0 + iw / 2.0f;
            float midY = y0 + ih / 2.0f;
            // Blades (X shape from top-center to bottom corners)
            afterhours::draw_line(static_cast<int>(midX), static_cast<int>(y0),
                                  static_cast<int>(x0 + 2), static_cast<int>(y0 + ih - 3), fg);
            afterhours::draw_line(static_cast<int>(midX), static_cast<int>(y0),
                                  static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + ih - 3), fg);
            // Handle circles at bottom
            afterhours::draw_circle_lines(static_cast<int>(x0 + 2), static_cast<int>(y0 + ih - 1),
                                          iw * 0.15f, fg);
            afterhours::draw_circle_lines(static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + ih - 1),
                                          iw * 0.15f, fg);
            // Cross point
            afterhours::draw_circle(static_cast<int>(midX), static_cast<int>(midY), 1.0f, fg);
            break;
        }
        case ToolbarIcon::Copy: {
            // Two overlapping pages
            float offset = iw * 0.2f;
            // Back page (offset right/down)
            afterhours::draw_rectangle(
                raylib::Rectangle{x0 + offset, y0 + offset, iw - offset, ih - offset},
                raylib::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{x0 + offset, y0 + offset, iw - offset, ih - offset}, fg);
            // Front page (top-left)
            afterhours::draw_rectangle(
                raylib::Rectangle{x0, y0, iw - offset, ih - offset},
                raylib::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{x0, y0, iw - offset, ih - offset}, fg);
            // Lines on front page
            float lineY = y0 + 3;
            for (int i = 0; i < 3 && lineY < y0 + ih - offset - 3; ++i) {
                afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(lineY),
                                      static_cast<int>(x0 + iw - offset - 2), static_cast<int>(lineY), fg);
                lineY += 3;
            }
            break;
        }
        case ToolbarIcon::Paste: {
            // Clipboard with page
            float clipW = iw * 0.7f;
            float clipX = x0 + (iw - clipW) / 2.0f;
            // Clipboard body
            afterhours::draw_rectangle(
                raylib::Rectangle{clipX, y0 + 2, clipW, ih - 2},
                raylib::Color{200, 180, 120, 255});
            afterhours::draw_rectangle_outline(
                raylib::Rectangle{clipX, y0 + 2, clipW, ih - 2}, fg);
            // Clip at top
            float clipTabW = clipW * 0.5f;
            float clipTabX = clipX + (clipW - clipTabW) / 2.0f;
            afterhours::draw_rectangle(
                raylib::Rectangle{clipTabX, y0, clipTabW, 4},
                raylib::Color{128, 128, 128, 255});
            // Paper on clipboard
            afterhours::draw_rectangle(
                raylib::Rectangle{clipX + 2, y0 + 5, clipW - 4, ih - 9},
                raylib::Color{255, 255, 255, 255});
            break;
        }
        case ToolbarIcon::Undo: {
            // Left-pointing curved arrow
            float arrowY = y0 + ih / 2.0f;
            // Arrow shaft (curved - approximate with horizontal line)
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY), fg);
            // Curved part going up from right
            afterhours::draw_line(static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + 3), fg);
            afterhours::draw_line(static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + 3),
                                  static_cast<int>(x0 + iw / 2), static_cast<int>(y0 + 3), fg);
            // Arrowhead (left-pointing)
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + 6), static_cast<int>(arrowY - 3), fg);
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + 6), static_cast<int>(arrowY + 3), fg);
            break;
        }
        case ToolbarIcon::Redo: {
            // Right-pointing curved arrow (mirror of undo)
            float arrowY = y0 + ih / 2.0f;
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY), fg);
            // Curved part going up from left
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + 2), static_cast<int>(y0 + 3), fg);
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(y0 + 3),
                                  static_cast<int>(x0 + iw / 2), static_cast<int>(y0 + 3), fg);
            // Arrowhead (right-pointing)
            afterhours::draw_line(static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + iw - 6), static_cast<int>(arrowY - 3), fg);
            afterhours::draw_line(static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY),
                                  static_cast<int>(x0 + iw - 6), static_cast<int>(arrowY + 3), fg);
            break;
        }
        case ToolbarIcon::AlignLeft: {
            // Horizontal lines, left-aligned (varying lengths)
            float lineSpacing = ih / 5.0f;
            float lengths[] = {iw, iw * 0.6f, iw * 0.85f, iw * 0.5f};
            for (int i = 0; i < 4; ++i) {
                float ly = y0 + lineSpacing * (static_cast<float>(i) + 1.0f);
                afterhours::draw_line(static_cast<int>(x0), static_cast<int>(ly),
                                      static_cast<int>(x0 + lengths[i]), static_cast<int>(ly), fg);
            }
            break;
        }
        case ToolbarIcon::AlignCenter: {
            // Horizontal lines, centered (varying lengths)
            float lineSpacing = ih / 5.0f;
            float lengths[] = {iw, iw * 0.6f, iw * 0.85f, iw * 0.5f};
            for (int i = 0; i < 4; ++i) {
                float ly = y0 + lineSpacing * (static_cast<float>(i) + 1.0f);
                float lx = x0 + (iw - lengths[i]) / 2.0f;
                afterhours::draw_line(static_cast<int>(lx), static_cast<int>(ly),
                                      static_cast<int>(lx + lengths[i]), static_cast<int>(ly), fg);
            }
            break;
        }
        case ToolbarIcon::AlignRight: {
            // Horizontal lines, right-aligned (varying lengths)
            float lineSpacing = ih / 5.0f;
            float lengths[] = {iw, iw * 0.6f, iw * 0.85f, iw * 0.5f};
            for (int i = 0; i < 4; ++i) {
                float ly = y0 + lineSpacing * (static_cast<float>(i) + 1.0f);
                afterhours::draw_line(static_cast<int>(x0 + iw - lengths[i]), static_cast<int>(ly),
                                      static_cast<int>(x0 + iw), static_cast<int>(ly), fg);
            }
            break;
        }
        case ToolbarIcon::AlignJustify: {
            // All lines full width
            float lineSpacing = ih / 5.0f;
            for (int i = 0; i < 4; ++i) {
                float ly = y0 + lineSpacing * (static_cast<float>(i) + 1.0f);
                afterhours::draw_line(static_cast<int>(x0), static_cast<int>(ly),
                                      static_cast<int>(x0 + iw), static_cast<int>(ly), fg);
            }
            break;
        }
        case ToolbarIcon::None:
            break;
    }
}

// Draw a small filled downward-pointing triangle for dropdown arrows
inline void drawDropdownTriangle(float cx, float cy, raylib::Color color) {
    float halfW = 3.0f;
    float halfH = 2.0f;
    raylib::Vector2 v1 = {cx - halfW, cy - halfH};
    raylib::Vector2 v2 = {cx + halfW, cy - halfH};
    raylib::Vector2 v3 = {cx, cy + halfH};
    afterhours::draw_triangle(v1, v3, v2, color);  // Note: raylib requires CCW winding
}

// Render system that draws toolbar icon overlays and dropdown triangles
// Must run AFTER afterhours UI render systems
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
        
        // Draw toolbar icons that don't overlap with any open popup
        for (const auto& item : toolbar.iconOverlays) {
            if (!overlapsPopup(item.x, item.y, item.w, item.h)) {
                drawToolbarIcon(item.icon, item.x, item.y, item.w, item.enabled);
            }
        }
        
        // Draw dropdown triangles that don't overlap with any open popup
        for (const auto& tri : toolbar.dropdownTriangles) {
            if (!overlapsPopup(tri.x - 4, tri.y - 4, 8, 8)) {
                drawDropdownTriangle(tri.x, tri.y, theme::BUTTON_TEXT);
            }
        }
    }
};

}  // namespace ecs
