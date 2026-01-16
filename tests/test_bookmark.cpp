#include "../src/editor/text_buffer.h"
#include "catch2/catch.hpp"

TEST_CASE("Bookmark add and get", "[bookmark]") {
    TextBuffer buffer;
    buffer.setText("Hello World\nSecond Line\nThird Line");

    SECTION("addBookmark at current caret position") {
        buffer.setCaret({0, 5});  // After "Hello"
        REQUIRE(buffer.addBookmark("hello_end"));
        
        const Bookmark* bm = buffer.getBookmark("hello_end");
        REQUIRE(bm != nullptr);
        REQUIRE(bm->name == "hello_end");
        REQUIRE(bm->offset == 5);
    }

    SECTION("addBookmarkAt specific offset") {
        REQUIRE(buffer.addBookmarkAt("start", 0));
        REQUIRE(buffer.addBookmarkAt("middle", 12));  // Start of "Second"
        
        const Bookmark* startBm = buffer.getBookmark("start");
        REQUIRE(startBm != nullptr);
        REQUIRE(startBm->offset == 0);
        
        const Bookmark* midBm = buffer.getBookmark("middle");
        REQUIRE(midBm != nullptr);
        REQUIRE(midBm->offset == 12);
    }

    SECTION("duplicate bookmark names fail") {
        REQUIRE(buffer.addBookmarkAt("dup", 5));
        REQUIRE_FALSE(buffer.addBookmarkAt("dup", 10));  // Should fail
        
        // Original should still exist at original position
        const Bookmark* bm = buffer.getBookmark("dup");
        REQUIRE(bm != nullptr);
        REQUIRE(bm->offset == 5);
    }
}

TEST_CASE("Bookmark navigation", "[bookmark]") {
    TextBuffer buffer;
    buffer.setText("Line one\nLine two\nLine three");

    buffer.addBookmarkAt("line2", 9);  // Start of "Line two"
    buffer.addBookmarkAt("line3", 18); // Start of "Line three"

    SECTION("goToBookmark sets caret position") {
        REQUIRE(buffer.goToBookmark("line2"));
        CaretPosition caret = buffer.caret();
        REQUIRE(caret.row == 1);
        REQUIRE(caret.column == 0);
    }

    SECTION("goToBookmark returns false for nonexistent bookmark") {
        REQUIRE_FALSE(buffer.goToBookmark("nonexistent"));
    }

    SECTION("hasBookmark returns true for existing bookmarks") {
        REQUIRE(buffer.hasBookmark("line2"));
        REQUIRE(buffer.hasBookmark("line3"));
        REQUIRE_FALSE(buffer.hasBookmark("line1"));
    }
}

TEST_CASE("Bookmark removal", "[bookmark]") {
    TextBuffer buffer;
    buffer.setText("Some text here");
    
    buffer.addBookmarkAt("bm1", 0);
    buffer.addBookmarkAt("bm2", 5);
    buffer.addBookmarkAt("bm3", 10);

    SECTION("removeBookmark removes existing bookmark") {
        REQUIRE(buffer.bookmarks().size() == 3);
        REQUIRE(buffer.removeBookmark("bm2"));
        REQUIRE(buffer.bookmarks().size() == 2);
        REQUIRE_FALSE(buffer.hasBookmark("bm2"));
    }

    SECTION("removeBookmark returns false for nonexistent") {
        REQUIRE_FALSE(buffer.removeBookmark("fake"));
        REQUIRE(buffer.bookmarks().size() == 3);
    }

    SECTION("clearBookmarks removes all") {
        buffer.clearBookmarks();
        REQUIRE(buffer.bookmarks().empty());
    }
}

TEST_CASE("Bookmark near position", "[bookmark]") {
    TextBuffer buffer;
    buffer.setText("0123456789ABCDEF");

    buffer.addBookmarkAt("at5", 5);
    buffer.addBookmarkAt("at10", 10);

    SECTION("exact match with tolerance 0") {
        const Bookmark* bm = buffer.bookmarkNear(5, 0);
        REQUIRE(bm != nullptr);
        REQUIRE(bm->name == "at5");
    }

    SECTION("no match when outside tolerance") {
        const Bookmark* bm = buffer.bookmarkNear(3, 1);  // 3 is 2 away from 5
        REQUIRE(bm == nullptr);
    }

    SECTION("match within tolerance") {
        const Bookmark* bm = buffer.bookmarkNear(4, 1);  // 4 is 1 away from 5
        REQUIRE(bm != nullptr);
        REQUIRE(bm->name == "at5");
    }
}

TEST_CASE("Bookmark offset adjustment on edit", "[bookmark]") {
    TextBuffer buffer;
    buffer.setText("Hello World");
    
    buffer.addBookmarkAt("world", 6);  // Start of "World"

    SECTION("insert before bookmark shifts it forward") {
        buffer.setCaret({0, 0});
        buffer.insertText("Hi ");  // "Hi Hello World"
        
        const Bookmark* bm = buffer.getBookmark("world");
        REQUIRE(bm != nullptr);
        REQUIRE(bm->offset == 9);  // Shifted by 3
    }

    SECTION("insert after bookmark doesn't move it") {
        buffer.setCaret({0, 11});  // End of line
        buffer.insertText("!");  // "Hello World!"
        
        const Bookmark* bm = buffer.getBookmark("world");
        REQUIRE(bm != nullptr);
        REQUIRE(bm->offset == 6);  // Unchanged
    }

    SECTION("delete before bookmark shifts it backward") {
        buffer.setCaret({0, 2});
        buffer.setSelectionAnchor({0, 0});
        buffer.updateSelectionToCaret();
        buffer.deleteSelection();  // Remove "He", now "llo World"
        
        const Bookmark* bm = buffer.getBookmark("world");
        REQUIRE(bm != nullptr);
        REQUIRE(bm->offset == 4);  // Shifted by -2
    }
}

TEST_CASE("Bookmark invalid operations", "[bookmark]") {
    TextBuffer buffer;
    buffer.setText("Short");

    SECTION("empty name is rejected") {
        REQUIRE_FALSE(buffer.addBookmarkAt("", 0));
        REQUIRE(buffer.bookmarks().empty());
    }

    SECTION("offset beyond text is rejected") {
        std::size_t textLen = buffer.getText().size();
        REQUIRE_FALSE(buffer.addBookmarkAt("invalid", textLen + 100));
        REQUIRE(buffer.bookmarks().empty());
    }

    SECTION("getBookmark returns nullptr for nonexistent") {
        REQUIRE(buffer.getBookmark("nope") == nullptr);
    }
}
