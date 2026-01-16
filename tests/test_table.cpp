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
