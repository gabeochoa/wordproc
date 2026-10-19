#include "../src/editor/revisions.h"

#include "catch2/catch.hpp"

// Track Changes (Batch 14): deletions are marked IN PLACE — a
// Delete revision's [start, start+text.size()) is a LIVE range
// in the buffer (text stays, renders red + strikethrough) until
// Accept really deletes it or Reject unmarks it. Replace-paths
// still write record-only Deletes (text absent); the match gate
// separates the two flavours (accept/reject act correctly on
// both). Every section that performs an accept/reject/group op
// installs the edit-shift hook first (accept/reject never shift
// manually — the buffer op fires the hook, exactly like the
// owning DocumentComponent does).

static Revision insertRev(std::size_t start, const std::string& text) {
    return Revision{RevisionType::Insert, start, text, "Gabe", 0};
}

static Revision deleteRev(std::size_t start, const std::string& text) {
    return Revision{RevisionType::Delete, start, text, "Gabe", 0};
}

// Buffer + revisions with the hook installed (the
// DocumentComponent's setup). `revs` must outlive the buffer's
// hook use within the section.
static void hook(TextBuffer& buffer, std::vector<Revision>& revs) {
    revisions::installShiftHook(buffer, revs);
}

TEST_CASE("Revision accept/reject (suggesting mode)", "[revisions]") {
    SECTION("accept insert keeps inserted text") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
    }
    SECTION("reject insert removes inserted text") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("accept a LIVE delete really deletes the marked text") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("accept a RECORD-ONLY delete keeps the deletion (text already gone)") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{deleteRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("reject a LIVE delete unmarks: the text stays, plain again") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
    }
    SECTION("reject a RECORD-ONLY delete restores the text") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{deleteRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
    }
    SECTION("reject insert re-anchors a later revision") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "bc");
        REQUIRE(revs[0].startOffset == 0);
        REQUIRE(revs[0].text == "b");
    }
    SECTION("accept a live delete re-anchors a later revision") {
        TextBuffer buffer;
        buffer.setText("abcdef");
        std::vector<Revision> revs{deleteRev(2, "c"), insertRev(5, "f")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abdef");
        REQUIRE(revs[0].startOffset == 4);
        REQUIRE(revs[0].text == "f");
        // The survivor still matches the buffer at its offset.
        REQUIRE(revisions::bufferMatchesAt(buffer, revs[0].startOffset,
                                           revs[0].text));
    }
    SECTION("reject insert with stale offset skips the buffer op") {
        TextBuffer buffer;
        buffer.setText("xyz");
        std::vector<Revision> revs{insertRev(0, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "xyz");
        REQUIRE(revs.empty());
    }
    SECTION("reject record-only delete does not shift a same-offset insert") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{insertRev(2, "c"), deleteRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevision(buffer, revs, 1));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs[0].startOffset == 2);
    }
    SECTION("reject all replays in reverse") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   insertRev(2, "c"), deleteRev(2, "c")};
        hook(buffer, revs);
        revisions::rejectAllRevisions(buffer, revs);
        REQUIRE(buffer.getText() == "");
        REQUIRE(revs.empty());
    }
    SECTION("accept all") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{insertRev(0, "a"), deleteRev(2, "c")};
        hook(buffer, revs);
        revisions::acceptAllRevisions(buffer, revs);
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("accept/reject through one undo step") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        buffer.undo();
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());  // List change is not undone; text is.
    }
    SECTION("out-of-range index is a no-op") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        hook(buffer, revs);
        REQUIRE_FALSE(revisions::acceptRevision(buffer, revs, 1));
        REQUIRE_FALSE(revisions::acceptRevision(buffer, revs, 99));
        REQUIRE_FALSE(revisions::rejectRevision(buffer, revs, 1));
        REQUIRE(revs.size() == 1);
    }
}

