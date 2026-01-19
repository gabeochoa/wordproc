#pragma once

// Centralized drawing functions that route through afterhours where available
// This provides a migration path from direct raylib calls to afterhours abstractions

#include <afterhours/src/drawing_helpers.h>
#include "../rl.h"

namespace draw {

// ============================================================
// Text Drawing (uses afterhours)
// ============================================================

inline void text(const char* content, float x, float y, float fontSize, 
                 afterhours::Color color) {
    afterhours::draw_text(content, x, y, fontSize, color);
}

inline void text(const std::string& content, float x, float y, float fontSize,
                 afterhours::Color color) {
    afterhours::draw_text(content.c_str(), x, y, fontSize, color);
}

inline void textEx(raylib::Font font, const char* content, 
                   raylib::Vector2 pos, float fontSize, 
                   float spacing, afterhours::Color color) {
    afterhours::draw_text_ex(font, content, pos, fontSize, spacing, color);
}

// ============================================================
// Rectangle Drawing (uses afterhours)
// ============================================================

inline void rectangle(raylib::Rectangle rect, afterhours::Color color) {
    afterhours::draw_rectangle(rect, color);
}

inline void rectangle(float x, float y, float width, float height, 
                      afterhours::Color color) {
    afterhours::draw_rectangle({x, y, width, height}, color);
}

inline void rectangleOutline(raylib::Rectangle rect, 
                             afterhours::Color color, float thickness = 1.0f) {
    afterhours::draw_rectangle_outline(rect, color, thickness);
}

inline void rectangleRounded(raylib::Rectangle rect, float roundness,
                             int segments, afterhours::Color color) {
    afterhours::draw_rectangle_rounded(rect, roundness, segments, color);
}

// ============================================================
// Line Drawing (still raylib - afterhours doesn't have this yet)
// ============================================================

inline void line(int x1, int y1, int x2, int y2, raylib::Color color) {
    raylib::DrawLine(x1, y1, x2, y2, color);
}

inline void lineEx(raylib::Vector2 start, raylib::Vector2 end, 
                   float thickness, raylib::Color color) {
    raylib::DrawLineEx(start, end, thickness, color);
}

// ============================================================
// Circle Drawing (still raylib - afterhours doesn't have this yet)
// ============================================================

inline void circle(int centerX, int centerY, float radius, raylib::Color color) {
    raylib::DrawCircle(centerX, centerY, radius, color);
}

inline void circleSector(raylib::Vector2 center, float radius, 
                         float startAngle, float endAngle, 
                         int segments, raylib::Color color) {
    raylib::DrawCircleSector(center, radius, startAngle, endAngle, segments, color);
}

// ============================================================
// Ring/Arc Drawing (uses afterhours)
// ============================================================

inline void ring(float centerX, float centerY, float innerRadius,
                 float outerRadius, int segments, afterhours::Color color) {
    afterhours::draw_ring(centerX, centerY, innerRadius, outerRadius, 
                          segments, color);
}

inline void ringSegment(float centerX, float centerY, float innerRadius,
                        float outerRadius, float startAngle, float endAngle,
                        int segments, afterhours::Color color) {
    afterhours::draw_ring_segment(centerX, centerY, innerRadius, outerRadius,
                                  startAngle, endAngle, segments, color);
}

// ============================================================
// Scissor/Clipping (uses afterhours)
// ============================================================

inline void beginScissor(int x, int y, int width, int height) {
    afterhours::begin_scissor_mode(x, y, width, height);
}

inline void endScissor() {
    afterhours::end_scissor_mode();
}

// ============================================================
// Color conversion helpers
// ============================================================

inline afterhours::Color toAh(raylib::Color c) {
    return {c.r, c.g, c.b, c.a};
}

inline raylib::Color toRl(afterhours::Color c) {
    return {c.r, c.g, c.b, c.a};
}

}  // namespace draw

