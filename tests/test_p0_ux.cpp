#include "../src/ui/menu_geometry.h"
#include "catch2/catch.hpp"

// P0-2: menu-bar headers carry ~7px padding per side, so the button width
// is label + 2 * padding. Reverting the header to the old +16 total (or
// dropping padding) breaks headerWidth's contract here.
TEST_CASE("P0-2 menu-bar header padding", "[p0][menu]") {
    SECTION("padding is 7px per side") {
        REQUIRE(menu_geometry::HEADER_PADDING_X == 7.0f);
    }
    SECTION("header width is label plus both paddings") {
        REQUIRE(menu_geometry::headerWidth(100.0f, 7.0f) == 114.0f);
        REQUIRE(menu_geometry::headerWidth(0.0f, 7.0f) == 14.0f);
    }
}
