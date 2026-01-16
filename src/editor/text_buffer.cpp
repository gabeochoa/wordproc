#include "text_buffer.h"

#include <algorithm>

TextBuffer::TextBuffer() { ensureNonEmpty(); }

const std::vector<std::string> &TextBuffer::lines() const { return lines_; }

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

void TextBuffer::insertChar(char ch) {
  ensureNonEmpty();

  if (ch == '\n') {
    std::string &line = lines_[caret_.row];
    std::string remainder = line.substr(caret_.column);
    line.erase(caret_.column);
    lines_.insert(lines_.begin() + static_cast<long>(caret_.row) + 1,
                  remainder);
    caret_.row += 1;
    caret_.column = 0;
    return;
  }

  std::string &line = lines_[caret_.row];
  line.insert(line.begin() + static_cast<long>(caret_.column), ch);
  caret_.column += 1;
}

void TextBuffer::insertText(const std::string &text) {
  for (char ch : text) {
    insertChar(ch);
  }
}

void TextBuffer::setText(const std::string &text) {
  lines_.clear();
  std::string current;

  for (char ch : text) {
    if (ch == '\n') {
      if (!current.empty() && current.back() == '\r') {
        current.pop_back();
      }
      lines_.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(ch);
  }

  if (!current.empty() && current.back() == '\r') {
    current.pop_back();
  }
  lines_.push_back(current);

  ensureNonEmpty();
  caret_.row = lines_.size() - 1;
  caret_.column = lines_[caret_.row].size();
  clearSelection();
}

std::string TextBuffer::getText() const {
  if (lines_.empty()) {
    return "";
  }

  std::string result;
  for (std::size_t i = 0; i < lines_.size(); ++i) {
    result += lines_[i];
    if (i + 1 < lines_.size()) {
      result += '\n';
    }
  }
  return result;
}

TextStyle TextBuffer::textStyle() const { return style_; }

void TextBuffer::setTextStyle(const TextStyle &style) { style_ = style; }

void TextBuffer::backspace() {
  ensureNonEmpty();

  if (caret_.column > 0) {
    std::string &line = lines_[caret_.row];
    line.erase(line.begin() + static_cast<long>(caret_.column) - 1);
    caret_.column -= 1;
    return;
  }

  if (caret_.row == 0) {
    return;
  }

  std::string &line = lines_[caret_.row];
  std::string &prev = lines_[caret_.row - 1];
  std::size_t prev_len = prev.size();
  prev += line;
  lines_.erase(lines_.begin() + static_cast<long>(caret_.row));
  caret_.row -= 1;
  caret_.column = prev_len;
}

void TextBuffer::del() {
  ensureNonEmpty();

  std::string &line = lines_[caret_.row];
  if (caret_.column < line.size()) {
    line.erase(line.begin() + static_cast<long>(caret_.column));
    return;
  }

  if (caret_.row + 1 >= lines_.size()) {
    return;
  }

  line += lines_[caret_.row + 1];
  lines_.erase(lines_.begin() + static_cast<long>(caret_.row) + 1);
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
  caret_.column = lines_[caret_.row].size();
}

void TextBuffer::moveRight() {
  std::string &line = lines_[caret_.row];
  if (caret_.column < line.size()) {
    caret_.column += 1;
    return;
  }
  if (caret_.row + 1 >= lines_.size()) {
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
  caret_.column =
      std::min(caret_.column, lines_[caret_.row].size());
}

void TextBuffer::moveDown() {
  if (caret_.row + 1 >= lines_.size()) {
    return;
  }
  caret_.row += 1;
  caret_.column =
      std::min(caret_.column, lines_[caret_.row].size());
}

void TextBuffer::ensureNonEmpty() {
  if (lines_.empty()) {
    lines_.emplace_back();
  }
  clampCaret();
}

void TextBuffer::clampCaret() {
  if (lines_.empty()) {
    caret_ = {};
    return;
  }
  if (caret_.row >= lines_.size()) {
    caret_.row = lines_.size() - 1;
  }
  std::size_t max_column = lines_[caret_.row].size();
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
