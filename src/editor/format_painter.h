#pragma once

// Format Painter (Standard toolbar): copy the character formatting
// of the current selection/caret, then apply it on the next click
// in the text.
//
// Subset honest note: TextBuffer carries ONE document-global
// TextStyle (textStyle()/setTextStyle()), not per-range styles —
// there is no per-word/per-selection style store to paint into. So
// copy captures the global style (bold/italic/underline,
// strikethrough, superscript/subscript, font, size, text color,
// highlight color) and apply sets the global style, exactly like
// the Bold/Italic/Underline toolbar buttons do. Applying to a
// clicked word therefore changes the document's current style (and
// what subsequently typed text looks like), rather than styling
// only that word. The copy/apply/arm/disarm state machine itself is
// complete and unit-tested here; per-range styles would need a
// TextBuffer range-style store first.

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

    // Apply the captured style; disarms. No-op when not armed.
    // Returns true when applied.
    bool apply(TextBuffer& buffer) {
        if (!armed) return false;
        buffer.setTextStyle(captured);
        armed = false;
        return true;
    }
};
