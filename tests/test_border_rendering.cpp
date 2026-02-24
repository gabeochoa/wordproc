#include "../src/editor/table.h"
#include "catch2/catch.hpp"

TEST_CASE("borderStyleThickness maps styles to line widths", "[rendering][table]") {
    REQUIRE(borderStyleThickness(BorderStyle::None) == 0.0f);
    REQUIRE(borderStyleThickness(BorderStyle::Thin) == 1.0f);
    REQUIRE(borderStyleThickness(BorderStyle::Medium) == 2.0f);
    REQUIRE(borderStyleThickness(BorderStyle::Thick) == 3.0f);

    SECTION("unimplemented styles return 0") {
        REQUIRE(borderStyleThickness(BorderStyle::Double) == 0.0f);
        REQUIRE(borderStyleThickness(BorderStyle::Dashed) == 0.0f);
        REQUIRE(borderStyleThickness(BorderStyle::Dotted) == 0.0f);
    }

    SECTION("thickness increases with weight") {
        REQUIRE(borderStyleThickness(BorderStyle::Thin) < borderStyleThickness(BorderStyle::Medium));
        REQUIRE(borderStyleThickness(BorderStyle::Medium) < borderStyleThickness(BorderStyle::Thick));
    }
}

TEST_CASE("CellBorders default to Thin", "[rendering][table]") {
    CellBorders borders;
    REQUIRE(borders.top == BorderStyle::Thin);
    REQUIRE(borders.bottom == BorderStyle::Thin);
    REQUIRE(borders.left == BorderStyle::Thin);
    REQUIRE(borders.right == BorderStyle::Thin);
}

TEST_CASE("Table cell borders integrate with thickness", "[rendering][table]") {
    Table table(2, 2);

    SECTION("setTableBorders propagates to thickness") {
        CellBorders noBorders;
        noBorders.top = BorderStyle::None;
        noBorders.bottom = BorderStyle::None;
        noBorders.left = BorderStyle::None;
        noBorders.right = BorderStyle::None;
        table.setTableBorders(noBorders);

        const TableCell& cell = table.cell(0, 0);
        REQUIRE(borderStyleThickness(cell.borders.top) == 0.0f);
        REQUIRE(borderStyleThickness(cell.borders.bottom) == 0.0f);
        REQUIRE(borderStyleThickness(cell.borders.left) == 0.0f);
        REQUIRE(borderStyleThickness(cell.borders.right) == 0.0f);
    }

    SECTION("mixed border styles per edge") {
        TableCell& cell = table.cell(0, 0);
        cell.borders.top = BorderStyle::Thin;
        cell.borders.bottom = BorderStyle::Medium;
        cell.borders.left = BorderStyle::Thick;
        cell.borders.right = BorderStyle::None;

        REQUIRE(borderStyleThickness(cell.borders.top) == 1.0f);
        REQUIRE(borderStyleThickness(cell.borders.bottom) == 2.0f);
        REQUIRE(borderStyleThickness(cell.borders.left) == 3.0f);
        REQUIRE(borderStyleThickness(cell.borders.right) == 0.0f);
    }
}
