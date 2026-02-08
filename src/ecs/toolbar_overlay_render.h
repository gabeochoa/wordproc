#pragma once

// Toolbar Overlay Render System
// Draws toolbar icons and dropdown triangles AFTER afterhours UI render pass.
// This is a workaround for afterhours not supporting bitmap icons or programmatic glyphs.

#include "../../vendor/afterhours/src/core/system.h"
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
            raylib::DrawRectangleLines(static_cast<int>(x0), static_cast<int>(y0),
                                       static_cast<int>(iw), static_cast<int>(ih), fg);
            // Inner white fill
            raylib::DrawRectangle(static_cast<int>(x0 + 1), static_cast<int>(y0 + 1),
                                  static_cast<int>(iw - 2), static_cast<int>(ih - 2),
                                  raylib::Color{255, 255, 255, 255});
            // Corner fold (triangle in top-right)
            raylib::DrawLine(static_cast<int>(x0 + iw - fold), static_cast<int>(y0),
                            static_cast<int>(x0 + iw - fold), static_cast<int>(y0 + fold), fg);
            raylib::DrawLine(static_cast<int>(x0 + iw - fold), static_cast<int>(y0 + fold),
                            static_cast<int>(x0 + iw), static_cast<int>(y0 + fold), fg);
            break;
        }
        case ToolbarIcon::Open: {
            // Folder shape
            float tabW = iw * 0.4f;
            float tabH = ih * 0.2f;
            // Folder tab (top-left)
            raylib::DrawRectangle(static_cast<int>(x0), static_cast<int>(y0),
                                  static_cast<int>(tabW), static_cast<int>(tabH),
                                  raylib::Color{255, 220, 100, 255});
            raylib::DrawRectangleLines(static_cast<int>(x0), static_cast<int>(y0),
                                       static_cast<int>(tabW), static_cast<int>(tabH), fg);
            // Folder body
            raylib::DrawRectangle(static_cast<int>(x0), static_cast<int>(y0 + tabH),
                                  static_cast<int>(iw), static_cast<int>(ih - tabH),
                                  raylib::Color{255, 220, 100, 255});
            raylib::DrawRectangleLines(static_cast<int>(x0), static_cast<int>(y0 + tabH),
                                       static_cast<int>(iw), static_cast<int>(ih - tabH), fg);
            break;
        }
        case ToolbarIcon::Save: {
            // Floppy disk
            // Outer rectangle
            raylib::DrawRectangle(static_cast<int>(x0), static_cast<int>(y0),
                                  static_cast<int>(iw), static_cast<int>(ih),
                                  raylib::Color{50, 50, 180, 255});
            raylib::DrawRectangleLines(static_cast<int>(x0), static_cast<int>(y0),
                                       static_cast<int>(iw), static_cast<int>(ih), fg);
            // Metal slider (top center)
            float sliderW = iw * 0.5f;
            float sliderH = ih * 0.25f;
            float sliderX = x0 + (iw - sliderW) / 2.0f;
            raylib::DrawRectangle(static_cast<int>(sliderX), static_cast<int>(y0),
                                  static_cast<int>(sliderW), static_cast<int>(sliderH),
                                  raylib::Color{192, 192, 192, 255});
            // Label area (bottom)
            float labelH = ih * 0.35f;
            raylib::DrawRectangle(static_cast<int>(x0 + 2), static_cast<int>(y0 + ih - labelH),
                                  static_cast<int>(iw - 4), static_cast<int>(labelH),
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
            raylib::DrawRectangle(static_cast<int>(paperX), static_cast<int>(y0),
                                  static_cast<int>(paperW), static_cast<int>(bodyY - y0 + 2),
                                  raylib::Color{255, 255, 255, 255});
            raylib::DrawRectangleLines(static_cast<int>(paperX), static_cast<int>(y0),
                                       static_cast<int>(paperW), static_cast<int>(bodyY - y0 + 2), fg);
            // Printer body
            raylib::DrawRectangle(static_cast<int>(x0), static_cast<int>(bodyY),
                                  static_cast<int>(iw), static_cast<int>(bodyH),
                                  raylib::Color{192, 192, 192, 255});
            raylib::DrawRectangleLines(static_cast<int>(x0), static_cast<int>(bodyY),
                                       static_cast<int>(iw), static_cast<int>(bodyH), fg);
            // Output tray
            float trayW = iw * 0.7f;
            float trayX = x0 + (iw - trayW) / 2.0f;
            raylib::DrawRectangle(static_cast<int>(trayX), static_cast<int>(bodyY + bodyH),
                                  static_cast<int>(trayW), static_cast<int>(ih - bodyH - ih * 0.25f),
                                  raylib::Color{192, 192, 192, 255});
            raylib::DrawRectangleLines(static_cast<int>(trayX), static_cast<int>(bodyY + bodyH),
                                       static_cast<int>(trayW), static_cast<int>(ih - bodyH - ih * 0.25f), fg);
            break;
        }
        case ToolbarIcon::Cut: {
            // Scissors: two crossing lines with circles at handles
            float midX = x0 + iw / 2.0f;
            float midY = y0 + ih / 2.0f;
            // Blades (X shape from top-center to bottom corners)
            raylib::DrawLine(static_cast<int>(midX), static_cast<int>(y0),
                            static_cast<int>(x0 + 2), static_cast<int>(y0 + ih - 3), fg);
            raylib::DrawLine(static_cast<int>(midX), static_cast<int>(y0),
                            static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + ih - 3), fg);
            // Handle circles at bottom
            raylib::DrawCircleLines(static_cast<int>(x0 + 2), static_cast<int>(y0 + ih - 1),
                                    iw * 0.15f, fg);
            raylib::DrawCircleLines(static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + ih - 1),
                                    iw * 0.15f, fg);
            // Cross point
            raylib::DrawCircle(static_cast<int>(midX), static_cast<int>(midY), 1.0f, fg);
            break;
        }
        case ToolbarIcon::Copy: {
            // Two overlapping pages
            float offset = iw * 0.2f;
            // Back page (offset right/down)
            raylib::DrawRectangle(static_cast<int>(x0 + offset), static_cast<int>(y0 + offset),
                                  static_cast<int>(iw - offset), static_cast<int>(ih - offset),
                                  raylib::Color{255, 255, 255, 255});
            raylib::DrawRectangleLines(static_cast<int>(x0 + offset), static_cast<int>(y0 + offset),
                                       static_cast<int>(iw - offset), static_cast<int>(ih - offset), fg);
            // Front page (top-left)
            raylib::DrawRectangle(static_cast<int>(x0), static_cast<int>(y0),
                                  static_cast<int>(iw - offset), static_cast<int>(ih - offset),
                                  raylib::Color{255, 255, 255, 255});
            raylib::DrawRectangleLines(static_cast<int>(x0), static_cast<int>(y0),
                                       static_cast<int>(iw - offset), static_cast<int>(ih - offset), fg);
            // Lines on front page
            float lineY = y0 + 3;
            for (int i = 0; i < 3 && lineY < y0 + ih - offset - 3; ++i) {
                raylib::DrawLine(static_cast<int>(x0 + 2), static_cast<int>(lineY),
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
            raylib::DrawRectangle(static_cast<int>(clipX), static_cast<int>(y0 + 2),
                                  static_cast<int>(clipW), static_cast<int>(ih - 2),
                                  raylib::Color{200, 180, 120, 255});
            raylib::DrawRectangleLines(static_cast<int>(clipX), static_cast<int>(y0 + 2),
                                       static_cast<int>(clipW), static_cast<int>(ih - 2), fg);
            // Clip at top
            float clipTabW = clipW * 0.5f;
            float clipTabX = clipX + (clipW - clipTabW) / 2.0f;
            raylib::DrawRectangle(static_cast<int>(clipTabX), static_cast<int>(y0),
                                  static_cast<int>(clipTabW), static_cast<int>(4),
                                  raylib::Color{128, 128, 128, 255});
            // Paper on clipboard
            raylib::DrawRectangle(static_cast<int>(clipX + 2), static_cast<int>(y0 + 5),
                                  static_cast<int>(clipW - 4), static_cast<int>(ih - 9),
                                  raylib::Color{255, 255, 255, 255});
            break;
        }
        case ToolbarIcon::Undo: {
            // Left-pointing curved arrow
            float arrowY = y0 + ih / 2.0f;
            // Arrow shaft (curved - approximate with horizontal line)
            raylib::DrawLine(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY), fg);
            // Curved part going up from right
            raylib::DrawLine(static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + 3), fg);
            raylib::DrawLine(static_cast<int>(x0 + iw - 2), static_cast<int>(y0 + 3),
                            static_cast<int>(x0 + iw / 2), static_cast<int>(y0 + 3), fg);
            // Arrowhead (left-pointing)
            raylib::DrawLine(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + 6), static_cast<int>(arrowY - 3), fg);
            raylib::DrawLine(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + 6), static_cast<int>(arrowY + 3), fg);
            break;
        }
        case ToolbarIcon::Redo: {
            // Right-pointing curved arrow (mirror of undo)
            float arrowY = y0 + ih / 2.0f;
            raylib::DrawLine(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY), fg);
            // Curved part going up from left
            raylib::DrawLine(static_cast<int>(x0 + 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + 2), static_cast<int>(y0 + 3), fg);
            raylib::DrawLine(static_cast<int>(x0 + 2), static_cast<int>(y0 + 3),
                            static_cast<int>(x0 + iw / 2), static_cast<int>(y0 + 3), fg);
            // Arrowhead (right-pointing)
            raylib::DrawLine(static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + iw - 6), static_cast<int>(arrowY - 3), fg);
            raylib::DrawLine(static_cast<int>(x0 + iw - 2), static_cast<int>(arrowY),
                            static_cast<int>(x0 + iw - 6), static_cast<int>(arrowY + 3), fg);
            break;
        }
        case ToolbarIcon::AlignLeft: {
            // Horizontal lines, left-aligned (varying lengths)
            float lineSpacing = ih / 5.0f;
            float lengths[] = {iw, iw * 0.6f, iw * 0.85f, iw * 0.5f};
            for (int i = 0; i < 4; ++i) {
                float ly = y0 + lineSpacing * (static_cast<float>(i) + 1.0f);
                raylib::DrawLine(static_cast<int>(x0), static_cast<int>(ly),
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
                raylib::DrawLine(static_cast<int>(lx), static_cast<int>(ly),
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
                raylib::DrawLine(static_cast<int>(x0 + iw - lengths[i]), static_cast<int>(ly),
                                static_cast<int>(x0 + iw), static_cast<int>(ly), fg);
            }
            break;
        }
        case ToolbarIcon::AlignJustify: {
            // All lines full width
            float lineSpacing = ih / 5.0f;
            for (int i = 0; i < 4; ++i) {
                float ly = y0 + lineSpacing * (static_cast<float>(i) + 1.0f);
                raylib::DrawLine(static_cast<int>(x0), static_cast<int>(ly),
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
    raylib::DrawTriangle(v1, v3, v2, color);  // Note: raylib requires CCW winding
}

// Render system that draws toolbar icon overlays, dropdown triangles, and menu access key underlines
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
        
        // Draw all toolbar icons
        for (const auto& item : toolbar.iconOverlays) {
            drawToolbarIcon(item.icon, item.x, item.y, item.w, item.enabled);
        }
        
        // Draw dropdown triangles
        for (const auto& tri : toolbar.dropdownTriangles) {
            drawDropdownTriangle(tri.x, tri.y, theme::BUTTON_TEXT);
        }
        
        // Draw menu access key underlines
        auto menuEntities = afterhours::EntityQuery({.force_merge = true})
                               .whereHasComponent<MenuComponent>()
                               .gen();
        if (!menuEntities.empty()) {
            const auto& menu = menuEntities[0].get().get<MenuComponent>();
            drawMenuAccessKeyUnderlines(menu);
        }
    }
    
    // Draw underlines under the access key (first character) of each menu header
    static void drawMenuAccessKeyUnderlines(const MenuComponent& menu) {
        int menuFontSize = 14;
        float headerX = theme::layout::scale(4.0f);
        float headerY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT);
        float menuBarHeight = theme::layout::scale(theme::layout::MENU_BAR_HEIGHT);
        
        for (size_t i = 0; i < menu.menus.size(); ++i) {
            const auto& menuDef = menu.menus[i];
            float buttonWidth = static_cast<float>(
                theme::MeasureUIText(menuDef.label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
            
            // The text is centered in the button. Calculate position of first character.
            float textW = static_cast<float>(theme::MeasureUIText(menuDef.label.c_str(), menuFontSize));
            float textStart = headerX + (buttonWidth - textW) / 2.0f;
            
            // Measure width of first character for underline
            if (!menuDef.label.empty()) {
                std::string firstChar = menuDef.label.substr(0, 1);
                float charW = static_cast<float>(theme::MeasureUIText(firstChar.c_str(), menuFontSize));
                
                // Determine color based on highlight state
                bool isOpen = menuDef.open;
                raylib::Color underlineColor = isOpen ? theme::MENU_TEXT_HOVER : theme::MENU_TEXT;
                
                // Draw underline 1px below the text baseline
                float underlineY = headerY + menuBarHeight - theme::layout::scale(4.0f);
                raylib::DrawLine(
                    static_cast<int>(textStart),
                    static_cast<int>(underlineY),
                    static_cast<int>(textStart + charW),
                    static_cast<int>(underlineY),
                    underlineColor);
            }
            
            headerX += buttonWidth;
        }
    }
};

}  // namespace ecs
