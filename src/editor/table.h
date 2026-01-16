#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "document_settings.h"

// Forward declarations
class TextBuffer;

// Cell position in a table
struct CellPosition {
    std::size_t row = 0;
    std::size_t col = 0;
    
    bool operator==(const CellPosition& other) const {
        return row == other.row && col == other.col;
    }
    bool operator!=(const CellPosition& other) const { return !(*this == other); }
};

// Cell merge span information
struct CellSpan {
    std::size_t rowSpan = 1;  // Number of rows this cell spans (1 = no merge)
    std::size_t colSpan = 1;  // Number of columns this cell spans (1 = no merge)
};

// Cell alignment options
enum class CellAlignment {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

// Cell border style
enum class BorderStyle {
    None,
    Thin,
    Medium,
    Thick,
    Double,
    Dashed,
    Dotted
};

// Cell borders (each side can have different style)
struct CellBorders {
    BorderStyle top = BorderStyle::Thin;
    BorderStyle bottom = BorderStyle::Thin;
    BorderStyle left = BorderStyle::Thin;
    BorderStyle right = BorderStyle::Thin;
};

// Individual table cell
struct TableCell {
    std::string content;           // Text content of the cell
    CellSpan span;                 // Merge span information
    CellAlignment alignment = CellAlignment::TopLeft;
    TextColor backgroundColor = TextColors::White;
    TextStyle textStyle;           // Text formatting in cell
    CellBorders borders;           // Cell border styles
    bool isMerged = false;         // True if this cell is covered by another cell's span
    CellPosition mergeParent;      // If merged, points to the spanning cell
    
    // Padding in pixels
    int paddingTop = 4;
    int paddingBottom = 4;
    int paddingLeft = 6;
    int paddingRight = 6;
};

// Table structure with row and column management
class Table {
public:
    Table() = default;
    Table(std::size_t rows, std::size_t cols);
    
    // Dimensions
    std::size_t rowCount() const { return rows_.size(); }
    std::size_t colCount() const { return colWidths_.size(); }
    bool isEmpty() const { return rows_.empty() || colWidths_.empty(); }
    
    // Cell access
    TableCell& cell(std::size_t row, std::size_t col);
    const TableCell& cell(std::size_t row, std::size_t col) const;
    TableCell& cell(CellPosition pos) { return cell(pos.row, pos.col); }
    const TableCell& cell(CellPosition pos) const { return cell(pos.row, pos.col); }
    
    // Cell content
    void setCellContent(std::size_t row, std::size_t col, const std::string& content);
    std::string getCellContent(std::size_t row, std::size_t col) const;
    
    // Row operations
    void insertRowAbove(std::size_t row);
    void insertRowBelow(std::size_t row);
    void deleteRow(std::size_t row);
    float rowHeight(std::size_t row) const;
    void setRowHeight(std::size_t row, float height);
    
    // Column operations
    void insertColumnLeft(std::size_t col);
    void insertColumnRight(std::size_t col);
    void deleteColumn(std::size_t col);
    float colWidth(std::size_t col) const;
    void setColWidth(std::size_t col, float width);
    
    // Merge/split cells
    bool mergeCells(CellPosition topLeft, CellPosition bottomRight);
    bool splitCell(CellPosition pos);
    bool canMerge(CellPosition topLeft, CellPosition bottomRight) const;
    bool isCellMerged(CellPosition pos) const;
    CellPosition getMergeParent(CellPosition pos) const;
    
    // Selection support
    bool hasSelection() const { return hasSelection_; }
    CellPosition selectionStart() const { return selectionStart_; }
    CellPosition selectionEnd() const { return selectionEnd_; }
    void setSelection(CellPosition start, CellPosition end);
    void clearSelection();
    void selectAll();
    void selectRow(std::size_t row);
    void selectColumn(std::size_t col);
    
    // Navigation
    CellPosition currentCell() const { return currentCell_; }
    void setCurrentCell(CellPosition pos);
    void moveToNextCell();      // Tab
    void moveToPrevCell();      // Shift+Tab
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    
    // Total dimensions for rendering
    float totalWidth() const;
    float totalHeight() const;
    
    // Border style for entire table
    CellBorders tableBorders() const { return tableBorders_; }
    void setTableBorders(const CellBorders& borders);
    
    // Get cell position from pixel coordinates (relative to table origin)
    CellPosition cellAtPoint(float x, float y) const;
    
    // Get pixel bounds of a cell (relative to table origin)
    struct CellBounds {
        float x, y, width, height;
    };
    CellBounds cellBounds(CellPosition pos) const;

private:
    void initializeGrid(std::size_t rows, std::size_t cols);
    void updateMergeInfo();
    bool isValidPosition(CellPosition pos) const;
    
    std::vector<std::vector<TableCell>> rows_;  // rows_[row][col]
    std::vector<float> colWidths_;               // Width of each column
    std::vector<float> rowHeights_;              // Height of each row
    
    CellPosition currentCell_;                   // Current editing cell
    bool hasSelection_ = false;
    CellPosition selectionStart_;
    CellPosition selectionEnd_;
    
    CellBorders tableBorders_;                   // Default borders for table
    float defaultColWidth_ = 100.0f;
    float defaultRowHeight_ = 24.0f;
};

// Factory function to create common table layouts
Table createTable(std::size_t rows, std::size_t cols);
Table createTableWithHeader(std::size_t rows, std::size_t cols, const std::vector<std::string>& headers);
