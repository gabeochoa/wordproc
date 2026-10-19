#include "table.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "table_formula.h"  // parseNumericCell (sort's numeric compare)

Table::Table(std::size_t rows, std::size_t cols) {
    initializeGrid(rows, cols);
}

void Table::initializeGrid(std::size_t rows, std::size_t cols) {
    rows_.clear();
    rows_.resize(rows);
    for (auto& row : rows_) {
        row.resize(cols);
    }
    
    colWidths_.assign(cols, defaultColWidth_);
    rowHeights_.assign(rows, defaultRowHeight_);
    
    currentCell_ = {0, 0};
    hasSelection_ = false;
}

bool Table::isValidPosition(CellPosition pos) const {
    return pos.row < rowCount() && pos.col < colCount();
}

TableCell& Table::cell(std::size_t row, std::size_t col) {
    if (row >= rowCount() || col >= colCount()) {
        throw std::out_of_range("Cell position out of range");
    }
    return rows_[row][col];
}

const TableCell& Table::cell(std::size_t row, std::size_t col) const {
    if (row >= rowCount() || col >= colCount()) {
        throw std::out_of_range("Cell position out of range");
    }
    return rows_[row][col];
}

void Table::setCellContent(std::size_t row, std::size_t col, const std::string& content) {
    cell(row, col).content = content;
}

std::string Table::getCellContent(std::size_t row, std::size_t col) const {
    return cell(row, col).content;
}

// Sort all rows by one column. See table.h for the contract:
// numeric-aware, stable, no header pin, merged tables refused.
bool Table::sortRows(std::size_t col, bool ascending) {
    if (isEmpty() || col >= colCount()) return false;
    for (const auto& row : rows_) {
        for (const auto& cell : row) {
            if (cell.isMerged || cell.span.rowSpan > 1 ||
                cell.span.colSpan > 1) {
                return false;
            }
        }
    }
    // Numeric parse: the whole trimmed content must be a number
    // (shared with the formula evaluator).
    auto parseNumeric = [](const std::string& content, double& out) {
        return table_formula::parseNumericCell(content, out);
    };
    auto lower = [](const std::string& value) {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });
        return result;
    };
    // -1 / 0 / +1 comparison on the sort column of two rows.
    auto compareRows = [&](std::size_t a, std::size_t b) {
        const std::string& aText = rows_[a][col].content;
        const std::string& bText = rows_[b][col].content;
        double aNum = 0.0;
        double bNum = 0.0;
        bool aIsNum = parseNumeric(aText, aNum);
        bool bIsNum = parseNumeric(bText, bNum);
        if (aIsNum && bIsNum) {
            if (aNum < bNum) return -1;
            if (aNum > bNum) return 1;
            return 0;
        }
        if (aIsNum != bIsNum) return aIsNum ? -1 : 1;  // numbers first
        std::string aLower = lower(aText);
        std::string bLower = lower(bText);
        if (aLower < bLower) return -1;
        if (aLower > bLower) return 1;
        return 0;
    };
    CellPosition caretBefore = currentCell_;
    std::vector<std::size_t> order(rowCount());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) {
                         int cmp = compareRows(a, b);
                         return ascending ? cmp < 0 : cmp > 0;
                     });
    std::vector<std::vector<TableCell>> sortedRows;
    std::vector<float> sortedHeights;
    sortedRows.reserve(rows_.size());
    sortedHeights.reserve(rowHeights_.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        sortedRows.push_back(std::move(rows_[order[i]]));
        sortedHeights.push_back(rowHeights_[order[i]]);
        if (order[i] == caretBefore.row) {
            currentCell_.row = i;
        }
    }
    rows_ = std::move(sortedRows);
    rowHeights_ = std::move(sortedHeights);
    clearSelection();
    return true;
}

// Row operations
void Table::insertRowAbove(std::size_t row) {
    if (row > rowCount()) {
        row = rowCount();
    }
    
    std::vector<TableCell> newRow(colCount());
    rows_.insert(rows_.begin() + static_cast<std::ptrdiff_t>(row), newRow);
    rowHeights_.insert(rowHeights_.begin() + static_cast<std::ptrdiff_t>(row), defaultRowHeight_);
    
    // Update merge info for cells that span across the new row
    updateMergeInfo();
}

void Table::insertRowBelow(std::size_t row) {
    insertRowAbove(row + 1);
}

