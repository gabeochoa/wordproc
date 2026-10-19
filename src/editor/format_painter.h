#pragma once

// Format Painter (Standard toolbar): copy the character formatting
// of the current selection/caret, then apply it on the next click
// in the text.
//
// Per-range truth (Batch 12): TextBuffer now carries per-range
// character styles (CharStyleSpan, document offsets) alongside
// the ONE document-global TextStyle. The global style is the
// typing / caret style and the fallback for unspanned text; a
// span overrides it for exactly [start, end). Capture reads the
// EFFECTIVE style at the selection start (or caret), so copying
// from a bold word captures bold even when the global style is
// plain. Apply writes a span over the target range through
// TextBuffer::applyCharStyleRange — the same store the Bold
// button writes through (applyStyleToCurrentRange) — so painting
// onto word B bolds only B, leaving A and C untouched. The old
// claim (global-only, "apply sets the global style") was wrong
// about the shipped behavior after Batch 12; before Batch 12 it
// was true, because no per-range store existed at all (there
// was no hidden side table: Bold toggled the global style and
// the renderer bolded whole lines).

#include "document_settings.h"
#include "text_buffer.h"

struct FormatPainter {
    bool armed = false;
    TextStyle captured;

    // Copy the current style and arm. Clicking the button again
    // disarms (see disarm()).
    void arm(const TextStyle& style) {
        captured = style;
        armed = true;
    }

    void disarm() { armed = false; }

    // Apply the captured style to an explicit range; disarms.
    // No-op when not armed. Returns true when applied.
    bool apply(TextBuffer& buffer, std::size_t start, std::size_t end) {
        if (!armed) return false;
        if (start >= end) return false;
        buffer.applyCharStyleRange(start, end, captured);
        armed = false;
        return true;
    }

    // Apply to the current range: the selection when one exists,
    // else the word at the caret (wordRangeAt), else the global
    // style (a bare caret on whitespace / at end of line sets the
    // typing style, like the old global apply). Disarms on any
    // applied path. No-op when not armed.
    bool apply(TextBuffer& buffer) {
        if (!armed) return false;
        if (buffer.hasSelection()) {
            std::size_t start =
                buffer.offsetForPosition(buffer.selectionStart());
            std::size_t end =
                buffer.offsetForPosition(buffer.selectionEnd());
            return apply(buffer, start, end);
        }
        auto [start, end] = buffer.wordRangeAt(buffer.caret());
        if (end > start) return apply(buffer, start, end);
        buffer.setTextStyle(captured);
        armed = false;
        return true;
    }
};
