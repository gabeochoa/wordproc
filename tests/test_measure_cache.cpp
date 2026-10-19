#include "../src/editor/measure_cache.h"

#include "catch2/catch.hpp"

// ============================================================================
// MeasureCache Tests
// ============================================================================
// The cache is a pure function of (fontKey, fontSize, text): a hit must
// return exactly what the measure function would return, and a change in
// any key component must re-measure. The stub counts underlying calls,
// so every case below pins the call count, not just the returned width.

TEST_CASE("MeasureCache - caches repeated measures", "[measure][cache]") {
    int calls = 0;
    MeasureCache cache{([&calls](std::string_view text, int size) {
        ++calls;
        return static_cast<int>(text.size()) * size;
    })};

    SECTION("Same text/size/fontKey measures once") {
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(calls == 1);
        REQUIRE(cache.hits() == 2);
        REQUIRE(cache.misses() == 1);
    }

    SECTION("Different size re-measures") {
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(cache.measure("hello", 20, 1) == 100);
        REQUIRE(calls == 2);
    }

    SECTION("Different fontKey re-measures") {
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(cache.measure("hello", 16, 2) == 80);
        REQUIRE(calls == 2);
    }

    SECTION("Different text re-measures") {
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(cache.measure("world", 16, 1) == 80);
        REQUIRE(calls == 2);
    }

    SECTION("Empty text is cached") {
        REQUIRE(cache.measure("", 16, 1) == 0);
        REQUIRE(cache.measure("", 16, 1) == 0);
        REQUIRE(calls == 1);
    }

    SECTION("clear() drops entries and resets stats") {
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(cache.size() == 1);
        cache.clear();
        REQUIRE(cache.size() == 0);
        REQUIRE(cache.hits() == 0);
        REQUIRE(cache.misses() == 0);
        REQUIRE(cache.measure("hello", 16, 1) == 80);
        REQUIRE(calls == 2);
    }
}

TEST_CASE("MeasureCache - LRU eviction at the cap", "[measure][cache]") {
    int calls = 0;
    MeasureCache cache{([&calls](std::string_view text, int) {
        ++calls;
        return static_cast<int>(text.size());
    }),
                       3};

    SECTION("Least recently used entry is evicted") {
        REQUIRE(cache.measure("aaa", 16, 1) == 3);
        REQUIRE(cache.measure("bbb", 16, 1) == 3);
        REQUIRE(cache.measure("ccc", 16, 1) == 3);
        REQUIRE(calls == 3);
        REQUIRE(cache.measure("ddd", 16, 1) == 3);  // Evicts "aaa".
        REQUIRE(cache.size() == 3);
        REQUIRE(cache.measure("bbb", 16, 1) == 3);
        REQUIRE(calls == 4);
        REQUIRE(cache.measure("aaa", 16, 1) == 3);  // Re-measures: evicted.
        REQUIRE(calls == 5);
    }

    SECTION("A hit refreshes recency") {
        REQUIRE(cache.measure("aaa", 16, 1) == 3);
        REQUIRE(cache.measure("bbb", 16, 1) == 3);
        REQUIRE(cache.measure("ccc", 16, 1) == 3);
        REQUIRE(cache.measure("aaa", 16, 1) == 3);  // "aaa" now newest.
        REQUIRE(cache.measure("ddd", 16, 1) == 3);  // Evicts "bbb".
        REQUIRE(cache.measure("aaa", 16, 1) == 3);
        REQUIRE(cache.measure("ccc", 16, 1) == 3);
        REQUIRE(calls == 4);
        REQUIRE(cache.measure("bbb", 16, 1) == 3);  // Re-measures: evicted.
        REQUIRE(calls == 5);
    }
}

TEST_CASE("MeasureCache - without a measure function answers 0",
          "[measure][cache]") {
    MeasureCache cache;
    REQUIRE(cache.measure("hello", 16, 1) == 0);
    REQUIRE(cache.measure("hello", 16, 1) == 0);
    REQUIRE(cache.size() == 1);
}