TEST_CASE("Revision dialog labels", "[revisions]") {
    SECTION("dialog label shows type, author, excerpt") {
        REQUIRE(revisions::label(insertRev(0, "abc")) ==
                "Insert — Gabe — \"abc\"");
        REQUIRE(revisions::label(deleteRev(0, "abc")) ==
                "Delete — Gabe — \"abc\"");
        REQUIRE(revisions::excerpt(insertRev(0, "line1\nline2")) ==
                "line1 line2");
    }
    SECTION("excerpt is short and quotes the text") {
        Revision rev;
        rev.type = RevisionType::Insert;
        rev.text = "hello world";
        // excerpt is the raw (truncated) text; label() adds quotes.
        REQUIRE(revisions::excerpt(rev) == "hello world");
        rev.type = RevisionType::Delete;
        rev.text = "supercalifragilisticexpialidocious";
        const std::string value = revisions::excerpt(rev);
        REQUIRE(value.size() < rev.text.size() + 2);
        REQUIRE(value.find("...") != std::string::npos);
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
}

TEST_CASE("Revision shiftForEdit (the one shift rule)", "[revisions]") {
    SECTION("insert at a live Delete mark's start shifts the mark") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        revisions::shiftForEdit(buffer, revs, 2, 1);
        REQUIRE(revs[0].startOffset == 3);
        REQUIRE(revs[0].text == "c");
    }
    SECTION("insert before a live Delete mark shifts it") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        revisions::shiftForEdit(buffer, revs, 0, 2);
        REQUIRE(revs[0].startOffset == 4);
    }
    SECTION("insert strictly inside a live Delete mark does not extend it") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(0, "abc")};
        revisions::shiftForEdit(buffer, revs, 1, 1);
        REQUIRE(revs[0].startOffset == 0);
        REQUIRE(revs[0].text == "abc");
    }
    SECTION("insert at a live Delete mark's end does not extend it") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(0, "abc")};
        revisions::shiftForEdit(buffer, revs, 3, 1);
        REQUIRE(revs[0].startOffset == 0);
        REQUIRE(revs[0].text == "abc");
    }
    SECTION("insert at an Insert revision's offset does not shift it") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        revisions::shiftForEdit(buffer, revs, 2, 1);
        REQUIRE(revs[0].startOffset == 2);
    }
    SECTION("insert before an Insert revision shifts it") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(2, "c")};
        revisions::shiftForEdit(buffer, revs, 1, 1);
        REQUIRE(revs[0].startOffset == 3);
    }
    SECTION("delete clips a live Delete mark's tail (kept head stays)") {
        TextBuffer buffer;
        buffer.setText("abcd");
        std::vector<Revision> revs{deleteRev(0, "abcd")};
        revisions::shiftForEdit(buffer, revs, 1, -1);
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 0);
        REQUIRE(revs[0].text == "acd");
    }
    SECTION("delete clips a live Delete mark's head (kept tail slides left)") {
        TextBuffer buffer;
        buffer.setText("xxcdef");
        std::vector<Revision> revs{deleteRev(2, "cdef")};
        revisions::shiftForEdit(buffer, revs, 0, -4);
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 0);
        REQUIRE(revs[0].text == "ef");
    }
    SECTION("delete fully overlapping a live Delete mark drops it") {
        TextBuffer buffer;
        buffer.setText("xxcd");
        std::vector<Revision> revs{deleteRev(2, "cd")};
        revisions::shiftForEdit(buffer, revs, 2, -2);
        REQUIRE(revs.empty());
    }
    SECTION("delete before a live Delete mark shifts it") {
        TextBuffer buffer;
        buffer.setText("xx___fg");
        std::vector<Revision> revs{deleteRev(5, "fg")};
        revisions::shiftForEdit(buffer, revs, 0, -2);
        REQUIRE(revs[0].startOffset == 3);
        REQUIRE(revs[0].text == "fg");
    }
    SECTION("delete clamps an Insert revision inside it to its start") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(1, "b")};
        revisions::shiftForEdit(buffer, revs, 0, -2);
        REQUIRE(revs[0].startOffset == 0);
    }
    SECTION("zero delta is a no-op") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        revisions::shiftForEdit(buffer, revs, 2, 0);
        REQUIRE(revs[0].startOffset == 2);
    }
    SECTION("a RECORD-ONLY Delete is an anchor, never clipped") {
        // The replace-paths' record of text that is already gone:
        // an insert at its offset does not shift it, and a
        // deletion overlapping it clamps it instead of
        // clipping/erasing it — rejecting it must still restore
        // the text.
        TextBuffer buffer;
        buffer.setText("xyz");
        std::vector<Revision> revs{deleteRev(2, "c")};
        revisions::shiftForEdit(buffer, revs, 2, 1);
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 2);
        REQUIRE(revs[0].text == "c");
        revisions::shiftForEdit(buffer, revs, 0, -3);
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 0);
        REQUIRE(revs[0].text == "c");
    }
    SECTION("rejecting a replaced word still restores it after the replacement is rejected") {
        // AutoCorrect "baseline" -> "Baseline": the Delete
        // record is record-only. Rejecting the Insert pair
        // deletes "Baseline" — the hook must NOT erase the
        // Delete record in the process, or Reject All can
        // never bring "baseline" back. (Regression: the hook
        // clipped every Delete, so Reject All left "".)
        TextBuffer buffer;
        buffer.setText("Baseline extra");
        std::vector<Revision> revs{deleteRev(0, "baseline"),
                                   insertRev(0, "Baseline ")};
        hook(buffer, revs);
        // Reject the Insert (the replacement): buffer "extra".
        REQUIRE(revisions::rejectRevision(buffer, revs, 1));
        REQUIRE(buffer.getText() == "extra");
        // The Delete record survived the deletion.
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].type == RevisionType::Delete);
        REQUIRE(revs[0].text == "baseline");
        // Reject it: "baseline" comes back at offset 0.
        REQUIRE(revisions::rejectRevision(buffer, revs, 0));
        REQUIRE(buffer.getText() == "baselineextra");
    }
}

