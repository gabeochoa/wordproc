#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct CaretPosition {
  std::size_t row = 0;
  std::size_t column = 0;
};

struct TextStyle {
  bool bold = false;
  bool italic = false;
  std::string font = "Gaegu-Bold";
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
  TextStyle textStyle() const;
  void setTextStyle(const TextStyle &style);
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
  TextStyle style_;
};
