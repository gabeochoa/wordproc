#pragma once

// Suggesting mode (Docs suggestion mode on top of Revisions): pure
// accept/reject over a TextBuffer + revision list.
//
// Model (the buffer is ground truth; deletions are NOT kept visible
// in the buffer — see the note below):
// - Typing with Track Changes on inserts text normally and records
//   Revision::Insert {offset, text}.
// - Backspace/Delete removes text normally and records
//   Revision::Delete {offset, text}.
// - Accept makes the change permanent; Reject makes it as if the
//   change never happened. Each accept/reject is one undo step
//   (buffer.replaceRange).
//
// HONEST PARTIAL — keep-visible deletions: rendering deleted text
// red-strikethrough IN PLACE would need the gap buffer to keep text
// it has deleted (per-character deleted flags, or overlay text that
// shifts caret/selection/x math). That conflicts fundamentally with
// the gap buffer, so deletes are NOT rendered in the text: they are
// revision records listed in Tools > Revisions... (type, author,
// excerpt), where Accept/Reject act on them. Inserts ARE rendered:
// green underlined while their text still matches the buffer at
// their offset. A revision whose text no longer matches the buffer
// at its offset (later edits, undo) is stale: its buffer op is
// skipped, but the record is still dropped.
//
// Offsets: a revision's startOffset is in the buffer state when it
// was recorded. After an accept/reject changes the buffer, the
// remaining revisions are re-anchored with a simple rule that is
// exact for the common cases (sequential typing, a delete of a
// contiguous range):
// - an op deleting [off, off+len): others at/after off+len shift
//   by -len; others starting inside the deleted range clamp to off.
// - an op inserting len at off: others starting strictly AFTER off
//   shift by +len; others at off stay (they refer to the text at
//   that point, e.g. an Insert for the very text re-inserted by
//   rejecting a Delete).
// Anything the rule gets wrong is caught by the match gate above:
// the op is skipped rather than applied at a wrong offset.

#include <algorithm>
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
// - Delete: the next revision deletes the char just before the
//   previous delete (a Backspace chain) or the char at the same
//   offset (a forward-Delete / selection chain, where the text
//   shifts left under the caret).
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
                contiguous = rev.startOffset + rev.text.size() ==
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

// Re-anchor after deleting [off, off+len) from the buffer.
inline void shiftForDelete(std::vector<Revision>& revs, std::size_t off,
                           std::size_t len) {
    for (auto& rev : revs) {
        if (rev.startOffset >= off + len) {
            rev.startOffset -= len;
        } else if (rev.startOffset >= off) {
            rev.startOffset = off;
        }
    }
}

// Re-anchor after inserting len chars at off.
inline void shiftForInsert(std::vector<Revision>& revs, std::size_t off,
                           std::size_t len) {
    for (auto& rev : revs) {
        if (rev.startOffset > off) rev.startOffset += len;
    }
}

// Accept one revision: make its change permanent. Insert: if the
// text is missing (e.g. undone), put it back. Delete: if the text
// is still present, remove it. Drops the record either way.
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
            shiftForInsert(revs, off, rev.text.size());
        }
    } else {
        if (bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            buffer.replaceRange(rev.startOffset, rev.text.size(), "");
            shiftForDelete(revs, rev.startOffset, rev.text.size());
        }
    }
    return true;
}

// Reject one revision: make it as if the change never happened.
// Insert: remove the text if still present. Delete: put it back if
// absent. Drops the record either way.
inline bool rejectRevision(TextBuffer& buffer, std::vector<Revision>& revs,
                           std::size_t index) {
    if (index >= revs.size()) return false;
    Revision rev = revs[index];
    revs.erase(revs.begin() + static_cast<std::ptrdiff_t>(index));
    if (rev.type == RevisionType::Insert) {
        if (bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            buffer.replaceRange(rev.startOffset, rev.text.size(), "");
            shiftForDelete(revs, rev.startOffset, rev.text.size());
        }
    } else {
        if (!bufferMatchesAt(buffer, rev.startOffset, rev.text)) {
            const std::size_t off =
                std::min(rev.startOffset, buffer.textSize());
            buffer.replaceRange(off, 0, rev.text);
            shiftForInsert(revs, off, rev.text.size());
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
// copy of the buffer plus a working copy of `revs`, tracking the
// one span the group's ops can touch. The net effect is then
// folded into a single replaceRange over that span — one
// ReplaceRangesCommand, so one undo restores the pre-group
// buffer. Members whose text no longer matches are skipped,
// exactly as the per-revision op would skip them, and a group
// with no net buffer change drops its records without recording
// a no-op undo step. If the folded span ever escapes the
// simulation (unreachable for disjoint contiguous members),
// fall back to the sequential per-member ops, which preserve
// semantics at the cost of one undo step per member.
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

    std::vector<Revision> work = revs;
    std::string text = original;
    for (std::size_t t = 0; t < group.count; ++t) {
        const std::size_t k =
            accept ? t : (group.count - 1 - t);
        const Revision member = work[group.firstIndex + k];
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
        if (doDelete) {
            shiftForDelete(work, opOff, oldLen);
        } else {
            shiftForInsert(work, opOff, newText.size());
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
        buffer.replaceRange(spanStart, spanOrigLen,
                            text.substr(spanStart, spanEnd - spanStart));
    }
    revs = work;
    revs.erase(revs.begin() +
                   static_cast<std::ptrdiff_t>(group.firstIndex),
               revs.begin() +
                   static_cast<std::ptrdiff_t>(group.firstIndex +
                                                group.count));
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
