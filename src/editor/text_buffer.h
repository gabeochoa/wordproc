#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "../vendor/afterhours/src/plugins/command_history.h"
#include "borders_shading.h"
#include "document_settings.h"
#include "drop_cap.h"
#include "gap_buffer.h"
#include "toc.h"

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

// Insert a single character
class InsertCharCommand : public afterhours::Command<TextBuffer> {
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
class DeleteCharCommand : public afterhours::Command<TextBuffer> {
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
class DeleteSelectionCommand : public afterhours::Command<TextBuffer> {
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

// Replace one or more ranges with new text, as a single undo step.
// Ranges are CaretPositions into the current buffer; oldText must match
// the buffer contents at start (callers read it from the buffer).
struct TextReplacement {
    CaretPosition start;
    std::string oldText;
    std::string newText;
};

// One undo step for a paragraph-indent change (ruler drag, or any
// future indent dialog). Stores the whole indent triple so undo
// restores all three markers at once.
struct ParagraphIndents {
    int left = 0;
    int firstLine = 0;
    int right = 0;
    bool operator==(const ParagraphIndents& other) const {
        return left == other.left && firstLine == other.firstLine &&
               right == other.right;
    }
};

class SetParagraphIndentsCommand : public afterhours::Command<TextBuffer> {
   public:
    SetParagraphIndentsCommand(std::size_t row, ParagraphIndents oldIndents,
                               ParagraphIndents newIndents)
        : row_(row), old_(oldIndents), new_(newIndents) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override { return "Set paragraph indents"; }

   private:
    std::size_t row_;
    ParagraphIndents old_;
    ParagraphIndents new_;
};

// One undo step for a paragraph-style change (Outline
// Promote / Demote). Stores old + new so undo restores the
// previous style, exactly like SetParagraphIndentsCommand.
// Whole-paragraph format snapshot: every field the Paragraph
// dialog edits, so one dialog OK is one undo step even across a
// multi-paragraph selection. Indents are the stored 72-dpi units,
// spacing is points (the stored spaceBefore/spaceAfter units).
struct ParagraphFormat {
    ParagraphIndents indents{};
    float lineSpacing = 1.0f;
    int spaceBefore = 0;
    int spaceAfter = 0;
    // Borders and shading (Format > Borders and Shading...): border
    // edges bitmask + border style + shading color index, stored
    // the way indents are stored. See borders_shading.h for the
    // edge/style/palette constants.
    int borderEdges = 0;
    int borderStyle = borders_shading::kBorderStyleNone;
    int shadingColorIndex = 0;
    // Paragraph dialog, "Indents and Spacing" tab: the paragraph
    // alignment (LineSpan::alignment). "Line and Page Breaks" tab:
    // pageBreakBefore starts the paragraph on a new page in the
    // paged renderer (a dashed break line in Normal view); it is
    // NOT the Ctrl+Enter manual break (LineSpan::hasPageBreakBefore).
    // keepWithNext keeps the paragraph on the same page as the
    // next one (paged renderer: a last-on-page paragraph with
    // keepWithNext moves with the next paragraph).
    TextAlignment alignment = TextAlignment::Left;
    bool pageBreakBefore = false;
    bool keepWithNext = false;
    // Drop Cap (Format > Drop Cap): 0 = off, 2-4 = first glyph
    // spans that many lines. See drop_cap.h.
    int dropCapLines = 0;
    bool operator==(const ParagraphFormat& other) const {
        return indents == other.indents &&
               lineSpacing == other.lineSpacing &&
               spaceBefore == other.spaceBefore &&
               spaceAfter == other.spaceAfter &&
               borderEdges == other.borderEdges &&
               borderStyle == other.borderStyle &&
               shadingColorIndex == other.shadingColorIndex &&
               alignment == other.alignment &&
               pageBreakBefore == other.pageBreakBefore &&
               keepWithNext == other.keepWithNext &&
               dropCapLines == other.dropCapLines;
    }
};

// One undo step for a paragraph-format change (Paragraph dialog
// OK). Holds one old/new pair per affected row, so a selection
// spanning several paragraphs still undoes in a single step.
struct ParagraphFormatChange {
    std::size_t row = 0;
    ParagraphFormat oldFormat{};
    ParagraphFormat newFormat{};
};

class SetParagraphFormatCommand : public afterhours::Command<TextBuffer> {
   public:
    explicit SetParagraphFormatCommand(
        std::vector<ParagraphFormatChange> changes)
        : changes_(std::move(changes)) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override {
        return "Set paragraph format";
    }