void Table::deleteRow(std::size_t row) {
    if (row >= rowCount() || rowCount() <= 1) {
        return;  // Can't delete if invalid or would leave empty table
    }
    
    rows_.erase(rows_.begin() + static_cast<std::ptrdiff_t>(row));
    rowHeights_.erase(rowHeights_.begin() + static_cast<std::ptrdiff_t>(row));
    
    // Adjust current cell if needed
    if (currentCell_.row >= rowCount()) {
        currentCell_.row = rowCount() - 1;
    }
    
    updateMergeInfo();
}

float Table::rowHeight(std::size_t row) const {
    if (row >= rowHeights_.size()) {
        return defaultRowHeight_;
    }
    return rowHeights_[row];
}

void Table::setRowHeight(std::size_t row, float height) {
    if (row < rowHeights_.size()) {
        rowHeights_[row] = std::max(10.0f, height);
    }
}

// Column operations
void Table::insertColumnLeft(std::size_t col) {
    if (col > colCount()) {
        col = colCount();
    }
    
    for (auto& row : rows_) {
        row.insert(row.begin() + static_cast<std::ptrdiff_t>(col), TableCell{});
    }
    colWidths_.insert(colWidths_.begin() + static_cast<std::ptrdiff_t>(col), defaultColWidth_);
    
    updateMergeInfo();
}

void Table::insertColumnRight(std::size_t col) {
    insertColumnLeft(col + 1);
}

void Table::deleteColumn(std::size_t col) {
    if (col >= colCount() || colCount() <= 1) {
        return;  // Can't delete if invalid or would leave empty table
    }
    
    for (auto& row : rows_) {
        row.erase(row.begin() + static_cast<std::ptrdiff_t>(col));
    }
    colWidths_.erase(colWidths_.begin() + static_cast<std::ptrdiff_t>(col));
    
    // Adjust current cell if needed
    if (currentCell_.col >= colCount()) {
        currentCell_.col = colCount() - 1;
    }
    
    updateMergeInfo();
}

float Table::colWidth(std::size_t col) const {
    if (col >= colWidths_.size()) {
        return defaultColWidth_;
    }
    return colWidths_[col];
}

void Table::setColWidth(std::size_t col, float width) {
    if (col < colWidths_.size()) {
        colWidths_[col] = std::max(20.0f, width);
    }
}

// Merge/split cells
bool Table::canMerge(CellPosition topLeft, CellPosition bottomRight) const {
    if (!isValidPosition(topLeft) || !isValidPosition(bottomRight)) {
        return false;
    }
    if (topLeft.row > bottomRight.row || topLeft.col > bottomRight.col) {
        return false;
    }
    
    // Check that no cell in the range is already part of a merge
    for (std::size_t r = topLeft.row; r <= bottomRight.row; ++r) {
        for (std::size_t c = topLeft.col; c <= bottomRight.col; ++c) {
            const auto& cell = rows_[r][c];
            if (cell.isMerged || cell.span.rowSpan > 1 || cell.span.colSpan > 1) {
                return false;
            }
        }
    }
    
    return true;
}

bool Table::mergeCells(CellPosition topLeft, CellPosition bottomRight) {
    if (!canMerge(topLeft, bottomRight)) {
        return false;
    }
    
    std::size_t rowSpan = bottomRight.row - topLeft.row + 1;
    std::size_t colSpan = bottomRight.col - topLeft.col + 1;
    
    // Set the top-left cell to span the range
    auto& masterCell = rows_[topLeft.row][topLeft.col];
    masterCell.span.rowSpan = rowSpan;
    masterCell.span.colSpan = colSpan;
    
    // Concatenate content from all cells
    std::string combinedContent;
    for (std::size_t r = topLeft.row; r <= bottomRight.row; ++r) {
        for (std::size_t c = topLeft.col; c <= bottomRight.col; ++c) {
            if (r == topLeft.row && c == topLeft.col) continue;
            const auto& cell = rows_[r][c];
            if (!cell.content.empty()) {
                if (!combinedContent.empty()) combinedContent += " ";
                combinedContent += cell.content;
            }
        }
    }
    if (!combinedContent.empty()) {
        masterCell.content += " " + combinedContent;
    }
    
    // Mark other cells as merged
    for (std::size_t r = topLeft.row; r <= bottomRight.row; ++r) {
        for (std::size_t c = topLeft.col; c <= bottomRight.col; ++c) {
            if (r == topLeft.row && c == topLeft.col) continue;
            auto& cell = rows_[r][c];
            cell.isMerged = true;
            cell.mergeParent = topLeft;
            cell.content.clear();
        }
    }
    
    return true;
}

