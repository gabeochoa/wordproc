#include "../src/editor/table.h"
#include "catch2/catch.hpp"

TEST_CASE("Table initialization", "[table]") {
    SECTION("default constructor creates empty table") {
        Table table;
        REQUIRE(table.isEmpty());
        REQUIRE(table.rowCount() == 0);
        REQUIRE(table.colCount() == 0);
    }

    SECTION("parameterized constructor creates table with dimensions") {
        Table table(3, 4);
        REQUIRE_FALSE(table.isEmpty());
        REQUIRE(table.rowCount() == 3);
        REQUIRE(table.colCount() == 4);
    }

    SECTION("createTable factory function works") {
        Table table = createTable(2, 3);
        REQUIRE(table.rowCount() == 2);
        REQUIRE(table.colCount() == 3);
    }
}

TEST_CASE("Table cell access", "[table]") {
    Table table(3, 3);

    SECTION("cell access returns valid cell") {
        TableCell& c = table.cell(0, 0);
        REQUIRE(c.content.empty());
        REQUIRE(c.span.rowSpan == 1);
        REQUIRE(c.span.colSpan == 1);
    }

    SECTION("out of range throws exception") {
        REQUIRE_THROWS_AS(table.cell(10, 0), std::out_of_range);
        REQUIRE_THROWS_AS(table.cell(0, 10), std::out_of_range);
    }

    SECTION("setCellContent and getCellContent work") {
        table.setCellContent(1, 1, "test content");
        REQUIRE(table.getCellContent(1, 1) == "test content");
    }

    SECTION("cell position access works") {
        CellPosition pos{1, 2};
        table.setCellContent(pos.row, pos.col, "hello");
        REQUIRE(table.cell(pos).content == "hello");
    }
}

TEST_CASE("Table row operations", "[table]") {
    Table table(3, 3);
    table.setCellContent(0, 0, "r0c0");
    table.setCellContent(1, 0, "r1c0");
    table.setCellContent(2, 0, "r2c0");

    SECTION("insertRowAbove adds row before specified index") {
        table.insertRowAbove(1);
        REQUIRE(table.rowCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "r0c0");
        REQUIRE(table.getCellContent(1, 0) == "");  // New row
        REQUIRE(table.getCellContent(2, 0) == "r1c0");
        REQUIRE(table.getCellContent(3, 0) == "r2c0");
    }

    SECTION("insertRowBelow adds row after specified index") {
        table.insertRowBelow(0);
        REQUIRE(table.rowCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "r0c0");
        REQUIRE(table.getCellContent(1, 0) == "");  // New row
        REQUIRE(table.getCellContent(2, 0) == "r1c0");
    }

    SECTION("deleteRow removes row") {
        table.deleteRow(1);
        REQUIRE(table.rowCount() == 2);
        REQUIRE(table.getCellContent(0, 0) == "r0c0");
        REQUIRE(table.getCellContent(1, 0) == "r2c0");
    }

    SECTION("cannot delete last row") {
        Table small(1, 3);
        small.deleteRow(0);
        REQUIRE(small.rowCount() == 1);  // Still has 1 row
    }

    SECTION("rowHeight can be get and set") {
        float defaultHeight = table.rowHeight(0);
        REQUIRE(defaultHeight > 0);
        
        table.setRowHeight(0, 50.0f);
        REQUIRE(table.rowHeight(0) == 50.0f);
    }

    SECTION("setRowHeight enforces minimum") {
        table.setRowHeight(0, 5.0f);  // Below minimum
        REQUIRE(table.rowHeight(0) >= 10.0f);
    }
}

TEST_CASE("Table column operations", "[table]") {
    Table table(3, 3);
    table.setCellContent(0, 0, "c0");
    table.setCellContent(0, 1, "c1");
    table.setCellContent(0, 2, "c2");

    SECTION("insertColumnLeft adds column before specified index") {
        table.insertColumnLeft(1);
        REQUIRE(table.colCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "c0");
        REQUIRE(table.getCellContent(0, 1) == "");  // New column
        REQUIRE(table.getCellContent(0, 2) == "c1");
        REQUIRE(table.getCellContent(0, 3) == "c2");
    }

    SECTION("insertColumnRight adds column after specified index") {
        table.insertColumnRight(0);
        REQUIRE(table.colCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "c0");
        REQUIRE(table.getCellContent(0, 1) == "");  // New column
        REQUIRE(table.getCellContent(0, 2) == "c1");
    }

    SECTION("deleteColumn removes column") {
        table.deleteColumn(1);
        REQUIRE(table.colCount() == 2);
        REQUIRE(table.getCellContent(0, 0) == "c0");
        REQUIRE(table.getCellContent(0, 1) == "c2");
    }

    SECTION("cannot delete last column") {
        Table small(3, 1);
        small.deleteColumn(0);
        REQUIRE(small.colCount() == 1);  // Still has 1 column
    }

    SECTION("colWidth can be get and set") {
        float defaultWidth = table.colWidth(0);
        REQUIRE(defaultWidth > 0);
        
        table.setColWidth(0, 200.0f);
        REQUIRE(table.colWidth(0) == 200.0f);
    }

    SECTION("setColWidth enforces minimum") {
        table.setColWidth(0, 10.0f);  // Below minimum
        REQUIRE(table.colWidth(0) >= 20.0f);
    }
}