   private:
    std::vector<ParagraphFormatChange> changes_;
};

class SetParagraphStyleCommand : public afterhours::Command<TextBuffer> {
   public:
    SetParagraphStyleCommand(std::size_t row, ParagraphStyle oldStyle,
                             ParagraphStyle newStyle)
        : row_(row), old_(oldStyle), new_(newStyle) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override { return "Set paragraph style"; }

   private:
    std::size_t row_;
    ParagraphStyle old_;
    ParagraphStyle new_;
};

// One undo step for swapping two ADJACENT line ranges
// [firstStart, boundary) and [boundary, secondEnd) -- the
// Outline Move Up / Down swap of two sibling blocks. Swap is
// not its own inverse when the blocks differ in size: after
// execute, the second block occupies [firstStart, firstStart +
// (secondEnd - boundary)) and the first block follows it, so
// undo swaps with the shifted boundary. Text AND line spans
// (styles, alignment, indents, ...) move together: a text-only
// swap (ReplaceRanges) would rebuild the moved lines with
// Normal style. Hyperlink / bookmark / footnote offsets inside
// the swapped ranges are NOT retargeted (they are document
// offsets, not line-owned); the moved text keeps its content,
// but an annotation anchored inside a moved block keeps its old
// offset.
class SwapAdjacentLineRangesCommand : public afterhours::Command<TextBuffer> {
   public:
    SwapAdjacentLineRangesCommand(std::size_t firstStart,
                                  std::size_t boundary,
                                  std::size_t secondEnd,
                                  std::size_t oldCaretRow,
                                  std::size_t newCaretRow)
        : firstStart_(firstStart),
          boundary_(boundary),
          secondEnd_(secondEnd),
          oldCaretRow_(oldCaretRow),
          newCaretRow_(newCaretRow) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override { return "Move outline block"; }

   private:
    std::size_t firstStart_;
    std::size_t boundary_;
    std::size_t secondEnd_;
    std::size_t oldCaretRow_;
    std::size_t newCaretRow_;
};

// Replace the TOC's marked region with a new block (or insert a
// block where there was none), as ONE undo step, carrying the
// mark with it: undo restores the old text AND the old mark, so
// a later Update never eats text past a stale end offset. The
// command stores absolute offsets; execute/undo apply via the
// low-level insertCharAt/deleteCharAt (no history, no offset
// adjustment — the mark is set explicitly here).
class TocReplaceCommand : public afterhours::Command<TextBuffer> {
   public:
    // Boundary style: the paragraph style of the line the block
    // abuts (the caret line on insert, the first line after the
    // block on update). The newline split in apply() gives the
    // remainder Normal, so execute/undo restore the style on
    // boundaryRowNew / boundaryRowOld. onlyIfNonEmpty is the
    // insert case: a remainder that is empty must stay Normal, or
    // it would become a phantom empty heading after the block.
    TocReplaceCommand(std::size_t start, std::string oldText,
                      std::string newText, bool oldHasToc,
                      std::size_t oldTocStart, std::size_t oldTocEnd,
                      bool newHasToc, std::size_t newTocStart,
                      std::size_t newTocEnd, bool hasBoundary = false,
                      ParagraphStyle boundaryStyle =
                          ParagraphStyle::Normal,
                      std::size_t boundaryRowNew = 0,
                      std::size_t boundaryRowOld = 0,
                      bool boundaryOnlyIfNonEmpty = false)
        : start_(start),
          oldText_(std::move(oldText)),
          newText_(std::move(newText)),
          oldHasToc_(oldHasToc),
          oldTocStart_(oldTocStart),
          oldTocEnd_(oldTocEnd),
          newHasToc_(newHasToc),
          newTocStart_(newTocStart),
          newTocEnd_(newTocEnd),
          hasBoundary_(hasBoundary),
          boundaryStyle_(boundaryStyle),
          boundaryRowNew_(boundaryRowNew),
          boundaryRowOld_(boundaryRowOld),
          boundaryOnlyIfNonEmpty_(boundaryOnlyIfNonEmpty) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override {
        return "Table of Contents";
    }