bool Table::splitCell(CellPosition pos) {
    if (!isValidPosition(pos)) {
        return false;
    }
    
    auto& masterCell = rows_[pos.row][pos.col];
    if (masterCell.span.rowSpan == 1 && masterCell.span.colSpan == 1) {
        return false;  // Cell is not merged
    }
    
    std::size_t endRow = pos.row + masterCell.span.rowSpan - 1;
    std::size_t endCol = pos.col + masterCell.span.colSpan - 1;
    
    // Reset all cells in the merged range
    for (std::size_t r = pos.row; r <= endRow; ++r) {
        for (std::size_t c = pos.col; c <= endCol; ++c) {
            auto& cell = rows_[r][c];
            cell.isMerged = false;
            cell.span.rowSpan = 1;
            cell.span.colSpan = 1;
            cell.mergeParent = {0, 0};
        }
    }
    
    return true;
}

bool Table::isCellMerged(CellPosition pos) const {
    if (!isValidPosition(pos)) {
        return false;
    }
    return rows_[pos.row][pos.col].isMerged;
}

CellPosition Table::getMergeParent(CellPosition pos) const {
    if (!isValidPosition(pos)) {
        return {0, 0};
    }
    const auto& cell = rows_[pos.row][pos.col];
    if (cell.isMerged) {
        return cell.mergeParent;
    }
    return pos;
}

// Selection support
void Table::setSelection(CellPosition start, CellPosition end) {
    if (!isValidPosition(start) || !isValidPosition(end)) {
        return;
    }
    hasSelection_ = true;
    selectionStart_ = start;
    selectionEnd_ = end;
}

void Table::clearSelection() {
    hasSelection_ = false;
    selectionStart_ = {0, 0};
    selectionEnd_ = {0, 0};
}

void Table::selectAll() {
    if (isEmpty()) return;
    hasSelection_ = true;
    selectionStart_ = {0, 0};
    selectionEnd_ = {rowCount() - 1, colCount() - 1};
}

void Table::selectRow(std::size_t row) {
    if (row >= rowCount()) return;
    hasSelection_ = true;
    selectionStart_ = {row, 0};
    selectionEnd_ = {row, colCount() - 1};
}

void Table::selectColumn(std::size_t col) {
    if (col >= colCount()) return;
    hasSelection_ = true;
    selectionStart_ = {0, col};
    selectionEnd_ = {rowCount() - 1, col};
}

// Navigation
void Table::setCurrentCell(CellPosition pos) {
    if (isValidPosition(pos)) {
        // If the cell is merged, navigate to the parent cell
        if (rows_[pos.row][pos.col].isMerged) {
            currentCell_ = rows_[pos.row][pos.col].mergeParent;
        } else {
            currentCell_ = pos;
        }
    }
}

void Table::moveToNextCell() {
    if (isEmpty()) return;
    
    std::size_t col = currentCell_.col + 1;
    std::size_t row = currentCell_.row;
    
    // Handle merged cells by skipping their span
    const auto& cell = rows_[currentCell_.row][currentCell_.col];
    col = currentCell_.col + cell.span.colSpan;
    
    if (col >= colCount()) {
        col = 0;
        row++;
        if (row >= rowCount()) {
            // At end of table, optionally add new row or wrap to start
            row = 0;
        }
    }
    
    setCurrentCell({row, col});
}

void Table::moveToPrevCell() {
    if (isEmpty()) return;
    
    std::size_t col = currentCell_.col;
    std::size_t row = currentCell_.row;
    
    if (col == 0) {
        if (row == 0) {
            row = rowCount() - 1;
        } else {
            row--;
        }
        col = colCount() - 1;
    } else {
        col--;
    }
    
    setCurrentCell({row, col});
}

void Table::moveUp() {
    if (isEmpty() || currentCell_.row == 0) return;
    setCurrentCell({currentCell_.row - 1, currentCell_.col});
}

void Table::moveDown() {
    if (isEmpty() || currentCell_.row >= rowCount() - 1) return;
    setCurrentCell({currentCell_.row + 1, currentCell_.col});
}

void Table::moveLeft() {
    if (isEmpty() || currentCell_.col == 0) return;
    setCurrentCell({currentCell_.row, currentCell_.col - 1});
}

void Table::moveRight() {
    if (isEmpty() || currentCell_.col >= colCount() - 1) return;
    setCurrentCell({currentCell_.row, currentCell_.col + 1});
}

