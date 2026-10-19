#include <set>

#include "../src/editor/symbol.h"
#include "catch2/catch.hpp"

TEST_CASE("Symbol grid", "[symbol]") {
    const auto& entries = symbol::entries();

    SECTION("all required characters are present") {
        auto contains = [&](const char* ch) {
            for (const auto& e : entries) {
                if (std::string(e.character) == ch) return true;
            }
            return false;
        };
        REQUIRE(contains("\xC2\xA9"));  // ©
        REQUIRE(contains("\xC2\xAE"));  // ®
        REQUIRE(contains("\xE2\x84\xA2"));  // ™
        REQUIRE(contains("\xC2\xA7"));  // §
        REQUIRE(contains("\xC2\xB6"));  // ¶
        REQUIRE(contains("\xE2\x80\xA2"));  // •
        REQUIRE(contains("\xE2\x80\x93"));  // –
        REQUIRE(contains("\xE2\x80\x94"));  // —
        REQUIRE(contains("\xE2\x80\x98"));  // '
        REQUIRE(contains("\xE2\x80\x99"));  // '
        REQUIRE(contains("\xE2\x80\x9C"));  // "
        REQUIRE(contains("\xE2\x80\x9D"));  // "
        REQUIRE(contains("\xC2\xAB"));  // «
        REQUIRE(contains("\xC2\xBB"));  // »
        REQUIRE(contains("\xE2\x82\xAC"));  // €
        REQUIRE(contains("\xC2\xA3"));  // £
        REQUIRE(contains("\xC2\xA5"));  // ¥
        REQUIRE(contains("\xC2\xB0"));  // °
        REQUIRE(contains("\xC2\xB1"));  // ±
        REQUIRE(contains("\xC2\xBD"));  // ½
        REQUIRE(contains("\xC3\xB7"));  // ÷
        REQUIRE(contains("\xC3\x97"));  // ×
        REQUIRE(contains("\xE2\x80\xA6"));  // …
        REQUIRE(contains("\xCE\xB1"));  // α
        REQUIRE(contains("\xCF\x89"));  // ω
    }

    SECTION("entries are unique and well-formed") {
        REQUIRE(entries.size() == 47);  // 23 symbols + 24 Greek
        std::set<std::string> chars;
        std::set<std::string> slugs;
        for (const auto& e : entries) {
            REQUIRE(e.character != nullptr);
            REQUIRE(e.name != nullptr);
            REQUIRE(e.slug != nullptr);
            REQUIRE(std::string(e.character).size() >= 2);
            REQUIRE(std::string(e.name).size() > 0);
            REQUIRE(std::string(e.slug).size() > 0);
            REQUIRE(chars.insert(e.character).second);
            REQUIRE(slugs.insert(e.slug).second);
        }
    }

    SECTION("codepoint matches the UTF-8 character, names are set") {
        auto decode = [](const char* s) {
            const auto* p = reinterpret_cast<const unsigned char*>(s);
            std::uint32_t cp = p[0];
            std::size_t len = 1;
            if ((p[0] & 0xE0) == 0xC0) {
                cp = p[0] & 0x1F;
                len = 2;
            } else if ((p[0] & 0xF0) == 0xE0) {
                cp = p[0] & 0x0F;
                len = 3;
            } else if ((p[0] & 0xF8) == 0xF0) {
                cp = p[0] & 0x07;
                len = 4;
            }
            for (std::size_t i = 1; i < len; ++i) {
                cp = (cp << 6) | (p[i] & 0x3F);
            }
            return cp;
        };
        for (const auto& e : entries) {
            REQUIRE(e.codepoint != 0);
            REQUIRE(e.codepoint == decode(e.character));
            REQUIRE(std::string(e.unicodeName).size() > 0);
        }
        REQUIRE(entries[0].codepoint == 0x00A9);
        REQUIRE(entries[2].codepoint == 0x2122);
        REQUIRE(entries[23].codepoint == 0x03B1);
    }

    SECTION("codeLabel and readout format") {
        REQUIRE(symbol::codeLabel(entries[0]) == "U+00A9");
        REQUIRE(symbol::codeLabel(entries[2]) == "U+2122");
        REQUIRE(symbol::codeLabel(entries[23]) == "U+03B1");
        REQUIRE(symbol::readout(entries[0]) == "U+00A9 Copyright Sign");
        REQUIRE(symbol::readout(entries[2]) == "U+2122 Trade Mark Sign");
    }

    SECTION("special subset is the 23 non-Greek entries") {
        auto special = symbol::specialIndices();
        REQUIRE(special.size() == 23);
        for (std::size_t idx : special) {
            REQUIRE(entries[idx].isSpecial);
        }
        REQUIRE(entries[special.front()].codepoint == 0x00A9);
        REQUIRE(entries[special.back()].codepoint == 0x2026);
        for (std::size_t i = 23; i < entries.size(); ++i) {
            REQUIRE_FALSE(entries[i].isSpecial);
        }
    }

    SECTION("shortcut hints match Word for the assigned entries") {
        auto bySlug = [&](const char* slug) {
            for (const auto& e : entries) {
                if (std::string(e.slug) == slug) return e;
            }
            return symbol::Entry{};
        };
        REQUIRE(std::string(bySlug("copyright").shortcut) == "Alt+Ctrl+C");
        REQUIRE(std::string(bySlug("registered").shortcut) == "Alt+Ctrl+R");
        REQUIRE(std::string(bySlug("trademark").shortcut) == "Alt+Ctrl+T");
        REQUIRE(std::string(bySlug("em_dash").shortcut) == "Alt+Ctrl+Num -");
        REQUIRE(std::string(bySlug("en_dash").shortcut) == "Ctrl+Num -");
        REQUIRE(std::string(bySlug("ellipsis").shortcut) == "Alt+Ctrl+.");
        REQUIRE(std::string(bySlug("alpha").shortcut).empty());
    }

    SECTION("font choices are the loaded families") {
        const auto& fonts = symbol::fontChoices();
        REQUIRE(fonts.size() == 4);
        REQUIRE(std::string(fonts[0].display) == "Roboto");
        REQUIRE(std::string(fonts[0].managerKey) == "__default");
        REQUIRE(std::string(fonts[1].display) == "Garamond");
        REQUIRE(std::string(fonts[1].managerKey) == "Garamond");
        REQUIRE(std::string(fonts[2].display) == "Gaegu");
        REQUIRE(std::string(fonts[3].display) == "Gaegu-Bold");
        std::set<std::string> slugs;
        for (const auto& f : fonts) {
            REQUIRE(std::string(f.display).size() > 0);
            REQUIRE(std::string(f.managerKey).size() > 0);
            REQUIRE(slugs.insert(f.slug).second);
        }
    }

    SECTION("Greek alpha-omega are the last 24 entries, in order") {
        REQUIRE(entries.size() >= 24);
        auto tail = std::vector<symbol::Entry>(
            entries.end() - 24, entries.end());
        REQUIRE(std::string(tail.front().character) == "\xCE\xB1");
        REQUIRE(std::string(tail.back().character) == "\xCF\x89");
        REQUIRE(std::string(tail.front().name) == "Alpha");
        REQUIRE(std::string(tail.back().name) == "Omega");
    }
}