TEST_CASE("Table merge and split", "[table]") {
    Table table(4, 4);
    table.setCellContent(0, 0, "A");
    table.setCellContent(0, 1, "B");
    table.setCellContent(1, 0, "C");
    table.setCellContent(1, 1, "D");

    SECTION("canMerge returns true for valid range") {
        REQUIRE(table.canMerge({0, 0}, {1, 1}));
    }

    SECTION("canMerge returns false for invalid positions") {
        REQUIRE_FALSE(table.canMerge({0, 0}, {10, 10}));
    }

    SECTION("canMerge returns false for reversed positions") {
        REQUIRE_FALSE(table.canMerge({1, 1}, {0, 0}));
    }

    SECTION("mergeCells creates merged cell") {
        bool result = table.mergeCells({0, 0}, {1, 1});
        REQUIRE(result);
        
        // Master cell has span
        const TableCell& master = table.cell(0, 0);
        REQUIRE(master.span.rowSpan == 2);
        REQUIRE(master.span.colSpan == 2);
        
        // Other cells are marked as merged
        REQUIRE(table.isCellMerged({0, 1}));
        REQUIRE(table.isCellMerged({1, 0}));
        REQUIRE(table.isCellMerged({1, 1}));
        REQUIRE_FALSE(table.isCellMerged({0, 0}));  // Master is not "merged"
    }

    SECTION("mergeCells combines content") {
        bool result = table.mergeCells({0, 0}, {0, 1});
        REQUIRE(result);
        // Content from B should be appended to A
        REQUIRE(table.getCellContent(0, 0).find("A") != std::string::npos);
        REQUIRE(table.getCellContent(0, 0).find("B") != std::string::npos);
    }

    SECTION("cannot merge already merged cells") {
        table.mergeCells({0, 0}, {1, 1});
        REQUIRE_FALSE(table.canMerge({0, 0}, {2, 2}));
    }

    SECTION("splitCell unmerges cells") {
        table.mergeCells({0, 0}, {1, 1});
        bool result = table.splitCell({0, 0});
        REQUIRE(result);
        
        // All cells should be unmerged
        REQUIRE_FALSE(table.isCellMerged({0, 0}));
        REQUIRE_FALSE(table.isCellMerged({0, 1}));
        REQUIRE_FALSE(table.isCellMerged({1, 0}));
        REQUIRE_FALSE(table.isCellMerged({1, 1}));
        
        // Span should be reset
        REQUIRE(table.cell(0, 0).span.rowSpan == 1);
        REQUIRE(table.cell(0, 0).span.colSpan == 1);
    }

    SECTION("splitCell returns false for non-merged cell") {
        REQUIRE_FALSE(table.splitCell({0, 0}));
    }

    SECTION("getMergeParent returns parent for merged cell") {
        table.mergeCells({0, 0}, {1, 1});
        CellPosition parent = table.getMergeParent({1, 1});
        REQUIRE(parent.row == 0);
        REQUIRE(parent.col == 0);
    }

    SECTION("getMergeParent returns self for non-merged cell") {
        CellPosition parent = table.getMergeParent({2, 2});
        REQUIRE(parent.row == 2);
        REQUIRE(parent.col == 2);
    }
}

TEST_CASE("Table selection", "[table]") {
    Table table(3, 3);

    SECTION("initially no selection") {
        REQUIRE_FALSE(table.hasSelection());
    }

    SECTION("setSelection creates selection") {
        table.setSelection({0, 0}, {1, 1});
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart().row == 0);
        REQUIRE(table.selectionStart().col == 0);
        REQUIRE(table.selectionEnd().row == 1);
        REQUIRE(table.selectionEnd().col == 1);
    }

    SECTION("clearSelection removes selection") {
        table.setSelection({0, 0}, {1, 1});
        table.clearSelection();
        REQUIRE_FALSE(table.hasSelection());
    }

    SECTION("selectAll selects entire table") {
        table.selectAll();
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart().row == 0);
        REQUIRE(table.selectionStart().col == 0);
        REQUIRE(table.selectionEnd().row == 2);
        REQUIRE(table.selectionEnd().col == 2);
    }

    SECTION("selectRow selects entire row") {
        table.selectRow(1);
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart().row == 1);
        REQUIRE(table.selectionStart().col == 0);
        REQUIRE(table.selectionEnd().row == 1);
        REQUIRE(table.selectionEnd().col == 2);
    }

    SECTION("selectColumn selects entire column") {
        table.selectColumn(1);
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart().row == 0);
        REQUIRE(table.selectionStart().col == 1);
        REQUIRE(table.selectionEnd().row == 2);
        REQUIRE(table.selectionEnd().col == 1);
    }
}