TEST_CASE("Revision verified ranges (render classification)",
          "[revisions]") {
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
        REQUIRE(revisions::verifiedDeleteRanges(buffer, revs).empty());
    }
    SECTION("only live Delete marks classify") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(0, "a"), deleteRev(1, "WRONG"),
                                   insertRev(2, "c")};
        const auto deletes =
            revisions::verifiedDeleteRanges(buffer, revs);
        REQUIRE(deletes.size() == 1);
        REQUIRE(deletes[0].first == 0);
        REQUIRE(deletes[0].second == 1);
        REQUIRE(revisions::offsetInDeleteRanges(0, deletes));
        REQUIRE_FALSE(revisions::offsetInDeleteRanges(1, deletes));
        REQUIRE_FALSE(revisions::offsetInDeleteRanges(2, deletes));
    }
    SECTION("a stale Delete (text moved) does not classify") {
        TextBuffer buffer;
        buffer.setText("xyz");
        std::vector<Revision> revs{deleteRev(0, "a")};
        REQUIRE(revisions::verifiedDeleteRanges(buffer, revs).empty());
    }
}

TEST_CASE("Revision caret skip (deleted text is non-editable)",
          "[revisions]") {
    TextBuffer buffer;
    buffer.setText("Hello world");
    std::vector<Revision> revs{deleteRev(6, "world")};

    SECTION("forward move landing in [s, e) jumps to e") {
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 8, true) ==
                11);
        // Landing exactly at the mark's start skips forward too.
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 6, true) ==
                11);
    }
    SECTION("forward move landing at e stays") {
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 11, true) ==
                11);
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 3, true) ==
                3);
    }
    SECTION("backward move landing in (s, e] jumps to s") {
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 8, false) ==
                6);
        // Landing exactly at the mark's end skips backward too.
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 11, false) ==
                6);
    }
    SECTION("backward move landing at s stays") {
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 6, false) ==
                6);
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, revs, 3, false) ==
                3);
    }
    SECTION("a stale mark does not block the caret") {
        std::vector<Revision> stale{deleteRev(6, "WRONG")};
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, stale, 8, true) ==
                8);
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer, stale, 8, false) ==
                8);
    }
    SECTION("adjacent marks chain") {
        TextBuffer buffer2;
        buffer2.setText("abcd");
        std::vector<Revision> adjacent{deleteRev(0, "ab"),
                                       deleteRev(2, "cd")};
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer2, adjacent, 1,
                                                 true) == 4);
        REQUIRE(revisions::nextUnmarkedCaretPos(buffer2, adjacent, 3,
                                                 false) == 0);
    }
    SECTION("click snap goes to the nearer boundary") {
        REQUIRE(revisions::snapOffsetOutOfDeleteRanges(buffer, revs, 8) ==
                6);
        REQUIRE(revisions::snapOffsetOutOfDeleteRanges(buffer, revs, 9) ==
                11);
        // Boundaries pass through.
        REQUIRE(revisions::snapOffsetOutOfDeleteRanges(buffer, revs, 6) ==
                6);
        REQUIRE(revisions::snapOffsetOutOfDeleteRanges(buffer, revs, 11) ==
                11);
        REQUIRE(revisions::snapOffsetOutOfDeleteRanges(buffer, revs, 3) ==
                3);
    }
}