// Total dimensions
float Table::totalWidth() const {
    float total = 0.0f;
    for (float w : colWidths_) {
        total += w;
    }
    return total;
}

float Table::totalHeight() const {
    float total = 0.0f;
    for (float h : rowHeights_) {
        total += h;
    }
    return total;
}

void Table::setTableBorders(const CellBorders& borders) {
    tableBorders_ = borders;
    // Apply to all cells
    for (auto& row : rows_) {
        for (auto& cell : row) {
            cell.borders = borders;
        }
    }
}

CellPosition Table::cellAtPoint(float x, float y) const {
    if (isEmpty() || x < 0 || y < 0) {
        return {0, 0};
    }
    
    // Find column
    float accX = 0.0f;
    std::size_t col = 0;
    for (std::size_t c = 0; c < colCount(); ++c) {
        if (x < accX + colWidths_[c]) {
            col = c;
            break;
        }
        accX += colWidths_[c];
        if (c == colCount() - 1) {
            col = c;
        }
    }
    
    // Find row
    float accY = 0.0f;
    std::size_t row = 0;
    for (std::size_t r = 0; r < rowCount(); ++r) {
        if (y < accY + rowHeights_[r]) {
            row = r;
            break;
        }
        accY += rowHeights_[r];
        if (r == rowCount() - 1) {
            row = r;
        }
    }
    
    return {row, col};
}

Table::CellBounds Table::cellBounds(CellPosition pos) const {
    if (!isValidPosition(pos)) {
        return {0, 0, 0, 0};
    }
    
    // Calculate x position
    float x = 0.0f;
    for (std::size_t c = 0; c < pos.col; ++c) {
        x += colWidths_[c];
    }
    
    // Calculate y position
    float y = 0.0f;
    for (std::size_t r = 0; r < pos.row; ++r) {
        y += rowHeights_[r];
    }
    
    // Calculate width and height (accounting for merge span)
    const auto& cell = rows_[pos.row][pos.col];
    float width = 0.0f;
    for (std::size_t c = 0; c < cell.span.colSpan && pos.col + c < colCount(); ++c) {
        width += colWidths_[pos.col + c];
    }
    
    float height = 0.0f;
    for (std::size_t r = 0; r < cell.span.rowSpan && pos.row + r < rowCount(); ++r) {
        height += rowHeights_[pos.row + r];
    }
    
    return {x, y, width, height};
}

void Table::updateMergeInfo() {
    // Reset all merge info
    for (auto& row : rows_) {
        for (auto& cell : row) {
            if (!cell.isMerged) {
                // Reset span for non-merged cells
                if (cell.span.rowSpan > 1 || cell.span.colSpan > 1) {
                    // This is a master cell, update its merged children
                    // (they may have been shifted by row/col insertion)
                }
            }
        }
    }
    // For now, splitting all merged cells on structure change
    // A more sophisticated implementation would adjust spans
}

