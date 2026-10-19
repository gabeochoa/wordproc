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

// How many computed boundaries a document with `lineCount` buffer
// rows has: boundaries at rows linesPerPage, 2 * linesPerPage, ...
// strictly below lineCount.
inline std::size_t pageBoundaryCount(std::size_t lineCount,
                                     int linesPerPage) {
    if (linesPerPage < 1 || lineCount == 0) return 0;
    return (lineCount - 1) / static_cast<std::size_t>(linesPerPage);
}

}  // namespace normal_view
