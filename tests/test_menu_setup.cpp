#include "../src/ui/menu_types.h"
#include "catch2/catch.hpp"

TEST_CASE("Menu bar creation", "[menu][e2e]") {
    auto menus = createMenuBarData();

    SECTION("creates all expected menus") { REQUIRE(menus.size() >= 8); }

    SECTION("File menu is first and has expected items") {
        REQUIRE(menus.size() >= 1);
        REQUIRE(menus[0].label == "File");
        bool hasNew = false;
        bool hasTemplate = false;
        bool hasOpen = false;
        bool hasSave = false;
        bool hasSaveAs = false;
        bool hasExit = false;
        for (const auto& item : menus[0].items) {
            if (item.label == "New") hasNew = true;
            if (item.label == "New from Template...") hasTemplate = true;
            if (item.label == "Open...") hasOpen = true;
            if (item.label == "Save") hasSave = true;
            if (item.label == "Save As...") hasSaveAs = true;
            if (item.label == "Exit") hasExit = true;
        }
        REQUIRE(hasNew);
        REQUIRE(hasTemplate);
        REQUIRE(hasOpen);
        REQUIRE(hasSave);
        REQUIRE(hasSaveAs);
        REQUIRE(hasExit);
    }

    SECTION("Edit menu has expected items") {
        REQUIRE(menus.size() >= 2);
        REQUIRE(menus[1].label == "Edit");
        bool hasUndo = false;
        bool hasRedo = false;
        bool hasTrack = false;
        bool hasCut = false;
        bool hasCopy = false;
        bool hasPaste = false;
        bool hasSelectAll = false;
        for (const auto& item : menus[1].items) {
            if (item.label == "Undo") hasUndo = true;
            if (item.label == "Redo") hasRedo = true;
            if (item.label == "Track Changes") hasTrack = true;
            if (item.label == "Cut") hasCut = true;
            if (item.label == "Copy") hasCopy = true;
            if (item.label == "Paste") hasPaste = true;
            if (item.label == "Select All") hasSelectAll = true;
        }
        REQUIRE(hasUndo);
        REQUIRE(hasRedo);
        REQUIRE(hasTrack);
        REQUIRE(hasCut);
        REQUIRE(hasCopy);
        REQUIRE(hasPaste);
        REQUIRE(hasSelectAll);
    }

    SECTION("View menu has expected items") {
        REQUIRE(menus.size() >= 3);
        REQUIRE(menus[2].label == "View");
        bool hasPageless = false;
        bool hasPaged = false;
        bool hasZoom = false;
        bool hasFocus = false;
        for (const auto& item : menus[2].items) {
            if (item.label == "Pageless Mode") hasPageless = true;
            if (item.label == "Paged Mode") hasPaged = true;
            if (item.label == "Zoom In") hasZoom = true;
            if (item.label == "Focus Mode") hasFocus = true;
        }
        REQUIRE(hasPageless);
        REQUIRE(hasPaged);
        REQUIRE(hasZoom);
        REQUIRE(hasFocus);
    }

    SECTION("Format menu has expected items") {
        REQUIRE(menus.size() >= 4);
        REQUIRE(menus[3].label == "Format");
        bool hasBold = false;
        bool hasItalic = false;
        bool hasSuperscript = false;
        bool hasDropCap = false;
        for (const auto& item : menus[3].items) {
            if (item.label == "Bold") hasBold = true;
            if (item.label == "Italic") hasItalic = true;
            if (item.label == "Superscript") hasSuperscript = true;
            if (item.label == "Drop Cap") hasDropCap = true;
        }
        REQUIRE(hasBold);
        REQUIRE(hasItalic);
        REQUIRE(hasSuperscript);
        REQUIRE(hasDropCap);
    }

    SECTION("Help menu exists and tools menu exists") {
        bool hasHelp = false;
        bool hasTools = false;
        for (const auto& menu : menus) {
            if (menu.label == "Help") hasHelp = true;
            if (menu.label == "Tools") hasTools = true;
        }
        REQUIRE(hasHelp);
        REQUIRE(hasTools);
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
        REQUIRE(fileMenu.items[0].shortcut == "Ctrl+N");  // New
        REQUIRE(fileMenu.items[2].shortcut == "Ctrl+O");  // Open
        REQUIRE(fileMenu.items[3].shortcut == "Ctrl+S");  // Save
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
