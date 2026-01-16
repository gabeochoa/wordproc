#include "catch2/catch.hpp"
#include "../src/ui/menu_types.h"

TEST_CASE("Menu bar creation", "[menu][e2e]") {
    auto menus = createMenuBarData();

    SECTION("creates all expected menus") {
        REQUIRE(menus.size() == 5);
    }

    SECTION("File menu is first and has expected items") {
        REQUIRE(menus.size() >= 1);
        REQUIRE(menus[0].label == "File");
        REQUIRE(menus[0].items.size() >= 5);
        REQUIRE(menus[0].items[0].label == "New");
        REQUIRE(menus[0].items[1].label == "Open...");
        REQUIRE(menus[0].items[2].label == "Save");
        REQUIRE(menus[0].items[3].label == "Save As...");
        // Item 4 is separator
        REQUIRE(menus[0].items[5].label == "Exit");
    }

    SECTION("Edit menu has expected items") {
        REQUIRE(menus.size() >= 2);
        REQUIRE(menus[1].label == "Edit");
        REQUIRE(menus[1].items.size() >= 7);
        REQUIRE(menus[1].items[0].label == "Undo");
        REQUIRE(menus[1].items[1].label == "Redo");
        REQUIRE(menus[1].items[3].label == "Cut");
        REQUIRE(menus[1].items[4].label == "Copy");
        REQUIRE(menus[1].items[5].label == "Paste");
        REQUIRE(menus[1].items[7].label == "Select All");
    }

    SECTION("View menu has expected items") {
        REQUIRE(menus.size() >= 3);
        REQUIRE(menus[2].label == "View");
        REQUIRE(menus[2].items.size() >= 3);
        REQUIRE(menus[2].items[0].label == "Pageless Mode");
        REQUIRE(menus[2].items[1].label == "Paged Mode");
    }

    SECTION("Format menu has expected items") {
        REQUIRE(menus.size() >= 4);
        REQUIRE(menus[3].label == "Format");
        REQUIRE(menus[3].items.size() >= 8);
        REQUIRE(menus[3].items[0].label == "Bold");
        REQUIRE(menus[3].items[1].label == "Italic");
    }

    SECTION("Help menu is last") {
        REQUIRE(menus.size() >= 5);
        REQUIRE(menus[4].label == "Help");
        REQUIRE(menus[4].items.size() >= 1);
        REQUIRE(menus[4].items[0].label == "About Wordproc");
    }
}

TEST_CASE("Menu items have correct properties", "[menu]") {
    auto menus = createMenuBarData();

    SECTION("Enabled items are marked as enabled") {
        for (const auto& menu : menus) {
            for (const auto& item : menu.items) {
                if (!item.isSeparator) {
                    REQUIRE(item.enabled == true);
                }
            }
        }
    }

    SECTION("Keyboard shortcuts are set correctly") {
        const auto& fileMenu = menus[0];
        REQUIRE(fileMenu.items[0].shortcut == "Ctrl+N"); // New
        REQUIRE(fileMenu.items[1].shortcut == "Ctrl+O"); // Open
        REQUIRE(fileMenu.items[2].shortcut == "Ctrl+S"); // Save
    }
}

TEST_CASE("File menu specifically exists", "[menu][e2e][regression]") {
    // Regression test: File menu must always be present and first
    // This catches the bug where menus weren't rendering properly
    auto menus = createMenuBarData();
    
    REQUIRE_FALSE(menus.empty());
    REQUIRE(menus[0].label == "File");
    REQUIRE_FALSE(menus[0].items.empty());
    
    // Verify essential file operations are available
    bool hasNew = false, hasOpen = false, hasSave = false, hasExit = false;
    for (const auto& item : menus[0].items) {
        if (item.label == "New") hasNew = true;
        if (item.label == "Open...") hasOpen = true;
        if (item.label == "Save") hasSave = true;
        if (item.label == "Exit") hasExit = true;
    }
    
    REQUIRE(hasNew);
    REQUIRE(hasOpen);
    REQUIRE(hasSave);
    REQUIRE(hasExit);
}
