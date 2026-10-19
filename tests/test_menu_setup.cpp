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
        // Font... is the FIRST Format item (Word 95 Font dialog).
        REQUIRE_FALSE(menus[3].items.empty());
        REQUIRE(menus[3].items[0].label == "Font...");
        REQUIRE(menus[3].items[1].label == "Normal");
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
                if (item.isSeparator) continue;
                // Except Window > Arrange All: disabled by design
                // (a single-window app cannot arrange windows).
                if (menu.label == "Window" &&
                    item.label == "Arrange All") {
                    REQUIRE(item.enabled == false);
                    continue;
                }
                // Except Tools > Mail Merge...: disabled by design
                // (no merge engine / data source; grayed like Word).
                if (menu.label == "Tools" &&
                    item.label == "Mail Merge...") {
                    REQUIRE(item.enabled == false);
                    continue;
                }
                REQUIRE(item.enabled == true);
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

TEST_CASE("File menu has Print Preview + Print after Page Setup", "[menu][print]") {
    auto menus = createMenuBarData();
    REQUIRE(menus.size() >= 1);
    REQUIRE(menus[0].label == "File");
    const auto& items = menus[0].items;

    // Word 95 File order: ... Page Setup..., Print Preview, Print...
    std::size_t pageSetup = items.size(), preview = items.size(),
                print = items.size();
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i].label == "Page Setup...") pageSetup = i;
        if (items[i].label == "Print Preview") preview = i;
        if (items[i].label == "Print...") print = i;
    }
    REQUIRE(pageSetup < items.size());
    REQUIRE(preview == pageSetup + 1);
    REQUIRE(print == preview + 1);
    REQUIRE(items[print].shortcut == "Ctrl+P");
    REQUIRE(items[print].enabled == true);
}

TEST_CASE("File menu has Version History... right after Save As...",
            "[menu]") {
    auto menus = createMenuBarData();
    const MenuData* file = nullptr;
    for (const auto& m : menus) {
        if (m.label == "File") file = &m;
    }
    REQUIRE(file != nullptr);
    std::size_t saveAs = file->items.size(), history = file->items.size();
    for (std::size_t i = 0; i < file->items.size(); ++i) {
        if (file->items[i].label == "Save As...") saveAs = i;
        if (file->items[i].label == "Version History...") history = i;
    }
    REQUIRE(saveAs < file->items.size());
    REQUIRE(history == saveAs + 1);
}

TEST_CASE("Format menu Paragraph... is first of alignment/spacing section", "[menu]") {
    auto menus = createMenuBarData();
    const MenuData* format = nullptr;
    for (const auto& m : menus) {
        if (m.label == "Format") format = &m;
    }
    REQUIRE(format != nullptr);
    const auto& items = format->items;
    // The alignment/spacing section is the second Align block:
    // Paragraph... immediately precedes its Align Left, followed by
    // the rest of that section's items in Word order.
    std::size_t paragraph = items.size(), secondAlign = items.size();
    int alignSeen = 0;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i].label == "Paragraph...") paragraph = i;
        if (items[i].label == "Align Left" && ++alignSeen == 2)
            secondAlign = i;
    }
    REQUIRE(paragraph < items.size());
    REQUIRE(secondAlign == paragraph + 1);
    REQUIRE(items[secondAlign + 1].label == "Align Center");
    REQUIRE(items[secondAlign + 2].label == "Align Right");
    REQUIRE(items[secondAlign + 3].label == "Justify");
    bool hasTabWidth = false;
    for (const auto& item : items) {
        if (item.label == "Tab Width...") hasTabWidth = true;
    }
    REQUIRE(hasTabWidth);
}

TEST_CASE("Format menu ends with Columns...", "[menu]") {
    auto menus = createMenuBarData();
    const MenuData* format = nullptr;
    for (const auto& m : menus) {
        if (m.label == "Format") format = &m;
    }
    REQUIRE(format != nullptr);
    REQUIRE_FALSE(format->items.empty());
    REQUIRE(format->items.back().label == "Columns...");
}

TEST_CASE("Format menu has Borders and Shading... right before Columns...",
            "[menu]") {
    auto menus = createMenuBarData();
    const MenuData* format = nullptr;
    for (const auto& m : menus) {
        if (m.label == "Format") format = &m;
    }
    REQUIRE(format != nullptr);
    std::size_t borders = format->items.size();
    for (std::size_t i = 0; i < format->items.size(); ++i) {
        if (format->items[i].label == "Borders and Shading...") borders = i;
    }
    REQUIRE(borders < format->items.size());
    REQUIRE(format->items[borders + 1].label == "Columns...");
    REQUIRE(format->items[borders].enabled == true);
}