   private:
    static void apply(TextBuffer& buffer, std::size_t start,
                      const std::string& oldText,
                      const std::string& newText);
    std::size_t start_ = 0;
    std::string oldText_;
    std::string newText_;
    bool oldHasToc_ = false;
    std::size_t oldTocStart_ = 0;
    std::size_t oldTocEnd_ = 0;
    bool newHasToc_ = false;
    std::size_t newTocStart_ = 0;
    std::size_t newTocEnd_ = 0;
    bool hasBoundary_ = false;
    ParagraphStyle boundaryStyle_ = ParagraphStyle::Normal;
    std::size_t boundaryRowNew_ = 0;
    std::size_t boundaryRowOld_ = 0;
    bool boundaryOnlyIfNonEmpty_ = false;
};

class ReplaceRangesCommand : public afterhours::Command<TextBuffer> {
   public:
    explicit ReplaceRangesCommand(std::vector<TextReplacement> replacements)
        : replacements_(std::move(replacements)) {}
    void execute(TextBuffer& buffer) override;
    void undo(TextBuffer& buffer) override;
    std::string description() const override { return "Replace text"; }

   private:
    static void applyRange(TextBuffer& buffer, CaretPosition start,
                           const std::string& oldText,
                           const std::string& newText);
    std::vector<TextReplacement> replacements_;
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
    int rightIndent = 0;      // Right margin indent for entire paragraph
    
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
    // Paragraph dialog "Line and Page Breaks" tab (Format >
    // Paragraph...): page-break-before attribute + keep-with-next.
    // Distinct from hasPageBreakBefore (the manual break).
    bool pageBreakBefore = false;
    bool keepWithNext = false;

    // Drop cap formatting
    bool hasDropCap = false;
    int dropCapLines = 2;  // Number of lines the drop cap spans

    // Paragraph borders and shading (Format > Borders and
    // Shading...): border edges bitmask + border style + shading
    // color index. Defaults draw nothing, like a fresh paragraph.
    int borderEdges = 0;
    int borderStyle = borders_shading::kBorderStyleNone;
    int shadingColorIndex = 0;
};

// Word count and document statistics
struct TextStats {
    std::size_t characters = 0;  // Excludes newlines
    std::size_t words = 0;
    std::size_t lines = 0;
    std::size_t paragraphs = 0;
    std::size_t sentences = 0;
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

    // Zero-copy line access: returns pointer + length directly into the gap buffer.
    // Returns {nullptr, 0} if the line spans the gap (rare — only the line being
    // edited). Callers should fall back to lineString() in that case.
    struct LineView {
        const char* data = nullptr;
        std::size_t length = 0;
        explicit operator bool() const { return data != nullptr; }
    };
    LineView lineView(std::size_t row) const;

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

    // Select the word containing pos (double-click). If pos is on
    // whitespace/punctuation, just places the caret and clears selection.
    void selectWordAt(CaretPosition pos);
    // Select the whole line containing pos (triple-click).
    void selectParagraphAt(CaretPosition pos);

    void insertChar(char ch);
    void insertText(const std::string& text);
    void setText(const std::string& text);
    std::string getText() const;

    // Zero-copy text access: write contents directly to a stream
    void writeTextTo(std::ostream& out) const;

    // Zero-copy text access: get size without building a string
    std::size_t textSize() const;

    // Direct access to gap buffer for zero-copy iteration
    const GapBuffer& chars() const { return chars_; }

    TextStats stats() const;
    TextStyle textStyle() const;
    void setTextStyle(const TextStyle& style);
    
    // Paragraph style for current line (where caret is)
    ParagraphStyle currentParagraphStyle() const;
    void setCurrentParagraphStyle(ParagraphStyle style);
    
    // Get paragraph style for a specific line
    ParagraphStyle lineParagraphStyle(std::size_t row) const;

