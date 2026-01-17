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

// Find options for search functionality
struct FindOptions {
    bool caseSensitive = false;
    bool wholeWord = false;
    bool wrapAround = true;
    bool useRegex = false;
};

// Find result containing match position
struct FindResult {
    bool found = false;
    CaretPosition start{0, 0};
    CaretPosition end{0, 0};
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
    ParagraphStyle style = ParagraphStyle::Normal;  // Paragraph style for this line
    TextAlignment alignment = TextAlignment::Left;  // Text alignment for this line
    
    // Indentation (in pixels or character widths)
    int leftIndent = 0;       // Left margin indent for entire paragraph
    int firstLineIndent = 0;  // Additional indent for first line only (can be negative for hanging)
    
    // Spacing
    float lineSpacing = 1.0f;   // Line height multiplier (1.0 = single, 1.5 = 1.5x, 2.0 = double)
    int spaceBefore = 0;        // Extra pixels of space before this paragraph
    int spaceAfter = 0;         // Extra pixels of space after this paragraph
    
    // List properties
    ListType listType = ListType::None;  // Bullet, numbered, or none
    int listLevel = 0;                    // Nesting level for multi-level lists (0 = top level)
    int listNumber = 1;                   // Current number for numbered lists
    
    // Page break
    bool hasPageBreakBefore = false;  // Insert page break before this line (Ctrl+Enter)

    // Drop cap formatting
    bool hasDropCap = false;
    int dropCapLines = 2;  // Number of lines the drop cap spans
};

// Word count and document statistics
struct TextStats {
    std::size_t characters = 0;  // Excludes newlines
    std::size_t words = 0;
    std::size_t lines = 0;
    std::size_t paragraphs = 0;
    std::size_t sentences = 0;
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
    TextStats stats() const;
    TextStyle textStyle() const;
    void setTextStyle(const TextStyle& style);
    
    // Paragraph style for current line (where caret is)
    ParagraphStyle currentParagraphStyle() const;
    void setCurrentParagraphStyle(ParagraphStyle style);
    
    // Get paragraph style for a specific line
    ParagraphStyle lineParagraphStyle(std::size_t row) const;
    
    // Text alignment for current line (where caret is)
    TextAlignment currentAlignment() const;
    void setCurrentAlignment(TextAlignment align);
    
    // Get text alignment for a specific line
    TextAlignment lineAlignment(std::size_t row) const;
    
    // Indentation for current line (where caret is)
    int currentLeftIndent() const;
    int currentFirstLineIndent() const;
    void setCurrentLeftIndent(int pixels);
    void setCurrentFirstLineIndent(int pixels);
    void increaseIndent(int amount = 20);  // Default 20px (approx 2 chars at 10px/char)
    void decreaseIndent(int amount = 20);
    
    // Get indentation for a specific line
    int lineLeftIndent(std::size_t row) const;
    int lineFirstLineIndent(std::size_t row) const;
    
    // Spacing for current line (where caret is)
    float currentLineSpacing() const;
    int currentSpaceBefore() const;
    int currentSpaceAfter() const;
    void setCurrentLineSpacing(float multiplier);  // 1.0 = single, 1.5 = 1.5x, 2.0 = double
    void setCurrentSpaceBefore(int pixels);
    void setCurrentSpaceAfter(int pixels);
    
    // Convenience methods for common line spacings
    void setLineSpacingSingle();    // 1.0
    void setLineSpacing1_5();       // 1.5
    void setLineSpacingDouble();    // 2.0
    
    // Get spacing for a specific line
    float lineSpacing(std::size_t row) const;
    int lineSpaceBefore(std::size_t row) const;
    int lineSpaceAfter(std::size_t row) const;
    
    // List management for current line (where caret is)
    ListType currentListType() const;
    int currentListLevel() const;
    void setCurrentListType(ListType type);
    void toggleBulletedList();
    void toggleNumberedList();
    void increaseListLevel();
    void decreaseListLevel();
    
    // Get list properties for a specific line
    ListType lineListType(std::size_t row) const;
    int lineListLevel(std::size_t row) const;
    int lineListNumber(std::size_t row) const;
    
    // Page break methods
    void insertPageBreak();  // Insert a page break before the current line
    bool hasPageBreakBefore(std::size_t row) const;  // Check if line has page break before it
    void togglePageBreak();  // Toggle page break before current line
    void clearPageBreak();   // Remove page break before current line

    // Drop cap formatting
    bool currentLineHasDropCap() const;
    void setCurrentLineDropCap(bool enabled, int spanLines = 2);
    void toggleCurrentLineDropCap();
    
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
    
    // Find and Replace
    // Find text from current caret position
    FindResult find(const std::string& needle, const FindOptions& options = {}) const;
    
    // Find next occurrence (continue from last find)
    FindResult findNext(const std::string& needle, const FindOptions& options = {}) const;
    
    // Find previous occurrence
    FindResult findPrevious(const std::string& needle, const FindOptions& options = {}) const;
    
    // Find all occurrences and return count
    std::vector<FindResult> findAll(const std::string& needle, const FindOptions& options = {}) const;
    
    // Replace current selection with replacement text (if selection matches needle)
    bool replace(const std::string& needle, const std::string& replacement, const FindOptions& options = {});
    
