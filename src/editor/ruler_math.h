#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// Pure ruler geometry: no backend, no ECS. The render pass, the drag
// system, and the unit tests share these so a marker drawn at X is the
// marker a drag at X grabs, and the indent a drag computes is the
// indent the text renderer applies.
//
// Coordinate contract:
//   originX = left edge of the text span (effective text area + text
//   padding + line-number gutter). Inch 0 sits at originX; inch N at
//   originX + N * pixelsPerInch, where pixelsPerInch is THE single
//   display scale (layout::pixelsPerInch in component_helpers):
//   72 * displayScale, displayScale = (paged ? pageScale(fit) : 1) *
//   zoomLevel. The page width is therefore 8.5 * pixelsPerInch and the
//   page edges sit 1" (one margin) either side of the white band:
//   page left = originX - TEXT_PADDING - marginPx, page right =
//   originX - TEXT_PADDING + pageWidthPx. The white band is the
//   effective text area; everything else on the ruler is gray.
// Indents are stored in TextBuffer in 72-dpi units (72 = 1 inch at
// scale 1); on screen they are indent * indentScale where
// indentScale = pixelsPerInch / 72, so a stored 72 sits exactly on
// the 1" tick at any page fit / zoom. The text renderer scales
// indents by the same factor, so marker, text, and ruler ticks agree.
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
    // Display pixels per inch used for ticks and indent scaling.
    // Must come from layout::pixelsPerInch — never a second constant.
    float pixelsPerInch = PIXELS_PER_INCH;
    float indentScale() const { return pixelsPerInch / PIXELS_PER_INCH; }
};

// textArea: the layout text area (ruler sits directly above it).
// effectiveX / effectiveWidth: the margin-respecting text area
// (component_helpers effectiveTextArea). gutterWidth: line-number
// gutter when shown, else 0. pixelsPerInch: layout::pixelsPerInch.
inline Geometry geometry(float textAreaY, float rulerHeight, float effectiveX,
                         float effectiveWidth, float gutterWidth,
                         float pixelsPerInch = PIXELS_PER_INCH) {
    Geometry g;
    g.rulerY = textAreaY - rulerHeight;
    g.rulerHeight = rulerHeight;
    g.originX = effectiveX + TEXT_PADDING + gutterWidth;
    g.textSpanWidth = effectiveWidth - 2.0f * TEXT_PADDING - gutterWidth;
    if (g.textSpanWidth < 0.0f) g.textSpanWidth = 0.0f;
    g.whiteLeft = effectiveX;
    g.whiteRight = effectiveX + effectiveWidth;
    g.pixelsPerInch = pixelsPerInch;
    return g;
}

inline float leftMarkerX(const Geometry& g, int leftIndent) {
    return g.originX + static_cast<float>(leftIndent) * g.indentScale();
}

inline float firstLineMarkerX(const Geometry& g, int leftIndent,
                              int firstLineIndent) {
    return g.originX +
           static_cast<float>(leftIndent + firstLineIndent) * g.indentScale();
}

inline float rightMarkerX(const Geometry& g, int rightIndent) {
    return g.originX + g.textSpanWidth -
           static_cast<float>(rightIndent) * g.indentScale();
}

// Inverse mappings, clamped to the text span. The first-line indent
// may go negative (hanging) but never left of the margin (0 absolute).
// Stored units: divide the screen distance by indentScale.
inline int indentFromX(const Geometry& g, float x) {
    float scale = g.indentScale();
    if (scale <= 0.0f) scale = 1.0f;
    int maxIndent = static_cast<int>(std::lround(g.textSpanWidth / scale));
    int v = static_cast<int>(std::lround((x - g.originX) / scale));
    return std::clamp(v, 0, maxIndent);
}

inline int firstLineIndentFromX(const Geometry& g, float x, int leftIndent) {
    float scale = g.indentScale();
    if (scale <= 0.0f) scale = 1.0f;
    int maxIndent = static_cast<int>(std::lround(g.textSpanWidth / scale));
    int v = static_cast<int>(
        std::lround((x - g.originX) / scale - static_cast<float>(leftIndent)));
    return std::clamp(v, -leftIndent, maxIndent - leftIndent);
}

inline int rightIndentFromX(const Geometry& g, float x) {
    float scale = g.indentScale();
    if (scale <= 0.0f) scale = 1.0f;
    int maxIndent = static_cast<int>(std::lround(g.textSpanWidth / scale));
    int v = static_cast<int>(
        std::lround((g.originX + g.textSpanWidth - x) / scale));
    return std::clamp(v, 0, maxIndent);
}

// Marker shapes for the render pass: Word 95 hourglass pair +
// rectangle base + right triangle. Directions point TOWARD the ruler
// centre (first-line apex down, hanging/right apex up), so the pair
// forms an hourglass when first-line == 0. The left (hanging) triangle
// sits on a small rectangle at the band's bottom edge.
struct Triangle {
    float apexX = 0.0f;
    float apexY = 0.0f;
    float baseY = 0.0f;
    float halfWidth = 4.0f;
};
struct MarkerShapes {
    Triangle firstLine;
    Triangle left;
    Triangle right;
    float rectX = 0.0f;
    float rectY = 0.0f;
    float rectW = 6.0f;
    float rectH = 2.0f;
};

inline MarkerShapes markerShapes(const Geometry& g, int leftIndent,
                                 int firstLineIndent, int rightIndent) {
    MarkerShapes s;
    float mid = g.rulerY + g.rulerHeight / 2.0f;
    float leftX = leftMarkerX(g, leftIndent);
    s.firstLine = {firstLineMarkerX(g, leftIndent, firstLineIndent),
                   mid - 1.0f, g.rulerY + 1.0f, 4.0f};
    s.left = {leftX, mid + 1.0f, g.rulerY + g.rulerHeight - 3.0f, 4.0f};
    s.right = {rightMarkerX(g, rightIndent), mid + 1.0f,
               g.rulerY + g.rulerHeight - 1.0f, 4.0f};
    s.rectX = leftX - 3.0f;
    s.rectY = g.rulerY + g.rulerHeight - 3.0f;
    s.rectW = 6.0f;
    s.rectH = 2.0f;
    return s;
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
