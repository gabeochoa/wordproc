#pragma once

#include <cstddef>
#include <functional>
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

// Returns line thickness for a border style (0 means don't draw)
inline float borderStyleThickness(BorderStyle style) {
    switch (style) {
        case BorderStyle::Thin:   return 1.0f;
        case BorderStyle::Medium: return 2.0f;
        case BorderStyle::Thick:  return 3.0f;
        case BorderStyle::None:
        case BorderStyle::Double:
        case BorderStyle::Dashed:
        case BorderStyle::Dotted:
        default:                  return 0.0f;
    }
}

// Gridline classification for the table renderer (renderTable):
// None and Thin draw the light-gray dotted gridline. Thin is
// the model's default for every new cell (CellBorders below), so
// it is the gridline, not an explicit border; None is a
// borderless cell, which Word still shows gridlines for in the
// editing view. Any other style (Medium, Thick, Double, Dashed,
// Dotted) was explicitly set — via setTableBorders or a file
// load — and keeps the black border path.
inline bool tableBorderIsGridline(BorderStyle style) {
    return style == BorderStyle::None || style == BorderStyle::Thin;
}

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
    
    // Sort ALL rows by one column (there is no header-row concept
    // in this model, so no row is pinned: the first row sorts with
    // the rest). Numeric-aware: cells that parse wholly as numbers
    // compare numerically (numbers sort before text); other cells
    // compare case-insensitively; equal keys keep their order
    // (stable). Row heights move with their rows; the caret cell
    // follows its row; any selection is cleared. A table with a
    // merged cell is not sorted (a row span cannot be permuted
    // safely) and the call returns false, leaving the table
    // untouched. One undo step is the command layer's job (it
    // snapshots the table before calling this).
    bool sortRows(std::size_t col, bool ascending);

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

// --- Cell editing (Batch 8) ---
// One active cell caret, stored on DocumentComponent (see
// DocumentComponent::cellEdit). Offsets are BYTE offsets into the
// cell's content string, always on UTF-8 codepoint boundaries;
// helpers below move/delete by whole codepoints, never bytes.
// The body caret/selection are untouched while a cell edit is
// active (the main text caret hides; see renderTextBuffer's
// caretVisible flag in the render system). Entering a cell also
// syncs Table::currentCell (the Sort / Formula target), and the
// command layer (DocumentComponent::tableUndo) snapshots the
// whole table before a typing run, so one undo step restores the
// pre-run table. A typing run ends (commit: undoPending=false)
// on navigation, Tab, click-away, or Esc — the snapshot entry
// itself stays live until cmd::undo consumes or drops it.
struct CellEditState {
    bool active = false;
    // Index into DocumentComponent::tables (not the table's line:
    // the line is tables[tableIndex].first).
    std::size_t tableIndex = 0;
    CellPosition cell{0, 0};
    std::size_t caretOffset = 0;
    std::size_t anchorOffset = 0;
    bool hasSelection = false;
    // A typing run is in progress, coalescing into the live
    // DocumentComponent::tableUndo entry (before = pre-run table).
    bool undoPending = false;

    std::size_t selectionStart() const {
        return std::min(caretOffset, anchorOffset);
    }
    std::size_t selectionEnd() const {
        return std::max(caretOffset, anchorOffset);
    }
    std::string selectedText(const std::string& content) const {
        if (!hasSelection) return "";
        std::size_t begin = selectionStart();
        std::size_t end = selectionEnd();
        if (begin >= content.size()) return "";
        return content.substr(begin, std::min(end, content.size()) - begin);
    }
    void clearSelection() {
        hasSelection = false;
        anchorOffset = caretOffset;
    }
    void clampTo(const std::string& content) {
        if (caretOffset > content.size()) caretOffset = content.size();
        if (anchorOffset > content.size()) anchorOffset = content.size();
    }
};

// One wrapped visual line of a cell's content. startOffset /
// endOffset are byte offsets into the cell content; text is the
// displayed slice (no trailing wrap space, no '\n'). Lines are
// contiguous except at word-wrap breaks (the wrapped space is
// skipped: next line starts after it) and '\n' breaks (next
// line starts after the newline).
struct WrappedCellLine {
    std::string text;
    std::size_t startOffset = 0;
    std::size_t endOffset = 0;
};

// measure(text, fontSize) -> pixel width. Render / command
// callers pass the real font measure; tests pass a stand-in.
using CellMeasureFn = std::function<float(const std::string&, int)>;

// Line height for a cell font size (same rule as body text).
inline int cellLineHeight(int fontSize) { return fontSize + 4; }

