#include "../src/editor/revisions.h"

#include "catch2/catch.hpp"

namespace {

Revision insertRev(std::size_t off, const std::string& text) {
    Revision rev;
    rev.type = RevisionType::Insert;
    rev.startOffset = off;
    rev.text = text;
    rev.author = "Gabe";
    return rev;
}

Revision deleteRev(std::size_t off, const std::string& text) {
    Revision rev;
    rev.type = RevisionType::Delete;
    rev.startOffset = off;
    rev.text = text;
    rev.author = "Gabe";
    return rev;
}

}  // namespace

TEST_CASE("Revision accept/reject (suggesting mode)", "[revisions]") {
    SECTION("accept insert keeps the text, drops the record") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        REQUIRE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
    }
    SECTION("reject insert removes the text") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("accept delete keeps the deletion (text already gone)") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{deleteRev(2, "c")};
        REQUIRE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("reject delete restores the text") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{deleteRev(2, "c")};
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
    }
    SECTION("accept delete removes text that came back via undo") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        REQUIRE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
    }
    SECTION("reject insert with stale offset skips the buffer op") {
        TextBuffer buffer;
        buffer.setText("xyz");
        std::vector<Revision> revs{insertRev(0, "c")};
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "xyz");
        REQUIRE(revs.empty());
    }
    SECTION("reject insert re-anchors a later revision") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b")};
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "bc");
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 0);
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "c");
    }
    SECTION("reject delete does not shift a same-offset insert") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{insertRev(2, "c"), deleteRev(2, "c")};
        REQUIRE(revisions::rejectRevision(buffer, revs, 1));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 2);
    }
    SECTION("reject all of type/delete/insert replays in reverse") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   insertRev(2, "c"), deleteRev(2, "c")};
        revisions::rejectAllRevisions(buffer, revs);
        REQUIRE(buffer.getText().empty());
        REQUIRE(revs.empty());
    }
    SECTION("accept all keeps the buffer, clears records") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{insertRev(0, "a"), deleteRev(2, "c")};
        revisions::acceptAllRevisions(buffer, revs);
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("accept/reject through one undo step") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(buffer.canUndo());
        buffer.undo();
        REQUIRE(buffer.getText() == "abc");
    }
    SECTION("dialog label shows type, author, excerpt") {
        REQUIRE(revisions::label(insertRev(0, "abc")) ==
                "Insert — Gabe — \"abc\"");
        REQUIRE(revisions::label(deleteRev(0, "abc")) ==
                "Delete — Gabe — \"abc\"");
        REQUIRE(revisions::excerpt(insertRev(0, "line1\nline2")) ==
                "line1 line2");
    }
    SECTION("human dialog rows: verb, excerpt, 1-based line") {
        TextBuffer buffer;
        buffer.setText("first\nsecond line");
        REQUIRE(revisions::dialogLabel(buffer, insertRev(0, "abc")) ==
                "Inserted \"abc\" — Ln 1");
        REQUIRE(revisions::dialogLabel(buffer, deleteRev(7, "c")) ==
                "Deleted \"c\" — Ln 2");
        // Long excerpt truncates at ~24 chars with an ellipsis.
        REQUIRE(revisions::dialogLabel(
                    buffer,
                    insertRev(0, "abcdefghijklmnopqrstuvwxyz1234")) ==
                "Inserted \"abcdefghijklmnopqrstuvwx...\" — Ln 1");
        // A past-end offset clamps to the last line, not a crash.
        REQUIRE(revisions::dialogLabel(buffer, insertRev(999, "x")) ==
                "Inserted \"x\" — Ln 2");
    }
    SECTION("verified insert ranges classify only matching inserts") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c"),
                                   deleteRev(0, "a"),
                                   insertRev(0, "z")};
        auto ranges = revisions::verifiedInsertRanges(buffer, revs);
        REQUIRE(ranges.size() == 1);
        REQUIRE(ranges[0].first == 2);
        REQUIRE(ranges[0].second == 3);
        // Boundaries: start is inside, end is outside.
        REQUIRE(revisions::offsetInInsertRanges(2, ranges));
        REQUIRE_FALSE(revisions::offsetInInsertRanges(3, ranges));
        REQUIRE_FALSE(revisions::offsetInInsertRanges(1, ranges));
        REQUIRE_FALSE(revisions::offsetInInsertRanges(0, ranges));
    }
    SECTION("verified ranges are empty with no revisions") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs;
        REQUIRE(revisions::verifiedInsertRanges(buffer, revs).empty());
    }
    SECTION("out-of-range index is a no-op false") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs;
        REQUIRE_FALSE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE_FALSE(revisions::rejectRevision(buffer, revs, 3));
    }
}