TEST_CASE("Table navigation", "[table]") {
    Table table(3, 3);

    SECTION("currentCell starts at 0,0") {
        CellPosition pos = table.currentCell();
        REQUIRE(pos.row == 0);
        REQUIRE(pos.col == 0);
    }

    SECTION("setCurrentCell changes current cell") {
        table.setCurrentCell({1, 2});
        CellPosition pos = table.currentCell();
        REQUIRE(pos.row == 1);
        REQUIRE(pos.col == 2);
    }

    SECTION("moveToNextCell moves right then down") {
        table.moveToNextCell();
        REQUIRE(table.currentCell().col == 1);
        
        table.setCurrentCell({0, 2});  // Last column
        table.moveToNextCell();
        REQUIRE(table.currentCell().row == 1);
        REQUIRE(table.currentCell().col == 0);
    }

    SECTION("moveToPrevCell moves left then up") {
        table.setCurrentCell({1, 0});  // First column
        table.moveToPrevCell();
        REQUIRE(table.currentCell().row == 0);
        REQUIRE(table.currentCell().col == 2);
    }

    SECTION("moveUp moves to row above") {
        table.setCurrentCell({1, 1});
        table.moveUp();
        REQUIRE(table.currentCell().row == 0);
        REQUIRE(table.currentCell().col == 1);
    }

    SECTION("moveUp does nothing at top row") {
        table.moveUp();
        REQUIRE(table.currentCell().row == 0);
    }

    SECTION("moveDown moves to row below") {
        table.moveDown();
        REQUIRE(table.currentCell().row == 1);
    }

    SECTION("moveDown does nothing at bottom row") {
        table.setCurrentCell({2, 0});
        table.moveDown();
        REQUIRE(table.currentCell().row == 2);
    }

    SECTION("moveLeft moves to column left") {
        table.setCurrentCell({0, 1});
        table.moveLeft();
        REQUIRE(table.currentCell().col == 0);
    }

    SECTION("moveRight moves to column right") {
        table.moveRight();
        REQUIRE(table.currentCell().col == 1);
    }

    SECTION("navigation to merged cell jumps to parent") {
        table.mergeCells({0, 0}, {1, 1});
        table.setCurrentCell({1, 1});  // Should jump to 0,0
        REQUIRE(table.currentCell().row == 0);
        REQUIRE(table.currentCell().col == 0);
    }
}

TEST_CASE("Table dimensions", "[table]") {
    Table table(3, 4);

    SECTION("totalWidth is sum of column widths") {
        float total = table.totalWidth();
        float expected = 0;
        for (std::size_t c = 0; c < table.colCount(); ++c) {
            expected += table.colWidth(c);
        }
        REQUIRE(total == expected);
    }

    SECTION("totalHeight is sum of row heights") {
        float total = table.totalHeight();
        float expected = 0;
        for (std::size_t r = 0; r < table.rowCount(); ++r) {
            expected += table.rowHeight(r);
        }
        REQUIRE(total == expected);
    }
}

TEST_CASE("Table cell bounds", "[table]") {
    Table table(2, 2);
    table.setColWidth(0, 100.0f);
    table.setColWidth(1, 150.0f);
    table.setRowHeight(0, 30.0f);
    table.setRowHeight(1, 40.0f);

    SECTION("cellBounds returns correct position and size") {
        auto bounds = table.cellBounds({0, 0});
        REQUIRE(bounds.x == 0.0f);
        REQUIRE(bounds.y == 0.0f);
        REQUIRE(bounds.width == 100.0f);
        REQUIRE(bounds.height == 30.0f);
    }

    SECTION("cellBounds accounts for position offset") {
        auto bounds = table.cellBounds({1, 1});
        REQUIRE(bounds.x == 100.0f);
        REQUIRE(bounds.y == 30.0f);
        REQUIRE(bounds.width == 150.0f);
        REQUIRE(bounds.height == 40.0f);
    }

    SECTION("cellBounds accounts for merge span") {
        table.mergeCells({0, 0}, {1, 1});
        auto bounds = table.cellBounds({0, 0});
        REQUIRE(bounds.x == 0.0f);
        REQUIRE(bounds.y == 0.0f);
        REQUIRE(bounds.width == 250.0f);  // 100 + 150
        REQUIRE(bounds.height == 70.0f);  // 30 + 40
    }
}