TEST_CASE("Revision Backspace/Delete targets skip live marks",
          "[revisions]") {
    SECTION("Backspace at caret right after a mark targets the char before it") {
        TextBuffer b;
        b.setText("abcXYdef");
        std::vector<Revision> revs{deleteRev(3, "XY")};
        // Caret at 5 (right after the mark): previous unmarked is 2.
        REQUIRE(revisions::prevUnmarkedChar(b, revs, 5) == 2);
        // Caret at 4 (inside the mark's span, one past its start):
        // still the char before the mark.
        REQUIRE(revisions::prevUnmarkedChar(b, revs, 4) == 2);
        // Caret at 3 (mark start): the char before the mark.
        REQUIRE(revisions::prevUnmarkedChar(b, revs, 3) == 2);
        // Caret at 2 (before the mark): normal Backspace target.
        REQUIRE(revisions::prevUnmarkedChar(b, revs, 2) == 1);
    }
    SECTION("Backspace at doc start has no target") {
        TextBuffer b;
        b.setText("abc");
        std::vector<Revision> revs;
        REQUIRE(revisions::prevUnmarkedChar(b, revs, 0) ==
                static_cast<std::size_t>(-1));
    }
    SECTION("Backspace with a mark reaching offset 0 has no target") {
        TextBuffer b;
        b.setText("abc");
        std::vector<Revision> revs{deleteRev(0, "ab")};
        REQUIRE(revisions::prevUnmarkedChar(b, revs, 2) ==
                static_cast<std::size_t>(-1));
    }
    SECTION("Delete at caret right before a mark targets the char after it") {
        TextBuffer b;
        b.setText("abcXYdef");
        std::vector<Revision> revs{deleteRev(3, "XY")};
        REQUIRE(revisions::nextUnmarkedChar(b, revs, 3) == 5);
        REQUIRE(revisions::nextUnmarkedChar(b, revs, 2) == 2);
    }
    SECTION("Delete at doc end has no target") {
        TextBuffer b;
        b.setText("abc");
        std::vector<Revision> revs;
        REQUIRE(revisions::nextUnmarkedChar(b, revs, 3) == 3);
    }
    SECTION("Delete with a mark reaching the end has no target") {
        TextBuffer b;
        b.setText("abc");
        std::vector<Revision> revs{deleteRev(1, "bc")};
        REQUIRE(revisions::nextUnmarkedChar(b, revs, 1) == 3);
    }
}