// --- Cell editing (Batch 8) ---
namespace cell_edit {

namespace {

bool isContinuationByte(unsigned char c) { return (c & 0xC0) == 0x80; }

std::size_t nextBoundary(const std::string& s, std::size_t i) {
    if (i >= s.size()) return s.size();
    ++i;
    while (i < s.size() && isContinuationByte(
                               static_cast<unsigned char>(s[i]))) {
        ++i;
    }
    return i;
}

std::size_t prevBoundary(const std::string& s, std::size_t i) {
    if (i == 0) return 0;
    --i;
    while (i > 0 && isContinuationByte(
                        static_cast<unsigned char>(s[i]))) {
        --i;
    }
    return i;
}

// Word char: ASCII alnum / '_' / any non-ASCII codepoint start
// (a continuation byte never starts a codepoint, so it is
// never examined here — callers pass codepoint starts).
bool isWordByte(unsigned char c) {
    return std::isalnum(c) != 0 || c == '_' || c >= 0x80;
}

bool validCell(const Table& table, CellPosition pos) {
    return !table.isEmpty() && pos.row < table.rowCount() &&
           pos.col < table.colCount();
}

// First editable (non-merged-covered) cell strictly after pos
// in row-major order; none when pos is the last editable cell.
bool nextEditable(const Table& table, CellPosition pos,
                  CellPosition& out) {
    for (std::size_t r = pos.row; r < table.rowCount(); ++r) {
        std::size_t c0 = (r == pos.row) ? pos.col + 1 : 0;
        for (std::size_t c = c0; c < table.colCount(); ++c) {
            if (!table.isCellMerged({r, c})) {
                out = {r, c};
                return true;
            }
        }
    }
    return false;
}

bool prevEditable(const Table& table, CellPosition pos,
                  CellPosition& out) {
    for (std::size_t r = pos.row + 1; r-- > 0;) {
        std::size_t cEnd =
            (r == pos.row) ? pos.col : table.colCount();
        for (std::size_t c = cEnd; c-- > 0;) {
            if (!table.isCellMerged({r, c})) {
                out = {r, c};
                return true;
            }
        }
        if (r == 0) break;
    }
    return false;
}

// Visual line holding caretOffset. A caret exactly at a line's
// end belongs to the NEXT line when that line starts at the
// same offset (a hard break with no gap); otherwise it belongs
// to the line it ends, matching the render caret placement.
std::size_t lineIndexForOffset(
    const std::vector<WrappedCellLine>& lines, std::size_t caret) {
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (caret < lines[i].endOffset) return i;
        if (caret == lines[i].endOffset) {
            if (i + 1 >= lines.size() ||
                lines[i + 1].startOffset > caret) {
                return i;
            }
        }
    }
    return lines.empty() ? 0 : lines.size() - 1;
}

// Nearest-boundary offset inside one wrapped line at pixel x
// (midpoint rule, like the body's hitTestColumn).
std::size_t offsetAtX(const WrappedCellLine& line, float x,
                      int fontSize, const CellMeasureFn& measure) {
    if (x <= 0.0f) return line.startOffset;
    float prevW = 0.0f;
    std::size_t prev = line.startOffset;
    std::size_t cur = line.startOffset;
    while (cur < line.endOffset) {
        // Next codepoint boundary within the line's text slice.
        std::size_t slicePos = cur - line.startOffset;
        std::size_t sliceNext = slicePos + 1;
        while (sliceNext < line.text.size() &&
               isContinuationByte(static_cast<unsigned char>(
                   line.text[sliceNext]))) {
            ++sliceNext;
        }
        std::string prefix = line.text.substr(0, sliceNext);
        float w = measure(prefix, fontSize);
        if (x < (prevW + w) / 2.0f) return prev;
        prevW = w;
        prev = line.startOffset + sliceNext;
        cur = prev;
    }
    return line.endOffset;
}

}  // namespace

std::vector<WrappedCellLine> wrapCellText(
    const std::string& content, float maxWidth, int fontSize,
    const CellMeasureFn& measure) {
    std::vector<WrappedCellLine> lines;
    std::size_t segStart = 0;
    while (true) {
        std::size_t nl = content.find('\n', segStart);
        std::size_t segEnd =
            (nl == std::string::npos) ? content.size() : nl;
        if (segStart == segEnd) {
            lines.push_back({"", segStart, segStart});
        } else if (maxWidth <= 0.0f || !measure) {
            lines.push_back({content.substr(segStart, segEnd - segStart),
                             segStart, segEnd});
        } else {
            std::size_t p = segStart;
            while (p < segEnd) {
                if (measure(content.substr(p, segEnd - p), fontSize) <=
                    maxWidth) {
                    lines.push_back({content.substr(p, segEnd - p), p,
                                     segEnd});
                    break;
                }
                // Furthest codepoint prefix that still fits.
                std::size_t lastFit = p;
                std::size_t i = p;
                while (i < segEnd) {
                    std::size_t nxt = nextBoundary(content, i);
                    if (measure(content.substr(p, nxt - p), fontSize) <=
                        maxWidth) {
                        lastFit = nxt;
                        i = nxt;
                    } else {
                        break;
                    }
                }
                if (lastFit == p) {
                    // One codepoint alone is wider than the cell:
                    // emit it rather than looping forever.
                    std::size_t nxt = nextBoundary(content, p);
                    lines.push_back({content.substr(p, nxt - p), p, nxt});
                    p = nxt;
                    continue;
                }
                // Last space in (p, lastFit): wrap before it,
                // skipping the space on the next line.
                std::size_t spaceIdx = std::string::npos;
                for (std::size_t k = lastFit; k > p + 1; --k) {
                    if (content[k - 1] == ' ') {
                        spaceIdx = k - 1;
                        break;
                    }
                }
                if (spaceIdx != std::string::npos) {
                    lines.push_back(
                        {content.substr(p, spaceIdx - p), p, spaceIdx});
                    p = spaceIdx + 1;
                } else {
                    lines.push_back({content.substr(p, lastFit - p), p,
                                     lastFit});
                    p = lastFit;
                }
            }
        }
        if (nl == std::string::npos) break;
        segStart = nl + 1;
        // A trailing '\n' still yields its empty final line.
        if (segStart == content.size()) {
            lines.push_back({"", segStart, segStart});
            break;
        }
    }
    if (lines.empty()) lines.push_back({"", 0, 0});
    return lines;
}