TEST_CASE("Table cellAtPoint", "[table]") {
    Table table(2, 2);
    table.setColWidth(0, 100.0f);
    table.setColWidth(1, 150.0f);
    table.setRowHeight(0, 30.0f);
    table.setRowHeight(1, 40.0f);

    SECTION("cellAtPoint returns correct cell for coordinates") {
        CellPosition pos = table.cellAtPoint(50.0f, 15.0f);
        REQUIRE(pos.row == 0);
        REQUIRE(pos.col == 0);

        pos = table.cellAtPoint(120.0f, 50.0f);
        REQUIRE(pos.row == 1);
        REQUIRE(pos.col == 1);
    }

    SECTION("cellAtPoint handles edge positions") {
        CellPosition pos = table.cellAtPoint(0.0f, 0.0f);
        REQUIRE(pos.row == 0);
        REQUIRE(pos.col == 0);

        pos = table.cellAtPoint(99.0f, 29.0f);
        REQUIRE(pos.row == 0);
        REQUIRE(pos.col == 0);

        pos = table.cellAtPoint(100.0f, 30.0f);
        REQUIRE(pos.row == 1);
        REQUIRE(pos.col == 1);
    }
}

TEST_CASE("Table borders", "[table]") {
    Table table(2, 2);

    SECTION("setTableBorders applies to all cells") {
        CellBorders borders;
        borders.top = BorderStyle::Thick;
        borders.bottom = BorderStyle::Double;
        borders.left = BorderStyle::Dashed;
        borders.right = BorderStyle::Dotted;
        
        table.setTableBorders(borders);
        
        REQUIRE(table.tableBorders().top == BorderStyle::Thick);
        REQUIRE(table.cell(0, 0).borders.top == BorderStyle::Thick);
        REQUIRE(table.cell(1, 1).borders.right == BorderStyle::Dotted);
    }
}

TEST_CASE("Table createTableWithHeader", "[table]") {
    std::vector<std::string> headers = {"Name", "Age", "City"};
    Table table = createTableWithHeader(4, 3, headers);

    SECTION("creates table with correct dimensions") {
        REQUIRE(table.rowCount() == 4);
        REQUIRE(table.colCount() == 3);
    }

    SECTION("header row has content") {
        REQUIRE(table.getCellContent(0, 0) == "Name");
        REQUIRE(table.getCellContent(0, 1) == "Age");
        REQUIRE(table.getCellContent(0, 2) == "City");
    }

    SECTION("header row cells are bold") {
        REQUIRE(table.cell(0, 0).textStyle.bold);
        REQUIRE(table.cell(0, 1).textStyle.bold);
        REQUIRE(table.cell(0, 2).textStyle.bold);
    }

    SECTION("header row has gray background") {
        REQUIRE(table.cell(0, 0).backgroundColor.r == 220);
        REQUIRE(table.cell(0, 0).backgroundColor.g == 220);
        REQUIRE(table.cell(0, 0).backgroundColor.b == 220);
    }

    SECTION("data rows are empty") {
        REQUIRE(table.getCellContent(1, 0).empty());
        REQUIRE(table.getCellContent(2, 1).empty());
    }
}

TEST_CASE("Table cell properties", "[table]") {
    Table table(2, 2);

    SECTION("cell has default alignment") {
        REQUIRE(table.cell(0, 0).alignment == CellAlignment::TopLeft);
    }

    SECTION("cell alignment can be changed") {
        table.cell(0, 0).alignment = CellAlignment::MiddleCenter;
        REQUIRE(table.cell(0, 0).alignment == CellAlignment::MiddleCenter);
    }

    SECTION("cell has default padding") {
        const TableCell& c = table.cell(0, 0);
        REQUIRE(c.paddingTop == 4);
        REQUIRE(c.paddingBottom == 4);
        REQUIRE(c.paddingLeft == 6);
        REQUIRE(c.paddingRight == 6);
    }

    SECTION("cell text style can be modified") {
        table.cell(0, 0).textStyle.bold = true;
        table.cell(0, 0).textStyle.italic = true;
        REQUIRE(table.cell(0, 0).textStyle.bold);
        REQUIRE(table.cell(0, 0).textStyle.italic);
    }

    SECTION("cell background color can be changed") {
        table.cell(0, 0).backgroundColor = TextColors::Yellow;
        REQUIRE(table.cell(0, 0).backgroundColor.r == TextColors::Yellow.r);
    }
}

