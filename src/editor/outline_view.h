#pragma once

// View > Outline mode helpers (Word 95 parity).
// Pure functions over paragraph styles so the visible-rows mapping
// is unit-testable; the renderer, the click hit-test, and the
// Outline toggle all share these (no second copy of the mapping).
// Outline shows ONLY Title / Subtitle / Heading 1-6 paragraphs:
// body (Normal) paragraphs are not rendered, scroll counts visible
// heading rows, and a click on visible row V lands on the heading's
// real buffer row.

#include <cstddef>
#include <vector>

#include "document_settings.h"

namespace outline_view {

// Outline level of a style, matching TextBuffer::getOutline:
// Title 0, Subtitle/Heading 1 = 1, Heading 2 = 2, ..., Heading 6
// = 6. Normal is not an outline row: -1.
inline int outlineLevel(ParagraphStyle style) {
    switch (style) {
        case ParagraphStyle::Title: return 0;
        case ParagraphStyle::Subtitle:
        case ParagraphStyle::Heading1: return 1;
        case ParagraphStyle::Heading2: return 2;
        case ParagraphStyle::Heading3: return 3;
        case ParagraphStyle::Heading4: return 4;
        case ParagraphStyle::Heading5: return 5;
        case ParagraphStyle::Heading6: return 6;
        case ParagraphStyle::Normal:
        case ParagraphStyle::Count:
        default: return -1;
    }
}

inline bool isHeadingStyle(ParagraphStyle style) {
    return outlineLevel(style) >= 0;
}

// Indent of a heading row in the outline: (level - 1) * 24px, so
// Title and Heading 1 sit at 0, Heading 2 at 24, Heading 3 at 48...
inline int outlineIndentPx(ParagraphStyle style) {
    int level = outlineLevel(style);
    if (level <= 1) return 0;
    return (level - 1) * 24;
}

// Real buffer rows of the outline rows, in document order, for a
// per-row style list (index = buffer row).
inline std::vector<std::size_t> outlineVisibleRows(
    const std::vector<ParagraphStyle>& styles) {
    std::vector<std::size_t> rows;
    for (std::size_t i = 0; i < styles.size(); ++i) {
        if (isHeadingStyle(styles[i])) rows.push_back(i);
    }
    return rows;
}

// Map a visible-row index to its real buffer row, clamping a
// past-the-end index (e.g. from an approximate hit-test row) to the
// last heading. Returns lineCount (no such row) when there are no
// outline rows.
inline std::size_t outlineRealRowForVisible(
    const std::vector<std::size_t>& visibleRows, std::size_t visibleIndex,
    std::size_t lineCount) {
    if (visibleRows.empty()) return lineCount;
    if (visibleIndex >= visibleRows.size()) {
        return visibleRows.back();
    }
    return visibleRows[visibleIndex];
}

}  // namespace outline_view