std::size_t caretOffsetAtPoint(
    const std::vector<WrappedCellLine>& lines, float relX, float relY,
    int fontSize, const CellMeasureFn& measure) {
    if (lines.empty()) return 0;
    int lineHeight = cellLineHeight(fontSize);
    int idx = lineHeight > 0
                  ? static_cast<int>(std::floor(relY / lineHeight))
                  : 0;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(lines.size())) {
        idx = static_cast<int>(lines.size()) - 1;
    }
    return offsetAtX(lines[static_cast<std::size_t>(idx)], relX, fontSize,
                     measure);
}

std::size_t wordStart(const std::string& content, std::size_t offset) {
    std::size_t i = std::min(offset, content.size());
    while (i > 0) {
        std::size_t prev = prevBoundary(content, i);
        if (!isWordByte(static_cast<unsigned char>(content[prev]))) break;
        i = prev;
    }
    return i;
}

std::size_t wordEnd(const std::string& content, std::size_t offset) {
    std::size_t i = std::min(offset, content.size());
    while (i < content.size() &&
           isWordByte(static_cast<unsigned char>(content[i]))) {
        i = nextBoundary(content, i);
    }
    return i;
}

void clampState(Table& table, CellEditState& state) {
    if (table.isEmpty()) return;
    if (state.cell.row >= table.rowCount()) {
        state.cell.row = table.rowCount() - 1;
    }
    if (state.cell.col >= table.colCount()) {
        state.cell.col = table.colCount() - 1;
    }
    state.cell = table.getMergeParent(state.cell);
    const std::string& content =
        table.cell(state.cell).content;
    // Snap offsets back onto codepoint boundaries, then clamp.
    auto snap = [&](std::size_t off) {
        off = std::min(off, content.size());
        while (off > 0 &&
               isContinuationByte(
                   static_cast<unsigned char>(content[off]))) {
            --off;
        }
        return off;
    };
    state.caretOffset = snap(state.caretOffset);
    state.anchorOffset = snap(state.anchorOffset);
    state.clampTo(content);
    table.setCurrentCell(state.cell);
}

float textWidth(const Table& table, CellPosition pos) {
    if (!validCell(table, pos)) return 0.0f;
    const TableCell& cell = table.cell(pos);
    Table::CellBounds bounds = table.cellBounds(pos);
    float w = bounds.width - static_cast<float>(cell.paddingLeft +
                                                cell.paddingRight);
    return w > 0.0f ? w : 0.0f;
}

float wrappedRowHeight(const Table& table, std::size_t row,
                       const CellMeasureFn& measure) {
    if (row >= table.rowCount()) return 24.0f;
    float best = 24.0f;
    for (std::size_t c = 0; c < table.colCount(); ++c) {
        const TableCell& cell = table.cell(row, c);
        if (cell.isMerged) continue;
        float w = textWidth(table, {row, c});
        std::size_t lines =
            wrapCellText(cell.content, w, cell.textStyle.fontSize,
                         measure)
                .size();
        float h = static_cast<float>(cell.paddingTop +
                                     cell.paddingBottom) +
                  static_cast<float>(lines) *
                      static_cast<float>(
                          cellLineHeight(cell.textStyle.fontSize));
        best = std::max(best, h);
    }
    return best;
}

void recomputeRowHeights(Table& table, const CellMeasureFn& measure) {
    for (std::size_t r = 0; r < table.rowCount(); ++r) {
        table.setRowHeight(r, wrappedRowHeight(table, r, measure));
    }
}

bool insertText(Table& table, CellEditState& state,
                const std::string& text) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    TableCell& cell = table.cell(state.cell);
    std::string& content = cell.content;
    if (state.hasSelection) {
        std::size_t begin = state.selectionStart();
        std::size_t end = state.selectionEnd();
        content.erase(begin, end - begin);
        state.caretOffset = begin;
    }
    if (!text.empty()) {
        content.insert(state.caretOffset, text);
        state.caretOffset += text.size();
    }
    state.clearSelection();
    return true;
}

