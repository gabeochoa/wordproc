#include "../src/ui/icon_registry.h"
#include "catch2/catch.hpp"

TEST_CASE("IconRegistry singleton", "[icon]") {
    auto& registry = IconRegistry::instance();
    auto& registry2 = IconRegistry::instance();
    REQUIRE(&registry == &registry2);
}

TEST_CASE("IconRegistry registered icons", "[icon]") {
    auto& registry = IconRegistry::instance();
    
    SECTION("file operations have icons") {
        REQUIRE(registry.hasIcon(Action::NewDocument));
        REQUIRE(registry.hasIcon(Action::Open));
        REQUIRE(registry.hasIcon(Action::Save));
        REQUIRE(registry.hasIcon(Action::Print));
    }
    
    SECTION("edit operations have icons") {
        REQUIRE(registry.hasIcon(Action::Undo));
        REQUIRE(registry.hasIcon(Action::Redo));
        REQUIRE(registry.hasIcon(Action::Cut));
        REQUIRE(registry.hasIcon(Action::Copy));
        REQUIRE(registry.hasIcon(Action::Paste));
    }
    
    SECTION("formatting operations have icons") {
        REQUIRE(registry.hasIcon(Action::Bold));
        REQUIRE(registry.hasIcon(Action::Italic));
        REQUIRE(registry.hasIcon(Action::Underline));
    }
    
    SECTION("alignment operations have icons") {
        REQUIRE(registry.hasIcon(Action::AlignLeft));
        REQUIRE(registry.hasIcon(Action::AlignCenter));
        REQUIRE(registry.hasIcon(Action::AlignRight));
        REQUIRE(registry.hasIcon(Action::AlignJustify));
    }
    
    SECTION("unregistered actions have no icons") {
        // Actions that shouldn't have icons (text-only menu items)
        // Note: This depends on what actions exist - update if needed
        REQUIRE_FALSE(registry.hasIcon(Action::None));
    }
}

TEST_CASE("IconInfo content", "[icon]") {
    auto& registry = IconRegistry::instance();
    
    SECTION("icon info has required fields") {
        const IconInfo* saveInfo = registry.iconForAction(Action::Save);
        REQUIRE(saveInfo != nullptr);
        REQUIRE_FALSE(saveInfo->id.empty());
        REQUIRE(saveInfo->name != nullptr);
        REQUIRE(saveInfo->description != nullptr);
    }
    
    SECTION("icon IDs are unique") {
        std::unordered_map<std::string, Action> seenIds;
        for (const auto& [action, info] : registry.allIcons()) {
            auto it = seenIds.find(info.id);
            if (it != seenIds.end()) {
                FAIL("Duplicate icon ID: " << info.id);
            }
            seenIds[info.id] = action;
        }
        REQUIRE(seenIds.size() > 0);
    }
}

TEST_CASE("Paired icons", "[icon]") {
    auto& registry = IconRegistry::instance();
    
    SECTION("undo/redo are paired") {
        const IconInfo* undo = registry.iconForAction(Action::Undo);
        const IconInfo* redo = registry.iconForAction(Action::Redo);
        REQUIRE(undo != nullptr);
        REQUIRE(redo != nullptr);
        REQUIRE(undo->isPaired);
        REQUIRE(redo->isPaired);
        REQUIRE(undo->pairedWith == redo->id);
        REQUIRE(redo->pairedWith == undo->id);
    }
    
    SECTION("zoom in/out are paired") {
        const IconInfo* zoomIn = registry.iconForAction(Action::ZoomIn);
        const IconInfo* zoomOut = registry.iconForAction(Action::ZoomOut);
        REQUIRE(zoomIn != nullptr);
        REQUIRE(zoomOut != nullptr);
        REQUIRE(zoomIn->isPaired);
        REQUIRE(zoomOut->isPaired);
        REQUIRE(zoomIn->pairedWith == zoomOut->id);
        REQUIRE(zoomOut->pairedWith == zoomIn->id);
    }
    
    SECTION("paired actions list is populated") {
        const auto& pairs = registry.pairedActions();
        REQUIRE_FALSE(pairs.empty());
    }
}

TEST_CASE("Helper functions", "[icon]") {
    SECTION("shouldShowIcon") {
        REQUIRE(shouldShowIcon(Action::Save));
        REQUIRE_FALSE(shouldShowIcon(Action::None));
    }
    
    SECTION("getIconId") {
        REQUIRE(getIconId(Action::Save) != nullptr);
        REQUIRE(std::string(getIconId(Action::Save)) == "save");
        REQUIRE(getIconId(Action::None) == nullptr);
    }
    
    SECTION("getIconName") {
        REQUIRE(getIconName(Action::Save) != nullptr);
        REQUIRE(std::string(getIconName(Action::Save)) == "Save");
        REQUIRE(getIconName(Action::None) == nullptr);
    }
}
