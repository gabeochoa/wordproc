#include "../src/editor/table.h"
#include "catch2/catch.hpp"

TEST_CASE("Table creation", "[table]") {
    SECTION("default constructor creates empty table") {
        Table table;
        REQUIRE(table.isEmpty());
        REQUIRE(table.rowCount() == 0);
        REQUIRE(table.colCount() == 0);
    }
    
    SECTION("constructor with dimensions creates proper grid") {
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
    
    SECTION("createTableWithHeader sets headers") {
        std::vector<std::string> headers = {"Name", "Age", "City"};
        Table table = createTableWithHeader(4, 3, headers);
        REQUIRE(table.rowCount() == 4);
        REQUIRE(table.colCount() == 3);
        REQUIRE(table.getCellContent(0, 0) == "Name");
        REQUIRE(table.getCellContent(0, 1) == "Age");
        REQUIRE(table.getCellContent(0, 2) == "City");
        // Header cells should be bold
        REQUIRE(table.cell(0, 0).textStyle.bold == true);
    }
}

TEST_CASE("Table cell access", "[table]") {
    Table table(3, 3);
    
    SECTION("can set and get cell content") {
        table.setCellContent(1, 2, "Hello");
        REQUIRE(table.getCellContent(1, 2) == "Hello");
    }
    
    SECTION("cells start empty") {
        REQUIRE(table.getCellContent(0, 0).empty());
        REQUIRE(table.getCellContent(2, 2).empty());
    }
    
    SECTION("out of range throws exception") {
        REQUIRE_THROWS_AS(table.cell(5, 0), std::out_of_range);
        REQUIRE_THROWS_AS(table.cell(0, 5), std::out_of_range);
    }
    
    SECTION("cell position overload works") {
        CellPosition pos{1, 1};
        table.setCellContent(1, 1, "Test");
        REQUIRE(table.cell(pos).content == "Test");
    }
}

TEST_CASE("Table row operations", "[table]") {
    Table table(3, 3);
    table.setCellContent(0, 0, "R0C0");
    table.setCellContent(1, 0, "R1C0");
    table.setCellContent(2, 0, "R2C0");
    
    SECTION("insert row above shifts content down") {
        table.insertRowAbove(1);
        REQUIRE(table.rowCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "R0C0");
        REQUIRE(table.getCellContent(1, 0).empty());  // New row
        REQUIRE(table.getCellContent(2, 0) == "R1C0");
        REQUIRE(table.getCellContent(3, 0) == "R2C0");
    }
    
    SECTION("insert row below shifts content") {
        table.insertRowBelow(0);
        REQUIRE(table.rowCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "R0C0");
        REQUIRE(table.getCellContent(1, 0).empty());  // New row
        REQUIRE(table.getCellContent(2, 0) == "R1C0");
    }
    
    SECTION("delete row removes content") {
        table.deleteRow(1);
        REQUIRE(table.rowCount() == 2);
        REQUIRE(table.getCellContent(0, 0) == "R0C0");
        REQUIRE(table.getCellContent(1, 0) == "R2C0");
    }
    
    SECTION("cannot delete last row") {
        Table singleRow(1, 3);
        singleRow.deleteRow(0);
        REQUIRE(singleRow.rowCount() == 1);  // Still has 1 row
    }
    
    SECTION("row height can be set and retrieved") {
        table.setRowHeight(1, 50.0f);
        REQUIRE(table.rowHeight(1) == 50.0f);
    }
    
    SECTION("row height has minimum") {
        table.setRowHeight(0, 5.0f);
        REQUIRE(table.rowHeight(0) >= 10.0f);
    }
}

TEST_CASE("Table column operations", "[table]") {
    Table table(3, 3);
    table.setCellContent(0, 0, "C0");
    table.setCellContent(0, 1, "C1");
    table.setCellContent(0, 2, "C2");
    
    SECTION("insert column left shifts content right") {
        table.insertColumnLeft(1);
        REQUIRE(table.colCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "C0");
        REQUIRE(table.getCellContent(0, 1).empty());  // New column
        REQUIRE(table.getCellContent(0, 2) == "C1");
        REQUIRE(table.getCellContent(0, 3) == "C2");
    }
    
    SECTION("insert column right shifts content") {
        table.insertColumnRight(0);
        REQUIRE(table.colCount() == 4);
        REQUIRE(table.getCellContent(0, 0) == "C0");
        REQUIRE(table.getCellContent(0, 1).empty());  // New column
        REQUIRE(table.getCellContent(0, 2) == "C1");
    }
    
    SECTION("delete column removes content") {
        table.deleteColumn(1);
        REQUIRE(table.colCount() == 2);
        REQUIRE(table.getCellContent(0, 0) == "C0");
        REQUIRE(table.getCellContent(0, 1) == "C2");
    }
    
    SECTION("cannot delete last column") {
        Table singleCol(3, 1);
        singleCol.deleteColumn(0);
        REQUIRE(singleCol.colCount() == 1);  // Still has 1 column
    }
    
    SECTION("column width can be set and retrieved") {
        table.setColWidth(1, 150.0f);
        REQUIRE(table.colWidth(1) == 150.0f);
    }
    
    SECTION("column width has minimum") {
        table.setColWidth(0, 10.0f);
        REQUIRE(table.colWidth(0) >= 20.0f);
    }
}

TEST_CASE("Table cell merging", "[table]") {
    Table table(4, 4);
    
    SECTION("can merge cells") {
        CellPosition topLeft{0, 0};
        CellPosition bottomRight{1, 1};
        
        REQUIRE(table.canMerge(topLeft, bottomRight));
        REQUIRE(table.mergeCells(topLeft, bottomRight));
        
        // Check that top-left has the span
        REQUIRE(table.cell(0, 0).span.rowSpan == 2);
        REQUIRE(table.cell(0, 0).span.colSpan == 2);
        
        // Other cells should be marked as merged
        REQUIRE(table.isCellMerged({0, 1}));
        REQUIRE(table.isCellMerged({1, 0}));
        REQUIRE(table.isCellMerged({1, 1}));
        
        // Non-merged cells unaffected
        REQUIRE_FALSE(table.isCellMerged({0, 2}));
        REQUIRE_FALSE(table.isCellMerged({2, 0}));
    }
    
    SECTION("merge combines cell content") {
        table.setCellContent(0, 0, "A");
        table.setCellContent(0, 1, "B");
        table.setCellContent(1, 0, "C");
        table.setCellContent(1, 1, "D");
        
        table.mergeCells({0, 0}, {1, 1});
        
        // Content should be combined in master cell
        std::string combined = table.getCellContent(0, 0);
        REQUIRE(combined.find("A") != std::string::npos);
        REQUIRE(combined.find("B") != std::string::npos);
        REQUIRE(combined.find("C") != std::string::npos);
        REQUIRE(combined.find("D") != std::string::npos);
    }
    
    SECTION("cannot merge already merged cells") {
        table.mergeCells({0, 0}, {1, 1});
        REQUIRE_FALSE(table.canMerge({0, 0}, {2, 2}));
    }
    
    SECTION("cannot merge invalid positions") {
        REQUIRE_FALSE(table.canMerge({5, 5}, {6, 6}));
        REQUIRE_FALSE(table.canMerge({2, 2}, {1, 1}));  // topLeft > bottomRight
    }
    
    SECTION("can split merged cells") {
        table.mergeCells({0, 0}, {1, 1});
        REQUIRE(table.splitCell({0, 0}));
        
        // All cells should be unmerged
        REQUIRE_FALSE(table.isCellMerged({0, 0}));
        REQUIRE_FALSE(table.isCellMerged({0, 1}));
        REQUIRE_FALSE(table.isCellMerged({1, 0}));
        REQUIRE_FALSE(table.isCellMerged({1, 1}));
        
        // Spans should be reset
        REQUIRE(table.cell(0, 0).span.rowSpan == 1);
        REQUIRE(table.cell(0, 0).span.colSpan == 1);
    }
    
    SECTION("splitting non-merged cell returns false") {
        REQUIRE_FALSE(table.splitCell({0, 0}));
    }
    
    SECTION("getMergeParent returns correct parent") {
        table.mergeCells({1, 1}, {2, 2});
        REQUIRE(table.getMergeParent({2, 2}) == CellPosition{1, 1});
        REQUIRE(table.getMergeParent({1, 1}) == CellPosition{1, 1});  // Master returns itself
    }
}

TEST_CASE("Table selection", "[table]") {
    Table table(3, 3);
    
    SECTION("no selection initially") {
        REQUIRE_FALSE(table.hasSelection());
    }
    
    SECTION("can set selection") {
        table.setSelection({0, 0}, {1, 1});
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart() == CellPosition{0, 0});
        REQUIRE(table.selectionEnd() == CellPosition{1, 1});
    }
    
    SECTION("can clear selection") {
        table.setSelection({0, 0}, {1, 1});
        table.clearSelection();
        REQUIRE_FALSE(table.hasSelection());
    }
    
    SECTION("selectAll selects entire table") {
        table.selectAll();
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart() == CellPosition{0, 0});
        REQUIRE(table.selectionEnd() == CellPosition{2, 2});
    }
    
    SECTION("selectRow selects entire row") {
        table.selectRow(1);
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart() == CellPosition{1, 0});
        REQUIRE(table.selectionEnd() == CellPosition{1, 2});
    }
    
    SECTION("selectColumn selects entire column") {
        table.selectColumn(2);
        REQUIRE(table.hasSelection());
        REQUIRE(table.selectionStart() == CellPosition{0, 2});
        REQUIRE(table.selectionEnd() == CellPosition{2, 2});
    }
}