bool backspace(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    if (state.hasSelection) return insertText(table, state, "");
    if (state.caretOffset == 0) return false;
    TableCell& cell = table.cell(state.cell);
    std::size_t prev = prevBoundary(cell.content, state.caretOffset);
    cell.content.erase(prev, state.caretOffset - prev);
    state.caretOffset = prev;
    state.clearSelection();
    return true;
}

bool del(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    if (state.hasSelection) return insertText(table, state, "");
    TableCell& cell = table.cell(state.cell);
    if (state.caretOffset >= cell.content.size()) return false;
    std::size_t next = nextBoundary(cell.content, state.caretOffset);
    cell.content.erase(state.caretOffset, next - state.caretOffset);
    state.clearSelection();
    return true;
}

bool moveLeft(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    if (state.caretOffset > 0) {
        state.caretOffset = prevBoundary(
            table.cell(state.cell).content, state.caretOffset);
        return true;
    }
    CellPosition prev;
    if (!prevEditable(table, state.cell, prev)) return false;
    state.cell = prev;
    state.caretOffset = table.cell(prev).content.size();
    table.setCurrentCell(state.cell);
    return true;
}

bool moveRight(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    const std::string& content = table.cell(state.cell).content;
    if (state.caretOffset < content.size()) {
        state.caretOffset = nextBoundary(content, state.caretOffset);
        return true;
    }
    CellPosition next;
    if (!nextEditable(table, state.cell, next)) return false;
    state.cell = next;
    state.caretOffset = 0;
    table.setCurrentCell(state.cell);
    return true;
}

bool moveUp(Table& table, CellEditState& state, float maxWidth,
            int fontSize, const CellMeasureFn& measure) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    const TableCell& cell = table.cell(state.cell);
    auto lines = wrapCellText(cell.content, maxWidth, fontSize, measure);
    std::size_t lineIdx = lineIndexForOffset(lines, state.caretOffset);
    float x = measure(cell.content.substr(
                          lines[lineIdx].startOffset,
                          state.caretOffset - lines[lineIdx].startOffset),
                      fontSize);
    if (lineIdx > 0) {
        state.caretOffset = offsetAtX(lines[lineIdx - 1], x, fontSize,
                                      measure);
        return true;
    }
    if (state.cell.row == 0) return false;
    CellPosition above =
        table.getMergeParent({state.cell.row - 1, state.cell.col});
    const TableCell& aboveCell = table.cell(above);
    auto aboveLines = wrapCellText(aboveCell.content,
                                   textWidth(table, above),
                                   aboveCell.textStyle.fontSize, measure);
    state.cell = above;
    state.caretOffset =
        aboveLines.empty()
            ? 0
            : offsetAtX(aboveLines.back(), x,
                        aboveCell.textStyle.fontSize, measure);
    table.setCurrentCell(state.cell);
    return true;
}

bool moveDown(Table& table, CellEditState& state, float maxWidth,
              int fontSize, const CellMeasureFn& measure) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    const TableCell& cell = table.cell(state.cell);
    auto lines = wrapCellText(cell.content, maxWidth, fontSize, measure);
    std::size_t lineIdx = lineIndexForOffset(lines, state.caretOffset);
    float x = measure(cell.content.substr(
                          lines[lineIdx].startOffset,
                          state.caretOffset - lines[lineIdx].startOffset),
                      fontSize);
    if (lineIdx + 1 < lines.size()) {
        state.caretOffset = offsetAtX(lines[lineIdx + 1], x, fontSize,
                                      measure);
        return true;
    }
    if (state.cell.row + 1 >= table.rowCount()) return false;
    CellPosition below =
        table.getMergeParent({state.cell.row + 1, state.cell.col});
    const TableCell& belowCell = table.cell(below);
    auto belowLines = wrapCellText(belowCell.content,
                                   textWidth(table, below),
                                   belowCell.textStyle.fontSize, measure);
    state.cell = below;
    state.caretOffset =
        belowLines.empty()
            ? 0
            : offsetAtX(belowLines.front(), x,
                        belowCell.textStyle.fontSize, measure);
    table.setCurrentCell(state.cell);
    return true;
}

bool moveToLineStart(Table& table, CellEditState& state, float maxWidth,
                     int fontSize, const CellMeasureFn& measure) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    auto lines = wrapCellText(table.cell(state.cell).content, maxWidth,
                              fontSize, measure);
    std::size_t idx = lineIndexForOffset(lines, state.caretOffset);
    state.caretOffset = lines[idx].startOffset;
    return true;
}

