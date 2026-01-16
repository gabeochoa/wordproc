#include "../src/editor/text_buffer.h"
#include "catch2/catch.hpp"

TEST_CASE("Footnote creation", "[text_buffer][footnote]") {
    TextBuffer buffer;
    buffer.setText("Hello World. This is a test.");
    
    SECTION("add footnote at current position") {
        buffer.setCaret({0, 5});  // After "Hello"
        REQUIRE(buffer.addFootnote("This is footnote 1"));
        REQUIRE(buffer.footnotes().size() == 1);
        REQUIRE(buffer.footnotes()[0].number == 1);
    }
    
    SECTION("cannot add footnote with empty content") {
        REQUIRE_FALSE(buffer.addFootnote(""));
    }
    
    SECTION("multiple footnotes are auto-numbered in order") {
        buffer.setCaret({0, 5});  // Position 5
        buffer.addFootnote("First footnote");
        
        buffer.setCaret({0, 12});  // Position 12
        buffer.addFootnote("Second footnote");
        
        buffer.setCaret({0, 20});  // Position 20
        buffer.addFootnote("Third footnote");
        
        REQUIRE(buffer.footnotes().size() == 3);
        REQUIRE(buffer.footnotes()[0].number == 1);
        REQUIRE(buffer.footnotes()[1].number == 2);
        REQUIRE(buffer.footnotes()[2].number == 3);
    }
    
    SECTION("footnotes are sorted by position") {
        // Add in reverse order
        buffer.setCaret({0, 20});
        buffer.addFootnote("Third");
        
        buffer.setCaret({0, 5});
        buffer.addFootnote("First");
        
        buffer.setCaret({0, 12});
        buffer.addFootnote("Second");
        
        // Should be sorted by position
        REQUIRE(buffer.footnotes()[0].referenceOffset == 5);
        REQUIRE(buffer.footnotes()[1].referenceOffset == 12);
        REQUIRE(buffer.footnotes()[2].referenceOffset == 20);
        
        // Numbers should reflect sorted order
        REQUIRE(buffer.footnotes()[0].number == 1);
        REQUIRE(buffer.footnotes()[1].number == 2);
        REQUIRE(buffer.footnotes()[2].number == 3);
    }
}

TEST_CASE("Footnote removal", "[text_buffer][footnote]") {
    TextBuffer buffer;
    buffer.setText("Hello World. Test.");
    
    buffer.setCaret({0, 5});
    buffer.addFootnote("First");
    buffer.setCaret({0, 12});
    buffer.addFootnote("Second");
    buffer.setCaret({0, 17});
    buffer.addFootnote("Third");
    
    SECTION("remove footnote by number") {
        REQUIRE(buffer.footnotes().size() == 3);
        REQUIRE(buffer.removeFootnote(2));  // Remove "Second"
        REQUIRE(buffer.footnotes().size() == 2);
    }
    
    SECTION("remaining footnotes are renumbered") {
        buffer.removeFootnote(2);  // Remove middle
        
        // After removal, numbers should be 1, 2
        REQUIRE(buffer.footnotes()[0].number == 1);
        REQUIRE(buffer.footnotes()[1].number == 2);
    }
    
    SECTION("remove non-existent footnote returns false") {
        REQUIRE_FALSE(buffer.removeFootnote(99));
    }
    
    SECTION("clearFootnotes removes all") {
        buffer.clearFootnotes();
        REQUIRE(buffer.footnotes().empty());
    }
}

TEST_CASE("Footnote retrieval", "[text_buffer][footnote]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    
    buffer.setCaret({0, 5});
    buffer.addFootnote("Test footnote");
    
    SECTION("getFootnote by number") {
        const Footnote* fn = buffer.getFootnote(1);
        REQUIRE(fn != nullptr);
        REQUIRE(fn->content == "Test footnote");
        REQUIRE(fn->number == 1);
    }
    
    SECTION("getFootnote returns nullptr for invalid number") {
        REQUIRE(buffer.getFootnote(99) == nullptr);
    }
    
    SECTION("footnoteAt finds footnote by offset") {
        const Footnote* fn = buffer.footnoteAt(5);
        REQUIRE(fn != nullptr);
        REQUIRE(fn->content == "Test footnote");
    }
    
    SECTION("footnoteAt returns nullptr for position without footnote") {
        REQUIRE(buffer.footnoteAt(0) == nullptr);
        REQUIRE(buffer.footnoteAt(10) == nullptr);
    }
}

TEST_CASE("Footnote struct", "[text_buffer][footnote]") {
    Footnote fn1, fn2;
    fn1.referenceOffset = 5;
    fn1.content = "Test";
    fn1.number = 1;
    
    fn2.referenceOffset = 5;
    fn2.content = "Test";
    fn2.number = 1;
    
    SECTION("equal footnotes") {
        REQUIRE(fn1 == fn2);
        REQUIRE_FALSE(fn1 != fn2);
    }
    
    SECTION("different offset") {
        fn2.referenceOffset = 10;
        REQUIRE_FALSE(fn1 == fn2);
    }
    
    SECTION("different content") {
        fn2.content = "Different";
        REQUIRE_FALSE(fn1 == fn2);
    }
    
    SECTION("comparison for sorting") {
        fn2.referenceOffset = 10;
        REQUIRE(fn1 < fn2);
        REQUIRE_FALSE(fn2 < fn1);
    }
}