TEST_CASE("Table navigation", "[table]") {
    Table table(3, 3);
    
    SECTION("current cell starts at 0,0") {
        REQUIRE(table.currentCell() == CellPosition{0, 0});
    }
    
    SECTION("can set current cell") {
        table.setCurrentCell({1, 2});
        REQUIRE(table.currentCell() == CellPosition{1, 2});
    }
    
    SECTION("moveToNextCell moves right then wraps") {
        table.setCurrentCell({0, 0});
        table.moveToNextCell();
        REQUIRE(table.currentCell() == CellPosition{0, 1});
        
        table.setCurrentCell({0, 2});
        table.moveToNextCell();
        REQUIRE(table.currentCell() == CellPosition{1, 0});  // Wrapped to next row
    }
    
    SECTION("moveToPrevCell moves left then wraps") {
        table.setCurrentCell({1, 0});
        table.moveToPrevCell();
        REQUIRE(table.currentCell() == CellPosition{0, 2});  // Wrapped to previous row
    }
    
    SECTION("moveUp/Down/Left/Right work") {
        table.setCurrentCell({1, 1});
        
        table.moveUp();
        REQUIRE(table.currentCell() == CellPosition{0, 1});
        
        table.setCurrentCell({1, 1});
        table.moveDown();
        REQUIRE(table.currentCell() == CellPosition{2, 1});
        
        table.setCurrentCell({1, 1});
        table.moveLeft();
        REQUIRE(table.currentCell() == CellPosition{1, 0});
        
        table.setCurrentCell({1, 1});
        table.moveRight();
        REQUIRE(table.currentCell() == CellPosition{1, 2});
    }
    
    SECTION("navigation respects bounds") {
        table.setCurrentCell({0, 0});
        table.moveUp();
        REQUIRE(table.currentCell() == CellPosition{0, 0});  // Can't go up
        
        table.moveLeft();
        REQUIRE(table.currentCell() == CellPosition{0, 0});  // Can't go left
        
        table.setCurrentCell({2, 2});
        table.moveDown();
        REQUIRE(table.currentCell() == CellPosition{2, 2});  // Can't go down
        
        table.moveRight();
        REQUIRE(table.currentCell() == CellPosition{2, 2});  // Can't go right
    }
    
    SECTION("navigation skips merged cells") {
        table.mergeCells({1, 0}, {1, 1});
        table.setCurrentCell({1, 0});
        table.moveToNextCell();
        REQUIRE(table.currentCell() == CellPosition{1, 2});  // Skipped merged cell
    }
}

