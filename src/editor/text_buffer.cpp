#include "text_buffer.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ostream>
#include <regex>

// Afterhours text utilities for word navigation.
// Vendored afterhours: not ours to fix. See ui_context.h for why the
// -Wswitch-default suppression is scoped around this include.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-default"
#endif
#include "../vendor/afterhours/src/plugins/ui/text_input/utils.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

// ============================================================================
// TextBuffer implementation (SoA with gap buffer)
// ============================================================================

TextBuffer::TextBuffer() { ensureNonEmpty(); }

TextBuffer& TextBuffer::operator=(TextBuffer&& other) noexcept {
    if (this == &other) return *this;
    // The edit-shift hook belongs to the DESTINATION's owner
    // (its revision list), not to the text being moved in:
    // keep it — the startup move-assign of the preloaded buffer
    // must not drop DocumentComponent's hook. Every other
    // member moves. (If a member is added to TextBuffer, add it
    // here: defaulting this operator would move the hook too.)
    EditShiftHook keptHook = std::move(editShiftHook_);
    chars_ = std::move(other.chars_);
    line_spans_ = std::move(other.line_spans_);
    charStyleSpans_ = std::move(other.charStyleSpans_);
    hyperlinks_ = std::move(other.hyperlinks_);
    bookmarks_ = std::move(other.bookmarks_);
    footnotes_ = std::move(other.footnotes_);
    sections_ = std::move(other.sections_);
    caret_ = other.caret_;
    has_selection_ = other.has_selection_;
    selection_anchor_ = other.selection_anchor_;
    selection_end_ = other.selection_end_;
    style_ = other.style_;
    stats_ = other.stats_;
    version_ = other.version_;
    history_ = std::move(other.history_);
    recordingHistory_ = other.recordingHistory_;
    statsDirty_ = other.statsDirty_;
    cachedWordCount_ = other.cachedWordCount_;
    cachedSentenceCount_ = other.cachedSentenceCount_;
    cachedParagraphCount_ = other.cachedParagraphCount_;
    tocHasMark_ = other.tocHasMark_;
    tocStart_ = other.tocStart_;
    tocEnd_ = other.tocEnd_;
    editShiftHook_ = std::move(keptHook);
    return *this;
}

std::size_t TextBuffer::lineCount() const { return line_spans_.size(); }

LineSpan TextBuffer::lineSpan(std::size_t row) const {
    if (row >= line_spans_.size()) {
        return {0, 0};
    }
    return line_spans_[row];
}

std::string TextBuffer::lineString(std::size_t row) const {
    if (row >= line_spans_.size()) {
        return "";
    }

    const LineSpan& span = line_spans_[row];
    if (span.length == 0) {
        return "";
    }

    // Try zero-copy first
    const char* ptr = chars_.data(span.offset, span.length);
    if (ptr) {
        return std::string(ptr, span.length);
    }

    // Line spans the gap — copy via bulk memcpy
    std::string result(span.length, '\0');
    chars_.copyTo(span.offset, span.length, &result[0]);
    return result;
}

TextBuffer::LineView TextBuffer::lineView(std::size_t row) const {
    if (row >= line_spans_.size()) {
        return {nullptr, 0};
    }

    const LineSpan& span = line_spans_[row];
    if (span.length == 0) {
        return {"", 0};  // Empty line — point to a valid empty string
    }

    const char* ptr = chars_.data(span.offset, span.length);
    if (ptr) {
        return {ptr, span.length};
    }

    // Line spans the gap — caller must fall back to lineString()
    return {nullptr, span.length};
}

std::vector<std::string> TextBuffer::lines() const {
    std::vector<std::string> result;
    result.reserve(line_spans_.size());

    for (std::size_t i = 0; i < line_spans_.size(); ++i) {
        result.push_back(lineString(i));
    }

    return result;
}

CaretPosition TextBuffer::caret() const { return caret_; }

bool TextBuffer::hasSelection() const { return has_selection_; }

CaretPosition TextBuffer::selectionStart() const {
    if (!has_selection_) {
        return caret_;
    }
    return comparePositions(selection_anchor_, selection_end_) <= 0
               ? selection_anchor_
               : selection_end_;
}

CaretPosition TextBuffer::selectionEnd() const {
    if (!has_selection_) {
        return caret_;
    }
    return comparePositions(selection_anchor_, selection_end_) <= 0
               ? selection_end_
               : selection_anchor_;
}

void TextBuffer::setCaret(CaretPosition caret) {
    caret_ = caret;
    clampCaret();
}

void TextBuffer::clearSelection() { has_selection_ = false; }

void TextBuffer::setSelectionAnchor(CaretPosition anchor) {
    has_selection_ = true;
    selection_anchor_ = anchor;
    selection_end_ = caret_;
}

void TextBuffer::updateSelectionToCaret() {
    if (!has_selection_) {
        return;
    }
    selection_end_ = caret_;
}

void TextBuffer::selectAll() {
    if (line_spans_.empty()) {
        return;
    }

    // Set anchor to beginning
    has_selection_ = true;
    selection_anchor_ = {0, 0};

    // Set end to end of last line
    std::size_t lastRow = line_spans_.size() - 1;
    selection_end_ = {lastRow, line_spans_[lastRow].length};

    // Move caret to end
    caret_ = selection_end_;
}

void TextBuffer::selectWordAt(CaretPosition pos) {
    if (pos.row >= line_spans_.size()) return;
    std::string line = lineString(pos.row);
    auto isWordChar = [](char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) || ch == '\'';
    };
    // Clicking past end-of-line is not on a word: caret only.
    if (line.empty() || pos.column > line.size()) {
        setCaret(pos);
        clearSelection();
        return;
    }
    std::size_t col = std::min(pos.column, line.size());
    // Caret at end of line: the char before it is the candidate.
    std::size_t at = col < line.size() ? col : (line.empty() ? 0 : line.size() - 1);
    if (!isWordChar(line[at])) {
        setCaret(pos);
        clearSelection();
        return;
    }
    std::size_t start = at;
    while (start > 0 && isWordChar(line[start - 1])) --start;
    std::size_t end = at;
    while (end < line.size() && isWordChar(line[end])) ++end;
    has_selection_ = true;
    selection_anchor_ = {pos.row, start};
    selection_end_ = {pos.row, end};
    caret_ = selection_end_;
}

void TextBuffer::selectParagraphAt(CaretPosition pos) {
    if (pos.row >= line_spans_.size()) return;
    std::size_t len = line_spans_[pos.row].length;
    has_selection_ = true;
    selection_anchor_ = {pos.row, 0};
    selection_end_ = {pos.row, len};
    caret_ = selection_end_;
}

std::string TextBuffer::getSelectedText() const {
    if (!has_selection_) {
        return "";
    }

    CaretPosition start = selectionStart();
    CaretPosition end = selectionEnd();

    std::size_t startOffset = positionToOffset(start);
    std::size_t endOffset = positionToOffset(end);

    if (endOffset <= startOffset) {
        return "";
    }

    std::size_t len = endOffset - startOffset;
    std::string result(len, '\0');
    chars_.copyTo(startOffset, len, &result[0]);
    return result;
}

bool TextBuffer::deleteSelection() {
    if (!has_selection_) {
        return false;
    }

    CaretPosition start = selectionStart();
    CaretPosition end = selectionEnd();

    // Convert positions to offsets
    std::size_t startOffset = positionToOffset(start);
    std::size_t endOffset = positionToOffset(end);

    if (endOffset <= startOffset) {
        clearSelection();
        return false;
    }

    std::size_t deleteCount = endOffset - startOffset;

    // Save the deleted text for undo before erasing
    std::string deletedText;
    if (recordingHistory_) {
        deletedText.reserve(deleteCount);
        for (std::size_t i = startOffset; i < endOffset; ++i) {
            deletedText.push_back(chars_.at(i));
        }
    }

    // Incremental stats: paragraphs of the touched lines before
    // the erase, vs the single merged line after it.
    std::size_t oldParagraphs = 0;
    for (std::size_t row = start.row; row <= end.row; ++row) {
        if (line_spans_[row].length > 0) {
            ++oldParagraphs;
        }
    }
    const std::size_t mergedLength =
        start.column + (line_spans_[end.row].length - end.column);
    const std::ptrdiff_t paragraphDelta =
        static_cast<std::ptrdiff_t>(mergedLength > 0 ? 1 : 0) -
        static_cast<std::ptrdiff_t>(oldParagraphs);
    noteStatsRangeErased(startOffset, deleteCount, paragraphDelta);

    // Revision marks shift BEFORE the edit, while their text
    // still matches the pre-edit buffer (see noteEditShift).
    noteEditShift(startOffset, -static_cast<std::ptrdiff_t>(deleteCount));
    // Erase the selected range
    chars_.erase(startOffset, deleteCount);
    stats_.total_deletes += deleteCount;
    version_++;
    
    // Adjust hyperlink and bookmark offsets for the deleted selection
    adjustHyperlinkOffsets(startOffset, -static_cast<std::ptrdiff_t>(deleteCount));
    adjustBookmarkOffsets(startOffset, -static_cast<std::ptrdiff_t>(deleteCount));
    adjustTocOffsets(startOffset, -static_cast<std::ptrdiff_t>(deleteCount));
    adjustCharStyleSpans(startOffset, -static_cast<std::ptrdiff_t>(deleteCount));

    // Rebuild line index since we may have deleted across lines
    rebuildLineIndex();

    // Move caret to start of deleted region
    caret_ = start;
    clampCaret();

    // Clear selection
    clearSelection();

    // Record for undo
    if (recordingHistory_ && !deletedText.empty()) {
        history_.push(
            std::make_unique<DeleteSelectionCommand>(start, end, deletedText));
    }

    return true;
}

std::size_t TextBuffer::positionToOffset(const CaretPosition& pos) const {
    if (pos.row >= line_spans_.size()) {
        return chars_.size();
    }

    const LineSpan& span = line_spans_[pos.row];
    std::size_t col = std::min(pos.column, span.length);
    return span.offset + col;
}

CaretPosition TextBuffer::offsetToPosition(std::size_t offset) const {
    for (std::size_t row = 0; row < line_spans_.size(); ++row) {
        const LineSpan& span = line_spans_[row];

        // Include the newline character in line range check
        std::size_t next_line_start = (row + 1 < line_spans_.size())
                                          ? line_spans_[row + 1].offset
                                          : chars_.size();

        if (offset < next_line_start) {
            std::size_t col =
                (offset >= span.offset) ? offset - span.offset : 0;
            col = std::min(col, span.length);
            return {row, col};
        }
    }

    // Past end - return end of last line
    if (!line_spans_.empty()) {
        std::size_t last = line_spans_.size() - 1;
        return {last, line_spans_[last].length};
    }
    return {0, 0};
}

void TextBuffer::rebuildLineIndex() {
  struct LineMetaSnapshot {
    std::size_t offset = 0;
    ParagraphStyle style = ParagraphStyle::Normal;
    TextAlignment alignment = TextAlignment::Left;
    int leftIndent = 0;
    int firstLineIndent = 0;
    int rightIndent = 0;
    float lineSpacing = 1.0f;
    int spaceBefore = 0;
    int spaceAfter = 0;
    ListType listType = ListType::None;
    int listLevel = 0;
    int listNumber = 1;
    bool hasPageBreakBefore = false;
    bool pageBreakBefore = false;
    bool keepWithNext = false;
    bool hasDropCap = false;
    int dropCapLines = 2;
    int borderEdges = 0;
    int borderStyle = borders_shading::kBorderStyleNone;
    int shadingColorIndex = 0;
  };

  std::vector<LineMetaSnapshot> savedMeta;
  savedMeta.reserve(line_spans_.size());
  for (const auto& span : line_spans_) {
    LineMetaSnapshot snapshot;
    snapshot.offset = span.offset;
    snapshot.style = span.style;
    snapshot.alignment = span.alignment;
    snapshot.leftIndent = span.leftIndent;
    snapshot.firstLineIndent = span.firstLineIndent;
    snapshot.rightIndent = span.rightIndent;
    snapshot.lineSpacing = span.lineSpacing;
    snapshot.spaceBefore = span.spaceBefore;
    snapshot.spaceAfter = span.spaceAfter;
    snapshot.listType = span.listType;
    snapshot.listLevel = span.listLevel;
    snapshot.listNumber = span.listNumber;
    snapshot.hasPageBreakBefore = span.hasPageBreakBefore;
    snapshot.pageBreakBefore = span.pageBreakBefore;
    snapshot.keepWithNext = span.keepWithNext;
    snapshot.hasDropCap = span.hasDropCap;
    snapshot.dropCapLines = span.dropCapLines;
    snapshot.borderEdges = span.borderEdges;
    snapshot.borderStyle = span.borderStyle;
    snapshot.shadingColorIndex = span.shadingColorIndex;
    savedMeta.push_back(snapshot);
  }

  line_spans_.clear();

  std::size_t total = chars_.size();
  std::size_t line_start = 0;

  for (std::size_t i = 0; i < total; ++i) {
    if (chars_.at(i) == '\n') {
      line_spans_.push_back({line_start, i - line_start});
      line_start = i + 1;
    }
  }

  // Add final line (may be empty)
  line_spans_.push_back({line_start, total - line_start});

  // Restore metadata based on line start offsets
  for (const auto& snapshot : savedMeta) {
    for (auto& span : line_spans_) {
      if (span.offset == snapshot.offset) {
        span.style = snapshot.style;
        span.alignment = snapshot.alignment;
        span.leftIndent = snapshot.leftIndent;
        span.firstLineIndent = snapshot.firstLineIndent;
        span.rightIndent = snapshot.rightIndent;
        span.lineSpacing = snapshot.lineSpacing;
        span.spaceBefore = snapshot.spaceBefore;
        span.spaceAfter = snapshot.spaceAfter;
        span.listType = snapshot.listType;
        span.listLevel = snapshot.listLevel;
        span.listNumber = snapshot.listNumber;
        span.hasPageBreakBefore = snapshot.hasPageBreakBefore;
        span.pageBreakBefore = snapshot.pageBreakBefore;
        span.keepWithNext = snapshot.keepWithNext;
        span.hasDropCap = snapshot.hasDropCap;
        span.dropCapLines = snapshot.dropCapLines;
        span.borderEdges = snapshot.borderEdges;
        span.borderStyle = snapshot.borderStyle;
        span.shadingColorIndex = snapshot.shadingColorIndex;
        break;
      }
    }
  }
}

