#include "table.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

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
