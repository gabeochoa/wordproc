#include <string>
#include <utility>
#include <vector>

#include "../src/ui/menu_geometry.h"
#include "catch2/catch.hpp"

namespace {

using menu_geometry::MenuWarmSite;

struct FakeItem {
    std::string label;
    std::string shortcut;
    bool separator = false;
    std::string displayLabel;
};

struct FakeMenu {
    std::string label;
    std::vector<FakeItem> items;
};

}  // namespace

TEST_CASE("Menu warm enumeration covers every string the dropdown measures",
          "[menu][perf]") {
    // The mark-prefix constants are the exact bytes the dropdown draws;
    // pin them so the enumeration cannot silently drift from rendering.
    REQUIRE(std::string(menu_geometry::MARK_PREFIX_NONE) == "  ");
    REQUIRE(std::string(menu_geometry::MARK_PREFIX_CHECKMARK) ==
            "\xE2\x88\x9A ");
    REQUIRE(std::string(menu_geometry::MARK_PREFIX_RADIO) == "\xE2\x80\xA2 ");
    REQUIRE(std::string(menu_geometry::MARK_PREFIX_DASH) == "- ");

    const std::vector<FakeMenu> menus = {
        {"File",
         {{"New", "Ctrl+N", false, ""},
          {"", "", true, ""},
          {"Recent: /a/very/long/path.doc", "", false,
           "Recent: /a/...doc"}}},
        {"", {}},
    };

    std::vector<std::pair<std::string, MenuWarmSite>> got;
    menu_geometry::forEachMenuWarmString(
        menus, [&](const std::string& text, MenuWarmSite site) {
            got.emplace_back(text, site);
        });

    const std::vector<std::pair<std::string, MenuWarmSite>> expected = {
        // File menu: title at both size conventions.
        {"File", MenuWarmSite::HeaderWidth},
        {"F", MenuWarmSite::MnemonicWidth},
        {"F", MenuWarmSite::Drawn},
        {"File", MenuWarmSite::Drawn},
        // "New": every mark-prefix variant (marks change at runtime),
        // then the shortcut at both conventions.
        {"  New", MenuWarmSite::Drawn},
        {"\xE2\x88\x9A New", MenuWarmSite::Drawn},
        {"\xE2\x80\xA2 New", MenuWarmSite::Drawn},
        {"- New", MenuWarmSite::Drawn},
        {"Ctrl+N", MenuWarmSite::ShortcutWidth},
        {"Ctrl+N", MenuWarmSite::Drawn},
        // Separator: nothing. Recent item: display label, never the
        // raw path — the dropdown measures it as displayed.
        {"  Recent: /a/...doc", MenuWarmSite::Drawn},
        {"\xE2\x88\x9A Recent: /a/...doc", MenuWarmSite::Drawn},
        {"\xE2\x80\xA2 Recent: /a/...doc", MenuWarmSite::Drawn},
        {"- Recent: /a/...doc", MenuWarmSite::Drawn},
        // Empty title: header + drawn, no mnemonic.
        {"", MenuWarmSite::HeaderWidth},
        {"", MenuWarmSite::Drawn},
    };
    REQUIRE(got == expected);

    // The raw (untruncated) recent path must never be warmed: the
    // dropdown never measures it.
    for (const auto& [text, site] : got) {
        (void)site;
        REQUIRE(text.find("/a/very/long/path.doc") == std::string::npos);
    }
}
