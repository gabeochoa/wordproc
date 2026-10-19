#pragma once

// Suggesting mode (Docs suggestion mode on top of Revisions): pure
// accept/reject over a TextBuffer + revision list.
//
// Model (Batch 14: deletions are marked IN PLACE):
// - Typing with Track Changes on inserts text normally and records
//   Revision::Insert {offset, text}.
// - Backspace/Delete with Track Changes on does NOT remove text:
//   it marks the target range as Revision::Delete, whose
//   [startOffset, startOffset + text.size()) is a LIVE range in
//   the buffer — the text stays, renders red + strikethrough, and
//   the caret skips it. Accept on a Delete really deletes the
//   text; Reject unmarks it (text plain again).
// - Replace-paths (Spelling / Grammar / Thesaurus / AutoCorrect /
//   Find-Replace / chips) still record a Revision::Delete for the
//   replaced text AND remove it: those Delete records are
//   record-only (their text is absent from the buffer). Accept on
//   one is a no-op for the buffer (the text is already gone);
//   Reject puts the text back.
// - Accept makes the change permanent; Reject makes it as if the
//   change never happened. Each accept/reject is one undo step
//   (buffer.replaceRange). Marking itself is one undo step
//   (RevisionListCommand in the buffer's history; see
//   TextBuffer::commitRevisionList): undo of a marking unmarks.
//
// Verification: a revision is LIVE when its text still matches the
// buffer at its offset (bufferMatchesAt). Only live revisions
// render, block the caret, or merge with a new mark. A revision
// whose text no longer matches (later edits, undo, a record-only
// Delete) is stale: its buffer op is skipped on accept, but on
// reject a stale Delete restores its text (the record-only case)
// while a live Delete is just unmarked. The match gate is what
// separates the two flavours — the same code handles both.
//
// Offsets: every buffer edit re-anchors the revisions through
// TextBuffer's edit-shift hook (installed by the owning
// DocumentComponent, or by installShiftHook in tests), which
// calls shiftForEdit — the single shift rule, the analogue of
// TextBuffer::adjustCharStyleSpans for spans. Accept/reject do NOT
// shift manually: their buffer ops fire the hook. Shift rules:
// - Insert len at off: a Delete mark starting at or after off
//   shifts (typed text before the marked word is not part of the
//   deletion); insertion strictly inside a Delete mark, or at
//   its end, does NOT extend it (typed text at the boundary
//   inserts outside the mark).
// - An Insert revision starting strictly AFTER off shifts; one
//   starting AT off stays (it refers to the text at that point,
//   e.g. an Insert for the very text re-inserted by rejecting
//   a Delete).
// - Delete [off, off+len): a Delete mark overlapping the range is
//   CLIPPED (the overlapped chars leave its text; a fully
//   overlapped mark is dropped); a mark after the range shifts by
//   -len. An Insert revision at/after off+len shifts by -len;
//   one starting inside the deleted range clamps to off.
// Anything the rule gets wrong is caught by the match gate above:
// the op is skipped rather than applied at a wrong offset.
// One known asymmetry, same class as the CharStyleSpan one
// (undo of a delete of a span's first char lands outside the
// span): undo of a real deletion that overlapped a mark
// re-inserts inside the clipped mark, which does not extend to
// re-cover it — that mark goes stale rather than swallowing
// text it should not own. Outline swaps
// (swapAdjacentLineRanges) do not retarget marks at all —
// like hyperlinks/bookmarks there — so a swapped mark goes
// stale rather than pointing at the wrong text.

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "document_settings.h"
#include "text_buffer.h"