TEST_CASE("Table dimensions", "[table]") {
    Table table(3, 4);
    
    SECTION("totalWidth sums column widths") {
        // Default width is 100.0f per column
        REQUIRE(table.totalWidth() == 400.0f);
        
        table.setColWidth(0, 150.0f);
        REQUIRE(table.totalWidth() == 450.0f);
    }
    
    SECTION("totalHeight sums row heights") {
        // Default height is 24.0f per row
        REQUIRE(table.totalHeight() == 72.0f);
        
        table.setRowHeight(0, 50.0f);
        REQUIRE(table.totalHeight() == 98.0f);
    }
}

TEST_CASE("Table cell bounds", "[table]") {
    Table table(3, 3);
    // Default: 100px wide columns, 24px tall rows
    
    SECTION("cellBounds returns correct position") {
        auto bounds = table.cellBounds({0, 0});
        REQUIRE(bounds.x == 0.0f);
        REQUIRE(bounds.y == 0.0f);
        REQUIRE(bounds.width == 100.0f);
        REQUIRE(bounds.height == 24.0f);
        
        bounds = table.cellBounds({1, 2});
        REQUIRE(bounds.x == 200.0f);  // 2 columns * 100
        REQUIRE(bounds.y == 24.0f);   // 1 row * 24
    }
    
    SECTION("cellBounds accounts for merged cells") {
        table.mergeCells({0, 0}, {1, 1});
        auto bounds = table.cellBounds({0, 0});
        REQUIRE(bounds.width == 200.0f);  // 2 columns
        REQUIRE(bounds.height == 48.0f);  // 2 rows
    }
    
    SECTION("cellAtPoint finds correct cell") {
        REQUIRE(table.cellAtPoint(50.0f, 12.0f) == CellPosition{0, 0});
        REQUIRE(table.cellAtPoint(150.0f, 12.0f) == CellPosition{0, 1});
        REQUIRE(table.cellAtPoint(50.0f, 36.0f) == CellPosition{1, 0});
    }
}

