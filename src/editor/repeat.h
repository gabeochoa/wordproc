#pragma once

// Edit > Repeat (F4): the last REPEATABLE action, as a closure-free
// descriptor (enum + payload) stored on DocumentComponent
// (repeatRecord). DocumentCommands re-applies it at the current
// caret/selection; Repeat never records itself, so F4 F4 repeats
// the same action. Formatting records capture the RESULTING value
// (SetBold{true}), not the toggle: re-applying is idempotent, so
// bolding a selection and F4-ing onto other text bolds it even
// though the toggle already ran once. Typing records coalesce a
// run of consecutive typed units into one InsertText, keyed on the
// caret staying at the run's end (moving the caret starts a new
// run). Paste records the pasted text. Backspace/Delete/Cut are
// not repeatable and clear the record; Undo/Redo leave it alone.

#include <cstddef>
#include <string>

#include "document_settings.h"
#include "text_buffer.h"

enum class RepeatKind {
    None,
    InsertText,  // A run of typed text (text = the run)
    Paste,       // A paste (text = the pasted text)
    SetBold,
    SetItalic,
    SetUnderline,
    SetStrikethrough,
    SetSuperscript,
    SetSubscript,
    SetParagraphStyle,  // intValue = (int)ParagraphStyle
    SetFontSize,        // intValue = absolute size
    SetFont,            // text = font name
    SetTextColor,       // color
    SetHighlightColor,  // color
    SetAlignment,       // intValue = (int)TextAlignment
    SetLeftIndent,      // intValue = absolute left indent, px
    SetListType,        // intValue = (int)ListType
    SetLineSpacing,     // intValue = spacing * 100 (100/150/200)
    SetSpaceBefore,     // intValue = px
    SetSpaceAfter,      // intValue = px
};

struct ActionRecord {
    RepeatKind kind = RepeatKind::None;
    std::string text;
    int intValue = 0;
    bool boolValue = false;
    TextColor color = TextColors::Black;
    // End offset of the last typed run (InsertText coalescing).
    std::size_t typedEndOffset = 0;
};

