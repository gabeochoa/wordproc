#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "document_settings.h"

struct CaretPosition {
    std::size_t row = 0;
    std::size_t column = 0;
};

// Forward declaration
class TextBuffer;

// Base class for undoable commands
class EditCommand {
   public:
    virtual ~EditCommand() = default;
    virtual void execute(TextBuffer& buffer) = 0;
    virtual void undo(TextBuffer& buffer) = 0;
    virtual std::string description() const = 0;
};

// Insert a single character
class InsertCharCommand : public EditCommand {
   public:
    InsertCharCommand(CaretPosition pos, char ch) : position_(pos), char_(ch) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override { return "Insert char"; }

   private:
    CaretPosition position_;
    char char_;
};

// Delete a single character (backspace or delete)
class DeleteCharCommand : public EditCommand {
   public:
    DeleteCharCommand(CaretPosition pos, char ch, bool isBackspace)
        : position_(pos), char_(ch), isBackspace_(isBackspace) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override { return "Delete char"; }

   private:
    CaretPosition position_;
    char char_;
    bool isBackspace_;
};

// Delete a selection
class DeleteSelectionCommand : public EditCommand {
   public:
    DeleteSelectionCommand(CaretPosition start, CaretPosition end,
                           std::string text)
        : start_(start), end_(end), deletedText_(std::move(text)) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override { return "Delete selection"; }

   private:
    CaretPosition start_;
    CaretPosition end_;
    std::string deletedText_;
};

// Command history for undo/redo
class CommandHistory {
   public:
    void execute(std::unique_ptr<EditCommand> cmd, TextBuffer& buffer);
    void record(std::unique_ptr<EditCommand> cmd);  // Record without executing
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    void undo(TextBuffer& buffer);
    void redo(TextBuffer& buffer);
    void clear() {
        undoStack_.clear();
        redoStack_.clear();
    }
    std::size_t undoStackSize() const { return undoStack_.size(); }
    std::size_t redoStackSize() const { return redoStack_.size(); }

   private:
    std::vector<std::unique_ptr<EditCommand>> undoStack_;
    std::vector<std::unique_ptr<EditCommand>> redoStack_;
};

// TextStyle is now defined in document_settings.h

// Line metadata for SoA layout - stores offset and length instead of copying
// strings
struct LineSpan {
    std::size_t offset = 0;  // Start offset in the character buffer
    std::size_t length = 0;  // Length of line (excluding newline)
};

// Gap buffer for efficient text editing
// Stores characters contiguously with a "gap" at the edit position
// This allows O(1) inserts and deletes at the cursor
class GapBuffer {
   public:
    GapBuffer(std::size_t initial_capacity = 4096);

    void insert(std::size_t pos, char ch);
    void insertString(std::size_t pos, const char* str, std::size_t len);
    void erase(std::size_t pos, std::size_t count = 1);

    char at(std::size_t pos) const;
    std::size_t size() const;
    bool empty() const;

    // Get substring without allocating (returns pointer and length)
    const char* data(std::size_t pos, std::size_t len) const;

    // Copy substring to output
    void copyTo(std::size_t pos, std::size_t len, char* out) const;

    // Get entire buffer as string (for compatibility)
    std::string toString() const;

    void clear();

    // Performance tracking
    std::size_t gapMoves() const { return gap_moves_; }
    std::size_t reallocations() const { return reallocations_; }
    void resetStats() {
        gap_moves_ = 0;
        reallocations_ = 0;
    }

   private:
    void moveGapTo(std::size_t pos);
    void ensureCapacity(std::size_t needed);

    std::vector<char> buffer_;
    std::size_t gap_start_ = 0;
    std::size_t gap_end_ = 0;
    std::size_t gap_moves_ = 0;
    std::size_t reallocations_ = 0;
};

// SoA (Structure of Arrays) text buffer using gap buffer + line spans
class TextBuffer {
   public:
    TextBuffer();

    // Read-only access to lines (returns span count for iteration)
    std::size_t lineCount() const;

    // Get line content as string_view-like access (offset + length)
    LineSpan lineSpan(std::size_t row) const;

    // Get line content as string (for compatibility - involves copy)
    std::string lineString(std::size_t row) const;

    // Legacy API - returns vector of strings (slower, allocates)
    // Kept for compatibility with existing tests and rendering
    std::vector<std::string> lines() const;

    CaretPosition caret() const;
    bool hasSelection() const;
    CaretPosition selectionStart() const;
    CaretPosition selectionEnd() const;

    void setCaret(CaretPosition caret);
    void clearSelection();
    void setSelectionAnchor(CaretPosition anchor);
    void updateSelectionToCaret();
    void selectAll();

    void insertChar(char ch);
    void insertText(const std::string& text);
    void setText(const std::string& text);
    std::string getText() const;
    TextStyle textStyle() const;
    void setTextStyle(const TextStyle& style);
    void backspace();
    void del();

    // Delete selected text and return true if there was a selection
    bool deleteSelection();

    // Get selected text as string (empty if no selection)
    std::string getSelectedText() const;

    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    // Word navigation (Ctrl+Arrow)
    void moveWordLeft();
    void moveWordRight();

    // Line navigation (Home/End)
    void moveToLineStart();
    void moveToLineEnd();

    // Document navigation (Ctrl+Home/End)
    void moveToDocumentStart();
    void moveToDocumentEnd();

    // Page navigation
    void movePageUp(std::size_t linesPerPage);
    void movePageDown(std::size_t linesPerPage);

    // Performance metrics
    struct PerfStats {
        std::size_t total_inserts = 0;
        std::size_t total_deletes = 0;
        std::size_t gap_moves = 0;
        std::size_t buffer_reallocations = 0;
    };
    PerfStats perfStats() const;
    void resetPerfStats();

    // Version counter - increments on every modification
    // Used by RenderCache to detect when rebuild is needed
    std::uint64_t version() const { return version_; }

    // Undo/Redo support
    bool canUndo() const { return history_.canUndo(); }
    bool canRedo() const { return history_.canRedo(); }
    void undo();
    void redo();
    void clearHistory() { history_.clear(); }

    // Low-level insert/delete for command execution (no history recording)
    void insertCharAt(CaretPosition pos, char ch);
    void deleteCharAt(CaretPosition pos);
    void insertTextAt(CaretPosition pos, const std::string& text);

   private:
    void ensureNonEmpty();
    void clampCaret();
    void rebuildLineIndex();
    std::size_t positionToOffset(const CaretPosition& pos) const;
    CaretPosition offsetToPosition(std::size_t offset) const;
    static int comparePositions(const CaretPosition& a, const CaretPosition& b);

    // Shift all line offsets from startRow onwards by delta
    // Used when inserting/deleting characters to maintain line_spans_
    // consistency
    void shiftLineOffsetsFrom(std::size_t startRow, std::ptrdiff_t delta);

    GapBuffer chars_;                   // Contiguous character storage
    std::vector<LineSpan> line_spans_;  // SoA line metadata
    CaretPosition caret_;
    bool has_selection_ = false;
    CaretPosition selection_anchor_;
    CaretPosition selection_end_;
    TextStyle style_;
    PerfStats stats_;
    std::uint64_t version_ = 0;       // Increments on every modification
    mutable CommandHistory history_;  // Undo/redo command history
    bool recordingHistory_ = true;    // Whether to record commands for undo
};
