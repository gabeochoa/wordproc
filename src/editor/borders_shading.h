#pragma once

// Format > Borders and Shading... dialog model (Word 95 parity).
// Pure value <-> stored-value mapping, unit-tested; the dialog in
// MenuUISystem owns the working copy and applies the result through
// TextBuffer::setLineParagraphFormat + commitParagraphFormat (one
// undo step, like the Paragraph dialog).
//
// Stored on LineSpan / ParagraphFormat the way indents are stored:
// a border-edges bitmask + a border style + a shading color index.
// Style is a single line only (Word 95's "single" border); the
// shading palette is None + the highlight palette, so shading stays
// readable (pale fills behind black text).

#include <array>

#include "document_settings.h"

namespace borders_shading {

// Border edges, bitmask values stored in ParagraphFormat::borderEdges.
inline constexpr int kEdgeTop = 1;
inline constexpr int kEdgeBottom = 2;
inline constexpr int kEdgeLeft = 4;
inline constexpr int kEdgeRight = 8;
inline constexpr int kEdgeAll = kEdgeTop | kEdgeBottom | kEdgeLeft | kEdgeRight;

// Border style stored in ParagraphFormat::borderStyle: no border,
// or one single (1px, Thin) line. A style of Single with no edges
// draws nothing, same as None.
inline constexpr int kBorderStyleNone = 0;
inline constexpr int kBorderStyleSingle = 1;

// Shading palette stored as an index into kShadingColors in
// ParagraphFormat::shadingColorIndex. Index 0 is None (no fill);
// the rest are the HighlightColors palette in menu order.
inline constexpr int kShadingCount = 9;
inline constexpr std::array<TextColor, kShadingCount> kShadingColors = {
    HighlightColors::None,   HighlightColors::Yellow,
    HighlightColors::Green,  HighlightColors::Cyan,
    HighlightColors::Pink,   HighlightColors::Orange,
    HighlightColors::Blue,   HighlightColors::Purple,
    HighlightColors::Gray,
};
inline constexpr const char* kShadingNames[kShadingCount] = {
    "None", "Yellow", "Green", "Cyan", "Pink",
    "Orange", "Blue", "Purple", "Gray",
};

// Shading color for a stored index; out-of-range reads as None
// (a stale file value never paints an arbitrary color).
inline TextColor shadingColorForIndex(int index) {
    if (index < 0 || index >= kShadingCount) return HighlightColors::None;
    return kShadingColors[static_cast<std::size_t>(index)];
}

// Clamp a working shading index to the palette range.
inline int clampShadingIndex(int index) {
    if (index < 0) return 0;
    if (index >= kShadingCount) return kShadingCount - 1;
    return index;
}

// True when the stored triple draws a border: Single style with
// at least one edge set.
inline bool hasParagraphBorder(int edges, int style) {
    return style == kBorderStyleSingle && (edges & kEdgeAll) != 0;
}

// True when the stored index draws a shading fill.
inline bool hasParagraphShading(int shadingIndex) {
    return shadingIndex != 0 && shadingColorForIndex(shadingIndex).a != 0;
}

// True when two adjacent paragraphs share the whole triple, so
// the renderer merges them into one box instead of drawing an
// internal horizontal edge between them.
inline bool sameBorderShading(int edgesA, int styleA, int shadingA,
                              int edgesB, int styleB, int shadingB) {
    return edgesA == edgesB && styleA == styleB && shadingA == shadingB;
}

// Border/shading rect padding, in display pixels. The rect is the
// paragraph's line box expanded on all sides: a 4px horizontal /
// 2px vertical base margin plus a 3px expansion, so glyphs never
// touch the stroke (the breathing room the reviewer asked for).
// Text position is unchanged — only the rect grows.
inline constexpr float kBorderBasePaddingX = 4.0f;
inline constexpr float kBorderBasePaddingY = 2.0f;
inline constexpr float kBorderPaddingExpansion = 3.0f;

struct BorderBox {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
};

// The border/shading rect for one line: the indented paragraph
// width (baseX + leftIndent .. baseX + availableWidth -
// rightIndent) and the line box (y .. y + lineHeight), each
// expanded by the base padding + the 3px expansion.
inline BorderBox borderBoxRect(float baseX, float leftIndent,
                               float availableWidth, float rightIndent,
                               float y, float lineHeight) {
    float padX = kBorderBasePaddingX + kBorderPaddingExpansion;
    float padY = kBorderBasePaddingY + kBorderPaddingExpansion;
    return {baseX + leftIndent - padX, y - padY,
            baseX + availableWidth - rightIndent + padX,
            y + lineHeight + padY};
}

// Which border edges to draw for one line inside a merged group.
// The top edge draws only on the group's first line and the bottom
// only on the group's last line, so consecutive identical
// paragraphs read as one box. Left/right draw on every line.
// When merging is off (columns / outline) the caller passes
// sameAsPrev = sameAsNext = false and every line gets all its edges.
struct EdgesToDraw {
    bool top = false;
    bool bottom = false;
    bool left = false;
    bool right = false;
};

inline EdgesToDraw edgesToDraw(int edges, int style, bool sameAsPrev,
                               bool sameAsNext) {
    EdgesToDraw out;
    if (!hasParagraphBorder(edges, style)) return out;
    out.top = !sameAsPrev && (edges & kEdgeTop) != 0;
    out.bottom = !sameAsNext && (edges & kEdgeBottom) != 0;
    out.left = (edges & kEdgeLeft) != 0;
    out.right = (edges & kEdgeRight) != 0;
    return out;
}

}  // namespace borders_shading