void TextBuffer::shiftLineOffsetsFrom(std::size_t startRow, std::ptrdiff_t delta) {
  for (std::size_t i = startRow; i < line_spans_.size(); ++i) {
    line_spans_[i].offset = static_cast<std::size_t>(
        static_cast<std::ptrdiff_t>(line_spans_[i].offset) + delta);
  }
}

void TextBuffer::insertChar(char ch) {
    ensureNonEmpty();

    // Delete any selected text first
    deleteSelection();

    // Record position before insert for undo
    CaretPosition insertPos = caret_;

    stats_.total_inserts++;
    version_++;  // Content changed - invalidate render cache

    std::size_t offset = positionToOffset(caret_);
    // Incremental stats: paragraphs of the split line before vs
    // the two pieces after (a newline), or +1 when a char lands
    // in an empty line.
    {
        const std::size_t oldLength = line_spans_[caret_.row].length;
        std::ptrdiff_t paragraphDelta = 0;
        if (ch == '\n') {
            const std::size_t first = caret_.column;
            const std::size_t second = oldLength - first;
            paragraphDelta =
                static_cast<std::ptrdiff_t>((first > 0 ? 1 : 0) +
                                            (second > 0 ? 1 : 0) -
                                            (oldLength > 0 ? 1 : 0));
        } else if (oldLength == 0) {
            paragraphDelta = 1;
        }
        noteStatsCharInserted(offset, ch, paragraphDelta);
    }
    noteEditShift(offset, 1);
    chars_.insert(offset, ch);

    // Adjust hyperlink and bookmark offsets for the inserted character
    adjustHyperlinkOffsets(offset, 1);
    adjustBookmarkOffsets(offset, 1);
    adjustTocOffsets(offset, 1);
    adjustCharStyleSpans(offset, 1);

    if (ch == '\n') {
        // Split current line - preserve paragraph styles
        std::size_t splitRow = caret_.row;
        LineSpan oldSpan = line_spans_[splitRow];
        
        // Current line ends at caret position
        line_spans_[splitRow].length = caret_.column;
        
        // New line starts after newline character
        LineSpan newSpan;
        newSpan.offset = offset + 1;
        newSpan.length = (oldSpan.offset + oldSpan.length) - offset;
        newSpan.style = ParagraphStyle::Normal;  // New line gets Normal style
        newSpan.alignment = oldSpan.alignment;   // Inherit alignment
        newSpan.leftIndent = oldSpan.leftIndent; // Inherit indentation
        newSpan.firstLineIndent = oldSpan.firstLineIndent;
        newSpan.rightIndent = oldSpan.rightIndent;
        newSpan.lineSpacing = oldSpan.lineSpacing;
        newSpan.listType = oldSpan.listType;     // Inherit list properties
        newSpan.listLevel = oldSpan.listLevel;
        if (oldSpan.listType != ListType::None) {
            newSpan.listNumber = oldSpan.listNumber + 1;
        }
        
        // Insert new line span
        line_spans_.insert(line_spans_.begin() + splitRow + 1, newSpan);
        
        // Shift offsets of subsequent lines
        shiftLineOffsetsFrom(splitRow + 2, 1);
        
        caret_.row += 1;
        caret_.column = 0;
    } else {
    // Update current line span
    if (caret_.row < line_spans_.size()) {
      line_spans_[caret_.row].length += 1;
      shiftLineOffsetsFrom(caret_.row + 1, 1);
    }
    caret_.column += 1;
    }

    // Record for undo
    if (recordingHistory_) {
        history_.push(std::make_unique<InsertCharCommand>(insertPos, ch));
    }
}

void TextBuffer::insertText(const std::string& text) {
    for (char ch : text) {
        insertChar(ch);
    }
}

void TextBuffer::setText(const std::string& text) {
    chars_.clear();
    line_spans_.clear();
    charStyleSpans_.clear();  // New text has no per-range styles
    hyperlinks_.clear();  // Clear all hyperlinks when setting new text
    clearTocMark();       // A new document has no TOC block
    version_++;  // Content changed - invalidate render cache

    if (!text.empty()) {
        // Fast path: check if we even have \r chars to strip.
        // Most files on Unix/macOS won't, so we can skip the copy entirely.
        bool hasCR = std::memchr(text.data(), '\r', text.size()) != nullptr;

        const char* data;
        std::size_t len;
        std::string cleaned;  // Only used if hasCR

        if (hasCR) {
            // Strip \r characters (Windows line endings)
            cleaned.reserve(text.size());
            const char* src = text.data();
            const char* end = src + text.size();
            while (src < end) {
                // Find next \r using memchr (often SIMD-optimized)
                const char* cr = static_cast<const char*>(
                    std::memchr(src, '\r', static_cast<std::size_t>(end - src)));
                if (!cr) {
                    cleaned.append(src, static_cast<std::size_t>(end - src));
                    break;
                }
                cleaned.append(src, static_cast<std::size_t>(cr - src));
                src = cr + 1;
            }
            data = cleaned.data();
            len = cleaned.size();
        } else {
            // Zero-copy: use the input directly
            data = text.data();
            len = text.size();
        }

        // Bulk load into gap buffer (single memcpy, no per-char overhead)
        chars_.setContent(data, len);

        // Single-pass line index build using memchr (SIMD-optimized).
        // Reserve generously to avoid reallocation on most documents.
        // Average line length ~27 chars for prose, ~40 for code.
        line_spans_.reserve(len / 20 + 1);
        std::size_t line_start = 0;
        const char* scan = data;
        const char* end = data + len;

        while (scan < end) {
            const char* nl = static_cast<const char*>(
                std::memchr(scan, '\n', static_cast<std::size_t>(end - scan)));
            if (!nl) break;
            std::size_t pos = static_cast<std::size_t>(nl - data);
            line_spans_.push_back({line_start, pos - line_start});
            line_start = pos + 1;
            scan = nl + 1;
        }

        // Add final line (may be empty)
        line_spans_.push_back({line_start, len - line_start});
    } else {
        // Empty document - single empty line
        line_spans_.push_back({0, 0});
    }

    // Incremental stats: mark dirty only — the first stats()
    // call or mutator after this rebuilds once (see header).
    statsDirty_ = true;

    // Move caret to end
    if (!line_spans_.empty()) {
        caret_.row = line_spans_.size() - 1;
        caret_.column = line_spans_[caret_.row].length;
    } else {
        caret_ = {0, 0};
    }
    clearSelection();
}

std::string TextBuffer::getText() const { return chars_.toString(); }

void TextBuffer::writeTextTo(std::ostream& out) const { chars_.writeTo(out); }

std::size_t TextBuffer::textSize() const { return chars_.size(); }

bool TextBuffer::isStatsWordChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) != 0;
}

bool TextBuffer::isStatsSentenceEnd(char ch) {
    return ch == '.' || ch == '!' || ch == '?';
}

bool TextBuffer::startsStatsWord(char ch, bool hasPrev, char prev) {
    return isStatsWordChar(ch) && (!hasPrev || !isStatsWordChar(prev));
}

namespace {

// Apply a signed paragraph delta to an unsigned counter without
// wrapping: callers compute deltas in [-lines, +1].
void applyParagraphDelta(std::size_t& count, std::ptrdiff_t delta) {
    if (delta >= 0) {
        count += static_cast<std::size_t>(delta);
    } else {
        count -= static_cast<std::size_t>(-delta);
    }
}

}  // namespace

void TextBuffer::ensureStatsValid() const {
    if (statsDirty_) {
        rebuildStatsFromScratch();
    }
}

void TextBuffer::noteStatsCharInserted(std::size_t offset, char ch,
                                       std::ptrdiff_t paragraphDelta) {
    ensureStatsValid();
    // Neighbours before the insert: only the inserted char's own
    // word start and the right neighbour's word start can change.
    const bool hasLeft = offset > 0;
    const char left = hasLeft ? chars_.at(offset - 1) : '\0';
    const bool hasRight = offset < chars_.size();
    if (hasRight) {
        const char right = chars_.at(offset);
        if (startsStatsWord(ch, hasLeft, left)) {
            ++cachedWordCount_;
        }
        if (startsStatsWord(right, true, ch)) {
            ++cachedWordCount_;
        }
        if (startsStatsWord(right, hasLeft, left)) {
            --cachedWordCount_;
        }
    } else if (startsStatsWord(ch, hasLeft, left)) {
        ++cachedWordCount_;
    }
    if (isStatsSentenceEnd(ch)) {
        ++cachedSentenceCount_;
    }
    applyParagraphDelta(cachedParagraphCount_, paragraphDelta);
}

void TextBuffer::noteStatsCharErased(std::size_t offset,
                                     std::ptrdiff_t paragraphDelta) {
    ensureStatsValid();
    // The erased char is still in the buffer: neighbours are
    // pre-erase.
    const char ch = chars_.at(offset);
    const bool hasLeft = offset > 0;
    const char left = hasLeft ? chars_.at(offset - 1) : '\0';
    const bool hasRight = offset + 1 < chars_.size();
    if (startsStatsWord(ch, hasLeft, left)) {
        --cachedWordCount_;
    }
    if (hasRight) {
        const char right = chars_.at(offset + 1);
        if (startsStatsWord(right, true, ch)) {
            --cachedWordCount_;
        }
        if (startsStatsWord(right, hasLeft, left)) {
            ++cachedWordCount_;
        }
    }
    if (isStatsSentenceEnd(ch)) {
        --cachedSentenceCount_;
    }
    applyParagraphDelta(cachedParagraphCount_, paragraphDelta);
}

void TextBuffer::noteStatsRangeErased(std::size_t startOffset,
                                     std::size_t count,
                                     std::ptrdiff_t paragraphDelta) {
    if (count == 0) {
        return;
    }
    ensureStatsValid();  // Rebuilds from the pre-erase text.
    // Word starts inside the range, with the char before the range
    // as the initial previous char (a range-leading alnum char
    // that continues a word is not a start).
    const bool hasBefore = startOffset > 0;
    const char before = hasBefore ? chars_.at(startOffset - 1) : '\0';
    bool hasPrev = hasBefore;
    char prev = before;
    std::size_t lostWordStarts = 0;
    std::size_t lostSentences = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const char ch = chars_.at(startOffset + i);
        if (startsStatsWord(ch, hasPrev, prev)) {
            ++lostWordStarts;
        }
        if (isStatsSentenceEnd(ch)) {
            ++lostSentences;
        }
        hasPrev = true;
        prev = ch;
    }
    cachedWordCount_ -= lostWordStarts;
    cachedSentenceCount_ -= lostSentences;
    // The char after the range (if any) had its word start against
    // the range's last char; after the erase it starts against the
    // char before the range.
    const std::size_t endOffset = startOffset + count;
    if (endOffset < chars_.size()) {
        const char right = chars_.at(endOffset);
        if (startsStatsWord(right, true, prev)) {
            --cachedWordCount_;
        }
        if (startsStatsWord(right, hasBefore, before)) {
            ++cachedWordCount_;
        }
    }
    applyParagraphDelta(cachedParagraphCount_, paragraphDelta);
}

void TextBuffer::rebuildStatsFromScratch() const {
    statsDirty_ = false;
    cachedWordCount_ = 0;
    cachedSentenceCount_ = 0;
    cachedParagraphCount_ = 0;
    // Iterate directly over gap buffer regions — no allocation, no copy.
    // inWord / lineLength persist across the gap boundary.
    bool inWord = false;
    std::size_t lineLength = 0;
    chars_.forEachRegion([&](const char* data, std::size_t len) {
        for (std::size_t i = 0; i < len; ++i) {
            const char ch = data[i];
            if (ch == '\n') {
                if (lineLength > 0) {
                    ++cachedParagraphCount_;
                }
                lineLength = 0;
                inWord = false;
            } else {
                ++lineLength;
                if (isStatsWordChar(ch)) {
                    if (!inWord) {
                        ++cachedWordCount_;
                        inWord = true;
                    }
                } else {
                    inWord = false;
                }
            }
            if (isStatsSentenceEnd(ch)) {
                ++cachedSentenceCount_;
            }
        }
    });
    if (lineLength > 0) {
        ++cachedParagraphCount_;
    }
}

TextStats TextBuffer::stats() const {
    ensureStatsValid();
    TextStats stats;
    stats.lines = line_spans_.size();
    // Every span except possibly the last ends with exactly one
    // '\n', so newlines = lines - 1 (no newlines when empty).
    const std::size_t newlines =
        line_spans_.empty() ? 0 : line_spans_.size() - 1;
    stats.characters = chars_.size() - newlines;
    stats.words = cachedWordCount_;
    stats.sentences = cachedSentenceCount_;
    stats.paragraphs = cachedParagraphCount_;
    return stats;
}

TextStyle TextBuffer::textStyle() const { return style_; }

void TextBuffer::setTextStyle(const TextStyle& style) { style_ = style; }

// --- Per-range character formatting (Batch 12) ---

TextStyle TextBuffer::effectiveStyleAt(std::size_t offset) const {
    const CharStyleSpan* span = charStyleSpanAt(offset);
    return span != nullptr ? span->style : style_;
}

std::vector<TextBuffer::StyledSegment> TextBuffer::styledSegments(
    std::size_t start, std::size_t end) const {
    return styledSegments(start, end, style_);
}

std::vector<TextBuffer::StyledSegment> TextBuffer::styledSegments(
    std::size_t start, std::size_t end, const TextStyle& base) const {
    std::vector<StyledSegment> segments;
    start = std::min(start, chars_.size());
    end = std::min(end, chars_.size());
    if (start >= end) return segments;
    // Spans are kept sorted and non-overlapping (see
    // normalizeCharStyleSpans), so one scan finds the covering
    // span or the next span after the cursor.
    std::size_t cursor = start;
    while (cursor < end) {
        const CharStyleSpan* covering = nullptr;
        std::size_t nextStart = end;
        for (const auto& span : charStyleSpans_) {
            if (span.endOffset <= cursor) continue;
            if (span.startOffset > cursor) {
                nextStart = span.startOffset;
                break;
            }
            covering = &span;
            break;
        }
        std::size_t segEnd = end;
        TextStyle segStyle = base;
        if (covering != nullptr) {
            segEnd = std::min(end, covering->endOffset);
            segStyle = covering->style;
        } else {
            segEnd = std::min(end, nextStart);
        }
        if (segEnd <= cursor) break;  // Defensive: never loop on an empty span.
        if (!segments.empty() && segments.back().style == segStyle &&
            segments.back().endOffset == cursor) {
            segments.back().endOffset = segEnd;
        } else {
            segments.push_back(StyledSegment{cursor, segEnd, segStyle});
        }
        cursor = segEnd;
    }
    return segments;
}