TEST_CASE("mergeDeleteMark (mark / merge semantics)", "[revisions]") {
    SECTION("marks a fresh range with the buffer substring") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs;
        auto result = revisions::mergeDeleteMark(buffer, revs, 6, 11);
        REQUIRE(result.merged.has_value());
        REQUIRE(result.merged->first == 6);
        REQUIRE(result.merged->second == 11);
        REQUIRE(result.revs.size() == 1);
        REQUIRE(result.revs[0].type == RevisionType::Delete);
        REQUIRE(result.revs[0].startOffset == 6);
        REQUIRE(result.revs[0].text == "world");
    }
    SECTION("adjacent marks merge into one") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs{deleteRev(6, "wor")};
        auto result = revisions::mergeDeleteMark(buffer, revs, 9, 11);
        REQUIRE(result.revs.size() == 1);
        REQUIRE(result.revs[0].startOffset == 6);
        REQUIRE(result.revs[0].text == "world");
        REQUIRE(result.merged->second == 11);
    }
    SECTION("overlapping marks merge into one") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs{deleteRev(6, "wor")};
        auto result = revisions::mergeDeleteMark(buffer, revs, 8, 11);
        REQUIRE(result.revs.size() == 1);
        REQUIRE(result.revs[0].text == "world");
    }
    SECTION("a bridging selection merges both neighbours (fixpoint)") {
        TextBuffer buffer;
        buffer.setText("abcdefgh");
        std::vector<Revision> revs{deleteRev(0, "ab"), deleteRev(5, "fg")};
        auto result = revisions::mergeDeleteMark(buffer, revs, 2, 5);
        REQUIRE(result.revs.size() == 1);
        REQUIRE(result.revs[0].startOffset == 0);
        REQUIRE(result.revs[0].text == "abcdefg");
    }
    SECTION("an Insert revision in the range is left alone") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(1, "b")};
        auto result = revisions::mergeDeleteMark(buffer, revs, 0, 3);
        REQUIRE(result.revs.size() == 2);
        REQUIRE(result.revs[0].type == RevisionType::Insert);
        REQUIRE(result.revs[1].type == RevisionType::Delete);
        REQUIRE(result.revs[1].text == "abc");
    }
    SECTION("re-marking an already-marked range is unchanged") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(0, "abc")};
        auto result = revisions::mergeDeleteMark(buffer, revs, 0, 3);
        REQUIRE(result.revs == revs);
        REQUIRE(result.merged.has_value());
    }
    SECTION("a stale mark does not merge") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(0, "WRONG")};
        auto result = revisions::mergeDeleteMark(buffer, revs, 0, 3);
        REQUIRE(result.revs.size() == 2);
        REQUIRE(result.revs[0].text == "WRONG");
        REQUIRE(result.revs[1].text == "abc");
    }
    SECTION("an empty range marks nothing") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs;
        auto result = revisions::mergeDeleteMark(buffer, revs, 1, 1);
        REQUIRE(result.revs.empty());
        REQUIRE_FALSE(result.merged.has_value());
    }
    SECTION("ranges clamp to the buffer") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs;
        auto result = revisions::mergeDeleteMark(buffer, revs, 0, 99);
        REQUIRE(result.revs[0].text == "abc");
    }
}

