#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// Pure ruler geometry: no backend, no ECS. The render pass, the drag
// system, and the unit tests share these so a marker drawn at X is the
// marker a drag at X grabs, and the indent a drag computes is the
// indent the text renderer applies.
//
// Coordinate contract (all screen pixels, unscaled — indents are raw
// pixels in TextBuffer, so the ruler is too):
//   originX = left edge of the text span (effective text area + text
//   padding + line-number gutter). Inch 0 sits at originX; inch N at
//   originX + N * PIXELS_PER_INCH. The white band is the effective
//   text area; everything else on the ruler is gray (outside margins).
//   left marker   = originX + leftIndent
//   first-line   = originX + leftIndent + firstLineIndent
//   right marker  = originX + textSpanWidth - rightIndent
// where textSpanWidth = effective width - 2*TEXT_PADDING - gutter, the
// span the text renderer lays text into.
namespace ruler_math {

inline constexpr float TEXT_PADDING = 8.0f;
inline constexpr float PIXELS_PER_INCH = 72.0f;
inline constexpr float GRAB_RADIUS = 7.0f;

struct Geometry {
    float rulerY = 0.0f;
    float rulerHeight = 0.0f;
    float originX = 0.0f;
    float textSpanWidth = 0.0f;
    float whiteLeft = 0.0f;
    float whiteRight = 0.0f;
};

// textArea: the layout text area (ruler sits directly above it).
// effectiveX / effectiveWidth: the margin-respecting text area
// (component_helpers effectiveTextArea). gutterWidth: line-number
// gutter when shown, else 0.
inline Geometry geometry(float textAreaY, float rulerHeight, float effectiveX,
                         float effectiveWidth, float gutterWidth) {
    Geometry g;
    g.rulerY = textAreaY - rulerHeight;
    g.rulerHeight = rulerHeight;
    g.originX = effectiveX + TEXT_PADDING + gutterWidth;
    g.textSpanWidth = effectiveWidth - 2.0f * TEXT_PADDING - gutterWidth;
    if (g.textSpanWidth < 0.0f) g.textSpanWidth = 0.0f;
    g.whiteLeft = effectiveX;
    g.whiteRight = effectiveX + effectiveWidth;
    return g;
}

inline float leftMarkerX(const Geometry& g, int leftIndent) {
    return g.originX + static_cast<float>(leftIndent);
}

inline float firstLineMarkerX(const Geometry& g, int leftIndent,
                              int firstLineIndent) {
    return g.originX + static_cast<float>(leftIndent + firstLineIndent);
}

inline float rightMarkerX(const Geometry& g, int rightIndent) {
    return g.originX + g.textSpanWidth - static_cast<float>(rightIndent);
}

// Inverse mappings, clamped to the text span. The first-line indent
// may go negative (hanging) but never left of the margin (0 absolute).
inline int indentFromX(const Geometry& g, float x) {
    int v = static_cast<int>(std::lround(x - g.originX));
    return std::clamp(v, 0, static_cast<int>(std::lround(g.textSpanWidth)));
}

inline int firstLineIndentFromX(const Geometry& g, float x, int leftIndent) {
    int v = static_cast<int>(
        std::lround(x - g.originX - static_cast<float>(leftIndent)));
    return std::clamp(v, -leftIndent,
                      static_cast<int>(std::lround(g.textSpanWidth)) - leftIndent);
}

inline int rightIndentFromX(const Geometry& g, float x) {
    int v = static_cast<int>(
        std::lround(g.originX + g.textSpanWidth - x));
    return std::clamp(v, 0, static_cast<int>(std::lround(g.textSpanWidth)));
}

enum class Marker { None, FirstLine, Left, Right };

// Which marker, if any, a press at (x, y) grabs. The top half of the
// ruler belongs to the first-line triangle; the bottom half to the
// left / right triangles (nearest wins).
inline Marker hitTest(const Geometry& g, int leftIndent, int firstLineIndent,
                      int rightIndent, float x, float y) {
    if (y < g.rulerY || y >= g.rulerY + g.rulerHeight) return Marker::None;
    float midY = g.rulerY + g.rulerHeight / 2.0f;
    if (y < midY) {
        if (std::abs(x - firstLineMarkerX(g, leftIndent, firstLineIndent)) <=
            GRAB_RADIUS)
            return Marker::FirstLine;
        return Marker::None;
    }
    float leftX = leftMarkerX(g, leftIndent);
    float rightX = rightMarkerX(g, rightIndent);
    bool nearLeft = std::abs(x - leftX) <= GRAB_RADIUS;
    bool nearRight = std::abs(x - rightX) <= GRAB_RADIUS;
    if (nearLeft && nearRight)
        return (std::abs(x - leftX) <= std::abs(x - rightX)) ? Marker::Left
                                                              : Marker::Right;
    if (nearLeft) return Marker::Left;
    if (nearRight) return Marker::Right;
    return Marker::None;
}

// Default tab stops implied by Tab-inserts-spaces: one stop per
// tabWidth spaces, spaced by the width of a single space at the
// current font size. Positions are ruler coordinates.
inline std::vector<float> tabStopXs(const Geometry& g, int tabWidthSpaces,
                                    float spaceWidth) {
    std::vector<float> stops;
    if (tabWidthSpaces <= 0 || spaceWidth <= 0.0f) return stops;
    float step = static_cast<float>(tabWidthSpaces) * spaceWidth;
    for (float x = g.originX + step; x < g.originX + g.textSpanWidth; x += step)
        stops.push_back(x);
    return stops;
}

}  // namespace ruler_math