const TextBuffer::CharStyleSpan* TextBuffer::charStyleSpanAt(
    std::size_t offset) const {
    for (const auto& span : charStyleSpans_) {
        if (offset >= span.startOffset && offset < span.endOffset) {
            return &span;
        }
    }
    return nullptr;
}

const TextBuffer::CharStyleSpan* TextBuffer::charStyleSpanAtCaret() const {
    std::size_t offset = caretOffset();
    const CharStyleSpan* span = charStyleSpanAt(offset);
    if (span != nullptr) return span;
    if (offset > 0) return charStyleSpanAt(offset - 1);
    return nullptr;
}

TextStyle TextBuffer::effectiveStyleAtCaretOrSelection() const {
    if (has_selection_) {
        return effectiveStyleAt(positionToOffset(selectionStart()));
    }
    const CharStyleSpan* span = charStyleSpanAtCaret();
    return span != nullptr ? span->style : style_;
}

void TextBuffer::normalizeCharStyleSpans() {
    std::sort(charStyleSpans_.begin(), charStyleSpans_.end(),
              [](const CharStyleSpan& a, const CharStyleSpan& b) {
                  return a.startOffset < b.startOffset;
              });
    std::vector<CharStyleSpan> kept;
    for (auto& span : charStyleSpans_) {
        if (span.startOffset >= span.endOffset) continue;
        span.endOffset = std::min(span.endOffset, chars_.size());
        if (span.startOffset >= span.endOffset) continue;
        if (!kept.empty() && kept.back().endOffset >= span.startOffset &&
            kept.back().style == span.style) {
            kept.back().endOffset =
                std::max(kept.back().endOffset, span.endOffset);
        } else {
            kept.push_back(span);
        }
    }
    charStyleSpans_ = std::move(kept);
}

void TextBuffer::applyCharStyleRange(std::size_t start, std::size_t end,
                                     const TextStyle& style) {
    start = std::min(start, chars_.size());
    end = std::min(end, chars_.size());
    if (start >= end) return;
    std::vector<CharStyleSpan> next;
    for (const auto& span : charStyleSpans_) {
        if (span.endOffset <= start || span.startOffset >= end) {
            next.push_back(span);
            continue;
        }
        // Overlap: keep the untouched head / tail.
        if (span.startOffset < start) {
            next.push_back(
                CharStyleSpan{span.startOffset, start, span.style});
        }
        if (span.endOffset > end) {
            next.push_back(CharStyleSpan{end, span.endOffset, span.style});
        }
    }
    next.push_back(CharStyleSpan{start, end, style});
    std::sort(next.begin(), next.end(),
              [](const CharStyleSpan& a, const CharStyleSpan& b) {
                  return a.startOffset < b.startOffset;
              });
    charStyleSpans_ = std::move(next);
    normalizeCharStyleSpans();
    version_++;
}

void TextBuffer::applyStyleToCurrentRange(const TextStyle& style) {
    if (has_selection_) {
        std::size_t start = positionToOffset(selectionStart());
        std::size_t end = positionToOffset(selectionEnd());
        if (end > start) {
            applyCharStyleRange(start, end, style);
            return;
        }
    }
    const CharStyleSpan* span = charStyleSpanAtCaret();
    if (span != nullptr) {
        // Copy the range first: applyCharStyleRange rebuilds the
        // vector, invalidating the pointer.
        std::size_t start = span->startOffset;
        std::size_t end = span->endOffset;
        applyCharStyleRange(start, end, style);
        return;
    }
    setTextStyle(style);
}

std::pair<std::size_t, std::size_t> TextBuffer::wordRangeAt(
    CaretPosition pos) const {
    if (pos.row >= line_spans_.size()) return {0, 0};
    std::string line = lineString(pos.row);
    auto isWordChar = [](char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) || ch == '\'';
    };
    if (line.empty() || pos.column > line.size()) return {0, 0};
    std::size_t col = std::min(pos.column, line.size());
    std::size_t at =
        col < line.size() ? col : line.size() - 1;
    if (!isWordChar(line[at])) return {0, 0};
    std::size_t start = at;
    while (start > 0 && isWordChar(line[start - 1])) --start;
    std::size_t end = at;
    while (end < line.size() && isWordChar(line[end])) ++end;
    std::size_t lineOffset = line_spans_[pos.row].offset;
    return {lineOffset + start, lineOffset + end};
}

void TextBuffer::adjustCharStyleSpans(std::size_t pos,
                                      std::ptrdiff_t delta) {
    if (delta == 0 || charStyleSpans_.empty()) return;
    if (delta > 0) {
        std::size_t count = static_cast<std::size_t>(delta);
        for (auto& span : charStyleSpans_) {
            if (pos <= span.startOffset) {
                span.startOffset += count;
                span.endOffset += count;
            } else if (pos <= span.endOffset) {
                span.endOffset += count;
            }
        }
        return;
    }
    std::size_t count = static_cast<std::size_t>(-delta);
    std::size_t delEnd = pos + count;
    for (auto& span : charStyleSpans_) {
        if (span.endOffset <= pos) continue;
        if (span.startOffset >= delEnd) {
            span.startOffset -= count;
            span.endOffset -= count;
            continue;
        }
        // Overlap: keep the kept-before + kept-after lengths.
        std::size_t before =
            span.startOffset < pos ? pos - span.startOffset : 0;
        std::size_t after =
            span.endOffset > delEnd ? span.endOffset - delEnd : 0;
        std::size_t newStart =
            span.startOffset < pos ? span.startOffset : pos;
        span.startOffset = newStart;
        span.endOffset = newStart + before + after;
    }
    normalizeCharStyleSpans();
}

ParagraphStyle TextBuffer::currentParagraphStyle() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].style;
    }
    return ParagraphStyle::Normal;
}

void TextBuffer::setCurrentParagraphStyle(ParagraphStyle style) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].style = style;
        version_++;  // Style change invalidates render cache
    }
}

void TextBuffer::setLineParagraphStyle(std::size_t row, ParagraphStyle style) {
    if (row < line_spans_.size()) {
        line_spans_[row].style = style;
        version_++;
    }
}

ParagraphStyle TextBuffer::lineParagraphStyle(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].style;
    }
    return ParagraphStyle::Normal;
}

TextAlignment TextBuffer::currentAlignment() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].alignment;
    }
    return TextAlignment::Left;
}

void TextBuffer::setCurrentAlignment(TextAlignment align) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].alignment = align;
        version_++;  // Alignment change invalidates render cache
    }
}

TextAlignment TextBuffer::lineAlignment(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].alignment;
    }
    return TextAlignment::Left;
}

int TextBuffer::currentLeftIndent() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].leftIndent;
    }
    return 0;
}

int TextBuffer::currentFirstLineIndent() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].firstLineIndent;
    }
    return 0;
}

void TextBuffer::setCurrentLeftIndent(int pixels) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].leftIndent = std::max(0, pixels);
        version_++;  // Indent change invalidates render cache
    }
}

void TextBuffer::setCurrentFirstLineIndent(int pixels) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].firstLineIndent = pixels;  // Can be negative for hanging indent
        version_++;
    }
}

int TextBuffer::currentRightIndent() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].rightIndent;
    }
    return 0;
}

void TextBuffer::setCurrentRightIndent(int pixels) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].rightIndent = std::max(0, pixels);
        version_++;
    }
}

void TextBuffer::increaseIndent(int amount) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].leftIndent += amount;
        version_++;
    }
}

void TextBuffer::decreaseIndent(int amount) {
    if (caret_.row < line_spans_.size()) {
        int current = line_spans_[caret_.row].leftIndent;
        line_spans_[caret_.row].leftIndent = std::max(0, current - amount);
        version_++;
    }
}

int TextBuffer::lineLeftIndent(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].leftIndent;
    }
    return 0;
}

int TextBuffer::lineFirstLineIndent(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].firstLineIndent;
    }
    return 0;
}

int TextBuffer::lineRightIndent(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].rightIndent;
    }
    return 0;
}

ParagraphIndents TextBuffer::lineIndents(std::size_t row) const {
    if (row < line_spans_.size()) {
        const LineSpan& span = line_spans_[row];
        return {span.leftIndent, span.firstLineIndent, span.rightIndent};
    }
    return {};
}

void TextBuffer::setLineIndents(std::size_t row, ParagraphIndents indents) {
    if (row >= line_spans_.size()) return;
    LineSpan& span = line_spans_[row];
    span.leftIndent = std::max(0, indents.left);
    span.firstLineIndent = indents.firstLine;
    span.rightIndent = std::max(0, indents.right);
    version_++;
}

void TextBuffer::commitParagraphIndents(std::size_t row,
                                        ParagraphIndents oldIndents,
                                        ParagraphIndents newIndents) {
    if (row >= line_spans_.size() || oldIndents == newIndents) return;
    // Already applied live; record only, so the whole drag is one step.
    history_.push(std::make_unique<SetParagraphIndentsCommand>(
        row, oldIndents, newIndents));
}

void TextBuffer::commitRevisionList(std::vector<Revision>* revs,
                                    std::vector<Revision> before,
                                    std::vector<Revision> after) {
    if (revs == nullptr || before == after) return;
    // Already applied live; record only, so the whole marking
    // (one range, or one merged mark) is one undo step.
    history_.push(std::make_unique<RevisionListCommand>(
        revs, std::move(before), std::move(after)));
    version_++;  // A mark changes the rendering (see header).
}

ParagraphFormat TextBuffer::lineParagraphFormat(std::size_t row) const {
    if (row >= line_spans_.size()) return {};
    const LineSpan& span = line_spans_[row];
    ParagraphFormat format;
    format.indents = {span.leftIndent, span.firstLineIndent,
                      span.rightIndent};
    format.lineSpacing = span.lineSpacing;
    format.spaceBefore = span.spaceBefore;
    format.spaceAfter = span.spaceAfter;
    format.borderEdges = span.borderEdges;
    format.borderStyle = span.borderStyle;
    format.shadingColorIndex = span.shadingColorIndex;
    format.alignment = span.alignment;
    format.pageBreakBefore = span.pageBreakBefore;
    format.keepWithNext = span.keepWithNext;
    format.dropCapLines = span.hasDropCap ? span.dropCapLines : 0;
    return format;
}

void TextBuffer::setLineParagraphFormat(std::size_t row,
                                        ParagraphFormat format) {
    if (row >= line_spans_.size()) return;
    setLineIndents(row, format.indents);
    LineSpan& span = line_spans_[row];
    span.lineSpacing = std::max(0.5f, std::min(3.0f, format.lineSpacing));
    span.spaceBefore = std::max(0, format.spaceBefore);
    span.spaceAfter = std::max(0, format.spaceAfter);
    span.borderEdges = format.borderEdges & borders_shading::kEdgeAll;
    span.borderStyle =
        format.borderStyle == borders_shading::kBorderStyleSingle
            ? borders_shading::kBorderStyleSingle
            : borders_shading::kBorderStyleNone;
    span.shadingColorIndex =
        borders_shading::clampShadingIndex(format.shadingColorIndex);
    span.alignment = format.alignment;
    span.pageBreakBefore = format.pageBreakBefore;
    span.keepWithNext = format.keepWithNext;
    // Drop cap: 0 = off (the last span is kept, so re-enabling
    // restores it); 2-4 = on with that span.
    if (format.dropCapLines >= drop_cap::kMinLines) {
        span.hasDropCap = true;
        span.dropCapLines =
            drop_cap::clampLines(format.dropCapLines);
    } else {
        span.hasDropCap = false;
    }
    version_++;
}

void TextBuffer::commitParagraphFormat(
    std::vector<ParagraphFormatChange> changes) {
    changes.erase(
        std::remove_if(changes.begin(), changes.end(),
                       [](const ParagraphFormatChange& c) {
                           return c.oldFormat == c.newFormat;
                       }),
        changes.end());
    if (changes.empty()) return;
    // Already applied live; record only, so the whole dialog OK
    // (every field, every selected paragraph) is one step.
    history_.push(
        std::make_unique<SetParagraphFormatCommand>(std::move(changes)));
}

void TextBuffer::commitParagraphStyle(std::size_t row,
                                      ParagraphStyle oldStyle,
                                      ParagraphStyle newStyle) {
    if (row >= line_spans_.size() || oldStyle == newStyle) return;
    // Already applied live; record only, so Promote / Demote is
    // one undo step.
    history_.push(std::make_unique<SetParagraphStyleCommand>(
        row, oldStyle, newStyle));
}

void TextBuffer::commitSwapAdjacentLineRanges(
    std::size_t firstStart, std::size_t boundary, std::size_t secondEnd,
    std::size_t oldCaretRow, std::size_t newCaretRow) {
    if (firstStart >= boundary || boundary >= secondEnd ||
        secondEnd > line_spans_.size()) {
        return;
    }
    // Already applied live; record only, so the Move is one step.
    history_.push(std::make_unique<SwapAdjacentLineRangesCommand>(
        firstStart, boundary, secondEnd, oldCaretRow, newCaretRow));
}