TEST_CASE("Marking is one undo step (RevisionListCommand)", "[revisions]") {
    SECTION("undo unmarks, redo re-marks") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs;
        hook(buffer, revs);
        std::vector<Revision> before = revs;
        auto merged = revisions::mergeDeleteMark(buffer, revs, 6, 11);
        REQUIRE(merged.merged.has_value());
        revs = merged.revs;
        buffer.commitRevisionList(&revs, before, revs);
        REQUIRE(revs.size() == 1);
        buffer.undo();
        REQUIRE(revs.empty());
        REQUIRE(buffer.getText() == "Hello world");
        buffer.redo();
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].text == "world");
    }
    SECTION("a no-op marking commits no undo step") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(0, "abc")};
        hook(buffer, revs);
        std::vector<Revision> before = revs;
        auto merged = revisions::mergeDeleteMark(buffer, revs, 0, 3);
        REQUIRE(merged.revs == before);
        buffer.commitRevisionList(&revs, before, merged.revs);
        REQUIRE_FALSE(buffer.canUndo());
    }
    SECTION("consecutive markings merge into one undo step (one run)") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs;
        hook(buffer, revs);
        // Five Backspaces, one per commit — the delete RUN is
        // one undo step: a single undo unmarks all of "world".
        for (std::size_t i = 0; i < 5; ++i) {
            std::vector<Revision> before = revs;
            auto merged = revisions::mergeDeleteMark(
                buffer, revs, 10 - i, 11 - i);
            REQUIRE(merged.merged.has_value());
            revs = merged.revs;
            buffer.commitRevisionList(&revs, before, revs);
        }
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].text == "world");
        buffer.undo();
        REQUIRE(revs.empty());
        REQUIRE(buffer.getText() == "Hello world");
        buffer.redo();
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].text == "world");
    }
    SECTION("markings separated by typing do not merge") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs;
        hook(buffer, revs);
        // Mark "a".
        {
            std::vector<Revision> before = revs;
            auto merged = revisions::mergeDeleteMark(buffer, revs, 0, 1);
            revs = merged.revs;
            buffer.commitRevisionList(&revs, before, revs);
        }
        // Type "X" (an Insert record joins the list, and a
        // buffer command joins the history).
        buffer.setCaret(buffer.positionForOffset(1));
        buffer.insertText("X");
        revs.push_back(insertRev(1, "X"));
        // Mark the typed "X".
        {
            std::vector<Revision> before = revs;
            auto merged = revisions::mergeDeleteMark(buffer, revs, 1, 2);
            revs = merged.revs;
            buffer.commitRevisionList(&revs, before, revs);
        }
        // One undo unmarks only the second run.
        buffer.undo();
        REQUIRE(revs.size() == 2);
        REQUIRE(revs[0].type == RevisionType::Delete);
        REQUIRE(revs[0].text == "a");
        REQUIRE(revs[1].type == RevisionType::Insert);
    }
}

TEST_CASE("Marks stay glued to buffer edits (edit-shift hook)",
          "[revisions]") {
    SECTION("typing before a mark shifts it") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs{deleteRev(6, "world")};
        hook(buffer, revs);
        buffer.setCaret({0, 0});
        buffer.insertText("Say: ");
        REQUIRE(buffer.getText() == "Say: Hello world");
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 11);
        REQUIRE(revs[0].text == "world");
    }
    SECTION("typing at a mark's end does not extend it") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs{deleteRev(6, "world")};
        hook(buffer, revs);
        buffer.setCaret(buffer.positionForOffset(11));
        buffer.insertText("!");
        REQUIRE(buffer.getText() == "Hello world!");
        REQUIRE(revs[0].startOffset == 6);
        REQUIRE(revs[0].text == "world");
        REQUIRE(revisions::bufferMatchesAt(buffer, 6, "world"));
    }
    SECTION("a real deletion overlapping a mark clips it") {
        TextBuffer buffer;
        buffer.setText("abcdef");
        std::vector<Revision> revs{deleteRev(1, "bcd")};
        hook(buffer, revs);
        // Delete "cd" at [2, 4): the mark keeps only its
        // surviving head "b" at [1, 2).
        buffer.replaceRange(2, 2, "");
        REQUIRE(buffer.getText() == "abef");
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 1);
        REQUIRE(revs[0].text == "b");
        REQUIRE(revisions::bufferMatchesAt(buffer, 1, "b"));
    }
    SECTION("undo of an edit re-anchors marks the other way") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c")};
        hook(buffer, revs);
        buffer.setCaret({0, 0});
        buffer.insertText("XY");
        REQUIRE(revs[0].startOffset == 4);
        // insertText is per-char history: two undos to rewind.
        buffer.undo();
        REQUIRE(buffer.getText() == "Xabc");
        REQUIRE(revs[0].startOffset == 3);
        buffer.undo();
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs[0].startOffset == 2);
        REQUIRE(revisions::bufferMatchesAt(buffer, 2, "c"));
    }
}