TEST_CASE("Table sortRows - numeric-aware, all rows sort, no header pin",
          "[table]") {
    Table table(3, 2);
    table.setCellContent(0, 0, "30");
    table.setCellContent(0, 1, "c");
    table.setCellContent(1, 0, "5");
    table.setCellContent(1, 1, "a");
    table.setCellContent(2, 0, "20");
    table.setCellContent(2, 1, "b");

    SECTION("Ascending numeric sort moves whole rows, first row included") {
        REQUIRE(table.sortRows(0, true));
        REQUIRE(table.getCellContent(0, 0) == "5");
        REQUIRE(table.getCellContent(0, 1) == "a");
        REQUIRE(table.getCellContent(1, 0) == "20");
        REQUIRE(table.getCellContent(1, 1) == "b");
        REQUIRE(table.getCellContent(2, 0) == "30");
        REQUIRE(table.getCellContent(2, 1) == "c");
    }
    SECTION("Descending numeric sort") {
        REQUIRE(table.sortRows(0, false));
        REQUIRE(table.getCellContent(0, 0) == "30");
        REQUIRE(table.getCellContent(1, 0) == "20");
        REQUIRE(table.getCellContent(2, 0) == "5");
    }
    SECTION("Sort key is the given column") {
        REQUIRE(table.sortRows(1, true));
        REQUIRE(table.getCellContent(0, 1) == "a");
        REQUIRE(table.getCellContent(0, 0) == "5");
        REQUIRE(table.getCellContent(2, 1) == "c");
    }
    SECTION("Out-of-range column refuses") {
        REQUIRE_FALSE(table.sortRows(5, true));
        REQUIRE(table.getCellContent(0, 0) == "30");
    }
    SECTION("Caret cell follows its row") {
        table.setCurrentCell({0, 0});  // the "30" row
        REQUIRE(table.sortRows(0, true));
        REQUIRE(table.currentCell().row == 2);
    }
}

TEST_CASE("Table sortRows - mixed numeric/text, case-insensitive, stable",
          "[table]") {
    Table table(5, 1);
    table.setCellContent(0, 0, "banana");
    table.setCellContent(1, 0, "10");
    table.setCellContent(2, 0, "Apple");
    table.setCellContent(3, 0, "2");
    table.setCellContent(4, 0, "apple");

    SECTION("Numbers sort before text, numerically; text case-insensitively") {
        REQUIRE(table.sortRows(0, true));
        REQUIRE(table.getCellContent(0, 0) == "2");
        REQUIRE(table.getCellContent(1, 0) == "10");
        REQUIRE(table.getCellContent(2, 0) == "Apple");
        REQUIRE(table.getCellContent(3, 0) == "apple");
        REQUIRE(table.getCellContent(4, 0) == "banana");
    }
    SECTION("Equal text keys keep their original order (stable)") {
        Table stable(3, 2);
        stable.setCellContent(0, 0, "same");
        stable.setCellContent(0, 1, "first");
        stable.setCellContent(1, 0, "same");
        stable.setCellContent(1, 1, "second");
        stable.setCellContent(2, 0, "aaa");
        stable.setCellContent(2, 1, "third");
        REQUIRE(stable.sortRows(0, true));
        REQUIRE(stable.getCellContent(0, 1) == "third");
        REQUIRE(stable.getCellContent(1, 1) == "first");
        REQUIRE(stable.getCellContent(2, 1) == "second");
    }
    SECTION("A '12x' cell is text, not the number 12") {
        Table mixed(3, 1);
        mixed.setCellContent(0, 0, "12x");
        mixed.setCellContent(1, 0, "3");
        mixed.setCellContent(2, 0, "20");
        REQUIRE(mixed.sortRows(0, true));
        REQUIRE(mixed.getCellContent(0, 0) == "3");
        REQUIRE(mixed.getCellContent(1, 0) == "20");
        REQUIRE(mixed.getCellContent(2, 0) == "12x");
    }
}

TEST_CASE("Table sortRows - merged cells refuse, table untouched",
          "[table]") {
    Table table(2, 2);
    table.setCellContent(0, 0, "b");
    table.setCellContent(1, 0, "a");
    REQUIRE(table.mergeCells({0, 0}, {0, 1}));
    REQUIRE_FALSE(table.sortRows(0, true));
    REQUIRE(table.getCellContent(0, 0) == "b");
    REQUIRE(table.getCellContent(1, 0) == "a");
}

namespace {
// Fixed-width stand-in measure: 10px per byte (ASCII tests).
float fixedMeasure(const std::string& text, int /*fontSize*/) {
    return static_cast<float>(text.size()) * 10.0f;
}
}  // namespace