void TextBuffer::swapAdjacentLineRanges(std::size_t firstStart,
                                        std::size_t boundary,
                                        std::size_t secondEnd) {
    if (firstStart >= boundary || boundary >= secondEnd ||
        secondEnd > line_spans_.size()) {
        return;
    }
    // Offsets of the three boundaries. The combined range is one
    // contiguous run of chars; swapping the two segments keeps
    // the total length (and every offset after secondEnd) unchanged.
    std::size_t startOffset = line_spans_[firstStart].offset;
    std::size_t boundaryOffset = line_spans_[boundary].offset;
    std::size_t endOffset =
        secondEnd < line_spans_.size()
            ? line_spans_[secondEnd].offset
            : chars_.size();
    // Char-style spans move with their text: split every span at
    // the three boundaries and remap each piece. Normal case
    // (range ends mid-document): B moves to the start, A follows.
    // End-of-document case: the swapped string is B + "\n" + A
    // minus A's trailing "\n" (that newline moves to the middle),
    // so A-body pieces gain len(B)+1 and the trailing newline
    // piece lands at start+len(B).
    if (!charStyleSpans_.empty()) {
        std::size_t lenA = boundaryOffset - startOffset;
        std::size_t lenB = endOffset - boundaryOffset;
        bool endsAtDocEnd = secondEnd >= line_spans_.size();
        std::vector<std::size_t> cuts = {startOffset, boundaryOffset,
                                         endOffset};
        if (endsAtDocEnd && lenA > 0) cuts.push_back(boundaryOffset - 1);
        std::sort(cuts.begin(), cuts.end());
        std::vector<CharStyleSpan> remapped;
        for (const auto& span : charStyleSpans_) {
            std::size_t cur = span.startOffset;
            while (cur < span.endOffset) {
                std::size_t next = span.endOffset;
                for (std::size_t cut : cuts) {
                    if (cut > cur && cut < next) next = cut;
                }
                std::size_t pieceStart = cur;
                std::size_t pieceEnd = next;
                std::size_t mappedStart = pieceStart;
                if (pieceStart < startOffset || pieceStart >= endOffset) {
                    mappedStart = pieceStart;
                } else if (endsAtDocEnd && pieceStart == boundaryOffset - 1 &&
                           pieceEnd == boundaryOffset) {
                    mappedStart = startOffset + lenB;
                } else if (pieceStart < boundaryOffset) {
                    mappedStart = pieceStart + lenB + (endsAtDocEnd ? 1 : 0);
                } else {
                    mappedStart = pieceStart - lenA;
                }
                remapped.push_back(CharStyleSpan{
                    mappedStart, mappedStart + (pieceEnd - pieceStart),
                    span.style});
                cur = next;
            }
        }
        charStyleSpans_ = std::move(remapped);
        normalizeCharStyleSpans();
    }
    std::string combined;
    combined.reserve(endOffset - startOffset);
    for (std::size_t i = startOffset; i < endOffset; ++i) {
        combined.push_back(chars_.at(i));
    }
    std::string firstSeg = combined.substr(
        0, boundaryOffset - startOffset);
    std::string secondSeg = combined.substr(boundaryOffset - startOffset);
    // Terminators: a line's "\n" separates it from the next line
    // unless it is the last line of the document (no terminator).
    // When the combined range ends mid-document, both segments
    // already end with the terminator their last line needs in
    // its new position. When it ends at the end of the document,
    // the first segment's trailing "\n" must move: the second
    // segment's last line (now in the middle) gains a terminator,
    // and the first segment's last line (now last) loses its one.
    std::string swapped;
    if (secondEnd < line_spans_.size()) {
        swapped = secondSeg + firstSeg;
    } else {
        if (!firstSeg.empty() && firstSeg.back() == '\n') {
            firstSeg.pop_back();
        }
        swapped = secondSeg + "\n" + firstSeg;
    }
    // Incremental stats: no update. The swap reorders whole
    // lines (and reassigns the trailing terminator when the range
    // ends at the document end), so the multiset of line strings
    // — hence words, sentences, and paragraphs — is unchanged.
    chars_.erase(startOffset, endOffset - startOffset);
    if (!swapped.empty()) {
        chars_.insertString(startOffset, swapped.data(), swapped.size());
    }
    // Spans move with their lines: second block's spans first.
    std::vector<LineSpan> newSpans;
    newSpans.reserve(secondEnd - firstStart);
    for (std::size_t i = boundary; i < secondEnd; ++i) {
        newSpans.push_back(line_spans_[i]);
    }
    for (std::size_t i = firstStart; i < boundary; ++i) {
        newSpans.push_back(line_spans_[i]);
    }
    for (std::size_t i = 0; i < newSpans.size(); ++i) {
        line_spans_[firstStart + i] = newSpans[i];
    }
    // The moved spans carry their OLD offsets: the first span in
    // the range must start at the range start, and every later
    // offset is recomputed from it. (Forgetting this left the
    // spans pointing at the pre-swap positions, so lineString()
    // read the wrong characters after a swap.)
    line_spans_[firstStart].offset = startOffset;
    // Recompute offsets from the range start to the end of the
    // document: the combined range's length is unchanged, so
    // offsets after it are in fact unchanged, but recomputing
    // through the end keeps the invariant (offset[i] = offset[i-1]
    // + length[i-1] + terminator) true by construction.
    for (std::size_t i = firstStart + 1; i < line_spans_.size(); ++i) {
        const LineSpan& prev = line_spans_[i - 1];
        std::size_t terminator =
            (i - 1 == line_spans_.size() - 1) ? 0 : 1;
        line_spans_[i].offset =
            prev.offset + prev.length + terminator;
    }
    version_++;
}

// ============================================================================
// Spacing methods
// ============================================================================

float TextBuffer::currentLineSpacing() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].lineSpacing;
    }
    return 1.0f;
}

int TextBuffer::currentSpaceBefore() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].spaceBefore;
    }
    return 0;
}

int TextBuffer::currentSpaceAfter() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].spaceAfter;
    }
    return 0;
}

void TextBuffer::setCurrentLineSpacing(float multiplier) {
    if (caret_.row < line_spans_.size()) {
        // Clamp to reasonable range (0.5 to 3.0)
        line_spans_[caret_.row].lineSpacing = std::max(0.5f, std::min(3.0f, multiplier));
        version_++;
    }
}

void TextBuffer::setCurrentSpaceBefore(int pixels) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].spaceBefore = std::max(0, pixels);
        version_++;
    }
}

void TextBuffer::setCurrentSpaceAfter(int pixels) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].spaceAfter = std::max(0, pixels);
        version_++;
    }
}

void TextBuffer::setLineSpacingSingle() {
    setCurrentLineSpacing(1.0f);
}

void TextBuffer::setLineSpacing1_5() {
    setCurrentLineSpacing(1.5f);
}

void TextBuffer::setLineSpacingDouble() {
    setCurrentLineSpacing(2.0f);
}

float TextBuffer::lineSpacing(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].lineSpacing;
    }
    return 1.0f;
}

int TextBuffer::lineSpaceBefore(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].spaceBefore;
    }
    return 0;
}

int TextBuffer::lineSpaceAfter(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].spaceAfter;
    }
    return 0;
}

ListType TextBuffer::currentListType() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].listType;
    }
    return ListType::None;
}

int TextBuffer::currentListLevel() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].listLevel;
    }
    return 0;
}

void TextBuffer::setCurrentListType(ListType type) {
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].listType = type;
        if (type == ListType::Numbered) {
            // Renumber from this line forward
            renumberListsFrom(caret_.row);
        }
        version_++;
    }
}

void TextBuffer::toggleBulletedList() {
    if (caret_.row < line_spans_.size()) {
        if (line_spans_[caret_.row].listType == ListType::Bulleted) {
            line_spans_[caret_.row].listType = ListType::None;
            line_spans_[caret_.row].listLevel = 0;
        } else {
            line_spans_[caret_.row].listType = ListType::Bulleted;
        }
        version_++;
    }
}

void TextBuffer::toggleNumberedList() {
    if (caret_.row < line_spans_.size()) {
        if (line_spans_[caret_.row].listType == ListType::Numbered) {
            line_spans_[caret_.row].listType = ListType::None;
            line_spans_[caret_.row].listLevel = 0;
        } else {
            line_spans_[caret_.row].listType = ListType::Numbered;
            renumberListsFrom(caret_.row);
        }
        version_++;
    }
}

void TextBuffer::increaseListLevel() {
    if (caret_.row < line_spans_.size()) {
        if (line_spans_[caret_.row].listType != ListType::None) {
            line_spans_[caret_.row].listLevel = std::min(8, line_spans_[caret_.row].listLevel + 1);
            if (line_spans_[caret_.row].listType == ListType::Numbered) {
                renumberListsFrom(caret_.row);
            }
            version_++;
        }
    }
}

void TextBuffer::decreaseListLevel() {
    if (caret_.row < line_spans_.size()) {
        if (line_spans_[caret_.row].listType != ListType::None && line_spans_[caret_.row].listLevel > 0) {
            line_spans_[caret_.row].listLevel--;
            if (line_spans_[caret_.row].listType == ListType::Numbered) {
                renumberListsFrom(caret_.row);
            }
            version_++;
        }
    }
}

ListType TextBuffer::lineListType(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].listType;
    }
    return ListType::None;
}

int TextBuffer::lineListLevel(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].listLevel;
    }
    return 0;
}

int TextBuffer::lineListNumber(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].listNumber;
    }
    return 1;
}

// Page break methods
void TextBuffer::insertPageBreak() {
    ensureNonEmpty();
    
    // Delete any selected text first
    deleteSelection();
    
    // Insert a newline (creates a new line)
    insertChar('\n');
    
    // Mark the new line as having a page break before it
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].hasPageBreakBefore = true;
        version_++;  // Content changed - invalidate render cache
    }
}

bool TextBuffer::hasPageBreakBefore(std::size_t row) const {
    if (row < line_spans_.size()) {
        return line_spans_[row].hasPageBreakBefore;
    }
    return false;
}

void TextBuffer::togglePageBreak() {
    ensureNonEmpty();
    if (caret_.row < line_spans_.size() && caret_.row > 0) {
        line_spans_[caret_.row].hasPageBreakBefore = 
            !line_spans_[caret_.row].hasPageBreakBefore;
        version_++;  // Content changed - invalidate render cache
    }
}

void TextBuffer::clearPageBreak() {
    ensureNonEmpty();
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].hasPageBreakBefore = false;
        version_++;  // Content changed - invalidate render cache
    }
}

bool TextBuffer::currentLineHasDropCap() const {
    if (caret_.row < line_spans_.size()) {
        return line_spans_[caret_.row].hasDropCap;
    }
    return false;
}

void TextBuffer::setCurrentLineDropCap(bool enabled, int spanLines) {
    ensureNonEmpty();
    if (caret_.row < line_spans_.size()) {
        line_spans_[caret_.row].hasDropCap = enabled;
        if (enabled) {
            line_spans_[caret_.row].dropCapLines =
                drop_cap::clampLines(spanLines);
        }
        version_++;  // Invalidate render cache
    }
}

int TextBuffer::lineDropCapLines(std::size_t row) const {
    if (row < line_spans_.size() && line_spans_[row].hasDropCap) {
        return drop_cap::clampLines(line_spans_[row].dropCapLines);
    }
    return 0;
}

int TextBuffer::currentDropCapLines() const {
    return lineDropCapLines(caret_.row);
}

void TextBuffer::toggleCurrentLineDropCap() {
    if (caret_.row < line_spans_.size()) {
        bool enabled = !line_spans_[caret_.row].hasDropCap;
        setCurrentLineDropCap(enabled, line_spans_[caret_.row].dropCapLines);
    }
}

void TextBuffer::renumberListsFrom(std::size_t startRow) {
    // Simple renumbering: count from startRow, respecting levels
    // Each level maintains its own counter
    std::vector<int> levelCounters(9, 0);  // Support up to 9 levels
    
    // Find the start of this list block (scan backwards)
    std::size_t blockStart = startRow;
    while (blockStart > 0 && line_spans_[blockStart - 1].listType == ListType::Numbered) {
        blockStart--;
    }
    
    // Reset counters and renumber from block start
    for (std::size_t row = blockStart; row < line_spans_.size(); row++) {
        if (line_spans_[row].listType != ListType::Numbered) {
            // End of numbered list block
            break;
        }
        int level = line_spans_[row].listLevel;
        levelCounters[static_cast<size_t>(level)]++;
        line_spans_[row].listNumber = levelCounters[static_cast<size_t>(level)];
        
        // Reset counters for deeper levels when we're at a shallower level
        for (int i = level + 1; i < 9; i++) {
            levelCounters[static_cast<size_t>(i)] = 0;
        }
    }
}

TextBuffer::PerfStats TextBuffer::perfStats() const {
    PerfStats stats;
    stats.total_inserts = stats_.total_inserts;
    stats.total_deletes = stats_.total_deletes;
    stats.gap_moves = chars_.gapMoves();
    stats.buffer_reallocations = chars_.reallocations();
    return stats;
}

void TextBuffer::resetPerfStats() {
    stats_ = {};
    chars_.resetStats();
}

void TextBuffer::backspace() {
    ensureNonEmpty();

    // If there's a selection, delete it instead of single char
    if (deleteSelection()) {
        return;
    }

    if (caret_.column > 0) {
        // Delete character before caret on same line
        std::size_t offset = positionToOffset(caret_);
        char deletedChar = chars_.at(offset - 1);
        CaretPosition deletePos = {caret_.row, caret_.column - 1};

        // Incremental stats: deleting the line's last char
        // removes its paragraph.
        const std::ptrdiff_t paragraphDelta =
            line_spans_[caret_.row].length == 1 ? -1 : 0;
        noteStatsCharErased(offset - 1, paragraphDelta);
        noteEditShift(offset - 1, -1);
        chars_.erase(offset - 1, 1);
        stats_.total_deletes++;
        version_++;  // Content changed - invalidate render cache

        // Adjust hyperlink and bookmark offsets for the deleted character
        adjustHyperlinkOffsets(offset - 1, -1);
        adjustBookmarkOffsets(offset - 1, -1);
        adjustTocOffsets(offset - 1, -1);
        adjustCharStyleSpans(offset - 1, -1);

        line_spans_[caret_.row].length -= 1;
        shiftLineOffsetsFrom(caret_.row + 1, -1);
        caret_.column -= 1;

        // Record for undo
        if (recordingHistory_) {
            history_.push(std::make_unique<DeleteCharCommand>(
                deletePos, deletedChar, true));
        }
        return;
    }

    if (caret_.row == 0) {
        return;
    }

    // Join with previous line - delete the newline
    std::size_t prev_line_len = line_spans_[caret_.row - 1].length;
    std::size_t newline_offset =
        line_spans_[caret_.row - 1].offset + prev_line_len;
    CaretPosition deletePos = {caret_.row - 1, prev_line_len};

    // Incremental stats: merging the previous line (a) with the
    // current line (b) drops to one paragraph iff the join is
    // non-empty.
    {
        const std::size_t first = prev_line_len;
        const std::size_t second = line_spans_[caret_.row].length;
        const std::ptrdiff_t paragraphDelta =
            static_cast<std::ptrdiff_t>((first + second > 0 ? 1 : 0) -
                                        (first > 0 ? 1 : 0) -
                                        (second > 0 ? 1 : 0));
        noteStatsCharErased(newline_offset, paragraphDelta);
    }
    noteEditShift(newline_offset, -1);
    chars_.erase(newline_offset, 1);
    stats_.total_deletes++;
    version_++;

    // Adjust hyperlink and bookmark offsets for the deleted newline
    adjustHyperlinkOffsets(newline_offset, -1);
    adjustBookmarkOffsets(newline_offset, -1);
    adjustTocOffsets(newline_offset, -1);
    adjustCharStyleSpans(newline_offset, -1);

    rebuildLineIndex();

    caret_.row -= 1;
    caret_.column = prev_line_len;

    // Record for undo (deleted a newline)
    if (recordingHistory_) {
        history_.push(
            std::make_unique<DeleteCharCommand>(deletePos, '\n', true));
    }
}