    // Set the paragraph style of one line (used by the document loader
    // to restore per-line styles, and by template adoption). Direct
    // span write: no history entry, like setCurrentParagraphStyle.
    void setLineParagraphStyle(std::size_t row, ParagraphStyle style);
    
    // Text alignment for current line (where caret is)
    TextAlignment currentAlignment() const;
    void setCurrentAlignment(TextAlignment align);
    
    // Get text alignment for a specific line
    TextAlignment lineAlignment(std::size_t row) const;
    
    // Indentation for current line (where caret is)
    int currentLeftIndent() const;
    int currentFirstLineIndent() const;
    int currentRightIndent() const;
    void setCurrentLeftIndent(int pixels);
    void setCurrentFirstLineIndent(int pixels);
    void setCurrentRightIndent(int pixels);
    void increaseIndent(int amount = 20);  // Default 20px (approx 2 chars at 10px/char)
    void decreaseIndent(int amount = 20);

    // Get indentation for a specific line
    int lineLeftIndent(std::size_t row) const;
    int lineFirstLineIndent(std::size_t row) const;
    int lineRightIndent(std::size_t row) const;

    // Whole-triple indent access. setLineIndents applies directly
    // (no history); commitParagraphIndents records one undo step for
    // a change already applied — the ruler drag applies live on every
    // move frame and commits once on release.
    ParagraphIndents lineIndents(std::size_t row) const;
    void setLineIndents(std::size_t row, ParagraphIndents indents);
    void commitParagraphIndents(std::size_t row, ParagraphIndents oldIndents,
                                ParagraphIndents newIndents);

    // Whole-format access for the Paragraph dialog: read the
    // caret paragraph's (or any row's) full format, apply one
    // directly (no history), and record one undo step for a
    // change already applied — the same live-apply-then-record
    // pattern as the ruler drag. commitParagraphFormat drops
    // unchanged rows and is a no-op when nothing changed, so a
    // dialog OK that edits nothing adds no undo step.
    ParagraphFormat lineParagraphFormat(std::size_t row) const;
    void setLineParagraphFormat(std::size_t row, ParagraphFormat format);
    void commitParagraphFormat(
        std::vector<ParagraphFormatChange> changes);

    // Record one undo step for a paragraph-style change already
    // applied (the caller applies via setLineParagraphStyle, then
    // commits -- same live-apply-then-record pattern as the ruler
    // drag). No-op when old == new.
    void commitParagraphStyle(std::size_t row, ParagraphStyle oldStyle,
                              ParagraphStyle newStyle);

    // Swap two adjacent line ranges, text + spans, as one undo
    // step (Outline Move Up / Down). The caller must have already
    // applied the swap via swapAdjacentLineRanges; this only
    // records it. oldCaretRow / newCaretRow are the caret rows
    // before / after the swap: the caret follows the heading it
    // was on. No-op on an empty or invalid range.
    void commitSwapAdjacentLineRanges(std::size_t firstStart,
                                      std::size_t boundary,
                                      std::size_t secondEnd,
                                      std::size_t oldCaretRow,
                                      std::size_t newCaretRow);

    // Low-level swap of two adjacent ranges [firstStart,
    // boundary) and [boundary, secondEnd): text + line spans,
    // no history, caret untouched (commands set it). See the
    // command's comment for the terminator handling at the end
    // of the document.
    void swapAdjacentLineRanges(std::size_t firstStart,
                                std::size_t boundary,
                                std::size_t secondEnd);
    
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
    // Drop-cap span of one line / the caret line: 0 = off, else
    // 2-4 (drop_cap::clampLines).
    int lineDropCapLines(std::size_t row) const;
    int currentDropCapLines() const;
    
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
    
