#include "action_map.h"

namespace input {

ActionMap::ActionMap() {
}

void ActionMap::bind(const KeyBinding& binding, Action action) {
  bindings_[binding] = action;
}

void ActionMap::unbind(const KeyBinding& binding) {
  bindings_.erase(binding);
}

bool ActionMap::isBindingPressed(const KeyBinding& binding) const {
  bool ctrl = raylib::IsKeyDown(raylib::KEY_LEFT_CONTROL) || 
              raylib::IsKeyDown(raylib::KEY_RIGHT_CONTROL);
  bool shift = raylib::IsKeyDown(raylib::KEY_LEFT_SHIFT) || 
               raylib::IsKeyDown(raylib::KEY_RIGHT_SHIFT);
  bool alt = raylib::IsKeyDown(raylib::KEY_LEFT_ALT) || 
             raylib::IsKeyDown(raylib::KEY_RIGHT_ALT);
  
  if (ctrl != binding.ctrl || shift != binding.shift || alt != binding.alt) {
    return false;
  }
  
  return raylib::IsKeyPressed(binding.key);
}

bool ActionMap::isActionPressed(Action action) const {
  for (const auto& [binding, act] : bindings_) {
    if (act == action && isBindingPressed(binding)) {
      return true;
    }
  }
  return false;
}

Action ActionMap::getActionForCurrentInput() const {
  for (const auto& [binding, action] : bindings_) {
    if (isBindingPressed(binding)) {
      return action;
    }
  }
  return Action::COUNT; // Invalid/no action
}

const char* ActionMap::actionName(Action action) {
  switch (action) {
    case Action::MoveLeft: return "MoveLeft";
    case Action::MoveRight: return "MoveRight";
    case Action::MoveUp: return "MoveUp";
    case Action::MoveDown: return "MoveDown";
    case Action::MoveWordLeft: return "MoveWordLeft";
    case Action::MoveWordRight: return "MoveWordRight";
    case Action::MoveLineStart: return "MoveLineStart";
    case Action::MoveLineEnd: return "MoveLineEnd";
    case Action::MoveDocumentStart: return "MoveDocumentStart";
    case Action::MoveDocumentEnd: return "MoveDocumentEnd";
    case Action::PageUp: return "PageUp";
    case Action::PageDown: return "PageDown";
    case Action::InsertNewline: return "InsertNewline";
    case Action::Backspace: return "Backspace";
    case Action::Delete: return "Delete";
    case Action::SelectAll: return "SelectAll";
    case Action::Copy: return "Copy";
    case Action::Cut: return "Cut";
    case Action::Paste: return "Paste";
    case Action::Undo: return "Undo";
    case Action::Redo: return "Redo";
    case Action::New: return "New";
    case Action::Open: return "Open";
    case Action::Save: return "Save";
    case Action::SaveAs: return "SaveAs";
    case Action::ToggleBold: return "ToggleBold";
    case Action::ToggleItalic: return "ToggleItalic";
    case Action::FontGaegu: return "FontGaegu";
    case Action::FontGaramond: return "FontGaramond";
    case Action::IncreaseFontSize: return "IncreaseFontSize";
    case Action::DecreaseFontSize: return "DecreaseFontSize";
    case Action::ResetFontSize: return "ResetFontSize";
    case Action::COUNT: return "NONE";
  }
  return "UNKNOWN";
}

ActionMap createDefaultActionMap() {
  ActionMap map;
  
  // Navigation
  map.bind({raylib::KEY_LEFT, false, false, false}, Action::MoveLeft);
  map.bind({raylib::KEY_RIGHT, false, false, false}, Action::MoveRight);
  map.bind({raylib::KEY_UP, false, false, false}, Action::MoveUp);
  map.bind({raylib::KEY_DOWN, false, false, false}, Action::MoveDown);
  map.bind({raylib::KEY_LEFT, true, false, false}, Action::MoveWordLeft);
  map.bind({raylib::KEY_RIGHT, true, false, false}, Action::MoveWordRight);
  map.bind({raylib::KEY_HOME, false, false, false}, Action::MoveLineStart);
  map.bind({raylib::KEY_END, false, false, false}, Action::MoveLineEnd);
  map.bind({raylib::KEY_HOME, true, false, false}, Action::MoveDocumentStart);
  map.bind({raylib::KEY_END, true, false, false}, Action::MoveDocumentEnd);
  map.bind({raylib::KEY_PAGE_UP, false, false, false}, Action::PageUp);
  map.bind({raylib::KEY_PAGE_DOWN, false, false, false}, Action::PageDown);
  
  // Editing
  map.bind({raylib::KEY_ENTER, false, false, false}, Action::InsertNewline);
  map.bind({raylib::KEY_KP_ENTER, false, false, false}, Action::InsertNewline);
  map.bind({raylib::KEY_BACKSPACE, false, false, false}, Action::Backspace);
  map.bind({raylib::KEY_DELETE, false, false, false}, Action::Delete);
  
  // Selection
  map.bind({raylib::KEY_A, true, false, false}, Action::SelectAll);
  
  // Clipboard
  map.bind({raylib::KEY_C, true, false, false}, Action::Copy);
  map.bind({raylib::KEY_X, true, false, false}, Action::Cut);
  map.bind({raylib::KEY_V, true, false, false}, Action::Paste);
  
  // Undo/Redo
  map.bind({raylib::KEY_Z, true, false, false}, Action::Undo);
  map.bind({raylib::KEY_Y, true, false, false}, Action::Redo);
  
  // File operations
  map.bind({raylib::KEY_N, true, false, false}, Action::New);
  map.bind({raylib::KEY_O, true, false, false}, Action::Open);
  map.bind({raylib::KEY_S, true, false, false}, Action::Save);
  
  // Formatting
  map.bind({raylib::KEY_B, true, false, false}, Action::ToggleBold);
  map.bind({raylib::KEY_I, true, false, false}, Action::ToggleItalic);
  map.bind({raylib::KEY_ONE, true, false, false}, Action::FontGaegu);
  map.bind({raylib::KEY_TWO, true, false, false}, Action::FontGaramond);
  map.bind({raylib::KEY_EQUAL, true, false, false}, Action::IncreaseFontSize);
  map.bind({raylib::KEY_KP_ADD, true, false, false}, Action::IncreaseFontSize);
  map.bind({raylib::KEY_MINUS, true, false, false}, Action::DecreaseFontSize);
  map.bind({raylib::KEY_KP_SUBTRACT, true, false, false}, Action::DecreaseFontSize);
  map.bind({raylib::KEY_ZERO, true, false, false}, Action::ResetFontSize);
  
  return map;
}

} // namespace input