void TextBuffer::del() {
    ensureNonEmpty();

    // If there's a selection, delete it instead of single char
    if (deleteSelection()) {
        return;
    }

    const LineSpan& span = line_spans_[caret_.row];

    if (caret_.column < span.length) {
        // Delete character at caret
        std::size_t offset = positionToOffset(caret_);
        char deletedChar = chars_.at(offset);
        CaretPosition deletePos = caret_;

        // Incremental stats: deleting the line's last char
        // removes its paragraph.
        const std::ptrdiff_t paragraphDelta =
            line_spans_[caret_.row].length == 1 ? -1 : 0;
        noteStatsCharErased(offset, paragraphDelta);
        noteEditShift(offset, -1);
        chars_.erase(offset, 1);
        stats_.total_deletes++;
        version_++;

        // Adjust hyperlink and bookmark offsets for the deleted character
        adjustHyperlinkOffsets(offset, -1);
        adjustBookmarkOffsets(offset, -1);
        adjustTocOffsets(offset, -1);
        adjustCharStyleSpans(offset, -1);

        line_spans_[caret_.row].length -= 1;
        shiftLineOffsetsFrom(caret_.row + 1, -1);

        // Record for undo
        if (recordingHistory_) {
            history_.push(std::make_unique<DeleteCharCommand>(
                deletePos, deletedChar, false));
        }
        return;
    }

    if (caret_.row + 1 >= line_spans_.size()) {
        return;
    }

    // Join with next line - delete the newline at end of current line
    std::size_t newline_offset = span.offset + span.length;
    CaretPosition deletePos = caret_;

    // Incremental stats: merging the current line (a) with the
    // next line (b) drops to one paragraph iff the join is
    // non-empty.
    {
        const std::size_t first = span.length;
        const std::size_t second =
            line_spans_[caret_.row + 1].length;
        const std::ptrdiff_t paragraphDelta =
            static_cast<std::ptrdiff_t>((first + second > 0 ? 1 : 0) -
                                        (first > 0 ? 1 : 0) -
                                        (second > 0 ? 1 : 0));
        noteStatsCharErased(newline_offset, paragraphDelta);
    }
    noteEditShift(newline_offset, -1);
    chars_.erase(newline_offset, 1);
    stats_.total_deletes++;
    version_++;

    // Adjust hyperlink and bookmark offsets for the deleted newline
    adjustHyperlinkOffsets(newline_offset, -1);
    adjustBookmarkOffsets(newline_offset, -1);
    adjustTocOffsets(newline_offset, -1);
    adjustCharStyleSpans(newline_offset, -1);

    rebuildLineIndex();

    // Record for undo (deleted a newline)
    if (recordingHistory_) {
        history_.push(
            std::make_unique<DeleteCharCommand>(deletePos, '\n', false));
    }
}

void TextBuffer::moveLeft() {
    if (caret_.column > 0) {
        caret_.column -= 1;
        return;
    }
    if (caret_.row == 0) {
        return;
    }
    caret_.row -= 1;
    caret_.column = line_spans_[caret_.row].length;
}

void TextBuffer::moveRight() {
    const LineSpan& span = line_spans_[caret_.row];
    if (caret_.column < span.length) {
        caret_.column += 1;
        return;
    }
    if (caret_.row + 1 >= line_spans_.size()) {
        return;
    }
    caret_.row += 1;
    caret_.column = 0;
}

void TextBuffer::moveUp() {
    if (caret_.row == 0) {
        return;
    }
    caret_.row -= 1;
    caret_.column = std::min(caret_.column, line_spans_[caret_.row].length);
}

void TextBuffer::moveDown() {
    if (caret_.row + 1 >= line_spans_.size()) {
        return;
    }
    caret_.row += 1;
    caret_.column = std::min(caret_.column, line_spans_[caret_.row].length);
}

void TextBuffer::moveWordLeft() {
    if (chars_.empty()) return;

    // Use afterhours utility for better UTF-8 support
    std::string text = getText();
    std::size_t offset = positionToOffset(caret_);
    std::size_t new_offset = afterhours::text_input::find_word_start(text, offset);
    caret_ = offsetToPosition(new_offset);
}

void TextBuffer::moveWordRight() {
    if (chars_.empty()) return;

    // Use afterhours utility for better UTF-8 support
    std::string text = getText();
    std::size_t offset = positionToOffset(caret_);
    std::size_t new_offset = afterhours::text_input::find_word_end(text, offset);
    caret_ = offsetToPosition(new_offset);
}

void TextBuffer::moveToLineStart() { caret_.column = 0; }

void TextBuffer::moveToLineEnd() {
    if (caret_.row < line_spans_.size()) {
        caret_.column = line_spans_[caret_.row].length;
    }
}

void TextBuffer::moveToDocumentStart() {
    caret_.row = 0;
    caret_.column = 0;
}

void TextBuffer::moveToDocumentEnd() {
    if (!line_spans_.empty()) {
        caret_.row = line_spans_.size() - 1;
        caret_.column = line_spans_[caret_.row].length;
    }
}

void TextBuffer::movePageUp(std::size_t linesPerPage) {
    if (line_spans_.empty()) return;

    if (caret_.row >= linesPerPage) {
        caret_.row -= linesPerPage;
    } else {
        caret_.row = 0;
    }
    caret_.column = std::min(caret_.column, line_spans_[caret_.row].length);
}

void TextBuffer::movePageDown(std::size_t linesPerPage) {
    if (line_spans_.empty()) return;

    caret_.row += linesPerPage;
    if (caret_.row >= line_spans_.size()) {
        caret_.row = line_spans_.size() - 1;
    }
    caret_.column = std::min(caret_.column, line_spans_[caret_.row].length);
}

void TextBuffer::ensureNonEmpty() {
    if (line_spans_.empty()) {
        line_spans_.push_back({0, 0});
    }
    clampCaret();
}

void TextBuffer::clampCaret() {
    if (line_spans_.empty()) {
        caret_ = {};
        return;
    }
    if (caret_.row >= line_spans_.size()) {
        caret_.row = line_spans_.size() - 1;
    }
    std::size_t max_column = line_spans_[caret_.row].length;
    if (caret_.column > max_column) {
        caret_.column = max_column;
    }
}

int TextBuffer::comparePositions(const CaretPosition& a,
                                 const CaretPosition& b) {
    if (a.row != b.row) {
        return a.row < b.row ? -1 : 1;
    }
    if (a.column != b.column) {
        return a.column < b.column ? -1 : 1;
    }
    return 0;
}

// ============================================================================
// EditCommand implementations for undo/redo
// ============================================================================

void InsertCharCommand::execute(TextBuffer& buffer) {
    buffer.insertCharAt(position_, char_);
}

void InsertCharCommand::undo(TextBuffer& buffer) {
    buffer.deleteCharAt(position_);
    buffer.setCaret(position_);
}

void DeleteCharCommand::execute(TextBuffer& buffer) {
    buffer.deleteCharAt(position_);
}

void DeleteCharCommand::undo(TextBuffer& buffer) {
    buffer.insertCharAt(position_, char_);
    if (isBackspace_) {
        if (char_ == '\n') {
            buffer.setCaret({position_.row + 1, 0});
        } else {
            buffer.setCaret({position_.row, position_.column + 1});
        }
    } else {
        buffer.setCaret(position_);
    }
}

void DeleteSelectionCommand::execute(TextBuffer& buffer) {
    buffer.setCaret(start_);
    buffer.setSelectionAnchor(start_);
    buffer.setCaret(end_);
    buffer.updateSelectionToCaret();
    buffer.deleteSelection();
}

void DeleteSelectionCommand::undo(TextBuffer& buffer) {
    buffer.insertTextAt(start_, deletedText_);
    buffer.setCaret(end_);
}

void SetParagraphStyleCommand::execute(TextBuffer& buffer) {
    buffer.setLineParagraphStyle(row_, new_);
}

void SetParagraphStyleCommand::undo(TextBuffer& buffer) {
    buffer.setLineParagraphStyle(row_, old_);
}

void SwapAdjacentLineRangesCommand::execute(TextBuffer& buffer) {
    buffer.swapAdjacentLineRanges(firstStart_, boundary_, secondEnd_);
    buffer.clearSelection();
    buffer.setCaret({newCaretRow_, 0});
}

void SwapAdjacentLineRangesCommand::undo(TextBuffer& buffer) {
    // After execute, the second block sits first: its size is
    // secondEnd_ - boundary_, so the shifted boundary is
    // firstStart_ + (secondEnd_ - boundary_). Swapping with that
    // boundary restores the original order.
    std::size_t shifted =
        firstStart_ + (secondEnd_ - boundary_);
    buffer.swapAdjacentLineRanges(firstStart_, shifted, secondEnd_);
    buffer.clearSelection();
    buffer.setCaret({oldCaretRow_, 0});
}

void SetParagraphFormatCommand::execute(TextBuffer& buffer) {
    for (const auto& change : changes_) {
        buffer.setLineParagraphFormat(change.row, change.newFormat);
    }
}

void SetParagraphFormatCommand::undo(TextBuffer& buffer) {
    for (const auto& change : changes_) {
        buffer.setLineParagraphFormat(change.row, change.oldFormat);
    }
}

void SetParagraphIndentsCommand::execute(TextBuffer& buffer) {
    buffer.setLineIndents(row_, new_);
}

void SetParagraphIndentsCommand::undo(TextBuffer& buffer) {
    buffer.setLineIndents(row_, old_);
}

void ReplaceRangesCommand::applyRange(TextBuffer& buffer, CaretPosition start,
                                      const std::string& oldText,
                                      const std::string& newText) {
    // Low-level ops only: no history recording, so this whole command is
    // one undo step no matter how many ranges or characters it covers.
    buffer.setCaret(start);
    for (std::size_t i = 0; i < oldText.size(); ++i) {
        buffer.deleteCharAt(start);
    }
    CaretPosition cur = start;
    for (char ch : newText) {
        buffer.insertCharAt(cur, ch);
        cur = buffer.caret();
    }
    buffer.setCaret(cur);
}

void ReplaceRangesCommand::execute(TextBuffer& buffer) {
    // Back to front so earlier offsets stay valid while later ones change.
    for (auto it = replacements_.rbegin(); it != replacements_.rend(); ++it) {
        applyRange(buffer, it->start, it->oldText, it->newText);
    }
}

void ReplaceRangesCommand::undo(TextBuffer& buffer) {
    // Same order as execute: undoing later ranges first leaves earlier
    // positions valid, since they sit before everything already restored.
    for (auto it = replacements_.rbegin(); it != replacements_.rend(); ++it) {
        applyRange(buffer, it->start, it->newText, it->oldText);
    }
}

// ============================================================================
// ============================================================================

// ============================================================================
// TextBuffer undo/redo methods
// ============================================================================

void TextBuffer::undo() {
    if (!canUndo()) return;
    recordingHistory_ = false;
    history_.undo(*this);
    recordingHistory_ = true;
    version_++;
}

void TextBuffer::redo() {
    if (!canRedo()) return;
    recordingHistory_ = false;
    history_.redo(*this);
    recordingHistory_ = true;
    version_++;
}

void TextBuffer::insertCharAt(CaretPosition pos, char ch) {
    setCaret(pos);
    std::size_t offset = positionToOffset(pos);
    // Incremental stats: same split/empty-line paragraph math as
    // insertChar.
    {
        const std::size_t oldLength = line_spans_[caret_.row].length;
        std::ptrdiff_t paragraphDelta = 0;
        if (ch == '\n') {
            const std::size_t first = caret_.column;
            const std::size_t second = oldLength - first;
            paragraphDelta =
                static_cast<std::ptrdiff_t>((first > 0 ? 1 : 0) +
                                            (second > 0 ? 1 : 0) -
                                            (oldLength > 0 ? 1 : 0));
        } else if (oldLength == 0) {
            paragraphDelta = 1;
        }
        noteStatsCharInserted(offset, ch, paragraphDelta);
    }
    noteEditShift(offset, 1);
    chars_.insert(offset, ch);
    version_++;
    adjustCharStyleSpans(offset, 1);

    if (ch == '\n') {
        // Split current line - preserve paragraph styles
        std::size_t splitRow = caret_.row;
        LineSpan oldSpan = line_spans_[splitRow];
        
        // Current line ends at caret position
        line_spans_[splitRow].length = caret_.column;
        
        // New line starts after newline character
        LineSpan newSpan;
        newSpan.offset = offset + 1;
        newSpan.length = (oldSpan.offset + oldSpan.length) - offset;
        newSpan.style = ParagraphStyle::Normal;
        newSpan.alignment = oldSpan.alignment;
        
        // Insert new line span
        line_spans_.insert(line_spans_.begin() + splitRow + 1, newSpan);
        shiftLineOffsetsFrom(splitRow + 2, 1);
        
        caret_.row += 1;
        caret_.column = 0;
    } else {
        if (caret_.row < line_spans_.size()) {
            line_spans_[caret_.row].length += 1;
            shiftLineOffsetsFrom(caret_.row + 1, 1);
        }
        caret_.column += 1;
    }
}

void TextBuffer::deleteCharAt(CaretPosition pos) {
    if (line_spans_.empty()) return;
    setCaret(pos);
    if (pos.row >= line_spans_.size()) return;

    const LineSpan& span = line_spans_[pos.row];
    if (pos.column < span.length) {
        // Incremental stats: deleting the line's last char
        // removes its paragraph.
        const std::ptrdiff_t paragraphDelta =
            span.length == 1 ? -1 : 0;
        std::size_t offset = positionToOffset(pos);
        noteStatsCharErased(offset, paragraphDelta);
        noteEditShift(offset, -1);
        chars_.erase(offset, 1);
        version_++;
        adjustCharStyleSpans(offset, -1);
        line_spans_[pos.row].length -= 1;
        shiftLineOffsetsFrom(pos.row + 1, -1);
    } else if (pos.row + 1 < line_spans_.size()) {
        // Deleting newline at end of line - merge with next line
        std::size_t offset = span.offset + span.length;
        // Incremental stats: merging this line (a) with the next
        // line (b) drops to one paragraph iff the join is
        // non-empty.
        {
            const std::size_t first = span.length;
            const std::size_t second = line_spans_[pos.row + 1].length;
            const std::ptrdiff_t paragraphDelta =
                static_cast<std::ptrdiff_t>(
                    (first + second > 0 ? 1 : 0) - (first > 0 ? 1 : 0) -
                    (second > 0 ? 1 : 0));
            noteStatsCharErased(offset, paragraphDelta);
        }
        noteEditShift(offset, -1);
        chars_.erase(offset, 1);
        version_++;
        adjustCharStyleSpans(offset, -1);

        // Merge next line into current line (keep current line's style)
        LineSpan& nextSpan = line_spans_[pos.row + 1];
        line_spans_[pos.row].length += nextSpan.length;
        
        // Remove the next line span
        line_spans_.erase(line_spans_.begin() + pos.row + 1);
        
        // Shift subsequent lines
        shiftLineOffsetsFrom(pos.row + 1, -1);
    }
}