bool moveToLineEnd(Table& table, CellEditState& state, float maxWidth,
                   int fontSize, const CellMeasureFn& measure) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    auto lines = wrapCellText(table.cell(state.cell).content, maxWidth,
                              fontSize, measure);
    std::size_t idx = lineIndexForOffset(lines, state.caretOffset);
    state.caretOffset = lines[idx].endOffset;
    return true;
}

bool moveToCellStart(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    state.caretOffset = 0;
    return true;
}

bool moveToCellEnd(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    state.caretOffset = table.cell(state.cell).content.size();
    return true;
}

bool moveWordLeft(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    const std::string& content = table.cell(state.cell).content;
    std::size_t i = state.caretOffset;
    std::size_t before = i;
    if (i > 0 && isWordByte(static_cast<unsigned char>(
                     content[prevBoundary(content, i)]))) {
        while (i > 0 && isWordByte(static_cast<unsigned char>(
                          content[prevBoundary(content, i)]))) {
            i = prevBoundary(content, i);
        }
    } else {
        while (i > 0 && !isWordByte(static_cast<unsigned char>(
                           content[prevBoundary(content, i)]))) {
            i = prevBoundary(content, i);
        }
        while (i > 0 && isWordByte(static_cast<unsigned char>(
                          content[prevBoundary(content, i)]))) {
            i = prevBoundary(content, i);
        }
    }
    state.caretOffset = i;
    return i != before;
}

bool moveWordRight(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    const std::string& content = table.cell(state.cell).content;
    std::size_t i = state.caretOffset;
    std::size_t before = i;
    if (i < content.size() &&
        isWordByte(static_cast<unsigned char>(content[i]))) {
        while (i < content.size() &&
               isWordByte(static_cast<unsigned char>(content[i]))) {
            i = nextBoundary(content, i);
        }
    } else {
        while (i < content.size() &&
               !isWordByte(static_cast<unsigned char>(content[i]))) {
            i = nextBoundary(content, i);
        }
        while (i < content.size() &&
               isWordByte(static_cast<unsigned char>(content[i]))) {
            i = nextBoundary(content, i);
        }
    }
    state.caretOffset = i;
    return i != before;
}

bool tabForward(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    CellPosition next;
    if (nextEditable(table, state.cell, next)) {
        state.cell = next;
        state.caretOffset = 0;
        state.clearSelection();
        table.setCurrentCell(state.cell);
        return true;
    }
    // Last cell: append a row (Word behavior).
    std::size_t lastRow = table.rowCount() - 1;
    table.insertRowBelow(lastRow);
    state.cell = {table.rowCount() - 1, 0};
    state.caretOffset = 0;
    state.clearSelection();
    table.setCurrentCell(state.cell);
    return true;
}

bool tabBackward(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return false;
    clampState(table, state);
    CellPosition prev;
    if (!prevEditable(table, state.cell, prev)) return false;
    state.cell = prev;
    state.caretOffset = 0;
    state.clearSelection();
    table.setCurrentCell(state.cell);
    return true;
}

void selectWord(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return;
    clampState(table, state);
    const std::string& content = table.cell(state.cell).content;
    std::size_t start = wordStart(content, state.caretOffset);
    std::size_t end = wordEnd(content, state.caretOffset);
    if (start >= end) {
        state.clearSelection();
        return;
    }
    state.anchorOffset = start;
    state.caretOffset = end;
    state.hasSelection = true;
}

void selectAllInCell(Table& table, CellEditState& state) {
    if (!validCell(table, state.cell)) return;
    clampState(table, state);
    state.anchorOffset = 0;
    state.caretOffset = table.cell(state.cell).content.size();
    state.hasSelection = state.caretOffset > 0;
}

}  // namespace cell_edit

// Factory functions
Table createTable(std::size_t rows, std::size_t cols) {
    return Table(rows, cols);
}

Table createTableWithHeader(std::size_t rows, std::size_t cols, 
                            const std::vector<std::string>& headers) {
    Table table(rows, cols);
    
    // Set header row content
    for (std::size_t c = 0; c < std::min(cols, headers.size()); ++c) {
        table.setCellContent(0, c, headers[c]);
        // Make header bold
        auto& cell = table.cell(0, c);
        cell.textStyle.bold = true;
        cell.backgroundColor = TextColor{220, 220, 220, 255};  // Light gray
    }
    
    return table;
}