TEST_CASE("Revision grouping (dialog model)", "[revisions]") {
    SECTION("adjacent typed inserts group into one row") {
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   insertRev(2, "c")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].count == 3);
        REQUIRE(groups[0].type == RevisionType::Insert);
        REQUIRE(groups[0].startOffset == 0);
        REQUIRE(groups[0].text == "abc");
        TextBuffer buffer;
        buffer.setText("abc");
        REQUIRE(revisions::groupLabel(buffer, groups[0]) ==
                "Inserted \"abc\" — Ln 1");
    }
    SECTION("a record-only Backspace chain groups") {
        std::vector<Revision> revs{deleteRev(2, "c"), deleteRev(1, "b"),
                                   deleteRev(0, "a")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].count == 3);
        REQUIRE(groups[0].type == RevisionType::Delete);
        // Group text is document order; Ln comes from the FIRST
        // member's offset (2), still Ln 1.
        REQUIRE(groups[0].text == "abc");
        REQUIRE(groups[0].startOffset == 2);
        TextBuffer buffer;
        buffer.setText("abc");
        REQUIRE(revisions::groupLabel(buffer, groups[0]) ==
                "Deleted \"abc\" — Ln 1");
    }
    SECTION("a record-only forward-delete chain groups") {
        std::vector<Revision> revs{deleteRev(0, "a"), deleteRev(0, "b"),
                                   deleteRev(0, "c")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].count == 3);
        REQUIRE(groups[0].text == "abc");
    }
    SECTION("adjacent live Delete marks group") {
        std::vector<Revision> revs{deleteRev(0, "ab"), deleteRev(2, "cd")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].count == 2);
        REQUIRE(groups[0].text == "abcd");
        TextBuffer buffer;
        buffer.setText("abcd");
        REQUIRE(revisions::groupLabel(buffer, groups[0]) ==
                "Deleted \"abcd\" — Ln 1");
    }
    SECTION("live Delete marks with a gap do not group") {
        std::vector<Revision> revs{deleteRev(0, "ab"), deleteRev(4, "ef")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 2);
    }
    SECTION("adjacent live marks of different types do not group") {
        std::vector<Revision> revs{insertRev(0, "ab"), deleteRev(2, "cd")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 2);
    }
    SECTION("a type change breaks the group") {
        std::vector<Revision> revs{insertRev(0, "a"), deleteRev(1, "b"),
                                   insertRev(2, "c")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 3);
    }
    SECTION("a gap breaks the group") {
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(2, "b")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 2);
    }
    SECTION("a caret jump breaks the group") {
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(2, "b"),
                                   insertRev(3, "c")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 2);
        REQUIRE(groups[1].count == 2);
    }
    SECTION("a paste right after a typed run joins the group's row") {
        std::vector<Revision> revs{insertRev(0, "ab"),
                                   insertRev(2, "cdef")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].count == 2);
        REQUIRE(groups[0].text == "abcdef");
    }
    SECTION("same-offset inserts do not group (caret jump)") {
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(0, "b")};
        REQUIRE(revisions::groupRevisions(revs).size() == 2);
    }
    SECTION("grouping keeps the underlying revisions intact") {
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   deleteRev(2, "c")};
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 2);
        REQUIRE(revs.size() == 3);
        REQUIRE(revs[0].text == "a");
        REQUIRE(revs[1].text == "b");
    }
    SECTION("out-of-range group index is a no-op") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(0, "a")};
        hook(buffer, revs);
        REQUIRE_FALSE(
            revisions::acceptRevisionGroup(buffer, revs, 1));
        REQUIRE_FALSE(
            revisions::rejectRevisionGroup(buffer, revs, 99));
        REQUIRE(revs.size() == 1);
    }
}