void TextBuffer::insertTextAt(CaretPosition pos, const std::string& text) {
    setCaret(pos);
    recordingHistory_ = false;
    for (char ch : text) {
        insertCharAt(caret_, ch);
    }
    recordingHistory_ = true;
}

void TextBuffer::replaceRange(std::size_t offset, std::size_t length,
                              const std::string& replacement) {
    if (offset > chars_.size() || length > chars_.size() - offset) return;
    CaretPosition start = offsetToPosition(offset);
    std::string oldText;
    oldText.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        oldText.push_back(chars_.at(offset + i));
    }
    replaceRanges({{start, oldText, replacement}});
}

void TextBuffer::replaceRanges(std::vector<TextReplacement> replacements) {
    if (replacements.empty()) return;
    // Ascending by (row, column) so back-to-front application in the
    // command keeps every range's position valid.
    std::sort(replacements.begin(), replacements.end(),
              [](const TextReplacement& a, const TextReplacement& b) {
                  if (a.start.row != b.start.row) return a.start.row < b.start.row;
                  return a.start.column < b.start.column;
              });
    history_.execute(
        std::make_unique<ReplaceRangesCommand>(std::move(replacements)), *this);
}

std::size_t TextBuffer::caretOffset() const {
    return positionToOffset(caret_);
}

std::size_t TextBuffer::offsetForPosition(const CaretPosition& pos) const {
    return positionToOffset(pos);
}

CaretPosition TextBuffer::positionForOffset(std::size_t offset) const {
    return offsetToPosition(offset);
}

char TextBuffer::charAtOffset(std::size_t offset) const {
    if (offset >= chars_.size()) {
        return '\0';
    }
    return chars_.at(offset);
}

// ============================================================================
// Find and Replace
// ============================================================================

// Helper to compare characters for case-insensitive matching
static bool charEquals(char a, char b, bool caseSensitive) {
    if (caseSensitive) return a == b;
    return std::tolower(static_cast<unsigned char>(a)) == 
           std::tolower(static_cast<unsigned char>(b));
}

// Helper to check if position is at a word boundary
static bool isWordBoundary(const std::string& text, std::size_t pos, bool atStart) {
    if (atStart) {
        // Start of word: previous char is not alphanumeric or is at start
        if (pos == 0) return true;
        return !std::isalnum(static_cast<unsigned char>(text[pos - 1]));
    } else {
        // End of word: current char is not alphanumeric or is at end
        if (pos >= text.size()) return true;
        return !std::isalnum(static_cast<unsigned char>(text[pos]));
    }
}

static bool regexFindForward(const std::string& text, const std::string& pattern,
                             std::size_t startOffset, bool wrapAround,
                             bool caseSensitive, std::size_t& outStart,
                             std::size_t& outLen) {
    std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
    if (!caseSensitive) {
        flags |= std::regex_constants::icase;
    }
    std::regex re(pattern, flags);

    auto searchFrom = [&](std::size_t offset) -> bool {
        if (offset > text.size()) return false;
        std::cmatch match;
        const char* startPtr = text.c_str() + offset;
        if (std::regex_search(startPtr, text.c_str() + text.size(), match, re)) {
            outStart = offset + static_cast<std::size_t>(match.position());
            outLen = static_cast<std::size_t>(match.length());
            return true;
        }
        return false;
    };

    if (searchFrom(startOffset)) {
        return true;
    }
    if (wrapAround && startOffset > 0) {
        return searchFrom(0);
    }
    return false;
}

FindResult TextBuffer::find(const std::string& needle, const FindOptions& options) const {
    if (needle.empty()) return {false, {0, 0}, {0, 0}};
    
    std::string text = getText();
    std::size_t startOffset = positionToOffset(caret_);

    if (options.useRegex) {
        std::string pattern = options.wholeWord ? ("\\b(?:" + needle + ")\\b") : needle;
        std::size_t matchStart = 0;
        std::size_t matchLen = 0;
        if (regexFindForward(text, pattern, startOffset, options.wrapAround,
                             options.caseSensitive, matchStart, matchLen)) {
            CaretPosition start = offsetToPosition(matchStart);
            CaretPosition end = offsetToPosition(matchStart + matchLen);
            return {true, start, end};
        }
        return {false, {0, 0}, {0, 0}};
    }
    
    // Search forward from caret position
    for (std::size_t i = startOffset; i + needle.length() <= text.length(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.length() && match; ++j) {
            if (!charEquals(text[i + j], needle[j], options.caseSensitive)) {
                match = false;
            }
        }
        if (match) {
            // Check whole word if required
            if (options.wholeWord) {
                if (!isWordBoundary(text, i, true) || 
                    !isWordBoundary(text, i + needle.length(), false)) {
                    continue;
                }
            }
            CaretPosition start = offsetToPosition(i);
            CaretPosition end = offsetToPosition(i + needle.length());
            return {true, start, end};
        }
    }
    
    // Wrap around if enabled
    if (options.wrapAround && startOffset > 0) {
        for (std::size_t i = 0; i < startOffset && i + needle.length() <= text.length(); ++i) {
            bool match = true;
            for (std::size_t j = 0; j < needle.length() && match; ++j) {
                if (!charEquals(text[i + j], needle[j], options.caseSensitive)) {
                    match = false;
                }
            }
            if (match) {
                if (options.wholeWord) {
                    if (!isWordBoundary(text, i, true) || 
                        !isWordBoundary(text, i + needle.length(), false)) {
                        continue;
                    }
                }
                CaretPosition start = offsetToPosition(i);
                CaretPosition end = offsetToPosition(i + needle.length());
                return {true, start, end};
            }
        }
    }
    
    return {false, {0, 0}, {0, 0}};
}

FindResult TextBuffer::findNext(const std::string& needle, const FindOptions& options) const {
    if (needle.empty()) return {false, {0, 0}, {0, 0}};
    
    std::string text = getText();
    // Start after current selection/caret
    std::size_t startOffset = positionToOffset(caret_);
    if (hasSelection()) {
        CaretPosition selEnd = selectionEnd();
        startOffset = positionToOffset(selEnd);
    }
    
    // Skip past current position to find next
    if (startOffset < text.length()) {
        startOffset++;
    }

    if (options.useRegex) {
        std::string pattern = options.wholeWord ? ("\\b(?:" + needle + ")\\b") : needle;
        std::size_t matchStart = 0;
        std::size_t matchLen = 0;
        if (regexFindForward(text, pattern, startOffset, options.wrapAround,
                             options.caseSensitive, matchStart, matchLen)) {
            CaretPosition start = offsetToPosition(matchStart);
            CaretPosition end = offsetToPosition(matchStart + matchLen);
            return {true, start, end};
        }
        return {false, {0, 0}, {0, 0}};
    }
    
    // Search forward
    for (std::size_t i = startOffset; i + needle.length() <= text.length(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.length() && match; ++j) {
            if (!charEquals(text[i + j], needle[j], options.caseSensitive)) {
                match = false;
            }
        }
        if (match) {
            if (options.wholeWord) {
                if (!isWordBoundary(text, i, true) || 
                    !isWordBoundary(text, i + needle.length(), false)) {
                    continue;
                }
            }
            CaretPosition start = offsetToPosition(i);
            CaretPosition end = offsetToPosition(i + needle.length());
            return {true, start, end};
        }
    }
    
    // Wrap around if enabled
    if (options.wrapAround) {
        for (std::size_t i = 0; i + needle.length() <= text.length() && i < startOffset; ++i) {
            bool match = true;
            for (std::size_t j = 0; j < needle.length() && match; ++j) {
                if (!charEquals(text[i + j], needle[j], options.caseSensitive)) {
                    match = false;
                }
            }
            if (match) {
                if (options.wholeWord) {
                    if (!isWordBoundary(text, i, true) || 
                        !isWordBoundary(text, i + needle.length(), false)) {
                        continue;
                    }
                }
                CaretPosition start = offsetToPosition(i);
                CaretPosition end = offsetToPosition(i + needle.length());
                return {true, start, end};
            }
        }
    }
    
    return {false, {0, 0}, {0, 0}};
}

FindResult TextBuffer::findPrevious(const std::string& needle, const FindOptions& options) const {
    if (needle.empty()) return {false, {0, 0}, {0, 0}};
    
    std::string text = getText();
    std::size_t endOffset = positionToOffset(caret_);
    if (endOffset > 0) endOffset--;  // Start before current position

    if (options.useRegex) {
        std::string pattern = options.wholeWord ? ("\\b(?:" + needle + ")\\b") : needle;
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        if (!options.caseSensitive) {
            flags |= std::regex_constants::icase;
        }
        std::regex re(pattern, flags);

        std::size_t matchStart = std::string::npos;
        std::size_t matchLen = 0;
        for (std::sregex_iterator it(text.begin(), text.end(), re), end; it != end; ++it) {
            std::size_t pos = static_cast<std::size_t>(it->position());
            if (pos <= endOffset) {
                matchStart = pos;
                matchLen = static_cast<std::size_t>(it->length());
            } else {
                break;
            }
        }
        if (matchStart != std::string::npos) {
            CaretPosition start = offsetToPosition(matchStart);
            CaretPosition end = offsetToPosition(matchStart + matchLen);
            return {true, start, end};
        }
        if (options.wrapAround) {
            std::size_t lastStart = std::string::npos;
            std::size_t lastLen = 0;
            for (std::sregex_iterator it(text.begin(), text.end(), re), end; it != end; ++it) {
                lastStart = static_cast<std::size_t>(it->position());
                lastLen = static_cast<std::size_t>(it->length());
            }
            if (lastStart != std::string::npos) {
                CaretPosition start = offsetToPosition(lastStart);
                CaretPosition end = offsetToPosition(lastStart + lastLen);
                return {true, start, end};
            }
        }
        return {false, {0, 0}, {0, 0}};
    }
    
    // Search backward
    for (std::size_t i = endOffset; i != static_cast<std::size_t>(-1); --i) {
        if (i + needle.length() > text.length()) continue;
        
        bool match = true;
        for (std::size_t j = 0; j < needle.length() && match; ++j) {
            if (!charEquals(text[i + j], needle[j], options.caseSensitive)) {
                match = false;
            }
        }
        if (match) {
            if (options.wholeWord) {
                if (!isWordBoundary(text, i, true) || 
                    !isWordBoundary(text, i + needle.length(), false)) {
                    continue;
                }
            }
            CaretPosition start = offsetToPosition(i);
            CaretPosition end = offsetToPosition(i + needle.length());
            return {true, start, end};
        }
        if (i == 0) break;
    }
    
    // Wrap around if enabled
    if (options.wrapAround && text.length() >= needle.length()) {
        for (std::size_t i = text.length() - needle.length(); i > endOffset; --i) {
            bool match = true;
            for (std::size_t j = 0; j < needle.length() && match; ++j) {
                if (!charEquals(text[i + j], needle[j], options.caseSensitive)) {
                    match = false;
                }
            }
            if (match) {
                if (options.wholeWord) {
                    if (!isWordBoundary(text, i, true) || 
                        !isWordBoundary(text, i + needle.length(), false)) {
                        continue;
                    }
                }
                CaretPosition start = offsetToPosition(i);
                CaretPosition end = offsetToPosition(i + needle.length());
                return {true, start, end};
            }
        }
    }
    
    return {false, {0, 0}, {0, 0}};
}

std::vector<FindResult> TextBuffer::findAll(const std::string& needle, const FindOptions& options) const {
    std::vector<FindResult> results;
    if (needle.empty()) return results;
    
    std::string text = getText();

    if (options.useRegex) {
        std::string pattern = options.wholeWord ? ("\\b(?:" + needle + ")\\b") : needle;
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        if (!options.caseSensitive) {
            flags |= std::regex_constants::icase;
        }
        std::regex re(pattern, flags);
        for (std::sregex_iterator it(text.begin(), text.end(), re), end; it != end; ++it) {
            std::size_t pos = static_cast<std::size_t>(it->position());
            std::size_t len = static_cast<std::size_t>(it->length());
            CaretPosition start = offsetToPosition(pos);
            CaretPosition endPos = offsetToPosition(pos + len);
            results.push_back({true, start, endPos});
        }
        return results;
    }
    
    for (std::size_t i = 0; i + needle.length() <= text.length(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.length() && match; ++j) {
            if (!charEquals(text[i + j], needle[j], options.caseSensitive)) {
                match = false;
            }
        }
        if (match) {
            if (options.wholeWord) {
                if (!isWordBoundary(text, i, true) || 
                    !isWordBoundary(text, i + needle.length(), false)) {
                    continue;
                }
            }
            CaretPosition start = offsetToPosition(i);
            CaretPosition end = offsetToPosition(i + needle.length());
            results.push_back({true, start, end});
        }
    }
    
    return results;
}

bool TextBuffer::replace(const std::string& needle, const std::string& replacement, 
                         const FindOptions& options) {
    if (!hasSelection() || needle.empty()) return false;
    
    // Check if selection matches needle
    std::string selected = getSelectedText();

    if (options.useRegex) {
        std::string pattern = options.wholeWord ? ("\\b(?:" + needle + ")\\b") : needle;
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        if (!options.caseSensitive) {
            flags |= std::regex_constants::icase;
        }
        std::regex re(pattern, flags);
        if (!std::regex_match(selected, re)) {
            return false;
        }
        deleteSelection();
        std::string replaced = std::regex_replace(selected, re, replacement,
                                                  std::regex_constants::format_first_only);
        insertText(replaced);
        return true;
    }

    bool matches = (selected.length() == needle.length());
    if (matches) {
        for (std::size_t i = 0; i < needle.length() && matches; ++i) {
            if (!charEquals(selected[i], needle[i], options.caseSensitive)) {
                matches = false;
            }
        }
    }
    if (!matches) return false;
    
    // Delete selection and insert replacement
    deleteSelection();
    insertText(replacement);
    return true;
}

