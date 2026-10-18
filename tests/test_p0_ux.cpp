#include "../src/editor/text_buffer.h"
#include "../src/editor/text_layout.h"
#include "../src/ui/menu_geometry.h"
#include "../src/ui/tooltip_math.h"
#include "catch2/catch.hpp"

// NOTE: P0-6 has no unit test: ah_win95_widgets.h pulls the afterhours
// graphics backend, which the test binary does not define. The E2E
// screenshot (pass_p0_dialog_buttons.e2e) is its check.

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

// P0-4: hit testing + word/paragraph selection + typing replaces selection.
TEST_CASE("P0-4 mouse selection", "[p0][selection]") {
    auto fixed10 = [](const std::string& prefix) {
        return static_cast<float>(prefix.size() * 10);
    };

    SECTION("hitTestColumn snaps to nearest boundary, up on ties") {
        REQUIRE(hitTestColumn("hello", -5.0f, fixed10) == 0);
        REQUIRE(hitTestColumn("hello", 0.0f, fixed10) == 0);
        REQUIRE(hitTestColumn("hello", 4.0f, fixed10) == 0);
        REQUIRE(hitTestColumn("hello", 5.0f, fixed10) == 1);
        REQUIRE(hitTestColumn("hello", 14.0f, fixed10) == 1);
        REQUIRE(hitTestColumn("hello", 25.0f, fixed10) == 3);
        REQUIRE(hitTestColumn("hello", 1000.0f, fixed10) == 5);
        REQUIRE(hitTestColumn("", 50.0f, fixed10) == 0);
    }

    SECTION("selectWordAt selects the word under the position") {
        TextBuffer buffer;
        buffer.setText("hello world foo");
        buffer.selectWordAt({0, 7});
        REQUIRE(buffer.hasSelection());
        REQUIRE(buffer.getSelectedText() == "world");
        buffer.selectWordAt({0, 0});
        REQUIRE(buffer.getSelectedText() == "hello");
        buffer.selectWordAt({0, 14});
        REQUIRE(buffer.getSelectedText() == "foo");
    }

    SECTION("selectWordAt on whitespace places the caret only") {
        TextBuffer buffer;
        buffer.setText("hello world");
        buffer.selectWordAt({0, 5});
        REQUIRE_FALSE(buffer.hasSelection());
        REQUIRE(buffer.caret().column == 5);
    }

    SECTION("selectParagraphAt selects the whole line") {
        TextBuffer buffer;
        buffer.setText("line one\nline two\nline three");
        buffer.selectParagraphAt({1, 3});
        REQUIRE(buffer.hasSelection());
        REQUIRE(buffer.getSelectedText() == "line two");
    }

    SECTION("typing replaces the selection") {
        TextBuffer buffer;
        buffer.setText("hello world");
        buffer.selectAll();
        REQUIRE(buffer.hasSelection());
        buffer.insertChar('X');
        REQUIRE(buffer.getText() == "X");
        REQUIRE_FALSE(buffer.hasSelection());
    }

    SECTION("backspace deletes the selection") {
        TextBuffer buffer;
        buffer.setText("hello world");
        buffer.selectWordAt({0, 1});
        buffer.backspace();
        REQUIRE(buffer.getText() == " world");
    }
}

// P0-8: dropdown insets — label left and shortcut right are symmetric
// 8px insets from the dropdown edge.
TEST_CASE("P0-8 dropdown insets", "[p0][menu]") {
    SECTION("insets are 8px, symmetric") {
        REQUIRE(menu_geometry::ITEM_PADDING_X == 8.0f);
        REQUIRE(menu_geometry::dropdownLabelLeft(100.0f) == 108.0f);
        REQUIRE(menu_geometry::dropdownShortcutRight(100.0f, 200.0f) == 292.0f);
        REQUIRE(menu_geometry::dropdownShortcutRight(100.0f, 200.0f) + 8.0f ==
                100.0f + 200.0f);
    }
    SECTION("button edge + text inset compose to the 8px label inset") {
        REQUIRE(menu_geometry::ITEM_BUTTON_EDGE +
                    (menu_geometry::ITEM_PADDING_X - menu_geometry::ITEM_BUTTON_EDGE) ==
                8.0f);
    }
}

// P0-7: tooltip geometry — 4px horizontal / 2px vertical padding, 0.5s delay.
TEST_CASE("P0-7 tooltip geometry", "[p0][tooltip]") {
    SECTION("padding is 4px horizontal, 2px vertical") {
        REQUIRE(tooltip_math::tooltipWidth(100.0f) == 108.0f);
        REQUIRE(tooltip_math::tooltipHeight(11.0f) == 15.0f);
    }
    SECTION("tooltip shows only after the hover delay") {
        REQUIRE_FALSE(tooltip_math::delayElapsed(0.0f));
        REQUIRE_FALSE(tooltip_math::delayElapsed(0.499f));
        REQUIRE(tooltip_math::delayElapsed(0.5f));
        REQUIRE(tooltip_math::delayElapsed(2.0f));
    }
}