TEST_CASE("Cell editing: wrapCellText word-wraps, hard-breaks, newlines",
          "[table][cell-edit]") {
    SECTION("Word wrap breaks before the overflowing word") {
        auto lines = cell_edit::wrapCellText("aaa bbb ccc", 50.0f, 16,
                                             fixedMeasure);
        REQUIRE(lines.size() == 3);
        REQUIRE(lines[0].text == "aaa");
        REQUIRE(lines[1].text == "bbb");
        REQUIRE(lines[2].text == "ccc");
        REQUIRE(lines[0].startOffset == 0);
        REQUIRE(lines[0].endOffset == 3);
        REQUIRE(lines[1].startOffset == 4);
        REQUIRE(lines[2].startOffset == 8);
    }
    SECTION("A single word wider than the cell hard-breaks") {
        auto lines = cell_edit::wrapCellText("123456", 25.0f, 16,
                                             fixedMeasure);
        REQUIRE(lines.size() == 3);
        REQUIRE(lines[0].text == "12");
        REQUIRE(lines[1].text == "34");
        REQUIRE(lines[2].text == "56");
    }
    SECTION("Newlines split lines, including a trailing empty line") {
        auto lines = cell_edit::wrapCellText("ab\ncd", 1000.0f, 16,
                                             fixedMeasure);
        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0].text == "ab");
        REQUIRE(lines[1].text == "cd");
        REQUIRE(lines[1].startOffset == 3);
        auto trailing = cell_edit::wrapCellText("ab\n", 1000.0f, 16,
                                                fixedMeasure);
        REQUIRE(trailing.size() == 2);
        REQUIRE(trailing[1].text.empty());
        REQUIRE(trailing[1].startOffset == 3);
    }
    SECTION("Fitting text stays one line; empty content is one empty line") {
        auto lines = cell_edit::wrapCellText("hi", 1000.0f, 16,
                                             fixedMeasure);
        REQUIRE(lines.size() == 1);
        auto empty = cell_edit::wrapCellText("", 1000.0f, 16,
                                             fixedMeasure);
        REQUIRE(empty.size() == 1);
        REQUIRE(empty[0].text.empty());
    }
}

TEST_CASE("Cell editing: caretOffsetAtPoint snaps to nearest boundary",
          "[table][cell-edit]") {
    auto lines = cell_edit::wrapCellText("abcdef", 1000.0f, 16,
                                         fixedMeasure);
    SECTION("Click near the start lands at 0, past the end clamps") {
        REQUIRE(cell_edit::caretOffsetAtPoint(lines, 2.0f, 5.0f, 16,
                                              fixedMeasure) == 0);
        REQUIRE(cell_edit::caretOffsetAtPoint(lines, 1000.0f, 5.0f, 16,
                                              fixedMeasure) == 6);
    }
    SECTION("Midpoint rule matches the body hit test") {
        // 10px/char: x=24 is inside char 2's second half (15..25
        // midpoint 20) -> boundary 2; x=26 -> boundary 3.
        REQUIRE(cell_edit::caretOffsetAtPoint(lines, 24.0f, 5.0f, 16,
                                              fixedMeasure) == 2);
        REQUIRE(cell_edit::caretOffsetAtPoint(lines, 26.0f, 5.0f, 16,
                                              fixedMeasure) == 3);
    }
}