namespace repeat {

inline void clear(ActionRecord& record) { record = ActionRecord{}; }

inline bool hasAction(const ActionRecord& record) {
    return record.kind != RepeatKind::None;
}

// Note one typed unit inserted at caretBefore (no selection at the
// time: hadSelection starts a new run). Coalesces onto the current
// InsertText run when the caret is exactly at the run's end;
// otherwise starts a new run.
inline void noteTyped(ActionRecord& record, const std::string& typed,
                      std::size_t caretBefore, bool hadSelection) {
    if (typed.empty()) return;
    if (!hadSelection && record.kind == RepeatKind::InsertText &&
        caretBefore == record.typedEndOffset) {
        record.text += typed;
        record.typedEndOffset += typed.size();
        return;
    }
    record = ActionRecord{};
    record.kind = RepeatKind::InsertText;
    record.text = typed;
    record.typedEndOffset = caretBefore + typed.size();
}

// Apply a record to the buffer at the current caret/selection, as
// ONE undo step for text (replaceRange) and a direct style/paragraph
// write for formatting (the same writes the original command made).
// Never touches the record itself.
inline void apply(TextBuffer& buffer, const ActionRecord& record) {
    switch (record.kind) {
        case RepeatKind::None:
            return;
        case RepeatKind::InsertText:
        case RepeatKind::Paste: {
            std::size_t offset = buffer.caretOffset();
            std::size_t length = 0;
            if (buffer.hasSelection()) {
                offset = buffer.offsetForPosition(buffer.selectionStart());
                length = buffer.offsetForPosition(buffer.selectionEnd()) - offset;
            }
            buffer.clearSelection();
            buffer.replaceRange(offset, length, record.text);
            return;
        }
        case RepeatKind::SetBold: {
            TextStyle style = buffer.textStyle();
            style.bold = record.boolValue;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetItalic: {
            TextStyle style = buffer.textStyle();
            style.italic = record.boolValue;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetUnderline: {
            TextStyle style = buffer.textStyle();
            style.underline = record.boolValue;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetStrikethrough: {
            TextStyle style = buffer.textStyle();
            style.strikethrough = record.boolValue;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetSuperscript: {
            TextStyle style = buffer.textStyle();
            style.superscript = record.boolValue;
            if (record.boolValue) style.subscript = false;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetSubscript: {
            TextStyle style = buffer.textStyle();
            style.subscript = record.boolValue;
            if (record.boolValue) style.superscript = false;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetParagraphStyle:
            buffer.setCurrentParagraphStyle(
                static_cast<ParagraphStyle>(record.intValue));
            return;
        case RepeatKind::SetFontSize: {
            TextStyle style = buffer.textStyle();
            style.fontSize = record.intValue;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetFont: {
            TextStyle style = buffer.textStyle();
            style.font = record.text;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetTextColor: {
            TextStyle style = buffer.textStyle();
            style.textColor = record.color;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetHighlightColor: {
            TextStyle style = buffer.textStyle();
            style.highlightColor = record.color;
            buffer.setTextStyle(style);
            return;
        }
        case RepeatKind::SetAlignment:
            buffer.setCurrentAlignment(
                static_cast<TextAlignment>(record.intValue));
            return;
        case RepeatKind::SetLeftIndent:
            buffer.setCurrentLeftIndent(record.intValue);
            return;
        case RepeatKind::SetListType: {
            // Toggle-based list API: drive the current type to the
            // target with the fewest toggles (idempotent re-apply).
            ListType target = static_cast<ListType>(record.intValue);
            ListType current = buffer.currentListType();
            if (current == target) return;
            if (current == ListType::Bulleted) buffer.toggleBulletedList();
            if (current == ListType::Numbered) buffer.toggleNumberedList();
            if (target == ListType::Bulleted) buffer.toggleBulletedList();
            if (target == ListType::Numbered) buffer.toggleNumberedList();
            return;
        }
        case RepeatKind::SetLineSpacing:
            if (record.intValue == 100) {
                buffer.setLineSpacingSingle();
            } else if (record.intValue == 150) {
                buffer.setLineSpacing1_5();
            } else if (record.intValue == 200) {
                buffer.setLineSpacingDouble();
            }
            return;
        case RepeatKind::SetSpaceBefore:
            buffer.setCurrentSpaceBefore(record.intValue);
            return;
        case RepeatKind::SetSpaceAfter:
            buffer.setCurrentSpaceAfter(record.intValue);
            return;
    }
}

// Factories: one per kind, so call sites capture the resulting
// value without spelling out field assignments.
inline ActionRecord setBool(RepeatKind kind, bool value) {
    ActionRecord record;
    record.kind = kind;
    record.boolValue = value;
    return record;
}

inline ActionRecord setInt(RepeatKind kind, int value) {
    ActionRecord record;
    record.kind = kind;
    record.intValue = value;
    return record;
}

inline ActionRecord setText(RepeatKind kind, const std::string& value) {
    ActionRecord record;
    record.kind = kind;
    record.text = value;
    return record;
}

// Debug name for a stored record (E2E `repeat_kind`).
inline const char* kindName(const ActionRecord& record) {
    switch (record.kind) {
        case RepeatKind::None: return "none";
        case RepeatKind::InsertText: return "insert";
        case RepeatKind::Paste: return "paste";
        case RepeatKind::SetBold: return "bold";
        case RepeatKind::SetItalic: return "italic";
        case RepeatKind::SetUnderline: return "underline";
        case RepeatKind::SetStrikethrough: return "strikethrough";
        case RepeatKind::SetSuperscript: return "superscript";
        case RepeatKind::SetSubscript: return "subscript";
        case RepeatKind::SetParagraphStyle: return "style";
        case RepeatKind::SetFontSize: return "size";
        case RepeatKind::SetFont: return "font";
        case RepeatKind::SetTextColor: return "color";
        case RepeatKind::SetHighlightColor: return "highlight";
        case RepeatKind::SetAlignment: return "alignment";
        case RepeatKind::SetLeftIndent: return "indent";
        case RepeatKind::SetListType: return "list";
        case RepeatKind::SetLineSpacing: return "spacing";
        case RepeatKind::SetSpaceBefore: return "space_before";
        case RepeatKind::SetSpaceAfter: return "space_after";
    }
    return "none";
}

inline ActionRecord setColor(RepeatKind kind, TextColor value) {
    ActionRecord record;
    record.kind = kind;
    record.color = value;
    return record;
}

}  // namespace repeat