    // Replace all occurrences and return count
    std::size_t replaceAll(const std::string& needle, const std::string& replacement, const FindOptions& options = {});

    // Hyperlink management
    // Add a hyperlink to the current selection (requires active selection)
    bool addHyperlink(const std::string& url, const std::string& tooltip = "");
    
    // Add a hyperlink at specific character offsets
    bool addHyperlinkAt(std::size_t startOffset, std::size_t endOffset, 
                        const std::string& url, const std::string& tooltip = "");
    
    // Edit an existing hyperlink's URL/tooltip
    bool editHyperlink(std::size_t offset, const std::string& newUrl, 
                       const std::string& newTooltip = "");
    
    // Remove hyperlink at given offset (keeps the text)
    bool removeHyperlink(std::size_t offset);
    
    // Get hyperlink at a given character offset (nullptr if none)
    const Hyperlink* hyperlinkAt(std::size_t offset) const;
    
    // Get hyperlink at caret position
    const Hyperlink* hyperlinkAtCaret() const;
    
    // Get all hyperlinks in the document
    const std::vector<Hyperlink>& hyperlinks() const { return hyperlinks_; }
    
    // Check if current selection has a hyperlink
    bool selectionHasHyperlink() const;
    
    // Get all hyperlinks that overlap with a range
    std::vector<const Hyperlink*> hyperlinksInRange(std::size_t startOffset, 
                                                     std::size_t endOffset) const;

    // Bookmark methods for internal document navigation
    bool addBookmark(const std::string& name);  // Add bookmark at current caret
    bool addBookmarkAt(const std::string& name, std::size_t offset);  // Add at specific offset
    bool removeBookmark(const std::string& name);  // Remove bookmark by name
    const Bookmark* getBookmark(const std::string& name) const;  // Get bookmark by name
    bool goToBookmark(const std::string& name);  // Navigate to bookmark
    bool hasBookmark(const std::string& name) const;  // Check if bookmark exists
    const Bookmark* bookmarkNear(std::size_t offset, std::size_t tolerance) const;  // Find nearby bookmark
    const std::vector<Bookmark>& bookmarks() const { return bookmarks_; }  // All bookmarks
    void clearBookmarks() { bookmarks_.clear(); version_++; }  // Remove all bookmarks
    
    // Footnote methods for document footnotes with auto-numbering
    bool addFootnote(const std::string& content);  // Add footnote at current caret, returns footnote number
    bool removeFootnote(std::size_t number);  // Remove footnote by number
    const Footnote* getFootnote(std::size_t number) const;  // Get footnote by number
    const Footnote* footnoteAt(std::size_t offset) const;  // Get footnote at specific offset
    const std::vector<Footnote>& footnotes() const { return footnotes_; }  // All footnotes
    void renumberFootnotes();  // Renumber all footnotes after edit
    void clearFootnotes() { footnotes_.clear(); version_++; }  // Remove all footnotes
    
    // Section break methods
    void insertSectionBreak(SectionBreakType type = SectionBreakType::NextPage);
    const DocumentSection* sectionAt(std::size_t line) const;
    const SectionSettings& sectionSettingsAt(std::size_t line) const;
    void updateSectionSettings(std::size_t line, const SectionSettings& settings);
    const std::vector<DocumentSection>& sections() const { return sections_; }
    void clearSections() { sections_.clear(); version_++; }
    
    // Outline extraction (for document navigation)
    // OutlineEntry represents a heading in the document outline
    struct OutlineEntry {
        std::size_t lineNumber = 0;     // Line number in document
        std::string text;                // Heading text (truncated)
        ParagraphStyle style;            // Heading style (H1-H6, Title, Subtitle)
        int level = 0;                   // Indentation level (0 = Title, 1 = H1, etc.)
    };
    std::vector<OutlineEntry> getOutline() const;  // Extract document outline from headings
    bool goToOutlineEntry(std::size_t lineNumber);  // Navigate to an outline entry
    
    // Table of contents generation
    std::string generateTableOfContents() const;  // Generate TOC text from headings
    void insertTableOfContents();  // Insert TOC at current caret position

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

    // Offset/position helpers
    std::size_t caretOffset() const;
    std::size_t offsetForPosition(const CaretPosition& pos) const;
    CaretPosition positionForOffset(std::size_t offset) const;
    char charAtOffset(std::size_t offset) const;

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
    
    // Renumber lists from a starting row (for numbered lists)
    void renumberListsFrom(std::size_t startRow);

    // Adjust hyperlink offsets when text is inserted/deleted
    void adjustHyperlinkOffsets(std::size_t pos, std::ptrdiff_t delta);

    // Adjust bookmark offsets when text is inserted/deleted
    void adjustBookmarkOffsets(std::size_t pos, std::ptrdiff_t delta);

    GapBuffer chars_;                   // Contiguous character storage
    std::vector<LineSpan> line_spans_;  // SoA line metadata
    std::vector<Hyperlink> hyperlinks_; // Hyperlinks in the document
    std::vector<Bookmark> bookmarks_;   // Bookmarks for internal navigation
    std::vector<Footnote> footnotes_;   // Footnotes with auto-numbering
    std::vector<DocumentSection> sections_;  // Document sections with per-section settings
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
