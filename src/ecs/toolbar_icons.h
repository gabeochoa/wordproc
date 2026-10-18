#pragma once

// Toolbar icon geometry.
//
// The icons are 16x16 pixel art assembled from lines, rectangles and triangles
// rather than textures. Each function takes the rect it should draw inside, so
// it can be handed to afterhours as a component's on_draw_fg callback and run
// inside the normal render pass (feature request #9).

#include <afterhours/src/drawing_helpers.h>
#include "../rl.h"
#include "../ui/theme.h"
#include "components.h"

namespace ecs {

// Draw a simple 16x16 pixel-art icon at (cx, cy) center position
// All icons use simple geometric primitives (lines, rectangles, triangles)
inline void drawToolbarIcon(ToolbarIcon icon, float cx, float cy, float size, bool enabled) {
    afterhours::Color fg = enabled ? theme::BUTTON_TEXT : theme::MENU_DISABLED;
    
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
                Rectangle{x0, y0, iw, ih}, fg);
            // Inner white fill
            afterhours::draw_rectangle(
                Rectangle{x0 + 1, y0 + 1, iw - 2, ih - 2},
                afterhours::Color{255, 255, 255, 255});
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
                Rectangle{x0, y0, tabW, tabH},
                afterhours::Color{255, 220, 100, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0, y0, tabW, tabH}, fg);
            // Folder body
            afterhours::draw_rectangle(
                Rectangle{x0, y0 + tabH, iw, ih - tabH},
                afterhours::Color{255, 220, 100, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0, y0 + tabH, iw, ih - tabH}, fg);
            break;
        }
        case ToolbarIcon::Save: {
            // Floppy disk
            // Outer rectangle
            afterhours::draw_rectangle(
                Rectangle{x0, y0, iw, ih},
                afterhours::Color{50, 50, 180, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0, y0, iw, ih}, fg);
            // Metal slider (top center)
            float sliderW = iw * 0.5f;
            float sliderH = ih * 0.25f;
            float sliderX = x0 + (iw - sliderW) / 2.0f;
            afterhours::draw_rectangle(
                Rectangle{sliderX, y0, sliderW, sliderH},
                afterhours::Color{192, 192, 192, 255});
            // Label area (bottom)
            float labelH = ih * 0.35f;
            afterhours::draw_rectangle(
                Rectangle{x0 + 2, y0 + ih - labelH, iw - 4, labelH},
                afterhours::Color{240, 240, 240, 255});
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
                Rectangle{paperX, y0, paperW, bodyY - y0 + 2},
                afterhours::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{paperX, y0, paperW, bodyY - y0 + 2}, fg);
            // Printer body
            afterhours::draw_rectangle(
                Rectangle{x0, bodyY, iw, bodyH},
                afterhours::Color{192, 192, 192, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0, bodyY, iw, bodyH}, fg);
            // Output tray
            float trayW = iw * 0.7f;
            float trayX = x0 + (iw - trayW) / 2.0f;
            afterhours::draw_rectangle(
                Rectangle{trayX, bodyY + bodyH, trayW, ih - bodyH - ih * 0.25f},
                afterhours::Color{192, 192, 192, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{trayX, bodyY + bodyH, trayW, ih - bodyH - ih * 0.25f}, fg);
            break;
        }
        case ToolbarIcon::PrintPreview: {
            // Page with a magnifying glass over it (Word 95 Print
            // Preview icon): page at left, lens bottom-right.
            afterhours::draw_rectangle(
                Rectangle{x0, y0, iw * 0.62f, ih},
                afterhours::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0, y0, iw * 0.62f, ih}, fg);
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(y0 + 3),
                                  static_cast<int>(x0 + iw * 0.62f - 2), static_cast<int>(y0 + 3), fg);
            afterhours::draw_line(static_cast<int>(x0 + 2), static_cast<int>(y0 + 6),
                                  static_cast<int>(x0 + iw * 0.62f - 2), static_cast<int>(y0 + 6), fg);
            // Magnifier lens + handle
            float lensX = x0 + iw * 0.68f;
            float lensY = y0 + ih * 0.62f;
            afterhours::draw_circle_lines(static_cast<int>(lensX), static_cast<int>(lensY),
                                          iw * 0.22f, fg);
            afterhours::draw_line(static_cast<int>(lensX + iw * 0.15f),
                                  static_cast<int>(lensY + ih * 0.15f),
                                  static_cast<int>(x0 + iw - 1),
                                  static_cast<int>(y0 + ih - 1), fg);
            break;
        }
        case ToolbarIcon::Spelling: {
            // "ABC" tiny letters with a green check mark (Word 95
            // Spelling icon): the ABC is drawn as text at 6px, the
            // check as two green strokes below-right of it.
            afterhours::draw_text("ABC", static_cast<int>(x0),
                                  static_cast<int>(y0), 6, fg);
            afterhours::Color check{0, 128, 0, 255};
            afterhours::draw_line(static_cast<int>(x0 + 1), static_cast<int>(y0 + ih * 0.66f),
                                  static_cast<int>(x0 + iw * 0.38f), static_cast<int>(y0 + ih * 0.88f), check);
            afterhours::draw_line(static_cast<int>(x0 + iw * 0.38f), static_cast<int>(y0 + ih * 0.88f),
                                  static_cast<int>(x0 + iw - 1), static_cast<int>(y0 + ih * 0.38f), check);
            break;
        }
        case ToolbarIcon::FormatPainter: {
            // Paintbrush, diagonal: bristle head top-left, ferrule,
            // handle to bottom-right.
            afterhours::draw_rectangle(
                Rectangle{x0 + 1, y0 + 1, iw * 0.34f, ih * 0.34f},
                afterhours::Color{205, 170, 90, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0 + 1, y0 + 1, iw * 0.34f, ih * 0.34f}, fg);
            afterhours::draw_rectangle(
                Rectangle{x0 + iw * 0.3f, y0 + ih * 0.3f, iw * 0.28f, ih * 0.2f},
                afterhours::Color{192, 192, 192, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0 + iw * 0.3f, y0 + ih * 0.3f, iw * 0.28f, ih * 0.2f}, fg);
            afterhours::draw_line(static_cast<int>(x0 + iw * 0.45f),
                                  static_cast<int>(y0 + ih * 0.45f),
                                  static_cast<int>(x0 + iw - 1),
                                  static_cast<int>(y0 + ih - 1), fg);
            afterhours::draw_line(static_cast<int>(x0 + iw * 0.45f + 1),
                                  static_cast<int>(y0 + ih * 0.45f),
                                  static_cast<int>(x0 + iw - 1),
                                  static_cast<int>(y0 + ih - 2), fg);
            break;
        }
        case ToolbarIcon::ShowParagraph: {
            // Pilcrow ¶ glyph (Show/Hide ¶), drawn as text: the
            // default font carries the Latin-1 ¶ (verified in the
            // toolbar screenshot).
            afterhours::draw_text("\xC2\xB6", static_cast<int>(x0 + 1),
                                  static_cast<int>(y0), 13, fg);
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
                Rectangle{x0 + offset, y0 + offset, iw - offset, ih - offset},
                afterhours::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0 + offset, y0 + offset, iw - offset, ih - offset}, fg);
            // Front page (top-left)
            afterhours::draw_rectangle(
                Rectangle{x0, y0, iw - offset, ih - offset},
                afterhours::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{x0, y0, iw - offset, ih - offset}, fg);
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
                Rectangle{clipX, y0 + 2, clipW, ih - 2},
                afterhours::Color{200, 180, 120, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{clipX, y0 + 2, clipW, ih - 2}, fg);
            // Clip at top
            float clipTabW = clipW * 0.5f;
            float clipTabX = clipX + (clipW - clipTabW) / 2.0f;
            afterhours::draw_rectangle(
                Rectangle{clipTabX, y0, clipTabW, 4},
                afterhours::Color{128, 128, 128, 255});
            // Paper on clipboard
            afterhours::draw_rectangle(
                Rectangle{clipX + 2, y0 + 5, clipW - 4, ih - 9},
                afterhours::Color{255, 255, 255, 255});
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
inline void drawDropdownTriangle(float cx, float cy, afterhours::Color color) {
    float halfW = 3.0f;
    float halfH = 2.0f;
    vec2 v1 = {cx - halfW, cy - halfH};
    vec2 v2 = {cx + halfW, cy - halfH};
    vec2 v3 = {cx, cy + halfH};
    afterhours::draw_triangle(v1, v3, v2, color);  // Note: CCW winding order
}

}  // namespace ecs
