#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct CaretPosition {
  std::size_t row = 0;
  std::size_t column = 0;
};

class TextBuffer {
public:
  TextBuffer();

  const std::vector<std::string> &lines() const;
  CaretPosition caret() const;
  bool hasSelection() const;
  CaretPosition selectionStart() const;
  CaretPosition selectionEnd() const;

  void setCaret(CaretPosition caret);
  void clearSelection();
  void setSelectionAnchor(CaretPosition anchor);
  void updateSelectionToCaret();

  void insertChar(char ch);
  void insertText(const std::string &text);
  void setText(const std::string &text);
  std::string getText() const;
  void backspace();
  void del();

  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();

private:
  void ensureNonEmpty();
  void clampCaret();
  static int comparePositions(const CaretPosition &a,
                              const CaretPosition &b);

  std::vector<std::string> lines_;
  CaretPosition caret_;
  bool has_selection_ = false;
  CaretPosition selection_anchor_;
  CaretPosition selection_end_;
};
