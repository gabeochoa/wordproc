#pragma once

// View > Normal (Word 95 parity).
// Normal IS the pageless view, renamed: continuous text across the
// full text-area width (not a page frame), with a dashed indicator
// line at each computed page boundary. The boundaries come from the
// same lines-per-page estimate Print Preview uses
// (scroll_math::previewLinesPerPage /
// columns_math::linesPerColumnForPage: page text height / line
// height), so the dashed line and "Page X of Y" agree on where a
// page ends. Like the preview estimate, headings / paragraph
// spacing shift the true breaks: the lines are indicators, not
// pagination. Pure functions so the renderer, the E2E break count,
// and the unit tests share one definition of "boundary".

#include <cstddef>

namespace normal_view {

// Whether buffer row `row` starts a new computed page: every
// `linesPerPage`-th row except row 0 (the document starts on page
// 1, it does not start with a break line). Degenerate
// `linesPerPage` (< 1) means no boundaries.
inline bool isPageBoundary(std::size_t row, int linesPerPage) {
    if (linesPerPage < 1 || row == 0) return false;
    return row % static_cast<std::size_t>(linesPerPage) == 0;
}

// Y for the dashed page-break line above the first row of a new
// page. rowTop is that row's top edge (its y after
// space-before); prevPaintedBottom is the previous row's painted
// bottom (its top + fontSize + 2: glyphs plus the spell squiggle
// below them, the lowest paint a row produces). The line sits at
// the top edge minus half the inter-line gap (rowTop -
// prevPaintedBottom), i.e. midway between the previous row's
// painted bottom and the new row's top edge, so it never overlaps
// either row's glyphs or squiggle. The old formula (rowTop - 2)
// assumed a 4px gap and landed exactly on the previous row's
// squiggle row, bisecting it visually. A degenerate gap (<= 0,
// e.g. tight line spacing) or no previous row drawn yet
// (prevPaintedBottom < 0: the boundary row is the first visible
// row) falls back to one pixel above the top edge.
inline int breakLineY(int rowTop, int prevPaintedBottom) {
    if (prevPaintedBottom < 0) return rowTop - 1;
    int gap = rowTop - prevPaintedBottom;
    if (gap <= 0) return rowTop - 1;
    return rowTop - gap / 2;
}

// How many computed boundaries a document with `lineCount` buffer
// rows has: boundaries at rows linesPerPage, 2 * linesPerPage, ...
// strictly below lineCount.
inline std::size_t pageBoundaryCount(std::size_t lineCount,
                                     int linesPerPage) {
    if (linesPerPage < 1 || lineCount == 0) return 0;
    return (lineCount - 1) / static_cast<std::size_t>(linesPerPage);
}

}  // namespace normal_view