TEST_CASE("Cell editing: insert / backspace / delete / selection replace",
          "[table][cell-edit]") {
    Table table(2, 2);
    CellEditState state;
    state.active = true;
    state.cell = {0, 0};

    SECTION("Insert appends at the caret and moves it") {
        REQUIRE(cell_edit::insertText(table, state, "Hi"));
        REQUIRE(table.getCellContent(0, 0) == "Hi");
        REQUIRE(state.caretOffset == 2);
        REQUIRE(cell_edit::insertText(table, state, "!"));
        REQUIRE(table.getCellContent(0, 0) == "Hi!");
        REQUIRE(state.caretOffset == 3);
    }
    SECTION("Backspace deletes one codepoint, Delete forward") {
        cell_edit::insertText(table, state, "ab");
        REQUIRE(cell_edit::backspace(table, state));
        REQUIRE(table.getCellContent(0, 0) == "a");
        state.caretOffset = 0;
        REQUIRE(cell_edit::del(table, state));
        REQUIRE(table.getCellContent(0, 0).empty());
    }
    SECTION("Backspace at 0 and Delete at end are no-ops") {
        cell_edit::insertText(table, state, "x");
        state.caretOffset = 0;
        REQUIRE_FALSE(cell_edit::backspace(table, state));
        state.caretOffset = 1;
        REQUIRE_FALSE(cell_edit::del(table, state));
    }
    SECTION("Typing replaces a selection") {
        cell_edit::insertText(table, state, "hello");
        state.anchorOffset = 1;
        state.caretOffset = 4;
        state.hasSelection = true;
        REQUIRE(cell_edit::insertText(table, state, "X"));
        REQUIRE(table.getCellContent(0, 0) == "hXo");
        REQUIRE(state.caretOffset == 2);
        REQUIRE_FALSE(state.hasSelection);
    }
    SECTION("Backspace with a selection deletes the selection") {
        cell_edit::insertText(table, state, "hello");
        state.anchorOffset = 0;
        state.caretOffset = 2;
        state.hasSelection = true;
        REQUIRE(cell_edit::backspace(table, state));
        REQUIRE(table.getCellContent(0, 0) == "llo");
        REQUIRE(state.caretOffset == 0);
    }
    SECTION("Multi-byte backspace deletes a whole codepoint") {
        cell_edit::insertText(table, state, "a\xC3\xA9");  // a + é
        REQUIRE(state.caretOffset == 3);
        REQUIRE(cell_edit::backspace(table, state));
        REQUIRE(table.getCellContent(0, 0) == "a");
        REQUIRE(state.caretOffset == 1);
    }
    SECTION("Enter inserts a newline the wrapper renders") {
        cell_edit::insertText(table, state, "a\nb");
        REQUIRE(table.getCellContent(0, 0) == "a\nb");
        auto lines = cell_edit::wrapCellText(
            table.getCellContent(0, 0), 1000.0f, 16, fixedMeasure);
        REQUIRE(lines.size() == 2);
    }
}

TEST_CASE("Cell editing: Left/Right cross cells at edges",
          "[table][cell-edit]") {
    Table table(2, 2);
    table.setCellContent(0, 0, "ab");
    table.setCellContent(0, 1, "cd");
    table.setCellContent(1, 0, "ef");
    CellEditState state;
    state.active = true;
    state.cell = {0, 1};
    state.caretOffset = 0;

    SECTION("Left at a cell start lands at the end of the previous cell") {
        REQUIRE(cell_edit::moveLeft(table, state));
        REQUIRE(state.cell == CellPosition{0, 0});
        REQUIRE(state.caretOffset == 2);
    }
    SECTION("Right at a cell end lands at the start of the next cell") {
        state.cell = {0, 0};
        state.caretOffset = 2;
        REQUIRE(cell_edit::moveRight(table, state));
        REQUIRE(state.cell == CellPosition{0, 1});
        REQUIRE(state.caretOffset == 0);
    }
    SECTION("Left at the first cell and Right at the last stay put") {
        state.cell = {0, 0};
        state.caretOffset = 0;
        REQUIRE_FALSE(cell_edit::moveLeft(table, state));
        state.cell = {1, 1};
        state.caretOffset = 0;
        REQUIRE_FALSE(cell_edit::moveRight(table, state));
    }
    SECTION("Left/Right move within the cell otherwise") {
        state.cell = {0, 0};
        state.caretOffset = 2;
        REQUIRE(cell_edit::moveLeft(table, state));
        REQUIRE(state.caretOffset == 1);
        REQUIRE(state.cell == CellPosition{0, 0});
    }
}

TEST_CASE("Cell editing: Up/Down cross rows, and within wrapped cells",
          "[table][cell-edit]") {
    Table table(2, 2);
    table.setCellContent(0, 0, "aaa bbb");
    table.setCellContent(1, 0, "ccc");
    CellEditState state;
    state.active = true;

    SECTION("Single-line cells: Down crosses, Up returns") {
        state.cell = {0, 1};
        state.caretOffset = 0;
        REQUIRE(cell_edit::moveDown(table, state, 1000.0f, 16,
                                    fixedMeasure));
        REQUIRE(state.cell == CellPosition{1, 1});
        REQUIRE(cell_edit::moveUp(table, state, 1000.0f, 16,
                                  fixedMeasure));
        REQUIRE(state.cell == CellPosition{0, 1});
    }
    SECTION("Up at the top row and Down at the bottom stay put") {
        state.cell = {0, 0};
        state.caretOffset = 0;
        REQUIRE_FALSE(cell_edit::moveUp(table, state, 1000.0f, 16,
                                        fixedMeasure));
        state.cell = {1, 0};
        REQUIRE_FALSE(cell_edit::moveDown(table, state, 1000.0f, 16,
                                          fixedMeasure));
    }
    SECTION("Down moves one visual line within a wrapped cell first") {
        state.cell = {0, 0};
        state.caretOffset = 0;
        // Width 50 wraps "aaa bbb" to "aaa" / "bbb".
        REQUIRE(cell_edit::moveDown(table, state, 50.0f, 16,
                                    fixedMeasure));
        REQUIRE(state.cell == CellPosition{0, 0});
        REQUIRE(state.caretOffset == 4);  // start of "bbb"
        // Down again crosses to the row below (last visual line).
        REQUIRE(cell_edit::moveDown(table, state, 50.0f, 16,
                                    fixedMeasure));
        REQUIRE(state.cell == CellPosition{1, 0});
    }
}

