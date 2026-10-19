#include "../src/editor/font_dialog.h"

#include "catch2/catch.hpp"

TEST_CASE("Font dialog selection seeds from the current style", "[font_dialog]") {
    std::vector<std::string> fonts = {"Times New Roman", "Arial", "Courier New"};
    TextStyle style;
    style.font = "Arial";
    style.bold = true;
    style.fontSize = 18;
    style.underline = true;
    style.textColor = TextColors::Red;

    auto sel = font_dialog::fontDialogSelectionFromStyle(style, fonts);
    REQUIRE(sel.fontIndex == 1);
    REQUIRE(sel.styleIndex == 1);  // Bold
    REQUIRE(sel.size == 18);
    REQUIRE(sel.underline);
    REQUIRE_FALSE(sel.strikethrough);
    REQUIRE(sel.colorIndex == 2);  // Red

    style.bold = true;
    style.italic = true;
    REQUIRE(font_dialog::fontDialogSelectionFromStyle(style, fonts).styleIndex ==
            3);  // Bold Italic
    style.bold = false;
    REQUIRE(font_dialog::fontDialogSelectionFromStyle(style, fonts).styleIndex ==
            2);  // Italic

    // Unknown font/color fall back to the first entry.
    style.font = "No Such Font";
    style.textColor = TextColor{1, 2, 3, 255};
    auto fallback = font_dialog::fontDialogSelectionFromStyle(style, fonts);
    REQUIRE(fallback.fontIndex == 0);
    REQUIRE(fallback.colorIndex == 0);
}

TEST_CASE("Font dialog preview size scales by the UI scale", "[font_dialog]") {
    // At ui_scale 1.0 the preview is the point size, unchanged.
    REQUIRE(font_dialog::previewFontPx(16, 1.0f) == 16);
    REQUIRE(font_dialog::previewFontPx(28, 1.0f) == 28);
    // At the default ui_scale 1.2 a 28pt preview renders 34px
    // (28 * 1.2 = 33.6, rounded), not the raw 28px: the E2E
    // font_preview_size=34 pins this.
    REQUIRE(font_dialog::previewFontPx(28, 1.2f) == 34);
    REQUIRE(font_dialog::previewFontPx(16, 1.2f) == 19);
    // Never below 1px.
    REQUIRE(font_dialog::previewFontPx(0, 1.2f) == 1);
}

TEST_CASE("Font dialog selection applies to a style", "[font_dialog]") {
    std::vector<std::string> fonts = {"Times New Roman", "Arial", "Courier New"};
    TextStyle base;
    font_dialog::FontDialogSelection sel;
    sel.fontIndex = 2;
    sel.styleIndex = 3;  // Bold Italic
    sel.size = 24;
    sel.strikethrough = true;
    sel.superscript = true;
    sel.subscript = true;  // mutually exclusive: superscript wins
    sel.colorIndex = 5;    // Blue

    TextStyle applied = font_dialog::fontDialogApplyStyle(base, sel, fonts);
    REQUIRE(applied.font == "Courier New");
    REQUIRE(applied.bold);
    REQUIRE(applied.italic);
    REQUIRE(applied.fontSize == 24);
    REQUIRE(applied.strikethrough);
    REQUIRE(applied.superscript);
    REQUIRE_FALSE(applied.subscript);
    REQUIRE(applied.textColor == TextColors::Blue);

    // Size clamps to the 8..72 range; out-of-range indices are ignored.
    sel.size = 200;
    sel.fontIndex = 99;
    sel.colorIndex = 99;
    TextStyle clamped = font_dialog::fontDialogApplyStyle(base, sel, fonts);
    REQUIRE(clamped.fontSize == 72);
    REQUIRE(clamped.font == base.font);
    sel.size = 2;
    REQUIRE(font_dialog::fontDialogApplyStyle(base, sel, fonts).fontSize == 8);
}
