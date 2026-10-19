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
inline std::string excerpt(const Revision& rev,
                           std::size_t maxLen = 24) {
    std::string out;
    out.reserve(std::min(rev.text.size(), maxLen));
    for (char ch : rev.text) {
        if (out.size() >= maxLen) break;
        out.push_back(ch == '\n' || ch == '\r' ? ' ' : ch);
    }
    if (rev.text.size() > maxLen) out += "...";
    return out;
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

// Dialog row label: Insert — Gabe — "abc".
inline std::string label(const Revision& rev) {
    return std::string(rev.type == RevisionType::Insert ? "Insert"
                                                        : "Delete") +
           " — " + rev.author + " — \"" + excerpt(rev) + "\"";
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

}  // namespace revisions