TEST_CASE("Cell editing: Tab / Shift+Tab, Tab appends a row",
          "[table][cell-edit]") {
    Table table(2, 2);
    CellEditState state;
    state.active = true;
    state.cell = {0, 0};

    SECTION("Tab moves to the next cell, Shift+Tab back") {
        REQUIRE(cell_edit::tabForward(table, state));
        REQUIRE(state.cell == CellPosition{0, 1});
        REQUIRE(cell_edit::tabBackward(table, state));
        REQUIRE(state.cell == CellPosition{0, 0});
    }
    SECTION("Shift+Tab in the first cell stays put (no wrap)") {
        REQUIRE_FALSE(cell_edit::tabBackward(table, state));
        REQUIRE(state.cell == CellPosition{0, 0});
    }
    SECTION("Tab in the last cell appends a row and lands in (2, 0)") {
        state.cell = {1, 1};
        REQUIRE(cell_edit::tabForward(table, state));
        REQUIRE(table.rowCount() == 3);
        REQUIRE(state.cell == CellPosition{2, 0});
        REQUIRE(state.caretOffset == 0);
    }
    SECTION("Tab wraps to the next row's first cell") {
        state.cell = {0, 1};
        REQUIRE(cell_edit::tabForward(table, state));
        REQUIRE(state.cell == CellPosition{1, 0});
    }
}

TEST_CASE("Cell editing: word moves, word select, select-all",
          "[table][cell-edit]") {
    Table table(1, 1);
    table.setCellContent(0, 0, "hello world foo");
    CellEditState state;
    state.active = true;
    state.cell = {0, 0};

    SECTION("Ctrl+Right jumps to the next word end") {
        state.caretOffset = 0;
        REQUIRE(cell_edit::moveWordRight(table, state));
        REQUIRE(state.caretOffset == 5);
        REQUIRE(cell_edit::moveWordRight(table, state));
        REQUIRE(state.caretOffset == 11);
    }
    SECTION("Ctrl+Left jumps to the previous word start") {
        state.caretOffset = 15;
        REQUIRE(cell_edit::moveWordLeft(table, state));
        REQUIRE(state.caretOffset == 12);
    }
    SECTION("Double-click word selects it") {
        state.caretOffset = 7;  // inside "world"
        cell_edit::selectWord(table, state);
        REQUIRE(state.hasSelection);
        REQUIRE(state.selectedText(table.getCellContent(0, 0)) ==
                "world");
    }
    SECTION("Select-all selects the whole cell") {
        cell_edit::selectAllInCell(table, state);
        REQUIRE(state.hasSelection);
        REQUIRE(state.selectedText(table.getCellContent(0, 0)) ==
                "hello world foo");
    }
}

TEST_CASE("Cell editing: row height grows from wrapped lines",
          "[table][cell-edit]") {
    Table table(1, 1);
    table.setColWidth(0, 60.0f);  // 48px of text at padding 6+6
    CellEditState state;
    state.active = true;
    state.cell = {0, 0};
    cell_edit::insertText(table, state, "aaa bbb ccc");
    // 48px at 10px/char wraps "aaa bbb ccc" to 3 lines:
    // 8 + 3*20 = 68 > 24 default.
    cell_edit::recomputeRowHeights(table, fixedMeasure);
    REQUIRE(table.rowHeight(0) == 68.0f);
    // Clearing the cell shrinks the row back to one line.
    cell_edit::selectAllInCell(table, state);
    cell_edit::backspace(table, state);
    cell_edit::recomputeRowHeights(table, fixedMeasure);
    REQUIRE(table.rowHeight(0) == 28.0f);
}

TEST_CASE("Cell editing: snapshot undo round-trip at model level",
          "[table][cell-edit]") {
    // The command layer snapshots the whole table before a
    // typing run and restores `before` on undo (cmd::undo does
    // exactly this assignment when the entry is still live).
    Table table(1, 2);
    CellEditState state;
    state.active = true;
    state.cell = {0, 0};
    Table before = table;
    cell_edit::insertText(table, state, "Al");
    cell_edit::insertText(table, state, "pha");
    REQUIRE(table.getCellContent(0, 0) == "Alpha");
    table = before;  // undo: one step restores the whole run
    REQUIRE(table.getCellContent(0, 0).empty());
}