TEST_CASE("Revision group accept/reject (one undo step)", "[revisions]") {
    SECTION("reject an insert group clears typed text") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   insertRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "");
        REQUIRE(revs.empty());
        buffer.undo();
        REQUIRE(buffer.getText() == "abc");
    }
    SECTION("accept an insert group keeps the text and records no undo step") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   insertRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
        REQUIRE_FALSE(buffer.canUndo());
    }
    SECTION("accept an insert group restores missing text") {
        TextBuffer buffer;
        buffer.setText("");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
        buffer.undo();
        REQUIRE(buffer.getText() == "");
    }
    SECTION("reject a record-only Backspace delete group restores it") {
        TextBuffer buffer;
        buffer.setText("a");
        std::vector<Revision> revs{deleteRev(2, "c"), deleteRev(1, "b"),
                                   deleteRev(0, "a")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
        buffer.undo();
        REQUIRE(buffer.getText() == "a");
    }
    SECTION("reject a record-only forward-delete group restores in order") {
        TextBuffer buffer;
        buffer.setText("");
        std::vector<Revision> revs{deleteRev(0, "a"), deleteRev(0, "b"),
                                   deleteRev(0, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abc");
        REQUIRE(revs.empty());
        buffer.undo();
        REQUIRE(buffer.getText() == "");
    }
    SECTION("accept a record-only delete group keeps the deletion") {
        TextBuffer buffer;
        buffer.setText("abc");
        std::vector<Revision> revs{deleteRev(2, "c"), deleteRev(1, "b"),
                                   deleteRev(0, "a")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "");
        REQUIRE(revs.empty());
        buffer.undo();
        REQUIRE(buffer.getText() == "abc");
    }
    SECTION("accept a LIVE delete group really deletes the marked text") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs{deleteRev(6, "world")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "Hello ");
        REQUIRE(revs.empty());
        buffer.undo();
        REQUIRE(buffer.getText() == "Hello world");
    }
    SECTION("reject a LIVE delete group unmarks: text stays plain") {
        TextBuffer buffer;
        buffer.setText("Hello world");
        std::vector<Revision> revs{deleteRev(6, "world")};
        hook(buffer, revs);
        REQUIRE(revisions::rejectRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "Hello world");
        REQUIRE(revs.empty());
        REQUIRE_FALSE(buffer.canUndo());
    }
    SECTION("accept a live delete group re-anchors a later revision") {
        TextBuffer buffer;
        buffer.setText("abcdef");
        std::vector<Revision> revs{deleteRev(2, "c"), insertRev(5, "f")};
        hook(buffer, revs);
        // One group for the delete (group 0) + one for the insert.
        const auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 2);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "abdef");
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].type == RevisionType::Insert);
        REQUIRE(revs[0].startOffset == 4);
        REQUIRE(revisions::bufferMatchesAt(buffer, 4, "f"));
    }
    SECTION("group op re-anchors a later revision and leaves the tail") {
        // "abc X": the space at 3 is untracked, so the X at 4 is
        // its own group; rejecting the "abc" group leaves " X"
        // and shifts the X record 4 -> 1.
        TextBuffer buffer;
        buffer.setText("abc X");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   insertRev(2, "c"), insertRev(4, "X")};
        hook(buffer, revs);
        auto groups = revisions::groupRevisions(revs);
        REQUIRE(groups.size() == 2);
        REQUIRE(revisions::rejectRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == " X");
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].startOffset == 1);
    }
    SECTION("accepting one group leaves the other group alone") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{insertRev(0, "a"), insertRev(1, "b"),
                                   deleteRev(2, "c")};
        hook(buffer, revs);
        REQUIRE(revisions::groupRevisions(revs).size() == 2);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.size() == 1);
        REQUIRE(revs[0].type == RevisionType::Delete);
    }
    SECTION("accept a delete group then undo restores the text") {
        TextBuffer buffer;
        buffer.setText("ab");
        std::vector<Revision> revs{deleteRev(1, "b")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "a");
        buffer.undo();
        REQUIRE(buffer.getText() == "ab");
        REQUIRE(revs.empty());
    }
    SECTION("a group with no net buffer change drops its records only") {
        TextBuffer buffer;
        buffer.setText("hello");
        std::vector<Revision> revs{insertRev(0, "hello")};
        hook(buffer, revs);
        REQUIRE(revisions::acceptRevisionGroup(buffer, revs, 0));
        REQUIRE(buffer.getText() == "hello");
        REQUIRE(revs.empty());
        REQUIRE_FALSE(buffer.canUndo());
    }
}
