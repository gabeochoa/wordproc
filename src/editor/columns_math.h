#pragma once

// Format > Columns... layout math (Word 95 parity).
// Pure functions shared by the paged renderer and the click
// hit-test, so both agree on where a line lives. A page holds
// linesPerColumn * columnCount lines: the first linesPerColumn
// lines fill column 1 top-to-bottom, the next linesPerColumn fill
// column 2, and so on; the next page starts after that.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>

namespace columns_math {

// Where one buffer line (global line index) lands.
struct ColumnAssignment {
    std::size_t page = 0;
    int column = 0;
    std::size_t rowInColumn = 0;
};

inline ColumnAssignment columnAssignment(std::size_t lineIndex,
                                         std::size_t linesPerColumn,
                                         int columnCount) {
    if (linesPerColumn == 0) linesPerColumn = 1;
    if (columnCount < 1) columnCount = 1;
    std::size_t pageSize =
        linesPerColumn * static_cast<std::size_t>(columnCount);
    ColumnAssignment result;
    result.page = lineIndex / pageSize;
    std::size_t within = lineIndex % pageSize;
    result.column =
        static_cast<int>(within / linesPerColumn);
    result.rowInColumn = within % linesPerColumn;
    return result;
}

// Width of one column: the text width split equally after the
// (count - 1) gutters are removed. Spacing and text width are in
// the same units (points in settings, display px in the renderer).
inline float columnWidth(float textWidth, int columnCount, float spacing) {
    if (columnCount < 1) columnCount = 1;
    if (textWidth < 0.0f) textWidth = 0.0f;
    if (spacing < 0.0f) spacing = 0.0f;
    return (textWidth - static_cast<float>(columnCount - 1) * spacing) /
           static_cast<float>(columnCount);
}

// X offset of a column's left edge relative to the text area.
inline float columnXOffset(int column, float colWidth, float spacing) {
    if (column < 0) column = 0;
    return static_cast<float>(column) * (colWidth + spacing);
}

// Lines per column on one page: the page's text height (page
// height minus top/bottom margins, in points, converted to display
// px) divided by the line height, at least one. The renderer and
// the hit-test both use this, so a click on a column lands on the
// line the renderer drew there.
inline std::size_t linesPerColumnForPage(float pageHeightPoints,
                                         float marginPoints,
                                         float displayScale,
                                         int lineHeightPx) {
    float textHeightPx =
        (pageHeightPoints - 2.0f * marginPoints) * displayScale;
    if (lineHeightPx <= 0) return 1;
    std::size_t lines = static_cast<std::size_t>(
        textHeightPx / static_cast<float>(lineHeightPx));
    return std::max<std::size_t>(lines, 1);
}

// Format a spacing in points as inches for the dialog field
// (36pt -> "0.5"), trimming trailing zeros.
inline std::string pointsToInchesString(float points) {
    float inches = points / 72.0f;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", inches);
    std::string text = buf;
    while (!text.empty() && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
    if (text.empty()) text = "0";
    return text;
}

}  // namespace columns_math
