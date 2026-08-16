#include "catch2/catch.hpp"

#include "../src/ui/win95_theme.h"

using ui_imm::createWin95Theme;
namespace c = ui_imm::win95_colors;

namespace {
bool same(const afterhours::Color& a, const afterhours::Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
}  // namespace

TEST_CASE("Win95 theme colors", "[ui][theme]") {
    const auto theme = createWin95Theme();

    SECTION("chrome and text") {
        REQUIRE(same(theme.background, c::WINDOW_BG));
        REQUIRE(same(theme.surface, c::TEXT_AREA));
        REQUIRE(same(theme.primary, c::TITLE_BAR));
        REQUIRE(same(theme.secondary, c::BUTTON_FACE));
        REQUIRE(same(theme.darkfont, c::TEXT));
        REQUIRE(same(theme.font_muted, c::TEXT_DISABLED));
    }

    SECTION("corners are sharp") {
        REQUIRE(theme.roundness == 0.0f);
        REQUIRE(theme.rounded_corners.none());
    }
}

// Feature request #5: afterhours now renders a focus ring for visual_focus_id,
// so the ring has to be themed rather than worked around. Zero thickness means
// "no ring" upstream, and an unset focus color falls back to theme.font -- which
// is the white used on the navy title bar and would be invisible on the gray
// chrome. Both are easy to lose in a later theme edit, so pin them.
TEST_CASE("Win95 theme has a visible keyboard focus ring", "[ui][theme][focus]") {
    const auto theme = createWin95Theme();

    REQUIRE(theme.focus_ring_thickness > 0.0f);
    REQUIRE(theme.focus_ring_thickness == ui_imm::WIN95_FOCUS_RING_THICKNESS);

    // Positive offset insets the ring, the way a Win95 focus rectangle sits
    // just inside the control's edge.
    REQUIRE(theme.focus_ring_offset > 0.0f);
    REQUIRE(theme.focus_ring_offset == ui_imm::WIN95_FOCUS_RING_OFFSET);

    REQUIRE(same(theme.focus, ui_imm::WIN95_FOCUS_RING_COLOR));
    REQUIRE_FALSE(same(theme.focus, theme.font));
    REQUIRE_FALSE(same(theme.focus, theme.background));
}

// Feature request #10: popups cast a hard 2px shadow (afterhours ShadowStyle::
// Hard). Menus and dropdown lists read these; a zero offset or a transparent
// colour would silently turn the shadow off.
TEST_CASE("Win95 popup shadow is visible and hard-edged", "[ui][theme][shadow]") {
    REQUIRE(ui_imm::WIN95_POPUP_SHADOW_OFFSET > 0.0f);
    REQUIRE(ui_imm::WIN95_POPUP_SHADOW_COLOR.a > 0);
}