std::size_t TextBuffer::replaceAll(const std::string& needle, const std::string& replacement,
                                   const FindOptions& options) {
    if (needle.empty()) return 0;
    
    std::size_t count = 0;
    
    // Find all occurrences first (to avoid modifying while searching)
    std::vector<FindResult> matches = findAll(needle, options);
    
    // Replace from end to start to preserve positions
    for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
        // Select the match
        setCaret(it->start);
        setSelectionAnchor(it->start);
        setCaret(it->end);
        updateSelectionToCaret();
        
        // Replace
        std::string selected = getSelectedText();
        deleteSelection();
        if (options.useRegex) {
            std::string pattern = options.wholeWord ? ("\\b(?:" + needle + ")\\b") : needle;
            std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
            if (!options.caseSensitive) {
                flags |= std::regex_constants::icase;
            }
            std::regex re(pattern, flags);
            std::string replaced =
                std::regex_replace(selected, re, replacement,
                                   std::regex_constants::format_first_only);
            insertText(replaced);
        } else {
            insertText(replacement);
        }
        count++;
    }
    
    return count;
}

// ============================================================================
// Hyperlink Management
// ============================================================================

bool TextBuffer::addHyperlink(const std::string& url, const std::string& tooltip) {
    if (!has_selection_ || url.empty()) {
        return false;
    }
    
    // Get selection offsets
    CaretPosition startPos = selectionStart();
    CaretPosition endPos = selectionEnd();
    std::size_t startOffset = positionToOffset(startPos);
    std::size_t endOffset = positionToOffset(endPos);
    
    return addHyperlinkAt(startOffset, endOffset, url, tooltip);
}

bool TextBuffer::addHyperlinkAt(std::size_t startOffset, std::size_t endOffset,
                                const std::string& url, const std::string& tooltip) {
    if (startOffset >= endOffset || url.empty()) {
        return false;
    }
    
    // Validate offsets are within document bounds
    if (endOffset > chars_.size()) {
        return false;
    }
    
    // Check for overlapping hyperlinks - remove them first
    for (auto it = hyperlinks_.begin(); it != hyperlinks_.end();) {
        if (it->overlaps(startOffset, endOffset)) {
            it = hyperlinks_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Add the new hyperlink
    Hyperlink link;
    link.startOffset = startOffset;
    link.endOffset = endOffset;
    link.url = url;
    link.tooltip = tooltip;
    hyperlinks_.push_back(link);
    
    // Sort hyperlinks by start offset for consistent ordering
    std::sort(hyperlinks_.begin(), hyperlinks_.end(),
              [](const Hyperlink& a, const Hyperlink& b) {
                  return a.startOffset < b.startOffset;
              });
    
    version_++;
    return true;
}

bool TextBuffer::editHyperlink(std::size_t offset, const std::string& newUrl,
                               const std::string& newTooltip) {
    for (auto& link : hyperlinks_) {
        if (link.contains(offset)) {
            link.url = newUrl;
            link.tooltip = newTooltip;
            version_++;
            return true;
        }
    }
    return false;
}

bool TextBuffer::removeHyperlink(std::size_t offset) {
    for (auto it = hyperlinks_.begin(); it != hyperlinks_.end(); ++it) {
        if (it->contains(offset)) {
            hyperlinks_.erase(it);
            version_++;
            return true;
        }
    }
    return false;
}

const Hyperlink* TextBuffer::hyperlinkAt(std::size_t offset) const {
    for (const auto& link : hyperlinks_) {
        if (link.contains(offset)) {
            return &link;
        }
    }
    return nullptr;
}

const Hyperlink* TextBuffer::hyperlinkAtCaret() const {
    std::size_t offset = positionToOffset(caret_);
    return hyperlinkAt(offset);
}

bool TextBuffer::selectionHasHyperlink() const {
    if (!has_selection_) {
        return false;
    }
    
    CaretPosition startPos = selectionStart();
    CaretPosition endPos = selectionEnd();
    std::size_t startOffset = positionToOffset(startPos);
    std::size_t endOffset = positionToOffset(endPos);
    
    for (const auto& link : hyperlinks_) {
        if (link.overlaps(startOffset, endOffset)) {
            return true;
        }
    }
    return false;
}

std::vector<const Hyperlink*> TextBuffer::hyperlinksInRange(std::size_t startOffset,
                                                             std::size_t endOffset) const {
    std::vector<const Hyperlink*> result;
    for (const auto& link : hyperlinks_) {
        if (link.overlaps(startOffset, endOffset)) {
            result.push_back(&link);
        }
    }
    return result;
}

void TextBuffer::adjustHyperlinkOffsets(std::size_t pos, std::ptrdiff_t delta) {
    for (auto it = hyperlinks_.begin(); it != hyperlinks_.end();) {
        // If deletion removes the entire hyperlink
        if (delta < 0 && pos <= it->startOffset && 
            pos + static_cast<std::size_t>(-delta) >= it->endOffset) {
            it = hyperlinks_.erase(it);
            continue;
        }
        
        // Adjust start offset
        if (pos < it->startOffset) {
            if (delta < 0 && it->startOffset < pos + static_cast<std::size_t>(-delta)) {
                // Deletion starts before hyperlink and overlaps
                it->startOffset = pos;
            } else {
                it->startOffset = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(it->startOffset) + delta);
            }
        } else if (pos < it->endOffset) {
            // Insertion/deletion within hyperlink - adjust end only
        }
        
        // Adjust end offset
        if (pos < it->endOffset) {
            if (delta < 0 && it->endOffset <= pos + static_cast<std::size_t>(-delta)) {
                // Deletion removes everything after pos
                it->endOffset = pos;
            } else {
                it->endOffset = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(it->endOffset) + delta);
            }
        }
        
        // Remove hyperlinks that have become empty
        if (it->startOffset >= it->endOffset) {
            it = hyperlinks_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Bookmark Management
// ============================================================================

bool TextBuffer::addBookmark(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    std::size_t offset = positionToOffset(caret_);
    return addBookmarkAt(name, offset);
}

bool TextBuffer::addBookmarkAt(const std::string& name, std::size_t offset) {
    if (name.empty() || offset > chars_.size()) {
        return false;
    }
    
    // Check if bookmark with this name already exists
    for (const auto& bm : bookmarks_) {
        if (bm.name == name) {
            return false;  // Duplicate name not allowed
        }
    }
    
    // Add new bookmark
    Bookmark bm;
    bm.name = name;
    bm.offset = offset;
    bookmarks_.push_back(bm);
    
    // Keep bookmarks sorted by position
    std::sort(bookmarks_.begin(), bookmarks_.end(),
             [](const Bookmark& a, const Bookmark& b) {
                 return a.offset < b.offset;
             });
    
    version_++;
    return true;
}

bool TextBuffer::removeBookmark(const std::string& name) {
    for (auto it = bookmarks_.begin(); it != bookmarks_.end(); ++it) {
        if (it->name == name) {
            bookmarks_.erase(it);
            version_++;
            return true;
        }
    }
    return false;
}

const Bookmark* TextBuffer::getBookmark(const std::string& name) const {
    for (const auto& bm : bookmarks_) {
        if (bm.name == name) {
            return &bm;
        }
    }
    return nullptr;
}

bool TextBuffer::goToBookmark(const std::string& name) {
    const Bookmark* bm = getBookmark(name);
    if (!bm) {
        return false;
    }
    
    // Convert offset to caret position
    CaretPosition pos = offsetToPosition(bm->offset);
    setCaret(pos);
    clearSelection();
    return true;
}

bool TextBuffer::hasBookmark(const std::string& name) const {
    return getBookmark(name) != nullptr;
}

// ============================================================================
// Outline Extraction
// ============================================================================

std::vector<TextBuffer::OutlineEntry> TextBuffer::getOutline() const {
    std::vector<OutlineEntry> outline;
    
    for (std::size_t i = 0; i < line_spans_.size(); ++i) {
        ParagraphStyle style = line_spans_[i].style;
        
        // Only include headings and titles in the outline
        if (style == ParagraphStyle::Normal) {
            continue;
        }
        
        OutlineEntry entry;
        entry.lineNumber = i;
        entry.style = style;
        
        // Get the text of this line (truncate if too long)
        std::string lineText = lineString(i);
        if (lineText.length() > 60) {
            lineText = lineText.substr(0, 57) + "...";
        }
        entry.text = lineText;
        
        // Calculate indentation level based on heading style
        switch (style) {
            case ParagraphStyle::Title:
                entry.level = 0;
                break;
            case ParagraphStyle::Subtitle:
                entry.level = 1;
                break;
            case ParagraphStyle::Heading1:
                entry.level = 1;
                break;
            case ParagraphStyle::Heading2:
                entry.level = 2;
                break;
            case ParagraphStyle::Heading3:
                entry.level = 3;
                break;
            case ParagraphStyle::Heading4:
                entry.level = 4;
                break;
            case ParagraphStyle::Heading5:
                entry.level = 5;
                break;
            case ParagraphStyle::Heading6:
                entry.level = 6;
                break;
            case ParagraphStyle::Normal:
            case ParagraphStyle::Count:
            default:
                entry.level = 0;
                break;
        }
        
        outline.push_back(entry);
    }
    
    return outline;
}

bool TextBuffer::goToOutlineEntry(std::size_t lineNumber) {
    if (lineNumber >= line_spans_.size()) {
        return false;
    }
    
    setCaret({lineNumber, 0});
    clearSelection();
    return true;
}

// ============================================================================
// Bookmark Offset Adjustment
// ============================================================================

const Bookmark* TextBuffer::bookmarkNear(std::size_t offset, std::size_t tolerance) const {
    for (const auto& bm : bookmarks_) {
        std::size_t dist;
        if (bm.offset >= offset) {
            dist = bm.offset - offset;
        } else {
            dist = offset - bm.offset;
        }
        if (dist <= tolerance) {
            return &bm;
        }
    }
    return nullptr;
}

void TextBuffer::adjustBookmarkOffsets(std::size_t pos, std::ptrdiff_t delta) {
    for (auto it = bookmarks_.begin(); it != bookmarks_.end();) {
        // If deletion removes the bookmark position
        if (delta < 0 && pos <= it->offset && 
            pos + static_cast<std::size_t>(-delta) > it->offset) {
            // Bookmark is within deleted range - move to deletion start
            it->offset = pos;
            ++it;
            continue;
        }
        
        // Adjust offset for insertions/deletions before this bookmark
        if (pos <= it->offset) {
            it->offset = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(it->offset) + delta);
        }
        ++it;
    }
    
    // Re-sort after adjustments
    std::sort(bookmarks_.begin(), bookmarks_.end());
}

// ============================================================================
// Table of Contents
// ============================================================================

// ===========================================================================
// Table of Contents (marked region)
// ===========================================================================

void TocReplaceCommand::apply(TextBuffer& buffer, std::size_t start,
                              const std::string& oldText,
                              const std::string& newText) {
    CaretPosition pos = buffer.positionForOffset(start);
    buffer.setCaret(pos);
    for (std::size_t i = 0; i < oldText.size(); ++i) {
        buffer.deleteCharAt(pos);
    }
    CaretPosition cur = pos;
    for (char ch : newText) {
        buffer.insertCharAt(cur, ch);
        cur = buffer.caret();
    }
    buffer.setCaret(cur);
}

void TocReplaceCommand::execute(TextBuffer& buffer) {
    apply(buffer, start_, oldText_, newText_);
    if (newHasToc_) {
        buffer.setTocRange(newTocStart_, newTocEnd_);
        // Generated lines are Normal: the block's first row is
        // the abutting line's span (the newline split keeps its
        // style), so without this the TOC title line would carry
        // the heading's style and getOutline would list the TOC
        // itself as a heading. Rows starting at/after the end
        // offset (the after-line) are outside the mark.
        for (std::size_t row = 0; row < buffer.lineCount(); ++row) {
            if (buffer.lineSpan(row).offset >= newTocEnd_) break;
            if (buffer.lineSpan(row).offset >= newTocStart_) {
                buffer.setLineParagraphStyle(
                    row, ParagraphStyle::Normal);
            }
        }
    } else {
        buffer.clearTocMark();
    }
    if (hasBoundary_ && boundaryRowNew_ < buffer.lineCount() &&
        (!boundaryOnlyIfNonEmpty_ ||
         buffer.lineSpan(boundaryRowNew_).length > 0)) {
        buffer.setLineParagraphStyle(boundaryRowNew_, boundaryStyle_);
    }
}

void TocReplaceCommand::undo(TextBuffer& buffer) {
    apply(buffer, start_, newText_, oldText_);
    if (oldHasToc_) {
        buffer.setTocRange(oldTocStart_, oldTocEnd_);
        for (std::size_t row = 0; row < buffer.lineCount(); ++row) {
            if (buffer.lineSpan(row).offset >= oldTocEnd_) break;
            if (buffer.lineSpan(row).offset >= oldTocStart_) {
                buffer.setLineParagraphStyle(
                    row, ParagraphStyle::Normal);
            }
        }
    } else {
        buffer.clearTocMark();
    }
    if (hasBoundary_ && boundaryRowOld_ < buffer.lineCount() &&
        (!boundaryOnlyIfNonEmpty_ ||
         buffer.lineSpan(boundaryRowOld_).length > 0)) {
        buffer.setLineParagraphStyle(boundaryRowOld_, boundaryStyle_);
    }
}

void TextBuffer::adjustTocOffsets(std::size_t pos, std::ptrdiff_t delta) {
    if (!tocHasMark_) return;
    if (delta > 0) {
        // Insertion of `delta` chars at pos.
        if (pos < tocStart_) {
            tocStart_ = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(tocStart_) + delta);
            tocEnd_ = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(tocEnd_) + delta);
        } else if (pos < tocEnd_) {
            tocEnd_ = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(tocEnd_) + delta);
        }
        return;
    }
    if (delta == 0) return;
    // Deletion of [-delta] chars at [pos, pos + -delta).
    const std::size_t delEnd =
        pos + static_cast<std::size_t>(-delta);
    if (delEnd <= tocStart_) {
        tocStart_ = static_cast<std::size_t>(
            static_cast<std::ptrdiff_t>(tocStart_) + delta);
        tocEnd_ = static_cast<std::size_t>(
            static_cast<std::ptrdiff_t>(tocEnd_) + delta);
        return;
    }
    if (pos >= tocEnd_) return;
    if (pos <= tocStart_ && delEnd >= tocEnd_) {
        // Deletion covers the whole block: the mark is gone.
        clearTocMark();
        return;
    }
    if (pos <= tocStart_) {
        tocStart_ = pos;
    }
    // End shrinks by the overlap with the deleted range.
    const std::size_t overlapStart = std::max(pos, tocStart_);
    const std::size_t overlapEnd = std::min(delEnd, tocEnd_);
    if (overlapEnd > overlapStart) {
        tocEnd_ -= overlapEnd - overlapStart;
    }
    if (tocEnd_ <= tocStart_) clearTocMark();
}