TEST_CASE("Insert menu has Date and Time before Special Character", "[menu]") {
    auto menus = createMenuBarData();
    const MenuData* insert = nullptr;
    for (const auto& m : menus) {
        if (m.label == "Insert") insert = &m;
    }
    REQUIRE(insert != nullptr);
    std::size_t dateTime = insert->items.size(), special = insert->items.size();
    for (std::size_t i = 0; i < insert->items.size(); ++i) {
        if (insert->items[i].label == "Date and Time...") dateTime = i;
        if (insert->items[i].label == "Special Character...") special = i;
    }
    REQUIRE(dateTime < insert->items.size());
    REQUIRE(special == dateTime + 1);
}

TEST_CASE("Tools menu is Word 95 order; UI Scale moved to View", "[menu]") {
    auto menus = createMenuBarData();
    const MenuData* tools = nullptr;
    const MenuData* view = nullptr;
    for (const auto& m : menus) {
        if (m.label == "Tools") tools = &m;
        if (m.label == "View") view = &m;
    }
    REQUIRE(tools != nullptr);
    REQUIRE(view != nullptr);

    // Tools: Spelling, Word Count, AutoCorrect, sep, Grammar,
    // AutoFormat, Revisions..., sep, Mail Merge, sep, Options.
    REQUIRE(tools->items.size() == 11);
    REQUIRE(tools->items[0].label == "Spelling...");
    REQUIRE(tools->items[0].shortcut == "F7");
    REQUIRE(tools->items[1].label == "Word Count...");
    REQUIRE(tools->items[2].label == "AutoCorrect...");
    REQUIRE(tools->items[3].isSeparator);
    REQUIRE(tools->items[4].label == "Grammar...");
    REQUIRE(tools->items[5].label == "AutoFormat...");
    REQUIRE(tools->items[6].label == "Revisions...");
    REQUIRE(tools->items[7].isSeparator);
    REQUIRE(tools->items[8].label == "Mail Merge...");
    REQUIRE(tools->items[8].enabled == false);
    REQUIRE(tools->items[9].isSeparator);
    REQUIRE(tools->items[10].label == "Options...");
    for (const auto& item : tools->items) {
        REQUIRE(item.label != "UI Scale...");
        REQUIRE(item.label != "Preferences...");
    }

    // View: UI Scale... is the last item, after Outline.
    REQUIRE(!view->items.empty());
    REQUIRE(view->items.back().label == "UI Scale...");
    std::size_t outline = view->items.size();
    for (std::size_t i = 0; i < view->items.size(); ++i) {
        if (view->items[i].label == "Outline") outline = i;
    }
    REQUIRE(outline < view->items.size());
    REQUIRE(view->items.back().label == "UI Scale...");
    REQUIRE(outline + 2 == view->items.size() - 1);  // sep between
    REQUIRE(view->items[outline + 1].isSeparator);
}

TEST_CASE("Window menu sits between Table and Help", "[menu]") {
    auto menus = createMenuBarData();

    std::size_t tableIdx = menus.size(), windowIdx = menus.size(),
                helpIdx = menus.size();
    for (std::size_t i = 0; i < menus.size(); ++i) {
        if (menus[i].label == "Table") tableIdx = i;
        if (menus[i].label == "Window") windowIdx = i;
        if (menus[i].label == "Help") helpIdx = i;
    }
    REQUIRE(tableIdx < menus.size());
    REQUIRE(windowIdx == tableIdx + 1);
    REQUIRE(helpIdx == windowIdx + 1);

    const auto& window = menus[windowIdx];
    REQUIRE(window.items.size() == 5);
    REQUIRE(window.items[0].label == "New Window");
    REQUIRE(window.items[0].enabled == true);
    REQUIRE(window.items[1].label == "Arrange All");
    REQUIRE(window.items[1].enabled == false);
    REQUIRE(window.items[2].label == "Split");
    REQUIRE(window.items[2].enabled == true);
    REQUIRE(window.items[3].isSeparator);
    REQUIRE(window.items[4].label == "1 Untitled");
    // Current-document item: ENABLED and CHECKED (was disabled).
    REQUIRE(window.items[4].enabled == true);
    REQUIRE(window.items[4].checked == true);
}
