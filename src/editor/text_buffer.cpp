#include "text_buffer.h"

#include <algorithm>
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
  std::size_t new_size = std::max(current_size * 2, current_size + needed - gap_size);
  
  std::vector<char> new_buffer(new_size);
  
  // Copy before gap
  if (gap_start_ > 0) {
    std::memcpy(&new_buffer[0], &buffer_[0], gap_start_);
  }
  
  // Copy after gap
  std::size_t after_gap = current_size - gap_end_;
  if (after_gap > 0) {
    std::memcpy(&new_buffer[new_size - after_gap], &buffer_[gap_end_], after_gap);
  }
  
  gap_end_ = new_size - after_gap;
  buffer_ = std::move(new_buffer);
}

void GapBuffer::insert(std::size_t pos, char ch) {
  moveGapTo(pos);
  ensureCapacity(1);
  buffer_[gap_start_++] = ch;
}

void GapBuffer::insertString(std::size_t pos, const char* str, std::size_t len) {
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

bool GapBuffer::empty() const {
  return size() == 0;
}

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

TextBuffer::TextBuffer() { 
  ensureNonEmpty(); 
}

std::size_t TextBuffer::lineCount() const {
  return line_spans_.size();
}

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
  
  // Rebuild line index since we may have deleted across lines
  rebuildLineIndex();
  
  // Move caret to start of deleted region
  caret_ = start;
  clampCaret();
  
  // Clear selection
  clearSelection();
  
  // Record for undo
  if (recordingHistory_ && !deletedText.empty()) {
    history_.record(std::make_unique<DeleteSelectionCommand>(start, end, deletedText));
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
      std::size_t col = (offset >= span.offset) ? offset - span.offset : 0;
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
  
  if (ch == '\n') {
    // Split current line
    rebuildLineIndex();
    caret_.row += 1;
    caret_.column = 0;
  } else {
    // Update current line span
    if (caret_.row < line_spans_.size()) {
      line_spans_[caret_.row].length += 1;
      
      // Shift subsequent line offsets
      for (std::size_t i = caret_.row + 1; i < line_spans_.size(); ++i) {
        line_spans_[i].offset += 1;
      }
    }
    caret_.column += 1;
  }
  
  // Record for undo
  if (recordingHistory_) {
    history_.record(std::make_unique<InsertCharCommand>(insertPos, ch));
  }
}

void TextBuffer::insertText(const std::string &text) {
  for (char ch : text) {
    insertChar(ch);
  }
}

void TextBuffer::setText(const std::string &text) {
  chars_.clear();
  line_spans_.clear();
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

std::string TextBuffer::getText() const {
  return chars_.toString();
}

TextStyle TextBuffer::textStyle() const { return style_; }

void TextBuffer::setTextStyle(const TextStyle &style) { style_ = style; }

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
    
    line_spans_[caret_.row].length -= 1;
    
    // Shift subsequent line offsets
    for (std::size_t i = caret_.row + 1; i < line_spans_.size(); ++i) {
      line_spans_[i].offset -= 1;
    }
    
    caret_.column -= 1;
    
    // Record for undo
    if (recordingHistory_) {
      history_.record(std::make_unique<DeleteCharCommand>(deletePos, deletedChar, true));
    }
    return;
  }

  if (caret_.row == 0) {
    return;
  }

  // Join with previous line - delete the newline
  std::size_t prev_line_len = line_spans_[caret_.row - 1].length;
  std::size_t newline_offset = line_spans_[caret_.row - 1].offset + prev_line_len;
  CaretPosition deletePos = {caret_.row - 1, prev_line_len};
  
  chars_.erase(newline_offset, 1);
  stats_.total_deletes++;
  version_++;
  
  rebuildLineIndex();
  
  caret_.row -= 1;
  caret_.column = prev_line_len;
  
  // Record for undo (deleted a newline)
  if (recordingHistory_) {
    history_.record(std::make_unique<DeleteCharCommand>(deletePos, '\n', true));
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
    
    line_spans_[caret_.row].length -= 1;
    
    // Shift subsequent line offsets
    for (std::size_t i = caret_.row + 1; i < line_spans_.size(); ++i) {
      line_spans_[i].offset -= 1;
    }
    
    // Record for undo
    if (recordingHistory_) {
      history_.record(std::make_unique<DeleteCharCommand>(deletePos, deletedChar, false));
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
  
  rebuildLineIndex();
  
  // Record for undo (deleted a newline)
  if (recordingHistory_) {
    history_.record(std::make_unique<DeleteCharCommand>(deletePos, '\n', false));
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

void TextBuffer::moveToLineStart() {
  caret_.column = 0;
}

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

int TextBuffer::comparePositions(const CaretPosition &a,
                                 const CaretPosition &b) {
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

void CommandHistory::execute(std::unique_ptr<EditCommand> cmd, TextBuffer& buffer) {
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
    rebuildLineIndex();
    caret_.row += 1;
    caret_.column = 0;
  } else {
    if (caret_.row < line_spans_.size()) {
      line_spans_[caret_.row].length += 1;
      for (std::size_t i = caret_.row + 1; i < line_spans_.size(); ++i) {
        line_spans_[i].offset += 1;
      }
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
    for (std::size_t i = pos.row + 1; i < line_spans_.size(); ++i) {
      line_spans_[i].offset -= 1;
    }
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
