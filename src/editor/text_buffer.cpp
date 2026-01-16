#include "text_buffer.h"

#include <algorithm>
#include <cctype>
#include <cstring>

// ============================================================================
// GapBuffer implementation
// ============================================================================

GapBuffer::GapBuffer(std::size_t initial_capacity) {
    buffer_.resize(initial_capacity);
    gap_start_ = 0;
    gap_end_ = initial_capacity;
}

void GapBuffer::moveGapTo(std::size_t pos) {
    if (pos == gap_start_) {
        return;
    }

    gap_moves_++;

    if (pos < gap_start_) {
        // Move gap backwards: shift characters forward
        std::size_t shift = gap_start_ - pos;
        std::memmove(&buffer_[gap_end_ - shift], &buffer_[pos], shift);
        gap_end_ -= shift;
        gap_start_ = pos;
    } else {
        // Move gap forwards: shift characters backward
        std::size_t shift = pos - gap_start_;
        std::memmove(&buffer_[gap_start_], &buffer_[gap_end_], shift);
        gap_start_ += shift;
        gap_end_ += shift;
    }
}

void GapBuffer::ensureCapacity(std::size_t needed) {
    std::size_t gap_size = gap_end_ - gap_start_;
    if (gap_size >= needed) {
        return;
    }

    reallocations_++;

    // Grow by 2x or to fit needed, whichever is larger
    std::size_t current_size = buffer_.size();
    std::size_t new_size =
        std::max(current_size * 2, current_size + needed - gap_size);

    std::vector<char> new_buffer(new_size);

    // Copy before gap
    if (gap_start_ > 0) {
        std::memcpy(&new_buffer[0], &buffer_[0], gap_start_);
    }

    // Copy after gap
    std::size_t after_gap = current_size - gap_end_;
    if (after_gap > 0) {
        std::memcpy(&new_buffer[new_size - after_gap], &buffer_[gap_end_],
                    after_gap);
    }

    gap_end_ = new_size - after_gap;
    buffer_ = std::move(new_buffer);
}

void GapBuffer::insert(std::size_t pos, char ch) {
    moveGapTo(pos);
    ensureCapacity(1);
    buffer_[gap_start_++] = ch;
}

void GapBuffer::insertString(std::size_t pos, const char* str,
                             std::size_t len) {
    if (len == 0) return;
    moveGapTo(pos);
    ensureCapacity(len);
    std::memcpy(&buffer_[gap_start_], str, len);
    gap_start_ += len;
}

void GapBuffer::erase(std::size_t pos, std::size_t count) {
    if (count == 0) return;
    moveGapTo(pos);
    // Expand gap to "delete" characters after gap
    gap_end_ = std::min(gap_end_ + count, buffer_.size());
}

char GapBuffer::at(std::size_t pos) const {
    if (pos < gap_start_) {
        return buffer_[pos];
    }
    return buffer_[gap_end_ + (pos - gap_start_)];
}

std::size_t GapBuffer::size() const {
    return buffer_.size() - (gap_end_ - gap_start_);
}

bool GapBuffer::empty() const { return size() == 0; }

const char* GapBuffer::data(std::size_t pos, std::size_t len) const {
    // This only works if the range doesn't span the gap
    if (pos < gap_start_ && pos + len <= gap_start_) {
        return &buffer_[pos];
    }
    if (pos >= gap_start_) {
        return &buffer_[gap_end_ + (pos - gap_start_)];
    }
    // Range spans gap - return nullptr to indicate copy needed
    return nullptr;
}

void GapBuffer::copyTo(std::size_t pos, std::size_t len, char* out) const {
    for (std::size_t i = 0; i < len; ++i) {
        out[i] = at(pos + i);
    }
}

std::string GapBuffer::toString() const {
    std::string result;
    result.reserve(size());

    // Before gap
    for (std::size_t i = 0; i < gap_start_; ++i) {
        result.push_back(buffer_[i]);
    }

    // After gap
    for (std::size_t i = gap_end_; i < buffer_.size(); ++i) {
        result.push_back(buffer_[i]);
    }

    return result;
}

void GapBuffer::clear() {
    gap_start_ = 0;
    gap_end_ = buffer_.size();
}

// ============================================================================
// TextBuffer implementation (SoA with gap buffer)
// ============================================================================