namespace revisions {

// Does the buffer still hold `text` at `offset`?
inline bool bufferMatchesAt(const TextBuffer& buffer, std::size_t offset,
                            const std::string& text) {
    if (text.empty()) return offset <= buffer.textSize();
    if (offset > buffer.textSize() ||
        text.size() > buffer.textSize() - offset) {
        return false;
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (buffer.charAtOffset(offset + i) != text[i]) return false;
    }
    return true;
}

// Short excerpt for dialog rows: newlines flattened, capped.
inline std::string excerptText(const std::string& text,
                               std::size_t maxLen = 24) {
    std::string out;
    out.reserve(std::min(text.size(), maxLen));
    for (char ch : text) {
        if (out.size() >= maxLen) break;
        out.push_back(ch == '\n' || ch == '\r' ? ' ' : ch);
    }
    if (text.size() > maxLen) out += "...";
    return out;
}

inline std::string excerpt(const Revision& rev,
                           std::size_t maxLen = 24) {
    return excerptText(rev.text, maxLen);
}

// Render classification for tracked inserts: the render pass draws
// text inside a verified Insert span in revision green (#008000)
// with a green underline, and spell squiggles lose to the span
// (a squiggle overlapping a span is skipped, like selection wins
// over squiggles). A span is verified only when its text still
// matches the buffer at its offset; deletes never classify (they
// have no in-text render), and a stale insert classifies nothing
// rather than colouring the wrong text.
inline std::vector<std::pair<std::size_t, std::size_t>>
verifiedInsertRanges(const TextBuffer& buffer,
                     const std::vector<Revision>& revs) {
    std::vector<std::pair<std::size_t, std::size_t>> out;
    for (const auto& rev : revs) {
        if (rev.type != RevisionType::Insert) continue;
        if (bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            out.emplace_back(rev.startOffset,
                             rev.startOffset + rev.text.size());
        }
    }
    return out;
}

// Is buffer offset `off` inside any verified insert range
// ([first, second) — the end offset itself is outside)?
inline bool offsetInInsertRanges(
    std::size_t off,
    const std::vector<std::pair<std::size_t, std::size_t>>& ranges) {
    for (const auto& range : ranges) {
        if (off >= range.first && off < range.second) return true;
    }
    return false;
}

// Render classification for tracked deletions: the render pass
// draws text inside a verified Delete span in revision red
// (#FF0000) with a strikethrough, winning over the spell
// squiggle and over an overlapping Insert span (inserted-then-
// deleted text reads as deleted), losing to the selection
// highlight like inserts do. Only LIVE Delete marks classify:
// a record-only Delete (text absent) has no in-text render.
// A Delete mark's end is computed, not stored: startOffset +
// text.size().
inline std::vector<std::pair<std::size_t, std::size_t>>
verifiedDeleteRanges(const TextBuffer& buffer,
                     const std::vector<Revision>& revs) {
    std::vector<std::pair<std::size_t, std::size_t>> out;
    for (const auto& rev : revs) {
        if (rev.type != RevisionType::Delete) continue;
        if (rev.text.empty()) continue;
        if (bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            out.emplace_back(rev.startOffset,
                             rev.startOffset + rev.text.size());
        }
    }
    return out;
}

// Is buffer offset `off` inside any verified delete range
// ([first, second) — the end offset itself is outside)?
inline bool offsetInDeleteRanges(
    std::size_t off,
    const std::vector<std::pair<std::size_t, std::size_t>>& ranges) {
    for (const auto& range : ranges) {
        if (off >= range.first && off < range.second) return true;
    }
    return false;
}

// Simulation anchor rules (applyGroup ONLY): re-anchor a
// list of revision offsets for an insert / delete at `pos`,
// treating every revision as an ANCHOR, not a live range —
// an insert at an anchor's offset does NOT shift it (so a
// same-offset forward-delete chain restores "abc", not
// "cba"), and a delete clamps anchors inside it to its start.
// Distinct from shiftForEdit below: live Delete marks shift
// on insert-at-start and clip on overlap; anchors do neither.
inline void shiftSimForDelete(std::vector<Revision>& revs, std::size_t off,
                              std::size_t len) {
    for (auto& rev : revs) {
        if (rev.startOffset >= off + len) {
            rev.startOffset -= len;
        } else if (rev.startOffset >= off) {
            rev.startOffset = off;
        }
    }
}

inline void shiftSimForInsert(std::vector<Revision>& revs, std::size_t off,
                              std::size_t len) {
    for (auto& rev : revs) {
        if (rev.startOffset > off) rev.startOffset += len;
    }
}

// The one shift rule for a buffer edit, fired by TextBuffer's
// edit-shift hook on every text edit (insert of `delta` chars
// at `pos` when delta > 0; delete of -delta chars starting at
// `pos` when delta < 0). Mirrors
// TextBuffer::adjustCharStyleSpans with one deliberate
// difference: an insertion strictly inside a Delete mark does
// NOT extend it (a CharStyleSpan expands; a deletion mark must
// not swallow text that was never deleted).
//
// Which rule a Delete revision follows depends on whether it
// is LIVE (its text still matches the buffer at its offset —
// a mark whose text is really there) or RECORD-ONLY (the
// replace-paths' record of text that is already gone):
// - LIVE marks are ranges: insert at/before their start
//   shifts them; an overlapping real deletion CLIPS the
//   overlapped chars out of their text, and a fully
//   overlapped mark empties and is dropped.
// - RECORD-ONLY Deletes are anchors, like Insert revisions:
//   an insert strictly after their offset shifts them (at
//   the offset it stays), a deletion clamps them into its
//   start, and they are NEVER clipped or erased. Clipping
//   one would destroy the record: rejecting the Delete of
//   an autocorrected word ("baseline" -> "Baseline") must
//   still restore "baseline" after the replacement is
//   rejected — the hook must not erase the record when the
//   replacement text is deleted.
inline void shiftForEdit(const TextBuffer& buffer,
                         std::vector<Revision>& revs, std::size_t pos,
                         std::ptrdiff_t delta) {
    if (delta == 0) return;
    if (delta > 0) {
        const std::size_t count = static_cast<std::size_t>(delta);
        for (auto& rev : revs) {
            const bool liveDelete =
                rev.type == RevisionType::Delete &&
                !rev.text.empty() &&
                bufferMatchesAt(buffer, rev.startOffset, rev.text);
            if (liveDelete) {
                // Insertion at or before the mark's start shifts
                // the whole mark; inside or at the end: untouched.
                if (pos <= rev.startOffset) rev.startOffset += count;
            } else {
                // Insert revisions + record-only Deletes: a start
                // strictly after the insertion point shifts; at
                // the point it stays.
                if (rev.startOffset > pos) rev.startOffset += count;
            }
        }
        return;
    }
    const std::size_t count = static_cast<std::size_t>(-delta);
    const std::size_t delEnd = pos + count;
    for (std::size_t i = 0; i < revs.size();) {
        Revision& rev = revs[i];
        const bool liveDelete =
            rev.type == RevisionType::Delete && !rev.text.empty() &&
            bufferMatchesAt(buffer, rev.startOffset, rev.text);
        if (liveDelete) {
            const std::size_t revEnd = rev.startOffset + rev.text.size();
            if (revEnd <= pos) {
                ++i;
                continue;  // Mark entirely before the deletion.
            }
            if (rev.startOffset >= delEnd) {
                rev.startOffset -= count;  // Mark entirely after.
                ++i;
                continue;
            }
            // Overlap: clip the overlapped chars out of the
            // mark's text (same kept-before + kept-after math as
            // adjustCharStyleSpans). A fully overlapped mark
            // empties and is dropped.
            const std::size_t overlapStart =
                std::max(rev.startOffset, pos);
            const std::size_t overlapEnd = std::min(revEnd, delEnd);
            const std::size_t cutBegin = overlapStart - rev.startOffset;
            const std::size_t cutLen = overlapEnd - overlapStart;
            rev.text.erase(cutBegin, cutLen);
            if (rev.startOffset < pos) {
                // Kept head stays; the kept tail slides left.
            } else {
                rev.startOffset = pos;
            }
            if (rev.text.empty()) {
                revs.erase(revs.begin() +
                           static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
            continue;
        }
        // Insert revisions + record-only Deletes: the anchor
        // clamp rule — at/after the deleted range shifts,
        // inside clamps to its start. Never clipped/erased.
        if (rev.startOffset >= delEnd) {
            rev.startOffset -= count;
        } else if (rev.startOffset >= pos) {
            rev.startOffset = pos;
        }
        ++i;
    }
}

// Install the edit-shift hook on a standalone buffer (tests):
// every buffer edit re-anchors `revs` via shiftForEdit. The
// owning DocumentComponent installs the same hook on its own
// buffer in its constructor; accept/reject rely on the hook
// and never shift manually.
inline void installShiftHook(TextBuffer& buffer,
                             std::vector<Revision>& revs) {
    buffer.setEditShiftHook(
        [&buffer, &revs](std::size_t pos, std::ptrdiff_t delta) {
            shiftForEdit(buffer, revs, pos, delta);
        });
}

// Caret skip: the deleted text is non-editable, so the caret
// may never rest strictly inside a live Delete mark. A forward
// move landing in [s, e) jumps to e; a backward move landing
// in (s, e] jumps to s. Boundaries are legal caret spots
// (typing at a mark's start inserts before it — the mark
// shifts; typing at its end inserts after it). Repeats for
// adjacent / overlapping marks. Only live marks block.
inline std::size_t nextUnmarkedCaretPos(const TextBuffer& buffer,
                                        const std::vector<Revision>& revs,
                                        std::size_t offset, bool forward) {
    for (std::size_t guard = 0; guard <= revs.size(); ++guard) {
        bool moved = false;
        for (const auto& rev : revs) {
            if (rev.type != RevisionType::Delete || rev.text.empty()) {
                continue;
            }
            const std::size_t s = rev.startOffset;
            const std::size_t e = s + rev.text.size();
            if (!bufferMatchesAt(buffer, s, rev.text)) continue;
            if (forward) {
                if (offset >= s && offset < e) {
                    offset = e;
                    moved = true;
                }
            } else {
                if (offset > s && offset <= e) {
                    offset = s;
                    moved = true;
                }
            }
        }
        if (!moved) break;
    }
    return offset;
}

// Backspace target: the previous UNMARKED char before `off`
// (off is the caret offset), skipping live Delete marks
// backwards (a mark ending at the caret is jumped over, so
// Backspace at caret right after a mark deletes the previous
// unmarked char). Returns std::size_t(-1) when there is no
// such char (caret at the document start, or a mark reaching
// offset 0 covers everything before it). Pure (no commit):
// the command layer (cmd::backspace) marks the returned char.
inline std::size_t prevUnmarkedChar(const TextBuffer& buffer,
                                    const std::vector<Revision>& revs,
                                    std::size_t off) {
    if (off == 0) return static_cast<std::size_t>(-1);
    std::size_t candidate = off - 1;
    for (std::size_t guard = 0; guard <= revs.size(); ++guard) {
        bool inside = false;
        for (const auto& rev : revs) {
            if (rev.type != RevisionType::Delete || rev.text.empty()) {
                continue;
            }
            const std::size_t s = rev.startOffset;
            const std::size_t e = s + rev.text.size();
            if (candidate >= s && candidate < e &&
                bufferMatchesAt(buffer, s, rev.text)) {
                if (s == 0) return static_cast<std::size_t>(-1);
                candidate = s - 1;
                inside = true;
                break;
            }
        }
        if (!inside) break;
    }
    return candidate;
}

// Forward-Delete target: the next UNMARKED char at/after `off`
// (off is the caret offset), skipping live Delete marks
// forwards (a mark starting at the caret is jumped over).
// Returns buffer.textSize() when there is no such char (caret
// at the document end, or a mark reaching the end covers
// everything after it).
inline std::size_t nextUnmarkedChar(const TextBuffer& buffer,
                                    const std::vector<Revision>& revs,
                                    std::size_t off) {
    std::size_t candidate = off;
    for (std::size_t guard = 0; guard <= revs.size(); ++guard) {
        bool inside = false;
        for (const auto& rev : revs) {
            if (rev.type != RevisionType::Delete || rev.text.empty()) {
                continue;
            }
            const std::size_t s = rev.startOffset;
            const std::size_t e = s + rev.text.size();
            if (candidate >= s && candidate < e &&
                bufferMatchesAt(buffer, s, rev.text)) {
                candidate = e;
                inside = true;
                break;
            }
        }
        if (!inside) break;
    }
    return candidate;
}

// Pure merge for marking [start, end) as a tracked deletion:
// the revision list after the marking, plus the merged range
// (nullopt when the input range is empty). Overlaps /
// adjacency with verified Delete marks merge into one
// revision (union, text = the buffer substring). Stale Delete
// records are left alone. Returns the input unchanged when
// the range is empty or was already exactly marked (the
// caller commits only when the list changed, so re-marking
// adds no undo step). No buffer mutation, no commit — the
// command layer (cmd::markDeletedRange) owns those.
struct DeleteMarkResult {
    std::vector<Revision> revs;
    std::optional<std::pair<std::size_t, std::size_t>> merged;
};
inline DeleteMarkResult mergeDeleteMark(
    const TextBuffer& buffer, const std::vector<Revision>& revs,
    std::size_t start, std::size_t end) {
    const std::size_t size = buffer.textSize();
    start = std::min(start, size);
    end = std::min(end, size);
    if (end <= start) return DeleteMarkResult{revs, std::nullopt};
    // Union with every verified Delete mark the range touches,
    // to a fixpoint: absorbing one mark can grow the range to
    // touch the next (a bridging selection over two marks).
    std::vector<bool> absorbed(revs.size(), false);
    std::size_t mergedStart = start;
    std::size_t mergedEnd = end;
    bool grew = true;
    while (grew) {
        grew = false;
        for (std::size_t i = 0; i < revs.size(); ++i) {
            if (absorbed[i]) continue;
            const Revision& rev = revs[i];
            if (rev.type != RevisionType::Delete || rev.text.empty()) {
                continue;
            }
            const std::size_t revEnd = rev.startOffset + rev.text.size();
            if (rev.startOffset <= mergedEnd && revEnd >= mergedStart &&
                bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
                mergedStart = std::min(mergedStart, rev.startOffset);
                mergedEnd = std::max(mergedEnd, revEnd);
                absorbed[i] = true;
                grew = true;
            }
        }
    }
    // Already exactly marked (one absorbed mark, same range):
    // the list is unchanged.
    std::size_t absorbedCount = 0;
    for (bool b : absorbed) absorbedCount += b ? 1 : 0;
    if (absorbedCount == 1) {
        for (std::size_t i = 0; i < revs.size(); ++i) {
            if (absorbed[i] &&
                revs[i].startOffset == mergedStart &&
                revs[i].startOffset + revs[i].text.size() == mergedEnd) {
                return DeleteMarkResult{
                    revs, std::make_pair(mergedStart, mergedEnd)};
            }
        }
    }
    std::vector<Revision> after;
    after.reserve(revs.size() + 1);
    for (std::size_t i = 0; i < revs.size(); ++i) {
        if (!absorbed[i]) after.push_back(revs[i]);
    }
    std::string text;
    text.reserve(mergedEnd - mergedStart);
    for (std::size_t i = mergedStart; i < mergedEnd; ++i) {
        text.push_back(buffer.charAtOffset(i));
    }
    Revision rev;
    rev.type = RevisionType::Delete;
    rev.startOffset = mergedStart;
    rev.text = std::move(text);
    rev.author = "Gabe";
    rev.timestamp = 0;  // The command layer stamps it.
    after.push_back(std::move(rev));
    return DeleteMarkResult{std::move(after),
                            std::make_pair(mergedStart, mergedEnd)};
}

// Click / undo landing: an offset strictly inside a live Delete
// mark snaps to the NEARER boundary (ties go to the start, so a
// click on the first half of a short mark lands before it).
// Boundary offsets pass through unchanged.
inline std::size_t snapOffsetOutOfDeleteRanges(
    const TextBuffer& buffer, const std::vector<Revision>& revs,
    std::size_t offset) {
    std::size_t best = offset;
    std::size_t bestDist = 0;
    bool found = false;
    for (const auto& rev : revs) {
        if (rev.type != RevisionType::Delete || rev.text.empty()) {
            continue;
        }
        const std::size_t s = rev.startOffset;
        const std::size_t e = s + rev.text.size();
        if (!bufferMatchesAt(buffer, s, rev.text)) continue;
        if (offset > s && offset < e) {
            const std::size_t toStart = offset - s;
            const std::size_t toEnd = e - offset;
            const std::size_t dist = std::min(toStart, toEnd);
            const std::size_t candidate =
                toStart <= toEnd ? s : e;
            if (!found || dist < bestDist) {
                found = true;
                bestDist = dist;
                best = candidate;
            }
        }
    }
    return best;
}

// Human dialog row label: `Inserted "abc" — Ln 3` /
// `Deleted "abc" — Ln 3`. The line is 1-based, from the revision's
// start offset (positionForOffset clamps a past-end offset to
// the last line, so a stale offset still yields a sane line).
// The excerpt caps at ~24 chars with "..." (see excerpt).
inline std::string dialogLabel(const TextBuffer& buffer,
                               const Revision& rev) {
    const std::size_t line =
        buffer.positionForOffset(rev.startOffset).row + 1;
    return std::string(rev.type == RevisionType::Insert ? "Inserted"
                                                        : "Deleted") +
           " \"" + excerpt(rev) + "\" — Ln " + std::to_string(line);
}

// Dialog row label: Insert — Gabe — "abc".
inline std::string label(const Revision& rev) {
    return std::string(rev.type == RevisionType::Insert ? "Insert"
                                                        : "Delete") +
           " — " + rev.author + " — \"" + excerpt(rev) + "\"";
}

// Dialog-model grouping: ADJACENT contiguous revisions of the same
// type collapse into one display row (`Inserted "abc"` for a typed
// run), while per-revision data stays intact in `revs` — a group
// only indexes its members. Contiguity is by recorded offsets:
// - Insert: the next revision starts where the previous one ends
//   (sequential typing, a paste right after a typed run).
// - Delete: the next revision's live range starts where the
//   previous one's ends (two merged marks, adjacent in the
//   buffer); or the next revision deletes the char just before
//   the previous delete (a record-only Backspace chain) or the
//   char at the same offset (a record-only forward-Delete /
//   selection chain, where the text shifts left under the
//   caret). Live marks merge on creation, so a live Delete group
//   is usually one revision; the chain rules cover the
//   record-only Delete revisions the replace-paths still write.
// A type change, a gap, an overlap, or a caret jump breaks the
// group. Group text is the members' texts in document order
// (Backspace deletes arrive in reverse document order; same-offset
// deletes keep list order, which is document order there).
struct RevisionGroup {
    std::size_t firstIndex = 0;  // index of the first member in revs
    std::size_t count = 0;       // members, all adjacent in revs
    RevisionType type = RevisionType::Insert;
    // First member's startOffset: Ln is computed from this.
    std::size_t startOffset = 0;
    // Member texts concatenated in document order.
    std::string text;
};

inline std::vector<RevisionGroup> groupRevisions(
    const std::vector<Revision>& revs) {
    std::vector<RevisionGroup> groups;
    for (std::size_t i = 0; i < revs.size(); ++i) {
        const Revision& rev = revs[i];
        bool contiguous = false;
        if (!groups.empty() && rev.type == groups.back().type) {
            const Revision& prev = revs[i - 1];
            if (rev.type == RevisionType::Insert) {
                contiguous = rev.startOffset ==
                             prev.startOffset + prev.text.size();
            } else {
                contiguous = rev.startOffset ==
                                 prev.startOffset + prev.text.size() ||
                             rev.startOffset + rev.text.size() ==
                                 prev.startOffset ||
                             rev.startOffset == prev.startOffset;
            }
        }
        if (contiguous) {
            ++groups.back().count;
            continue;
        }
        RevisionGroup group;
        group.firstIndex = i;
        group.count = 1;
        group.type = rev.type;
        group.startOffset = rev.startOffset;
        groups.push_back(std::move(group));
    }
    for (auto& group : groups) {
        std::vector<std::size_t> members;
        members.reserve(group.count);
        for (std::size_t k = 0; k < group.count; ++k) {
            members.push_back(group.firstIndex + k);
        }
        std::stable_sort(members.begin(), members.end(),
                         [&](std::size_t a, std::size_t b) {
                             return revs[a].startOffset <
                                    revs[b].startOffset;
                         });
        group.text.clear();
        for (std::size_t idx : members) group.text += revs[idx].text;
    }
    return groups;
}

// Group dialog row label: `Inserted "abc" — Ln 1`. The line comes
// from the group's first member's offset (see RevisionGroup).
inline std::string groupLabel(const TextBuffer& buffer,
                              const RevisionGroup& group) {
    const std::size_t line =
        buffer.positionForOffset(group.startOffset).row + 1;
    return std::string(group.type == RevisionType::Insert ? "Inserted"
                                                           : "Deleted") +
           " \"" + excerptText(group.text) + "\" — Ln " +
           std::to_string(line);
}

// Accept one revision: make its change permanent. Insert: if the
// text is missing (e.g. undone), put it back. Delete: if the text
// is still present (a live mark), remove it now; if absent (a
// record-only Delete) the deletion already happened. Drops the
// record either way. The buffer op fires the edit-shift hook,
// which re-anchors the remaining revisions — no manual shift
// here (the record is erased first, so the hook never sees it).
inline bool acceptRevision(TextBuffer& buffer, std::vector<Revision>& revs,
                           std::size_t index) {
    if (index >= revs.size()) return false;
    Revision rev = revs[index];
    revs.erase(revs.begin() + static_cast<std::ptrdiff_t>(index));
    if (rev.type == RevisionType::Insert) {
        if (!bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            const std::size_t off =
                std::min(rev.startOffset, buffer.textSize());
            buffer.replaceRange(off, 0, rev.text);
        }
    } else {
        if (bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            buffer.replaceRange(rev.startOffset, rev.text.size(), "");
        }
    }
    return true;
}

// Reject one revision: make it as if the change never happened.
// Insert: remove the text if still present. Delete: a live mark
// is just unmarked (the text stays, plain again); a record-only
// Delete puts its text back if absent. Drops the record either
// way. Same hook rule as acceptRevision: no manual shift.
inline bool rejectRevision(TextBuffer& buffer, std::vector<Revision>& revs,
                           std::size_t index) {
    if (index >= revs.size()) return false;
    Revision rev = revs[index];
    revs.erase(revs.begin() + static_cast<std::ptrdiff_t>(index));
    if (rev.type == RevisionType::Insert) {
        if (bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            buffer.replaceRange(rev.startOffset, rev.text.size(), "");
        }
    } else {
        if (!bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            const std::size_t off =
                std::min(rev.startOffset, buffer.textSize());
            buffer.replaceRange(off, 0, rev.text);
        }
    }
    return true;
}

inline void acceptAllRevisions(TextBuffer& buffer,
                               std::vector<Revision>& revs) {
    while (!revs.empty()) acceptRevision(buffer, revs, 0);
}

inline void rejectAllRevisions(TextBuffer& buffer,
                               std::vector<Revision>& revs) {
    for (std::size_t i = revs.size(); i-- > 0;) {
        rejectRevision(buffer, revs, i);
    }
}

// Does `text` still hold `needle` at `offset`? bufferMatchesAt
// over a plain string, for the group simulation below.
inline bool stringMatchesAt(const std::string& text, std::size_t offset,
                            const std::string& needle) {
    if (needle.empty()) return offset <= text.size();
    if (offset > text.size() || needle.size() > text.size() - offset) {
        return false;
    }
    return text.compare(offset, needle.size(), needle) == 0;
}

// Accept/Reject one GROUP (groupRevisions index): the op applies
// to all its members, as ONE undo step.
//
// Implementation: simulate the per-member ops (same match gates
// and re-anchoring as acceptRevision/rejectRevision) on a string
// copy of the buffer plus working copies of the members and of
// the OTHER revisions (the members are resolved by the op, so
// only the others survive — split out up front). The net effect
// is then folded into a single replaceRange over the one span
// the group's ops can touch — one ReplaceRangesCommand, so one
// undo restores the pre-group buffer. Members whose text no
// longer matches are skipped, exactly as the per-revision op
// would skip them, and a group with no net buffer change drops
// its records without recording a no-op undo step.
// Two shift rules meet here, and they must not be mixed:
// - The MEMBERS' offsets during the simulation are anchors
//   in the simulated string: they shift with the old
//   anchor rules (shiftSimForInsert / shiftSimForDelete —
//   an insert at a member's offset does NOT shift it, so a
//   same-offset forward-delete chain restores back to front
//   as "abc", not "cba"). A live-mark rule (insert at the
//   start shifts) would break that restore order.
// - The SURVIVORS are re-anchored once, by the FOLDED op
//   (delete the span, then insert the new span text) with
//   the live shiftForEdit — exactly what the hook does to the
//   real list during the folded replaceRange (whose hook
//   effects are discarded: `revs = others` carries the
//   simulation's result). Shifting survivors per member op
//   would be wrong: a survivor overlapping the group span
//   would be clipped once per member op instead of once for
//   the folded span.
// If the folded span ever escapes the simulation (unreachable
// for disjoint contiguous members), fall back to the sequential
// per-member ops, which preserve semantics at the cost of one
// undo step per member.
// Order: accept processes members in list order; reject in
// REVERSE list order (rejectAll's order) — a same-offset
// forward-delete chain restored front-first comes out reversed
// ("cba" for "abc"), because re-inserting at the same offset
// does not shift its same-offset siblings.
inline bool applyGroup(TextBuffer& buffer, std::vector<Revision>& revs,
                       std::size_t groupIndex, bool accept) {
    const std::vector<RevisionGroup> groups = groupRevisions(revs);
    if (groupIndex >= groups.size()) return false;
    const RevisionGroup group = groups[groupIndex];
    const std::string original = buffer.getText();
    const std::size_t origSize = original.size();

    std::size_t minStart = revs[group.firstIndex].startOffset;
    std::size_t totalLen = 0;
    for (std::size_t k = 0; k < group.count; ++k) {
        const Revision& member = revs[group.firstIndex + k];
        minStart = std::min(minStart, member.startOffset);
        totalLen += member.text.size();
    }
    const std::size_t spanStart = std::min(minStart, origSize);
    std::size_t spanOrigLen = std::min(totalLen, origSize - spanStart);
    std::size_t spanEnd = spanStart + spanOrigLen;

    // Members (copies, re-anchored by earlier member ops as the
    // simulation runs) and the surviving others (re-anchored the
    // same way; the members are dropped at the end).
    std::vector<Revision> members;
    members.reserve(group.count);
    for (std::size_t k = 0; k < group.count; ++k) {
        members.push_back(revs[group.firstIndex + k]);
    }
    std::vector<Revision> others;
    others.reserve(revs.size() - group.count);
    for (std::size_t i = 0; i < revs.size(); ++i) {
        if (i < group.firstIndex ||
            i >= group.firstIndex + group.count) {
            others.push_back(revs[i]);
        }
    }
    std::string text = original;
    for (std::size_t t = 0; t < group.count; ++t) {
        const std::size_t k =
            accept ? t : (group.count - 1 - t);
        const Revision member = members[k];
        const bool matches =
            stringMatchesAt(text, member.startOffset, member.text);
        bool doInsert = false;
        bool doDelete = false;
        if (group.type == RevisionType::Insert) {
            if (accept) {
                doInsert = !matches;
            } else {
                doDelete = matches;
            }
        } else {
            if (accept) {
                doDelete = matches;
            } else {
                doInsert = !matches;
            }
        }
        if (!doInsert && !doDelete) continue;
        std::size_t opOff = member.startOffset;
        std::size_t oldLen = 0;
        std::string newText;
        if (doDelete) {
            oldLen = member.text.size();
        } else {
            opOff = std::min(opOff, text.size());
            newText = member.text;
        }
        // Keep the span covering the op: an op past the span end
        // pulls the tail chars between into the span.
        if (opOff > spanEnd) {
            spanOrigLen += opOff - spanEnd;
            spanEnd = opOff;
        }
        if (opOff + oldLen > spanEnd) {
            spanOrigLen += opOff + oldLen - spanEnd;
            spanEnd = opOff + oldLen;
        }
        text.replace(opOff, oldLen, newText);
        // Member anchors shift with the simulation rules only;
        // the survivors shift once, by the folded op, below.
        if (doDelete) {
            shiftSimForDelete(members, opOff, oldLen);
        } else {
            shiftSimForInsert(members, opOff, newText.size());
        }
        // Signed delta: a deletion shrinks the span (new < old).
        spanEnd = static_cast<std::size_t>(
            static_cast<std::ptrdiff_t>(spanEnd) +
            static_cast<std::ptrdiff_t>(newText.size()) -
            static_cast<std::ptrdiff_t>(oldLen));
    }
    const bool safe = spanOrigLen <= origSize - spanStart &&
                      spanEnd >= spanStart &&
                      spanEnd - spanStart <= text.size() - spanStart;
    if (!safe) {
        // Sequential fallback in the same orders as the
        // simulation above: accept front-first (each op slides
        // the next member into firstIndex); reject last-first
        // (erasing a later member leaves earlier indices alone).
        for (std::size_t t = 0; t < group.count; ++t) {
            const std::size_t index =
                accept ? group.firstIndex
                       : group.firstIndex + group.count - 1 - t;
            const bool ok = accept
                                ? acceptRevision(buffer, revs, index)
                                : rejectRevision(buffer, revs, index);
            if (!ok) return false;
        }
        return true;
    }
    if (text != original) {
        const std::string newSpanText =
            text.substr(spanStart, spanEnd - spanStart);
        // Survivors shift once, by the folded op (delete the
        // span, then insert the new span text) — the live rule,
        // exactly what the hook does to the real list during
        // the replaceRange below (whose hook effects are
        // discarded: `revs = others` carries this result).
        // A survivor clipped to empty is dropped: its text
        // really left the buffer.
        shiftForEdit(buffer, others, spanStart,
                     -static_cast<std::ptrdiff_t>(spanOrigLen));
        shiftForEdit(buffer, others, spanStart,
                     static_cast<std::ptrdiff_t>(newSpanText.size()));
        buffer.replaceRange(spanStart, spanOrigLen, newSpanText);
    }
    revs = others;
    return true;
}

// Accept one group: make all its members' changes permanent.
inline bool acceptRevisionGroup(TextBuffer& buffer,
                               std::vector<Revision>& revs,
                               std::size_t groupIndex) {
    return applyGroup(buffer, revs, groupIndex, true);
}

// Reject one group: make it as if none of its members happened.
inline bool rejectRevisionGroup(TextBuffer& buffer,
                               std::vector<Revision>& revs,
                               std::size_t groupIndex) {
    return applyGroup(buffer, revs, groupIndex, false);
}

}  // namespace revisions