void TextBuffer::clearTocMark() {
    tocHasMark_ = false;
    tocStart_ = 0;
    tocEnd_ = 0;
    version_++;
}

void TextBuffer::setTocRange(std::size_t start, std::size_t end) {
    // Accepted only when in bounds and starting at the TOC title
    // line (setTocRange is the load path: a stale or corrupt
    // "tableOfContents" object must not mark arbitrary text).
    // Title check is on the title LINE: the range must start at a
    // line start and the line must read exactly the title.
    if (start > end || end > chars_.size()) {
        clearTocMark();
        return;
    }
    if (start > 0 && chars_.at(start - 1) != '\n') {
        clearTocMark();
        return;
    }
    CaretPosition pos = offsetToPosition(start);
    if (pos.row >= line_spans_.size() ||
        lineString(pos.row) != toc::kTitle) {
        clearTocMark();
        return;
    }
    tocHasMark_ = true;
    tocStart_ = start;
    tocEnd_ = end;
    version_++;
}

std::size_t TextBuffer::tocEntryCount() const {
    if (!tocHasMark_ || tocEnd_ <= tocStart_ ||
        tocEnd_ > chars_.size()) {
        return 0;
    }
    std::size_t count = 0;
    std::size_t lineBegin = tocStart_;
    while (lineBegin < tocEnd_) {
        std::size_t lineEnd = lineBegin;
        while (lineEnd < tocEnd_ && chars_.at(lineEnd) != '\n') ++lineEnd;
        std::string line;
        line.reserve(lineEnd - lineBegin);
        for (std::size_t i = lineBegin; i < lineEnd; ++i) {
            line.push_back(chars_.at(i));
        }
        if (toc::isEntryLine(line)) ++count;
        lineBegin = lineEnd + 1;
    }
    return count;
}

bool TextBuffer::resolveTocRange(std::size_t& start,
                                 std::size_t& end) const {
    if (!tocHasMark_) return false;
    // Stored range, trusted when its first line is still the title.
    if (tocStart_ < tocEnd_ && tocEnd_ <= chars_.size()) {
        CaretPosition pos = offsetToPosition(tocStart_);
        if (pos.row < line_spans_.size() &&
            lineString(pos.row) == toc::kTitle) {
            start = tocStart_;
            end = tocEnd_;
            return true;
        }
    }
    // Rescan: a line reading exactly the title, followed by
    // entry lines. The end is the start of the line after the
    // last entry line (its terminating '\n' belongs to the block).
    for (std::size_t row = 0; row < line_spans_.size(); ++row) {
        if (lineString(row) != toc::kTitle) continue;
        std::size_t blockStart = line_spans_[row].offset;
        std::size_t blockEnd = blockStart + line_spans_[row].length;
        if (blockEnd < chars_.size() && chars_.at(blockEnd) == '\n') {
            ++blockEnd;
        }
        std::size_t r = row + 1;
        while (r < line_spans_.size() &&
               toc::isEntryLine(lineString(r))) {
            blockEnd = line_spans_[r].offset + line_spans_[r].length;
            if (blockEnd < chars_.size() && chars_.at(blockEnd) == '\n') {
                ++blockEnd;
            }
            ++r;
        }
        start = blockStart;
        end = blockEnd;
        return true;
    }
    // No block found: fall back to the stored range when it is
    // still in bounds (a hand-renamed title still updates).
    if (tocStart_ < tocEnd_ && tocEnd_ <= chars_.size()) {
        start = tocStart_;
        end = tocEnd_;
        return true;
    }
    return false;
}

std::vector<toc::Entry> TextBuffer::tocEntriesForBlock(
    std::size_t insertLine, std::size_t blockLines,
    std::size_t oldStartLine, std::size_t oldEndLine, bool hasOld,
    int linesPerPage) const {
    // Entry count is known up front: the block is 1 title line +
    // one line per entry, so blockLines is exact for page math
    // whether inserting or replacing.
    std::vector<toc::Entry> entries;
    for (std::size_t row = 0; row < line_spans_.size(); ++row) {
        if (hasOld && row >= oldStartLine && row < oldEndLine) continue;
        ParagraphStyle style = line_spans_[row].style;
        int level = -1;
        switch (style) {
            case ParagraphStyle::Title: level = 0; break;
            case ParagraphStyle::Heading1: level = 1; break;
            case ParagraphStyle::Heading2: level = 2; break;
            case ParagraphStyle::Heading3: level = 3; break;
            // Non-heading styles (and the Count sentinel) are not
            // TOC entries: same fall-through as the old default.
            case ParagraphStyle::Normal:
            case ParagraphStyle::Subtitle:
            case ParagraphStyle::Heading4:
            case ParagraphStyle::Heading5:
            case ParagraphStyle::Heading6:
            case ParagraphStyle::Count:
            default: break;
        }
        if (level < 0) continue;
        std::size_t effectiveRow = row;
        if (hasOld) {
            if (row >= oldEndLine) {
                effectiveRow = row + blockLines -
                               (oldEndLine - oldStartLine);
            }
        } else if (row >= insertLine) {
            effectiveRow = row + blockLines;
        }
        entries.push_back(toc::Entry{lineString(row), level, row,
                                     toc::pageForLine(effectiveRow,
                                                      linesPerPage)});
    }
    return entries;
}

std::string TextBuffer::generateTableOfContents(
    int linesPerPage, const toc::Layout& tocLayout) const {
    if (line_spans_.empty()) return "";
    auto entries = tocEntriesForBlock(
        line_spans_.size(), 0, 0, 0, false, linesPerPage);
    if (entries.empty()) return "";
    return toc::blockText(entries, tocLayout);
}

bool TextBuffer::insertTableOfContents(int linesPerPage,
                                       const toc::Layout& tocLayout) {
    // Replacing the existing block is the same operation with
    // the old range as the replaced span.
    if (tocHasMark_) return updateTableOfContents(linesPerPage, tocLayout);

    std::size_t insertOffset = caretOffset();
    std::size_t insertLine = caret_.row;
    // Count entries first: the block length shifts the page
    // numbers of headings at/after the insertion line.
    auto entries = tocEntriesForBlock(
        insertLine, 0, 0, 0, false, linesPerPage);
    if (entries.empty()) return false;
    const std::size_t blockLines = entries.size() + 1;
    entries = tocEntriesForBlock(insertLine, blockLines, 0, 0, false,
                                 linesPerPage);
    std::string block = toc::blockText(entries, tocLayout);

    // The block must occupy whole lines, or the mark's first
    // line would be a fragment of the caret line. A leading /
    // trailing '\n' outside the marked region splits the caret
    // line around the block — only when the caret is mid-line
    // (column > 0): at a line start the block's own trailing
    // '\n' already ends the block's last line, and an extra
    // '\n' would leave a blank line between TOC and body.
    std::string inserted;
    std::size_t markStart = insertOffset;
    bool midLine = caret_.column > 0;
    if (midLine) {
        inserted += '\n';
        markStart = insertOffset + 1;
    }
    inserted += block;
    if (midLine && insertOffset < chars_.size() &&
        chars_.at(insertOffset) != '\n') {
        inserted += '\n';
    }
    std::size_t markEnd = markStart + block.size();
    // The caret line's paragraph style is stolen by the newline
    // split in the command (insertCharAt gives the remainder
    // Normal): the command carries the boundary restore, only
    // when the remainder is non-empty — at end of line / EOF the
    // remainder is an empty row that must stay Normal, or it
    // would become a phantom empty heading.
    ParagraphStyle boundaryStyle = lineParagraphStyle(insertLine);
    std::size_t boundaryRowAfter =
        insertLine +
        static_cast<std::size_t>(std::count(inserted.begin(),
                                            inserted.end(), '\n'));
    history_.execute(
        std::make_unique<TocReplaceCommand>(
            insertOffset, "", inserted, false, 0, 0, true, markStart,
            markEnd, true, boundaryStyle, boundaryRowAfter, insertLine,
            true),
        *this);
    // Caret-safe: the caret was at the insertion point; the
    // inserted block now starts there, so leave the caret at the
    // end of the block.
    setCaret(positionForOffset(markEnd));
    version_++;
    return true;
}

bool TextBuffer::updateTableOfContents(int linesPerPage,
                                       const toc::Layout& tocLayout) {
    std::size_t oldStart = 0;
    std::size_t oldEnd = 0;
    if (!resolveTocRange(oldStart, oldEnd)) return false;
    CaretPosition oldStartPos = offsetToPosition(oldStart);
    CaretPosition oldEndPos = offsetToPosition(oldEnd);
    const std::size_t oldStartLine = oldStartPos.row;
    std::size_t oldEndLine = oldEndPos.row;
    // oldEnd sits at a line start (after the block's last '\n')
    // except at EOF, where it is the last line's end.
    if (oldEnd < chars_.size() ||
        (oldEndLine > 0 && oldEnd == line_spans_[oldEndLine].offset)) {
        // oldEnd is exactly a line start: oldEndLine is the first
        // line after the block.
    } else if (oldEnd == chars_.size() && oldEndLine < line_spans_.size()) {
        oldEndLine = oldEndLine + 1;  // Block ran to EOF
    }
    const std::size_t oldBlockLines = oldEndLine - oldStartLine;

    std::size_t caretBefore = caretOffset();
    std::string oldText;
    oldText.reserve(oldEnd - oldStart);
    for (std::size_t i = oldStart; i < oldEnd; ++i) {
        oldText.push_back(chars_.at(i));
    }
    // The first line after the block loses its paragraph style
    // to the delete/insert of the block's trailing newline (same
    // newline-split steal as insert): snapshot it, restore it on
    // that line's new row after the command.
    bool hasAfterLine = oldEndLine < line_spans_.size();
    ParagraphStyle afterLineStyle =
        hasAfterLine ? lineParagraphStyle(oldEndLine)
                     : ParagraphStyle::Normal;

    auto entries = tocEntriesForBlock(oldStartLine, oldBlockLines,
                                      oldStartLine, oldEndLine, true,
                                      linesPerPage);
    if (entries.empty()) {
        // No headings left: erase the block, clear the mark.
        history_.execute(
            std::make_unique<TocReplaceCommand>(
                oldStart, oldText, "", true, oldStart, oldEnd, false,
                0, 0, hasAfterLine, afterLineStyle, oldStartLine,
                oldEndLine, false),
            *this);
        // Caret-safe: a caret after the erased block shifts back
        // by the block's length; before / inside lands at its start.
        std::size_t erasedCaret =
            caretBefore > oldEnd
                ? caretBefore - oldText.size()
                : std::min(caretBefore, oldStart);
        setCaret(positionForOffset(erasedCaret));
        version_++;
        return false;
    }
    const std::size_t newBlockLines = entries.size() + 1;
    if (newBlockLines != oldBlockLines) {
        // Page numbers depend on the new block's length.
        entries = tocEntriesForBlock(oldStartLine, newBlockLines,
                                     oldStartLine, oldEndLine, true,
                                     linesPerPage);
    }
    std::string block = toc::blockText(entries, tocLayout);
    if (block == oldText) return true;  // Already fresh: no undo step
    history_.execute(
        std::make_unique<TocReplaceCommand>(
            oldStart, oldText, block, true, oldStart, oldEnd, true,
            oldStart, oldStart + block.size(), hasAfterLine,
            afterLineStyle, oldStartLine + newBlockLines, oldEndLine,
            false),
        *this);
    // Caret-safe: before the block -> unchanged; inside -> end of
    // the new block; after -> shifted by the length delta.
    std::size_t newCaret = caretBefore;
    if (caretBefore >= oldStart && caretBefore <= oldEnd) {
        newCaret = oldStart + block.size();
    } else if (caretBefore > oldEnd) {
        newCaret = caretBefore + block.size() - oldText.size();
    }
    setCaret(positionForOffset(newCaret));
    version_++;
    return true;
}

// ============================================================================
// Section Break Management
// ============================================================================

static SectionSettings defaultSectionSettings;

void TextBuffer::insertSectionBreak(SectionBreakType type) {
    // Insert a page break first
    insertPageBreak();
    
    // Create a new section starting at the current line
    DocumentSection section;
    section.startLine = caret_.row;
    section.settings.breakType = type;
    
    // Insert in sorted order
    sections_.push_back(section);
    std::sort(sections_.begin(), sections_.end());
    
    version_++;
}

const DocumentSection* TextBuffer::sectionAt(std::size_t line) const {
    // Find the section that contains this line
    const DocumentSection* result = nullptr;
    for (const auto& section : sections_) {
        if (section.startLine <= line) {
            result = &section;
        } else {
            break;  // Sections are sorted, so we can stop
        }
    }
    return result;
}

const SectionSettings& TextBuffer::sectionSettingsAt(std::size_t line) const {
    const DocumentSection* section = sectionAt(line);
    if (section) {
        return section->settings;
    }
    return defaultSectionSettings;
}

// Footnote support
bool TextBuffer::addFootnote(const std::string& content) {
    Footnote fn;
    fn.referenceOffset = caretOffset();
    fn.content = content;
    fn.number = static_cast<int>(footnotes_.size()) + 1;
    footnotes_.push_back(fn);
    // Insert footnote marker at caret
    std::string marker = "[" + std::to_string(fn.number) + "]";
    insertText(marker);
    version_++;
    return true;
}

bool TextBuffer::removeFootnote(std::size_t number) {
    for (auto it = footnotes_.begin(); it != footnotes_.end(); ++it) {
        if (static_cast<std::size_t>(it->number) == number) {
            footnotes_.erase(it);
            renumberFootnotes();
            version_++;
            return true;
        }
    }
    return false;
}

const Footnote* TextBuffer::getFootnote(std::size_t number) const {
    for (const auto& fn : footnotes_) {
        if (static_cast<std::size_t>(fn.number) == number) return &fn;
    }
    return nullptr;
}

const Footnote* TextBuffer::footnoteAt(std::size_t offset) const {
    for (const auto& fn : footnotes_) {
        if (fn.referenceOffset == offset) return &fn;
    }
    return nullptr;
}

void TextBuffer::renumberFootnotes() {
    std::sort(footnotes_.begin(), footnotes_.end());
    for (int i = 0; i < static_cast<int>(footnotes_.size()); ++i) {
        footnotes_[i].number = i + 1;
    }
}

void TextBuffer::updateSectionSettings(std::size_t line, const SectionSettings& settings) {
    for (auto& section : sections_) {
        if (section.startLine == line) {
            section.settings = settings;
            version_++;
            return;
        }
    }
    // If no section at this line, create one
    DocumentSection section;
    section.startLine = line;
    section.settings = settings;
    sections_.push_back(section);
    std::sort(sections_.begin(), sections_.end());
    version_++;
}