TextBuffer::TextBuffer() { ensureNonEmpty(); }

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

    std::string result(span.length, '\0');
    chars_.copyTo(span.offset, span.length, &result[0]);
    return result;
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

    // Erase the selected range
    chars_.erase(startOffset, deleteCount);
    stats_.total_deletes += deleteCount;
    version_++;
    
    // Adjust hyperlink and bookmark offsets for the deleted selection
    adjustHyperlinkOffsets(startOffset, -static_cast<std::ptrdiff_t>(deleteCount));
    adjustBookmarkOffsets(startOffset, -static_cast<std::ptrdiff_t>(deleteCount));

    // Rebuild line index since we may have deleted across lines
    rebuildLineIndex();

    // Move caret to start of deleted region
    caret_ = start;
    clampCaret();

    // Clear selection
    clearSelection();

    // Record for undo
    if (recordingHistory_ && !deletedText.empty()) {
        history_.record(
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
  // Preserve existing line styles by offset before clearing
  std::vector<std::pair<std::size_t, ParagraphStyle>> savedStyles;
  for (const auto& span : line_spans_) {
    if (span.style != ParagraphStyle::Normal) {
      savedStyles.push_back({span.offset, span.style});
    }
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
  
  // Restore styles based on line start offsets
  for (const auto& [offset, style] : savedStyles) {
    for (auto& span : line_spans_) {
      if (span.offset == offset) {
        span.style = style;
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
    chars_.insert(offset, ch);
    
    // Adjust hyperlink and bookmark offsets for the inserted character
    adjustHyperlinkOffsets(offset, 1);
    adjustBookmarkOffsets(offset, 1);

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
        history_.record(std::make_unique<InsertCharCommand>(insertPos, ch));
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
    hyperlinks_.clear();  // Clear all hyperlinks when setting new text
    version_++;  // Content changed - invalidate render cache

    if (!text.empty()) {
        // Remove \r from CRLF line endings
        std::string cleaned;
        cleaned.reserve(text.size());
        for (char ch : text) {
            if (ch != '\r') {
                cleaned.push_back(ch);
            }
        }

        chars_.insertString(0, cleaned.c_str(), cleaned.size());
    }

    rebuildLineIndex();

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

TextStyle TextBuffer::textStyle() const { return style_; }

void TextBuffer::setTextStyle(const TextStyle& style) { style_ = style; }

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

        chars_.erase(offset - 1, 1);
        stats_.total_deletes++;
        version_++;  // Content changed - invalidate render cache
        
        // Adjust hyperlink and bookmark offsets for the deleted character
        adjustHyperlinkOffsets(offset - 1, -1);
        adjustBookmarkOffsets(offset - 1, -1);

        line_spans_[caret_.row].length -= 1;
        shiftLineOffsetsFrom(caret_.row + 1, -1);
        caret_.column -= 1;

        // Record for undo
        if (recordingHistory_) {
            history_.record(std::make_unique<DeleteCharCommand>(
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

    chars_.erase(newline_offset, 1);
    stats_.total_deletes++;
    version_++;
    
    // Adjust hyperlink and bookmark offsets for the deleted newline
    adjustHyperlinkOffsets(newline_offset, -1);
    adjustBookmarkOffsets(newline_offset, -1);

    rebuildLineIndex();

    caret_.row -= 1;
    caret_.column = prev_line_len;

    // Record for undo (deleted a newline)
    if (recordingHistory_) {
        history_.record(
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

        chars_.erase(offset, 1);
        stats_.total_deletes++;
        version_++;
        
        // Adjust hyperlink and bookmark offsets for the deleted character
        adjustHyperlinkOffsets(offset, -1);
        adjustBookmarkOffsets(offset, -1);

        line_spans_[caret_.row].length -= 1;
        shiftLineOffsetsFrom(caret_.row + 1, -1);

        // Record for undo
        if (recordingHistory_) {
            history_.record(std::make_unique<DeleteCharCommand>(
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

    chars_.erase(newline_offset, 1);
    stats_.total_deletes++;
    version_++;
    
    // Adjust hyperlink and bookmark offsets for the deleted newline
    adjustHyperlinkOffsets(newline_offset, -1);
    adjustBookmarkOffsets(newline_offset, -1);

    rebuildLineIndex();

    // Record for undo (deleted a newline)
    if (recordingHistory_) {
        history_.record(
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

    // Skip any whitespace/punctuation to the left
    while (caret_.column > 0 || caret_.row > 0) {
        if (caret_.column == 0) {
            if (caret_.row == 0) break;
            caret_.row--;
            caret_.column = line_spans_[caret_.row].length;
            continue;
        }

        std::size_t offset = positionToOffset(caret_);
        if (offset == 0) break;
        char ch = chars_.at(offset - 1);
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            break;
        }
        caret_.column--;
    }

    // Move to start of current word
    while (caret_.column > 0) {
        std::size_t offset = positionToOffset(caret_);
        if (offset == 0) break;
        char ch = chars_.at(offset - 1);
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            break;
        }
        caret_.column--;
    }
}

void TextBuffer::moveWordRight() {
    if (chars_.empty()) return;

    std::size_t totalLines = line_spans_.size();
    std::size_t totalChars = chars_.size();

    // Skip current word
    while (caret_.row < totalLines) {
        const LineSpan& span = line_spans_[caret_.row];
        if (caret_.column >= span.length) {
            // Move to next line
            if (caret_.row + 1 < totalLines) {
                caret_.row++;
                caret_.column = 0;
                continue;
            }
            break;
        }

        std::size_t offset = positionToOffset(caret_);
        if (offset >= totalChars) break;
        char ch = chars_.at(offset);
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            break;
        }
        caret_.column++;
    }

    // Skip whitespace/punctuation
    while (caret_.row < totalLines) {
        const LineSpan& span = line_spans_[caret_.row];
        if (caret_.column >= span.length) {
            if (caret_.row + 1 < totalLines) {
                caret_.row++;
                caret_.column = 0;
                continue;
            }
            break;
        }

        std::size_t offset = positionToOffset(caret_);
        if (offset >= totalChars) break;
        char ch = chars_.at(offset);
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            break;
        }
        caret_.column++;
    }
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

// ============================================================================
// CommandHistory implementation
// ============================================================================

void CommandHistory::execute(std::unique_ptr<EditCommand> cmd,
                             TextBuffer& buffer) {
    cmd->execute(buffer);
    undoStack_.push_back(std::move(cmd));
    redoStack_.clear();
}

void CommandHistory::record(std::unique_ptr<EditCommand> cmd) {
    undoStack_.push_back(std::move(cmd));
    redoStack_.clear();
}

void CommandHistory::undo(TextBuffer& buffer) {
    if (undoStack_.empty()) return;
    auto cmd = std::move(undoStack_.back());
    undoStack_.pop_back();
    cmd->undo(buffer);
    redoStack_.push_back(std::move(cmd));
}

void CommandHistory::redo(TextBuffer& buffer) {
    if (redoStack_.empty()) return;
    auto cmd = std::move(redoStack_.back());
    redoStack_.pop_back();
    cmd->execute(buffer);
    undoStack_.push_back(std::move(cmd));
}

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
    chars_.insert(offset, ch);
    version_++;

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
        std::size_t offset = positionToOffset(pos);
        chars_.erase(offset, 1);
        version_++;
        line_spans_[pos.row].length -= 1;
        shiftLineOffsetsFrom(pos.row + 1, -1);
    } else if (pos.row + 1 < line_spans_.size()) {
        std::size_t offset = span.offset + span.length;
        chars_.erase(offset, 1);
        version_++;
        rebuildLineIndex();
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

FindResult TextBuffer::find(const std::string& needle, const FindOptions& options) const {
    if (needle.empty()) return {false, {0, 0}, {0, 0}};
    
    std::string text = getText();
    std::size_t startOffset = positionToOffset(caret_);
    
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
        deleteSelection();
        insertText(replacement);
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
    for (auto& bm : bookmarks_) {
        if (bm.name == name) {
            // Update existing bookmark
            bm.offset = offset;
            // Re-sort after update
            std::sort(bookmarks_.begin(), bookmarks_.end(),
                     [](const Bookmark& a, const Bookmark& b) {
                         return a.offset < b.offset;
                     });
            version_++;
            return true;
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

std::string TextBuffer::generateTableOfContents() const {
    auto outline = getOutline();
    if (outline.empty()) {
        return "";
    }
    
    std::string toc = "Table of Contents\n";
    toc += "=================\n\n";
    
    for (const auto& entry : outline) {
        // Add indentation based on level
        for (std::size_t i = 0; i < entry.level; ++i) {
            toc += "  ";
        }
        toc += entry.text + "\n";
    }
    
    return toc;
}

void TextBuffer::insertTableOfContents() {
    std::string toc = generateTableOfContents();
    if (!toc.empty()) {
        insertText(toc);
    }
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
