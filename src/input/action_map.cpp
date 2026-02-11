#include "action_map.h"
#include <afterhours/src/core/key_codes.h>
// test_input:: and input_injector:: available via rl.h -> external.h
// afterhours::graphics::is_key_pressed_repeat via graphics.h (included through rl.h)

namespace input {

ActionMap::ActionMap() {}

void ActionMap::bind(const KeyBinding& binding, Action action) {
    bindings_[binding] = action;
}

void ActionMap::unbind(const KeyBinding& binding) { bindings_.erase(binding); }

// Helper to check if a key is down (test-aware via test_input)
static bool isKeyDownHelper(int key) {
    return test_input::is_key_down(key);
}

// Helper to check if a key was just pressed (test-aware via test_input)
static bool isKeyPressedHelper(int key) {
    return test_input::is_key_pressed(key);
}

// Helper for key press with repeat (for held navigation keys)
// Uses built-in key repeat, with fallback for test mode
static bool isKeyPressedRepeatHelper(int key) {
    // In test mode, just use regular press (no repeat in tests)
    if (test_input::is_key_pressed(key)) {
        return true;
    }
    // Use platform key repeat for real input
    return afterhours::graphics::is_key_pressed_repeat(key);
}

bool ActionMap::isBindingPressed(const KeyBinding& binding) const {
    bool ctrl = isKeyDownHelper(afterhours::keys::LEFT_CONTROL) ||
                isKeyDownHelper(afterhours::keys::RIGHT_CONTROL);
    bool shift = isKeyDownHelper(afterhours::keys::LEFT_SHIFT) ||
                 isKeyDownHelper(afterhours::keys::RIGHT_SHIFT);
    bool alt = isKeyDownHelper(afterhours::keys::LEFT_ALT) ||
               isKeyDownHelper(afterhours::keys::RIGHT_ALT);

    if (ctrl != binding.ctrl || shift != binding.shift || alt != binding.alt) {
        return false;
    }

    return isKeyPressedHelper(binding.key);
}

bool ActionMap::isActionPressed(Action action) const {
    for (const auto& [binding, act] : bindings_) {
        if (act == action && isBindingPressed(binding)) {
            return true;
        }
    }
    return false;
}

bool ActionMap::isBindingPressedRepeat(const KeyBinding& binding) const {
    bool ctrl = isKeyDownHelper(afterhours::keys::LEFT_CONTROL) ||
                isKeyDownHelper(afterhours::keys::RIGHT_CONTROL);
    bool shift = isKeyDownHelper(afterhours::keys::LEFT_SHIFT) ||
                 isKeyDownHelper(afterhours::keys::RIGHT_SHIFT);
    bool alt = isKeyDownHelper(afterhours::keys::LEFT_ALT) ||
               isKeyDownHelper(afterhours::keys::RIGHT_ALT);

    if (ctrl != binding.ctrl || shift != binding.shift || alt != binding.alt) {
        return false;
    }

    return isKeyPressedRepeatHelper(binding.key);
}

bool ActionMap::isActionPressedRepeat(Action action) const {
    for (const auto& [binding, act] : bindings_) {
        if (act == action && isBindingPressedRepeat(binding)) {
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
    return Action::COUNT;  // Invalid/no action
}

const char* ActionMap::actionName(Action action) {
    switch (action) {
        case Action::MoveLeft:
            return "MoveLeft";
        case Action::MoveRight:
            return "MoveRight";
        case Action::MoveUp:
            return "MoveUp";
        case Action::MoveDown:
            return "MoveDown";
        case Action::MoveWordLeft:
            return "MoveWordLeft";
        case Action::MoveWordRight:
            return "MoveWordRight";
        case Action::MoveLineStart:
            return "MoveLineStart";
        case Action::MoveLineEnd:
            return "MoveLineEnd";
        case Action::MoveDocumentStart:
            return "MoveDocumentStart";
        case Action::MoveDocumentEnd:
            return "MoveDocumentEnd";
        case Action::PageUp:
            return "PageUp";
        case Action::PageDown:
            return "PageDown";
        case Action::InsertNewline:
            return "InsertNewline";
        case Action::Backspace:
            return "Backspace";
        case Action::Delete:
            return "Delete";
        case Action::SelectAll:
            return "SelectAll";
        case Action::Copy:
            return "Copy";
        case Action::Cut:
            return "Cut";
        case Action::Paste:
            return "Paste";
        case Action::Undo:
            return "Undo";
        case Action::Redo:
            return "Redo";
        case Action::New:
            return "New";
        case Action::Open:
            return "Open";
        case Action::Save:
            return "Save";
        case Action::SaveAs:
            return "SaveAs";
        case Action::ToggleBold:
            return "ToggleBold";
        case Action::ToggleItalic:
            return "ToggleItalic";
        case Action::ToggleUnderline:
            return "ToggleUnderline";
        case Action::ToggleStrikethrough:
            return "ToggleStrikethrough";
        case Action::ToggleSuperscript:
            return "ToggleSuperscript";
        case Action::ToggleSubscript:
            return "ToggleSubscript";
        case Action::FontGaegu:
            return "FontGaegu";
        case Action::FontGaramond:
            return "FontGaramond";
        case Action::IncreaseFontSize:
            return "IncreaseFontSize";
        case Action::DecreaseFontSize:
            return "DecreaseFontSize";
        case Action::ResetFontSize:
            return "ResetFontSize";
        case Action::StyleNormal:
            return "StyleNormal";
        case Action::StyleTitle:
            return "StyleTitle";
        case Action::StyleSubtitle:
            return "StyleSubtitle";
        case Action::StyleHeading1:
            return "StyleHeading1";
        case Action::StyleHeading2:
            return "StyleHeading2";
        case Action::StyleHeading3:
            return "StyleHeading3";
        case Action::StyleHeading4:
            return "StyleHeading4";
        case Action::StyleHeading5:
            return "StyleHeading5";
        case Action::StyleHeading6:
            return "StyleHeading6";
        case Action::AlignLeft:
            return "AlignLeft";
        case Action::AlignCenter:
            return "AlignCenter";
        case Action::AlignRight:
            return "AlignRight";
        case Action::AlignJustify:
            return "AlignJustify";
        case Action::IndentIncrease:
            return "IndentIncrease";
        case Action::IndentDecrease:
            return "IndentDecrease";
        case Action::LineSpacingSingle:
            return "LineSpacingSingle";
        case Action::LineSpacing1_5:
            return "LineSpacing1_5";
        case Action::LineSpacingDouble:
            return "LineSpacingDouble";
        case Action::ZoomIn:
            return "ZoomIn";
        case Action::ZoomOut:
            return "ZoomOut";
        case Action::ZoomReset:
            return "ZoomReset";
        case Action::ToggleFocusMode:
            return "ToggleFocusMode";
        case Action::ToggleSplitView:
            return "ToggleSplitView";
        case Action::ToggleDarkMode:
            return "ToggleDarkMode";
        case Action::ToggleBulletedList:
            return "ToggleBulletedList";
        case Action::ToggleNumberedList:
            return "ToggleNumberedList";
        case Action::Find:
            return "Find";
        case Action::FindNext:
            return "FindNext";
        case Action::FindPrevious:
            return "FindPrevious";
        case Action::Replace:
            return "Replace";
        case Action::IncreaseSpaceBefore:
            return "IncreaseSpaceBefore";
        case Action::DecreaseSpaceBefore:
            return "DecreaseSpaceBefore";
        case Action::IncreaseSpaceAfter:
            return "IncreaseSpaceAfter";
        case Action::DecreaseSpaceAfter:
            return "DecreaseSpaceAfter";
        case Action::InsertTable:
            return "InsertTable";
        case Action::TableInsertRowAbove:
            return "TableInsertRowAbove";
        case Action::TableInsertRowBelow:
            return "TableInsertRowBelow";
        case Action::TableInsertColumnLeft:
            return "TableInsertColumnLeft";
        case Action::TableInsertColumnRight:
            return "TableInsertColumnRight";
        case Action::TableDeleteRow:
            return "TableDeleteRow";
        case Action::TableDeleteColumn:
            return "TableDeleteColumn";
        case Action::TableMergeCells:
            return "TableMergeCells";
        case Action::TableSplitCell:
            return "TableSplitCell";
        case Action::TableMoveNextCell:
            return "TableMoveNextCell";
        case Action::TableMovePrevCell:
            return "TableMovePrevCell";
        case Action::InsertPageBreak:
            return "InsertPageBreak";
        case Action::TogglePageBreak:
            return "TogglePageBreak";
        case Action::InsertHyperlink:
            return "InsertHyperlink";
        case Action::RemoveHyperlink:
            return "RemoveHyperlink";
        case Action::COUNT:
        default:
            return "NONE";
    }
}

const char* presetName(Preset preset) {
    switch (preset) {
        case Preset::SystemDefault:
            return "System Default";
        case Preset::WindowsCtrl:
            return "Windows (Ctrl)";
        case Preset::MacOSCmd:
            return "macOS (Cmd)";
        default:
            return "Unknown";
    }
}

// Helper to bind common navigation keys (same across all presets)
static void bindNavigationKeys(ActionMap& map) {
    map.bind({afterhours::keys::LEFT, false, false, false}, Action::MoveLeft);
    map.bind({afterhours::keys::RIGHT, false, false, false}, Action::MoveRight);
    map.bind({afterhours::keys::UP, false, false, false}, Action::MoveUp);
    map.bind({afterhours::keys::DOWN, false, false, false}, Action::MoveDown);
    map.bind({afterhours::keys::PAGE_UP, false, false, false}, Action::PageUp);
    map.bind({afterhours::keys::PAGE_DOWN, false, false, false}, Action::PageDown);
    
    // Also bind with shift for selection (shift is handled separately in NavigationSystem)
    map.bind({afterhours::keys::LEFT, false, true, false}, Action::MoveLeft);
    map.bind({afterhours::keys::RIGHT, false, true, false}, Action::MoveRight);
    map.bind({afterhours::keys::UP, false, true, false}, Action::MoveUp);
    map.bind({afterhours::keys::DOWN, false, true, false}, Action::MoveDown);
    map.bind({afterhours::keys::PAGE_UP, false, true, false}, Action::PageUp);
    map.bind({afterhours::keys::PAGE_DOWN, false, true, false}, Action::PageDown);

    // Editing (same across presets)
    map.bind({afterhours::keys::ENTER, false, false, false}, Action::InsertNewline);
    map.bind({afterhours::keys::KP_ENTER, false, false, false},
             Action::InsertNewline);
    map.bind({afterhours::keys::BACKSPACE, false, false, false}, Action::Backspace);
    map.bind({afterhours::keys::DELETE_KEY, false, false, false}, Action::Delete);
}

// Windows-style bindings: Ctrl+key
static void bindWindowsPreset(ActionMap& map) {
    bindNavigationKeys(map);

    // Word/line navigation: Ctrl+Arrow, Home/End
    map.bind({afterhours::keys::LEFT, true, false, false}, Action::MoveWordLeft);
    map.bind({afterhours::keys::RIGHT, true, false, false}, Action::MoveWordRight);
    map.bind({afterhours::keys::HOME, false, false, false}, Action::MoveLineStart);
    map.bind({afterhours::keys::END, false, false, false}, Action::MoveLineEnd);
    map.bind({afterhours::keys::HOME, true, false, false}, Action::MoveDocumentStart);
    map.bind({afterhours::keys::END, true, false, false}, Action::MoveDocumentEnd);
    
    // Word/line navigation with shift (for selection)
    map.bind({afterhours::keys::LEFT, true, true, false}, Action::MoveWordLeft);
    map.bind({afterhours::keys::RIGHT, true, true, false}, Action::MoveWordRight);
    map.bind({afterhours::keys::HOME, false, true, false}, Action::MoveLineStart);
    map.bind({afterhours::keys::END, false, true, false}, Action::MoveLineEnd);
    map.bind({afterhours::keys::HOME, true, true, false}, Action::MoveDocumentStart);
    map.bind({afterhours::keys::END, true, true, false}, Action::MoveDocumentEnd);

    // Selection, clipboard, undo/redo: Ctrl+key
    map.bind({afterhours::keys::A, true, false, false}, Action::SelectAll);
    map.bind({afterhours::keys::C, true, false, false}, Action::Copy);
    map.bind({afterhours::keys::X, true, false, false}, Action::Cut);
    map.bind({afterhours::keys::V, true, false, false}, Action::Paste);
    map.bind({afterhours::keys::Z, true, false, false}, Action::Undo);
    map.bind({afterhours::keys::Y, true, false, false}, Action::Redo);

    // File operations: Ctrl+key
    map.bind({afterhours::keys::N, true, false, false}, Action::New);
    map.bind({afterhours::keys::O, true, false, false}, Action::Open);
    map.bind({afterhours::keys::S, true, false, false}, Action::Save);

    // Formatting: Ctrl+key
    map.bind({afterhours::keys::B, true, false, false}, Action::ToggleBold);
    map.bind({afterhours::keys::I, true, false, false}, Action::ToggleItalic);
    map.bind({afterhours::keys::U, true, false, false}, Action::ToggleUnderline);
    map.bind({afterhours::keys::S, true, true, false}, Action::ToggleStrikethrough);  // Ctrl+Shift+S
    map.bind({afterhours::keys::EQUAL, true, true, false}, Action::ToggleSuperscript);
    map.bind({afterhours::keys::MINUS, true, true, false}, Action::ToggleSubscript);
    map.bind({afterhours::keys::ONE, true, false, false}, Action::FontGaegu);
    map.bind({afterhours::keys::TWO, true, false, false}, Action::FontGaramond);
    map.bind({afterhours::keys::EQUAL, true, false, false}, Action::IncreaseFontSize);
    map.bind({afterhours::keys::KP_ADD, true, false, false},
             Action::IncreaseFontSize);
    map.bind({afterhours::keys::MINUS, true, false, false}, Action::DecreaseFontSize);
    map.bind({afterhours::keys::KP_SUBTRACT, true, false, false},
             Action::DecreaseFontSize);
    map.bind({afterhours::keys::ZERO, true, false, false}, Action::ResetFontSize);
    
    // Paragraph styles: Ctrl+Alt+number for headings, Ctrl+Alt+0 for normal
    map.bind({afterhours::keys::ZERO, true, false, true}, Action::StyleNormal);
    map.bind({afterhours::keys::ONE, true, false, true}, Action::StyleHeading1);
    map.bind({afterhours::keys::TWO, true, false, true}, Action::StyleHeading2);
    map.bind({afterhours::keys::THREE, true, false, true}, Action::StyleHeading3);
    map.bind({afterhours::keys::FOUR, true, false, true}, Action::StyleHeading4);
    map.bind({afterhours::keys::FIVE, true, false, true}, Action::StyleHeading5);
    map.bind({afterhours::keys::SIX, true, false, true}, Action::StyleHeading6);
    
    // Text alignment: Ctrl+L/E/R/J (standard Word shortcuts)
    map.bind({afterhours::keys::L, true, false, false}, Action::AlignLeft);
    map.bind({afterhours::keys::E, true, false, false}, Action::AlignCenter);
    map.bind({afterhours::keys::R, true, false, false}, Action::AlignRight);
    map.bind({afterhours::keys::J, true, false, false}, Action::AlignJustify);
    
    // Indentation: Ctrl+] to increase, Ctrl+[ to decrease (standard Word shortcuts)
    map.bind({afterhours::keys::RIGHT_BRACKET, true, false, false}, Action::IndentIncrease);
    map.bind({afterhours::keys::LEFT_BRACKET, true, false, false}, Action::IndentDecrease);
    
    // Line spacing: Ctrl+Shift+1/5/2 for single/1.5/double
    map.bind({afterhours::keys::ONE, true, true, false}, Action::LineSpacingSingle);
    map.bind({afterhours::keys::FIVE, true, true, false}, Action::LineSpacing1_5);
    map.bind({afterhours::keys::TWO, true, true, false}, Action::LineSpacingDouble);

    // View controls
    map.bind({afterhours::keys::EQUAL, true, false, true}, Action::ZoomIn);
    map.bind({afterhours::keys::MINUS, true, false, true}, Action::ZoomOut);
    map.bind({afterhours::keys::ZERO, true, false, true}, Action::ZoomReset);
    map.bind({afterhours::keys::F11, false, false, false}, Action::ToggleFocusMode);
    map.bind({afterhours::keys::V, true, false, true}, Action::ToggleSplitView);
    map.bind({afterhours::keys::D, true, false, true}, Action::ToggleDarkMode);
    
    // Lists: Ctrl+Shift+8 for bullets, Ctrl+Shift+7 for numbers
    map.bind({afterhours::keys::EIGHT, true, true, false}, Action::ToggleBulletedList);
    map.bind({afterhours::keys::SEVEN, true, true, false}, Action::ToggleNumberedList);
    
    // Find and Replace
    map.bind({afterhours::keys::F, true, false, false}, Action::Find);
    map.bind({afterhours::keys::G, true, false, false}, Action::FindNext);
    map.bind({afterhours::keys::F3, false, false, false}, Action::FindNext);
    map.bind({afterhours::keys::G, true, true, false}, Action::FindPrevious);
    map.bind({afterhours::keys::F3, false, true, false}, Action::FindPrevious);
    map.bind({afterhours::keys::H, true, false, false}, Action::Replace);
    
    // Paragraph spacing: Ctrl+Alt+Up/Down for before, Ctrl+Shift+Alt+Up/Down for after
    map.bind({afterhours::keys::UP, true, false, true}, Action::IncreaseSpaceBefore);
    map.bind({afterhours::keys::DOWN, true, false, true}, Action::DecreaseSpaceBefore);
    map.bind({afterhours::keys::UP, true, true, true}, Action::IncreaseSpaceAfter);
    map.bind({afterhours::keys::DOWN, true, true, true}, Action::DecreaseSpaceAfter);
}

// macOS-style bindings: uses Ctrl as Cmd equivalent
// Key differences from Windows:
// - Option+Arrow for word navigation (Alt key)
// - Cmd+Arrow for line/document navigation (Ctrl as Cmd substitute)
// - Cmd+Shift+Z for Redo instead of Cmd+Y
static void bindMacOSPreset(ActionMap& map) {
    bindNavigationKeys(map);

    // Word navigation: Option+Arrow (Alt key)
    map.bind({afterhours::keys::LEFT, false, false, true}, Action::MoveWordLeft);
    map.bind({afterhours::keys::RIGHT, false, false, true}, Action::MoveWordRight);

    // Line navigation: Cmd+Arrow (Ctrl as Cmd substitute)
    map.bind({afterhours::keys::LEFT, true, false, false}, Action::MoveLineStart);
    map.bind({afterhours::keys::RIGHT, true, false, false}, Action::MoveLineEnd);
    map.bind({afterhours::keys::UP, true, false, false}, Action::MoveDocumentStart);
    map.bind({afterhours::keys::DOWN, true, false, false}, Action::MoveDocumentEnd);

    // Home/End also work for line start/end
    map.bind({afterhours::keys::HOME, false, false, false}, Action::MoveLineStart);
    map.bind({afterhours::keys::END, false, false, false}, Action::MoveLineEnd);

    // Selection, clipboard: Cmd+key (Ctrl as substitute)
    map.bind({afterhours::keys::A, true, false, false}, Action::SelectAll);
    map.bind({afterhours::keys::C, true, false, false}, Action::Copy);
    map.bind({afterhours::keys::X, true, false, false}, Action::Cut);
    map.bind({afterhours::keys::V, true, false, false}, Action::Paste);

    // Undo: Cmd+Z, Redo: Cmd+Shift+Z (macOS style)
    map.bind({afterhours::keys::Z, true, false, false}, Action::Undo);
    map.bind({afterhours::keys::Z, true, true, false}, Action::Redo);  // Cmd+Shift+Z

    // File operations: Cmd+key (Ctrl as substitute)
    map.bind({afterhours::keys::N, true, false, false}, Action::New);
    map.bind({afterhours::keys::O, true, false, false}, Action::Open);
    map.bind({afterhours::keys::S, true, false, false}, Action::Save);

    // Formatting: Cmd+key
    map.bind({afterhours::keys::B, true, false, false}, Action::ToggleBold);
    map.bind({afterhours::keys::I, true, false, false}, Action::ToggleItalic);
    map.bind({afterhours::keys::U, true, false, false}, Action::ToggleUnderline);
    map.bind({afterhours::keys::S, true, true, false}, Action::ToggleStrikethrough);  // Cmd+Shift+S
    map.bind({afterhours::keys::EQUAL, true, true, false}, Action::ToggleSuperscript);
    map.bind({afterhours::keys::MINUS, true, true, false}, Action::ToggleSubscript);
    map.bind({afterhours::keys::ONE, true, false, false}, Action::FontGaegu);
    map.bind({afterhours::keys::TWO, true, false, false}, Action::FontGaramond);
    map.bind({afterhours::keys::EQUAL, true, false, false}, Action::IncreaseFontSize);
    map.bind({afterhours::keys::KP_ADD, true, false, false},
             Action::IncreaseFontSize);
    map.bind({afterhours::keys::MINUS, true, false, false}, Action::DecreaseFontSize);
    map.bind({afterhours::keys::KP_SUBTRACT, true, false, false},
             Action::DecreaseFontSize);
    map.bind({afterhours::keys::ZERO, true, false, false}, Action::ResetFontSize);
    
    // Paragraph styles: Ctrl+Alt+number for headings (same as Windows)
    map.bind({afterhours::keys::ZERO, true, false, true}, Action::StyleNormal);
    map.bind({afterhours::keys::ONE, true, false, true}, Action::StyleHeading1);
    map.bind({afterhours::keys::TWO, true, false, true}, Action::StyleHeading2);
    map.bind({afterhours::keys::THREE, true, false, true}, Action::StyleHeading3);
    map.bind({afterhours::keys::FOUR, true, false, true}, Action::StyleHeading4);
    map.bind({afterhours::keys::FIVE, true, false, true}, Action::StyleHeading5);
    map.bind({afterhours::keys::SIX, true, false, true}, Action::StyleHeading6);
    
    // Text alignment: Cmd+L/E/R/J (same shortcuts as Windows)
    map.bind({afterhours::keys::L, true, false, false}, Action::AlignLeft);
    map.bind({afterhours::keys::E, true, false, false}, Action::AlignCenter);
    map.bind({afterhours::keys::R, true, false, false}, Action::AlignRight);
    map.bind({afterhours::keys::J, true, false, false}, Action::AlignJustify);
    
    // Indentation: Cmd+] to increase, Cmd+[ to decrease
    map.bind({afterhours::keys::RIGHT_BRACKET, true, false, false}, Action::IndentIncrease);
    map.bind({afterhours::keys::LEFT_BRACKET, true, false, false}, Action::IndentDecrease);
    
    // Line spacing: Cmd+Shift+1/5/2 for single/1.5/double
    map.bind({afterhours::keys::ONE, true, true, false}, Action::LineSpacingSingle);
    map.bind({afterhours::keys::FIVE, true, true, false}, Action::LineSpacing1_5);
    map.bind({afterhours::keys::TWO, true, true, false}, Action::LineSpacingDouble);

    // View controls
    map.bind({afterhours::keys::EQUAL, true, false, true}, Action::ZoomIn);
    map.bind({afterhours::keys::MINUS, true, false, true}, Action::ZoomOut);
    map.bind({afterhours::keys::ZERO, true, false, true}, Action::ZoomReset);
    map.bind({afterhours::keys::F11, false, false, false}, Action::ToggleFocusMode);
    map.bind({afterhours::keys::V, true, false, true}, Action::ToggleSplitView);
    map.bind({afterhours::keys::D, true, false, true}, Action::ToggleDarkMode);
    
    // Lists: Cmd+Shift+8 for bullets, Cmd+Shift+7 for numbers
    map.bind({afterhours::keys::EIGHT, true, true, false}, Action::ToggleBulletedList);
    map.bind({afterhours::keys::SEVEN, true, true, false}, Action::ToggleNumberedList);
    
    // Find and Replace
    map.bind({afterhours::keys::F, true, false, false}, Action::Find);
    map.bind({afterhours::keys::G, true, false, false}, Action::FindNext);
    map.bind({afterhours::keys::F3, false, false, false}, Action::FindNext);
    map.bind({afterhours::keys::G, true, true, false}, Action::FindPrevious);
    map.bind({afterhours::keys::F3, false, true, false}, Action::FindPrevious);
    map.bind({afterhours::keys::H, true, false, false}, Action::Replace);
    
    // Paragraph spacing: Cmd+Alt+Up/Down for before, Cmd+Shift+Alt+Up/Down for after
    map.bind({afterhours::keys::UP, true, false, true}, Action::IncreaseSpaceBefore);
    map.bind({afterhours::keys::DOWN, true, false, true}, Action::DecreaseSpaceBefore);
    map.bind({afterhours::keys::UP, true, true, true}, Action::IncreaseSpaceAfter);
    map.bind({afterhours::keys::DOWN, true, true, true}, Action::DecreaseSpaceAfter);
}

ActionMap createActionMapWithPreset(Preset preset) {
    ActionMap map;

    switch (preset) {
        case Preset::SystemDefault:
#ifdef __APPLE__
            bindMacOSPreset(map);
#else
            bindWindowsPreset(map);
#endif
            break;
        case Preset::WindowsCtrl:
            bindWindowsPreset(map);
            break;
        case Preset::MacOSCmd:
            bindMacOSPreset(map);
            break;
        default:
            bindWindowsPreset(map);
            break;
    }

    return map;
}

ActionMap createDefaultActionMap() {
    return createActionMapWithPreset(Preset::SystemDefault);
}

const char* actionDisplayName(Action action) {
    switch (action) {
        case Action::MoveLeft:
            return "Move Left";
        case Action::MoveRight:
            return "Move Right";
        case Action::MoveUp:
            return "Move Up";
        case Action::MoveDown:
            return "Move Down";
        case Action::MoveWordLeft:
            return "Move Word Left";
        case Action::MoveWordRight:
            return "Move Word Right";
        case Action::MoveLineStart:
            return "Move to Line Start";
        case Action::MoveLineEnd:
            return "Move to Line End";
        case Action::MoveDocumentStart:
            return "Move to Document Start";
        case Action::MoveDocumentEnd:
            return "Move to Document End";
        case Action::PageUp:
            return "Page Up";
        case Action::PageDown:
            return "Page Down";
        case Action::InsertNewline:
            return "Insert New Line";
        case Action::Backspace:
            return "Backspace";
        case Action::Delete:
            return "Delete";
        case Action::SelectAll:
            return "Select All";
        case Action::Copy:
            return "Copy";
        case Action::Cut:
            return "Cut";
        case Action::Paste:
            return "Paste";
        case Action::Undo:
            return "Undo";
        case Action::Redo:
            return "Redo";
        case Action::New:
            return "New Document";
        case Action::Open:
            return "Open";
        case Action::Save:
            return "Save";
        case Action::SaveAs:
            return "Save As";
        case Action::ToggleBold:
            return "Toggle Bold";
        case Action::ToggleItalic:
            return "Toggle Italic";
        case Action::ToggleUnderline:
            return "Toggle Underline";
        case Action::ToggleStrikethrough:
            return "Toggle Strikethrough";
        case Action::ToggleSuperscript:
            return "Toggle Superscript";
        case Action::ToggleSubscript:
            return "Toggle Subscript";
        case Action::FontGaegu:
            return "Font: Gaegu";
        case Action::FontGaramond:
            return "Font: Garamond";
        case Action::IncreaseFontSize:
            return "Increase Font Size";
        case Action::DecreaseFontSize:
            return "Decrease Font Size";
        case Action::ResetFontSize:
            return "Reset Font Size";
        case Action::StyleNormal:
            return "Normal Text";
        case Action::StyleTitle:
            return "Title";
        case Action::StyleSubtitle:
            return "Subtitle";
        case Action::StyleHeading1:
            return "Heading 1";
        case Action::StyleHeading2:
            return "Heading 2";
        case Action::StyleHeading3:
            return "Heading 3";
        case Action::StyleHeading4:
            return "Heading 4";
        case Action::StyleHeading5:
            return "Heading 5";
        case Action::StyleHeading6:
            return "Heading 6";
        case Action::AlignLeft:
            return "Align Left";
        case Action::AlignCenter:
            return "Align Center";
        case Action::AlignRight:
            return "Align Right";
        case Action::AlignJustify:
            return "Justify";
        case Action::IndentIncrease:
            return "Increase Indent";
        case Action::IndentDecrease:
            return "Decrease Indent";
        case Action::LineSpacingSingle:
            return "Single Spacing";
        case Action::LineSpacing1_5:
            return "1.5 Line Spacing";
        case Action::LineSpacingDouble:
            return "Double Spacing";
        case Action::ZoomIn:
            return "Zoom In";
        case Action::ZoomOut:
            return "Zoom Out";
        case Action::ZoomReset:
            return "Zoom Reset";
        case Action::ToggleFocusMode:
            return "Toggle Focus Mode";
        case Action::ToggleSplitView:
            return "Toggle Split View";
        case Action::ToggleDarkMode:
            return "Toggle Dark Mode";
        case Action::ToggleBulletedList:
            return "Toggle Bullets";
        case Action::ToggleNumberedList:
            return "Toggle Numbering";
        case Action::Find:
            return "Find";
        case Action::FindNext:
            return "Find Next";
        case Action::FindPrevious:
            return "Find Previous";
        case Action::Replace:
            return "Replace";
        case Action::IncreaseSpaceBefore:
            return "Increase Space Before";
        case Action::DecreaseSpaceBefore:
            return "Decrease Space Before";
        case Action::IncreaseSpaceAfter:
            return "Increase Space After";
        case Action::DecreaseSpaceAfter:
            return "Decrease Space After";
        case Action::InsertTable:
            return "Insert Table";
        case Action::TableInsertRowAbove:
            return "Insert Row Above";
        case Action::TableInsertRowBelow:
            return "Insert Row Below";
        case Action::TableInsertColumnLeft:
            return "Insert Column Left";
        case Action::TableInsertColumnRight:
            return "Insert Column Right";
        case Action::TableDeleteRow:
            return "Delete Row";
        case Action::TableDeleteColumn:
            return "Delete Column";
        case Action::TableMergeCells:
            return "Merge Cells";
        case Action::TableSplitCell:
            return "Split Cell";
        case Action::TableMoveNextCell:
            return "Next Cell";
        case Action::TableMovePrevCell:
            return "Previous Cell";
        case Action::InsertPageBreak:
            return "Page Break";
        case Action::TogglePageBreak:
            return "Toggle Page Break";
        case Action::InsertHyperlink:
            return "Insert Hyperlink";
        case Action::RemoveHyperlink:
            return "Remove Hyperlink";
        case Action::COUNT:
        default:
            return "";
    }
}

std::string keyName(int keyCode) {
    switch (keyCode) {
        // Letters
        case afterhours::keys::A:
            return "A";
        case afterhours::keys::B:
            return "B";
        case afterhours::keys::C:
            return "C";
        case afterhours::keys::D:
            return "D";
        case afterhours::keys::E:
            return "E";
        case afterhours::keys::F:
            return "F";
        case afterhours::keys::G:
            return "G";
        case afterhours::keys::H:
            return "H";
        case afterhours::keys::I:
            return "I";
        case afterhours::keys::J:
            return "J";
        case afterhours::keys::K:
            return "K";
        case afterhours::keys::L:
            return "L";
        case afterhours::keys::M:
            return "M";
        case afterhours::keys::N:
            return "N";
        case afterhours::keys::O:
            return "O";
        case afterhours::keys::P:
            return "P";
        case afterhours::keys::Q:
            return "Q";
        case afterhours::keys::R:
            return "R";
        case afterhours::keys::S:
            return "S";
        case afterhours::keys::T:
            return "T";
        case afterhours::keys::U:
            return "U";
        case afterhours::keys::V:
            return "V";
        case afterhours::keys::W:
            return "W";
        case afterhours::keys::X:
            return "X";
        case afterhours::keys::Y:
            return "Y";
        case afterhours::keys::Z:
            return "Z";

        // Numbers
        case afterhours::keys::ZERO:
            return "0";
        case afterhours::keys::ONE:
            return "1";
        case afterhours::keys::TWO:
            return "2";
        case afterhours::keys::THREE:
            return "3";
        case afterhours::keys::FOUR:
            return "4";
        case afterhours::keys::FIVE:
            return "5";
        case afterhours::keys::SIX:
            return "6";
        case afterhours::keys::SEVEN:
            return "7";
        case afterhours::keys::EIGHT:
            return "8";
        case afterhours::keys::NINE:
            return "9";

        // Function keys
        case afterhours::keys::F1:
            return "F1";
        case afterhours::keys::F2:
            return "F2";
        case afterhours::keys::F3:
            return "F3";
        case afterhours::keys::F4:
            return "F4";
        case afterhours::keys::F5:
            return "F5";
        case afterhours::keys::F6:
            return "F6";
        case afterhours::keys::F7:
            return "F7";
        case afterhours::keys::F8:
            return "F8";
        case afterhours::keys::F9:
            return "F9";
        case afterhours::keys::F10:
            return "F10";
        case afterhours::keys::F11:
            return "F11";
        case afterhours::keys::F12:
            return "F12";

        // Special keys
        case afterhours::keys::SPACE:
            return "Space";
        case afterhours::keys::ESCAPE:
            return "Escape";
        case afterhours::keys::ENTER:
            return "Enter";
        case afterhours::keys::TAB:
            return "Tab";
        case afterhours::keys::BACKSPACE:
            return "Backspace";
        case afterhours::keys::INSERT:
            return "Insert";
        case afterhours::keys::DELETE_KEY:
            return "Delete";
        case afterhours::keys::HOME:
            return "Home";
        case afterhours::keys::END:
            return "End";
        case afterhours::keys::PAGE_UP:
            return "Page Up";
        case afterhours::keys::PAGE_DOWN:
            return "Page Down";

        // Arrow keys
        case afterhours::keys::UP:
            return "Up";
        case afterhours::keys::DOWN:
            return "Down";
        case afterhours::keys::LEFT:
            return "Left";
        case afterhours::keys::RIGHT:
            return "Right";

        // Symbols
        case afterhours::keys::MINUS:
            return "-";
        case afterhours::keys::EQUAL:
            return "=";
        case afterhours::keys::COMMA:
            return ",";
        case afterhours::keys::PERIOD:
            return ".";
        case afterhours::keys::SLASH:
            return "/";
        case afterhours::keys::SEMICOLON:
            return ";";
        case afterhours::keys::APOSTROPHE:
            return "'";
        case afterhours::keys::LEFT_BRACKET:
            return "[";
        case afterhours::keys::RIGHT_BRACKET:
            return "]";
        case afterhours::keys::BACKSLASH:
            return "\\";
        case afterhours::keys::GRAVE:
            return "`";

        // Keypad
        case afterhours::keys::KP_ADD:
            return "Num+";
        case afterhours::keys::KP_SUBTRACT:
            return "Num-";
        case afterhours::keys::KP_MULTIPLY:
            return "Num*";
        case afterhours::keys::KP_DIVIDE:
            return "Num/";
        case afterhours::keys::KP_ENTER:
            return "NumEnter";

        default:
            return "???";
    }
}

std::string formatBinding(const KeyBinding& binding) {
    std::string result;
    if (binding.ctrl) result += "Ctrl+";
    if (binding.alt) result += "Alt+";
    if (binding.shift) result += "Shift+";
    result += keyName(binding.key);
    return result;
}

std::vector<BindingInfo> getBindingsList(const ActionMap& /*map*/) {
    // For now, return the default bindings since ActionMap doesn't expose its
    // internal map In a full implementation, we'd iterate over map.bindings_
    std::vector<BindingInfo> result;

    // Use Windows preset as reference (most common)
    ActionMap defaultMap = createActionMapWithPreset(Preset::SystemDefault);

    // Hard-coded for now - ideally ActionMap would have a getBindings() method
    auto addBinding = [&](Action action, KeyBinding binding) {
        result.push_back(
            {action, actionDisplayName(action), formatBinding(binding)});
    };

    // Navigation
    addBinding(Action::MoveLeft, {afterhours::keys::LEFT, false, false, false});
    addBinding(Action::MoveRight, {afterhours::keys::RIGHT, false, false, false});
    addBinding(Action::MoveUp, {afterhours::keys::UP, false, false, false});
    addBinding(Action::MoveDown, {afterhours::keys::DOWN, false, false, false});
    addBinding(Action::MoveWordLeft, {afterhours::keys::LEFT, true, false, false});
    addBinding(Action::MoveWordRight, {afterhours::keys::RIGHT, true, false, false});
    addBinding(Action::MoveLineStart, {afterhours::keys::HOME, false, false, false});
    addBinding(Action::MoveLineEnd, {afterhours::keys::END, false, false, false});
    addBinding(Action::MoveDocumentStart,
               {afterhours::keys::HOME, true, false, false});
    addBinding(Action::MoveDocumentEnd, {afterhours::keys::END, true, false, false});
    addBinding(Action::PageUp, {afterhours::keys::PAGE_UP, false, false, false});
    addBinding(Action::PageDown, {afterhours::keys::PAGE_DOWN, false, false, false});

    // Editing
    addBinding(Action::InsertNewline, {afterhours::keys::ENTER, false, false, false});
    addBinding(Action::Backspace, {afterhours::keys::BACKSPACE, false, false, false});
    addBinding(Action::Delete, {afterhours::keys::DELETE_KEY, false, false, false});

    // Clipboard
    addBinding(Action::SelectAll, {afterhours::keys::A, true, false, false});
    addBinding(Action::Copy, {afterhours::keys::C, true, false, false});
    addBinding(Action::Cut, {afterhours::keys::X, true, false, false});
    addBinding(Action::Paste, {afterhours::keys::V, true, false, false});

    // Undo/Redo
    addBinding(Action::Undo, {afterhours::keys::Z, true, false, false});
    addBinding(Action::Redo, {afterhours::keys::Y, true, false, false});

    // File
    addBinding(Action::New, {afterhours::keys::N, true, false, false});
    addBinding(Action::Open, {afterhours::keys::O, true, false, false});
    addBinding(Action::Save, {afterhours::keys::S, true, false, false});

    // Formatting
    addBinding(Action::ToggleBold, {afterhours::keys::B, true, false, false});
    addBinding(Action::ToggleItalic, {afterhours::keys::I, true, false, false});
    addBinding(Action::ToggleUnderline, {afterhours::keys::U, true, false, false});
    addBinding(Action::ToggleStrikethrough, {afterhours::keys::S, true, true, false});
    addBinding(Action::ToggleSuperscript, {afterhours::keys::EQUAL, true, true, false});
    addBinding(Action::ToggleSubscript, {afterhours::keys::MINUS, true, true, false});
    addBinding(Action::IncreaseFontSize,
               {afterhours::keys::EQUAL, true, false, false});
    addBinding(Action::DecreaseFontSize,
               {afterhours::keys::MINUS, true, false, false});
    addBinding(Action::ResetFontSize, {afterhours::keys::ZERO, true, false, false});
    
    // Paragraph styles
    addBinding(Action::StyleNormal, {afterhours::keys::ZERO, true, false, true});
    addBinding(Action::StyleHeading1, {afterhours::keys::ONE, true, false, true});
    addBinding(Action::StyleHeading2, {afterhours::keys::TWO, true, false, true});
    addBinding(Action::StyleHeading3, {afterhours::keys::THREE, true, false, true});
    addBinding(Action::StyleHeading4, {afterhours::keys::FOUR, true, false, true});
    addBinding(Action::StyleHeading5, {afterhours::keys::FIVE, true, false, true});
    addBinding(Action::StyleHeading6, {afterhours::keys::SIX, true, false, true});
    
    // Alignment
    addBinding(Action::AlignLeft, {afterhours::keys::L, true, false, false});
    addBinding(Action::AlignCenter, {afterhours::keys::E, true, false, false});
    addBinding(Action::AlignRight, {afterhours::keys::R, true, false, false});
    addBinding(Action::AlignJustify, {afterhours::keys::J, true, false, false});
    
    // Indentation
    addBinding(Action::IndentIncrease, {afterhours::keys::RIGHT_BRACKET, true, false, false});
    addBinding(Action::IndentDecrease, {afterhours::keys::LEFT_BRACKET, true, false, false});
    
    // Line spacing: Ctrl+Shift+1/5/2 (Word-like shortcuts with Shift modifier)
    addBinding(Action::LineSpacingSingle, {afterhours::keys::ONE, true, true, false});
    addBinding(Action::LineSpacing1_5, {afterhours::keys::FIVE, true, true, false});
    addBinding(Action::LineSpacingDouble, {afterhours::keys::TWO, true, true, false});

    // View controls
    addBinding(Action::ZoomIn, {afterhours::keys::EQUAL, true, false, true});
    addBinding(Action::ZoomOut, {afterhours::keys::MINUS, true, false, true});
    addBinding(Action::ZoomReset, {afterhours::keys::ZERO, true, false, true});
    addBinding(Action::ToggleFocusMode, {afterhours::keys::F11, false, false, false});
    addBinding(Action::ToggleSplitView, {afterhours::keys::V, true, false, true});
    addBinding(Action::ToggleDarkMode, {afterhours::keys::D, true, false, true});
    
    // Lists: Ctrl+Shift+8 for bullets, Ctrl+Shift+7 for numbers (like some word processors)
    addBinding(Action::ToggleBulletedList, {afterhours::keys::EIGHT, true, true, false});
    addBinding(Action::ToggleNumberedList, {afterhours::keys::SEVEN, true, true, false});
    
    // Find and Replace
    addBinding(Action::Find, {afterhours::keys::F, true, false, false});
    addBinding(Action::FindNext, {afterhours::keys::G, true, false, false});
    addBinding(Action::FindPrevious, {afterhours::keys::G, true, true, false});
    addBinding(Action::Replace, {afterhours::keys::H, true, false, false});
    
    // Paragraph spacing
    addBinding(Action::IncreaseSpaceBefore, {afterhours::keys::UP, true, false, true});
    addBinding(Action::DecreaseSpaceBefore, {afterhours::keys::DOWN, true, false, true});
    addBinding(Action::IncreaseSpaceAfter, {afterhours::keys::UP, true, true, true});
    addBinding(Action::DecreaseSpaceAfter, {afterhours::keys::DOWN, true, true, true});
    
    // Page breaks
    addBinding(Action::InsertPageBreak, {afterhours::keys::ENTER, true, false, false});  // Ctrl+Enter
    
    // Hyperlinks
    addBinding(Action::InsertHyperlink, {afterhours::keys::K, true, false, false});  // Ctrl+K

    return result;
}

}  // namespace input