namespace cell_edit {

// Word-wrap content into visual lines at maxWidth pixels.
// Splits first on '\n' (each newline starts a new visual line,
// including a trailing empty line after a final '\n'), then
// greedily word-wraps each paragraph; a single word wider than
// maxWidth hard-breaks by codepoint. maxWidth <= 0 means no
// wrapping (one line per paragraph).
std::vector<WrappedCellLine> wrapCellText(
    const std::string& content, float maxWidth, int fontSize,
    const CellMeasureFn& measure);

// Caret offset for a click at (relX, relY) relative to the
// cell's text origin (padding-adjusted). Nearest-boundary rule,
// same as the body's hitTestColumn; relY clamps to the first /
// last visual line.
std::size_t caretOffsetAtPoint(
    const std::vector<WrappedCellLine>& lines, float relX, float relY,
    int fontSize, const CellMeasureFn& measure);

// Word boundaries around offset (word = run of word chars:
// ASCII alnum / '_' / any non-ASCII codepoint; everything else
// is a separator). Used for double-click word selection and
// Ctrl+Left / Ctrl+Right.
std::size_t wordStart(const std::string& content, std::size_t offset);
std::size_t wordEnd(const std::string& content, std::size_t offset);

// Clamp state to a valid cell (merged-covered cells resolve to
// their merge parent) and valid offsets. No-op on an empty table.
void clampState(Table& table, CellEditState& state);

// Text width available inside a cell (bounds width minus both
// paddings); never negative.
float textWidth(const Table& table, CellPosition pos);

// Height a row needs to show every cell's wrapped text at the
// render width: paddingTop + paddingBottom + lines * lineHeight
// per cell, maxed over the row's master cells, floored at the
// default row height (24). Clearing a long cell shrinks its row
// back. Merged masters spanning several rows count only toward
// their first row.
float wrappedRowHeight(const Table& table, std::size_t row,
                       const CellMeasureFn& measure);

// Recompute every row's stored height from wrapped text.
void recomputeRowHeights(Table& table, const CellMeasureFn& measure);

// Replace the selection (if any) and insert text at the caret.
// Returns false when the table is empty / the cell is invalid.
bool insertText(Table& table, CellEditState& state,
                const std::string& text);

// Delete the selection, or one codepoint before the caret.
bool backspace(Table& table, CellEditState& state);
// Delete the selection, or one codepoint after the caret.
bool del(Table& table, CellEditState& state);

// Move one codepoint left / right. At a cell edge, cross to
// the adjacent cell in row-major order (skipping merged-covered
// cells): Left at offset 0 lands at the END of the previous
// cell, Right at the end lands at the START of the next cell.
// At the table edges the caret stays put. Returns false when
// nothing moved.
bool moveLeft(Table& table, CellEditState& state);
bool moveRight(Table& table, CellEditState& state);

// Move one visual line up / down, preserving the caret's x
// within the line (nearest boundary). On the first / last
// visual line, cross to the cell above / below in the same
// column, landing on its last / first visual line at the same
// x. Single-line cells always cross. At the table's top /
// bottom row the caret stays put.
bool moveUp(Table& table, CellEditState& state, float maxWidth,
            int fontSize, const CellMeasureFn& measure);
bool moveDown(Table& table, CellEditState& state, float maxWidth,
              int fontSize, const CellMeasureFn& measure);

// Start / end of the caret's visual line.
bool moveToLineStart(Table& table, CellEditState& state, float maxWidth,
                     int fontSize, const CellMeasureFn& measure);
bool moveToLineEnd(Table& table, CellEditState& state, float maxWidth,
                   int fontSize, const CellMeasureFn& measure);
// Start / end of the whole cell text.
bool moveToCellStart(Table& table, CellEditState& state);
bool moveToCellEnd(Table& table, CellEditState& state);
// Word moves (Ctrl+Left / Ctrl+Right) inside the cell only:
// they never cross to another cell.
bool moveWordLeft(Table& table, CellEditState& state);
bool moveWordRight(Table& table, CellEditState& state);

// Tab / Shift+Tab. Tab in the LAST cell appends a new row
// (Word behavior) and lands in its first cell; Shift+Tab in
// the first cell stays put (no wrap). Both clear the selection.
bool tabForward(Table& table, CellEditState& state);
bool tabBackward(Table& table, CellEditState& state);

// Selection: double-click word, triple-click / Ctrl+A whole cell.
void selectWord(Table& table, CellEditState& state);
void selectAllInCell(Table& table, CellEditState& state);

}  // namespace cell_edit