    // Table of contents generation. generateTableOfContents formats
    // the current Title / Heading 1-3 paragraphs with dot leaders
    // and estimated page numbers (Batch 6 page math, linesPerPage
    // rows per page). insertTableOfContents inserts the block at
    // the caret, or REPLACES the existing marked TOC block when
    // one exists; updateTableOfContents regenerates in place.
    // All three are one undo step; the marked region is
    // [tocStartOffset, tocEndOffset). Returns false from the
    // insert/update when there are no headings (update with no
    // headings erases an existing block and clears the mark).
    std::string generateTableOfContents(
        int linesPerPage = toc::kDefaultLinesPerPage,
        const toc::Layout& tocLayout = toc::Layout{}) const;
    bool insertTableOfContents(
        int linesPerPage = toc::kDefaultLinesPerPage,
        const toc::Layout& tocLayout = toc::Layout{});
    bool updateTableOfContents(
        int linesPerPage = toc::kDefaultLinesPerPage,
        const toc::Layout& tocLayout = toc::Layout{});

    // The marked TOC region (DocumentComponent/docSettings
    // persistence: document_io writes these as the
    // "tableOfContents" object; the mark itself lives here, next
    // to the bookmarks, so buffer edits shift it with the same
    // adjust*Offsets mechanism). Text inside the region is
    // ordinary buffer text, so it saves/loads as text.
    bool hasToc() const { return tocHasMark_; }
    std::size_t tocStartOffset() const { return tocStart_; }
    std::size_t tocEndOffset() const { return tocEnd_; }
    // Number of generated entry lines inside the marked region.
    std::size_t tocEntryCount() const;
    // Set the mark from loaded file data: only accepted when the
    // range is in bounds and starts at the TOC title line. Used
    // by document_io; anything else clears the mark.
    void setTocRange(std::size_t start, std::size_t end);
    void clearTocMark();

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
    bool canUndo() const { return history_.can_undo(); }
    bool canRedo() const { return history_.can_redo(); }
    void undo();
    void redo();
    void clearHistory() { history_.clear(); }

    // Low-level insert/delete for command execution (no history recording)
    void insertCharAt(CaretPosition pos, char ch);
    void deleteCharAt(CaretPosition pos);
    void insertTextAt(CaretPosition pos, const std::string& text);

    // Replace the range [offset, offset + length) with replacement text, as
    // a single undo step. No-op if the range is out of bounds.
    void replaceRange(std::size_t offset, std::size_t length,
                      const std::string& replacement);

    // Replace several ranges with new text, as a single undo step.
    // Ranges are sorted by position (ascending) before applying, so
    // callers may pass them in any order. Used by Change All and AutoCorrect.
    void replaceRanges(std::vector<TextReplacement> replacements);

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

    // Adjust the TOC mark the same way (insert/delete before the
    // mark shifts it, inside it grows/shrinks the end, a deletion
    // covering it clears it). Called at the same sites as
    // adjustBookmarkOffsets.
    void adjustTocOffsets(std::size_t pos, std::ptrdiff_t delta);
    // The marked region, trusted when its title line still reads
    // "Table of Contents"; otherwise rescanned for a title line
    // followed by entry lines (recovers the region after an
    // undo of a plain edit, which the low-level undo ops do not
    // retarget — the same limitation bookmarks have). Falls back
    // to the stored range when no rescan finds a block, so a
    // hand-renamed title still updates in place.
    bool resolveTocRange(std::size_t& start, std::size_t& end) const;
    // Headings for a (re)generated block, with page numbers
    // computed for the document AFTER the block is in place:
    // headings at/after `insertLine` shift by `blockLines`, and
    // headings inside [oldStartLine, oldEndLine) (a replaced
    // block) are skipped. Only Title / Heading 1-3 are entries.
    std::vector<toc::Entry> tocEntriesForBlock(
        std::size_t insertLine, std::size_t blockLines,
        std::size_t oldStartLine, std::size_t oldEndLine,
        bool hasOld, int linesPerPage) const;

    // Marked TOC region [tocStart_, tocEnd_); tocHasMark_ is the
    // flag. Ordinary buffer text inside the region; hand edits
    // there are allowed and are lost on the next update.
    bool tocHasMark_ = false;
    std::size_t tocStart_ = 0;
    std::size_t tocEnd_ = 0;

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
    std::uint64_t version_ = 0;  // Increments on every modification
    mutable afterhours::CommandHistory<TextBuffer> history_;  // Undo/redo
    bool recordingHistory_ = true;  // Whether to record commands for undo
};