TEST_CASE("Table borders", "[table]") {
    Table table(2, 2);
    
    SECTION("setTableBorders applies to all cells") {
        CellBorders borders;
        borders.top = BorderStyle::Double;
        borders.bottom = BorderStyle::Dashed;
        
        table.setTableBorders(borders);
        
        REQUIRE(table.cell(0, 0).borders.top == BorderStyle::Double);
        REQUIRE(table.cell(1, 1).borders.bottom == BorderStyle::Dashed);
    }
}

TEST_CASE("Table cell properties", "[table]") {
    Table table(2, 2);
    
    SECTION("cell alignment can be set") {
        auto& cell = table.cell(0, 0);
        cell.alignment = CellAlignment::MiddleCenter;
        REQUIRE(table.cell(0, 0).alignment == CellAlignment::MiddleCenter);
    }
    
    SECTION("cell background color can be set") {
        auto& cell = table.cell(0, 0);
        cell.backgroundColor = TextColors::Yellow;
        REQUIRE(table.cell(0, 0).backgroundColor.r == TextColors::Yellow.r);
    }
    
    SECTION("cell padding can be set") {
        auto& cell = table.cell(0, 0);
        cell.paddingTop = 10;
        cell.paddingLeft = 15;
        REQUIRE(table.cell(0, 0).paddingTop == 10);
        REQUIRE(table.cell(0, 0).paddingLeft == 15);
    }
}
