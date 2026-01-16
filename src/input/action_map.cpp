#include "action_map.h"

namespace input {

ActionMap::ActionMap() {}

void ActionMap::bind(const KeyBinding& binding, Action action) {
    bindings_[binding] = action;
}

void ActionMap::unbind(const KeyBinding& binding) { bindings_.erase(binding); }

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
    map.bind({raylib::KEY_LEFT, false, false, false}, Action::MoveLeft);
    map.bind({raylib::KEY_RIGHT, false, false, false}, Action::MoveRight);
    map.bind({raylib::KEY_UP, false, false, false}, Action::MoveUp);
    map.bind({raylib::KEY_DOWN, false, false, false}, Action::MoveDown);
    map.bind({raylib::KEY_PAGE_UP, false, false, false}, Action::PageUp);
    map.bind({raylib::KEY_PAGE_DOWN, false, false, false}, Action::PageDown);

    // Editing (same across presets)
    map.bind({raylib::KEY_ENTER, false, false, false}, Action::InsertNewline);
    map.bind({raylib::KEY_KP_ENTER, false, false, false},
             Action::InsertNewline);
    map.bind({raylib::KEY_BACKSPACE, false, false, false}, Action::Backspace);
    map.bind({raylib::KEY_DELETE, false, false, false}, Action::Delete);
}

// Windows-style bindings: Ctrl+key
static void bindWindowsPreset(ActionMap& map) {
    bindNavigationKeys(map);

    // Word/line navigation: Ctrl+Arrow, Home/End
    map.bind({raylib::KEY_LEFT, true, false, false}, Action::MoveWordLeft);
    map.bind({raylib::KEY_RIGHT, true, false, false}, Action::MoveWordRight);
    map.bind({raylib::KEY_HOME, false, false, false}, Action::MoveLineStart);
    map.bind({raylib::KEY_END, false, false, false}, Action::MoveLineEnd);
    map.bind({raylib::KEY_HOME, true, false, false}, Action::MoveDocumentStart);
    map.bind({raylib::KEY_END, true, false, false}, Action::MoveDocumentEnd);

    // Selection, clipboard, undo/redo: Ctrl+key
    map.bind({raylib::KEY_A, true, false, false}, Action::SelectAll);
    map.bind({raylib::KEY_C, true, false, false}, Action::Copy);
    map.bind({raylib::KEY_X, true, false, false}, Action::Cut);
    map.bind({raylib::KEY_V, true, false, false}, Action::Paste);
    map.bind({raylib::KEY_Z, true, false, false}, Action::Undo);
    map.bind({raylib::KEY_Y, true, false, false}, Action::Redo);

    // File operations: Ctrl+key
    map.bind({raylib::KEY_N, true, false, false}, Action::New);
    map.bind({raylib::KEY_O, true, false, false}, Action::Open);
    map.bind({raylib::KEY_S, true, false, false}, Action::Save);

    // Formatting: Ctrl+key
    map.bind({raylib::KEY_B, true, false, false}, Action::ToggleBold);
    map.bind({raylib::KEY_I, true, false, false}, Action::ToggleItalic);
    map.bind({raylib::KEY_U, true, false, false}, Action::ToggleUnderline);
    map.bind({raylib::KEY_S, true, true, false}, Action::ToggleStrikethrough);  // Ctrl+Shift+S
    map.bind({raylib::KEY_ONE, true, false, false}, Action::FontGaegu);
    map.bind({raylib::KEY_TWO, true, false, false}, Action::FontGaramond);
    map.bind({raylib::KEY_EQUAL, true, false, false}, Action::IncreaseFontSize);
    map.bind({raylib::KEY_KP_ADD, true, false, false},
             Action::IncreaseFontSize);
    map.bind({raylib::KEY_MINUS, true, false, false}, Action::DecreaseFontSize);
    map.bind({raylib::KEY_KP_SUBTRACT, true, false, false},
             Action::DecreaseFontSize);
    map.bind({raylib::KEY_ZERO, true, false, false}, Action::ResetFontSize);
    
    // Paragraph styles: Ctrl+Alt+number for headings, Ctrl+Alt+0 for normal
    map.bind({raylib::KEY_ZERO, true, false, true}, Action::StyleNormal);
    map.bind({raylib::KEY_ONE, true, false, true}, Action::StyleHeading1);
    map.bind({raylib::KEY_TWO, true, false, true}, Action::StyleHeading2);
    map.bind({raylib::KEY_THREE, true, false, true}, Action::StyleHeading3);
    map.bind({raylib::KEY_FOUR, true, false, true}, Action::StyleHeading4);
    map.bind({raylib::KEY_FIVE, true, false, true}, Action::StyleHeading5);
    map.bind({raylib::KEY_SIX, true, false, true}, Action::StyleHeading6);
    
    // Text alignment: Ctrl+L/E/R/J (standard Word shortcuts)
    map.bind({raylib::KEY_L, true, false, false}, Action::AlignLeft);
    map.bind({raylib::KEY_E, true, false, false}, Action::AlignCenter);
    map.bind({raylib::KEY_R, true, false, false}, Action::AlignRight);
    map.bind({raylib::KEY_J, true, false, false}, Action::AlignJustify);
    
    // Indentation: Ctrl+] to increase, Ctrl+[ to decrease (standard Word shortcuts)
    map.bind({raylib::KEY_RIGHT_BRACKET, true, false, false}, Action::IndentIncrease);
    map.bind({raylib::KEY_LEFT_BRACKET, true, false, false}, Action::IndentDecrease);
}

// macOS-style bindings: uses Ctrl as Cmd equivalent (raylib doesn't expose Cmd)
// Key differences from Windows:
// - Option+Arrow for word navigation (Alt in raylib)
// - Cmd+Arrow for line/document navigation (Ctrl in raylib, mapped as Cmd
// substitute)
// - Cmd+Shift+Z for Redo instead of Cmd+Y
static void bindMacOSPreset(ActionMap& map) {
    bindNavigationKeys(map);

    // Word navigation: Option+Arrow (Alt in raylib)
    map.bind({raylib::KEY_LEFT, false, false, true}, Action::MoveWordLeft);
    map.bind({raylib::KEY_RIGHT, false, false, true}, Action::MoveWordRight);

    // Line navigation: Cmd+Arrow (Ctrl in raylib as Cmd substitute)
    map.bind({raylib::KEY_LEFT, true, false, false}, Action::MoveLineStart);
    map.bind({raylib::KEY_RIGHT, true, false, false}, Action::MoveLineEnd);
    map.bind({raylib::KEY_UP, true, false, false}, Action::MoveDocumentStart);
    map.bind({raylib::KEY_DOWN, true, false, false}, Action::MoveDocumentEnd);

    // Home/End also work for line start/end
    map.bind({raylib::KEY_HOME, false, false, false}, Action::MoveLineStart);
    map.bind({raylib::KEY_END, false, false, false}, Action::MoveLineEnd);

    // Selection, clipboard: Cmd+key (Ctrl as substitute)
    map.bind({raylib::KEY_A, true, false, false}, Action::SelectAll);
    map.bind({raylib::KEY_C, true, false, false}, Action::Copy);
    map.bind({raylib::KEY_X, true, false, false}, Action::Cut);
    map.bind({raylib::KEY_V, true, false, false}, Action::Paste);

    // Undo: Cmd+Z, Redo: Cmd+Shift+Z (macOS style)
    map.bind({raylib::KEY_Z, true, false, false}, Action::Undo);
    map.bind({raylib::KEY_Z, true, true, false}, Action::Redo);  // Cmd+Shift+Z

    // File operations: Cmd+key (Ctrl as substitute)
    map.bind({raylib::KEY_N, true, false, false}, Action::New);
    map.bind({raylib::KEY_O, true, false, false}, Action::Open);
    map.bind({raylib::KEY_S, true, false, false}, Action::Save);

    // Formatting: Cmd+key
    map.bind({raylib::KEY_B, true, false, false}, Action::ToggleBold);
    map.bind({raylib::KEY_I, true, false, false}, Action::ToggleItalic);
    map.bind({raylib::KEY_U, true, false, false}, Action::ToggleUnderline);
    map.bind({raylib::KEY_S, true, true, false}, Action::ToggleStrikethrough);  // Cmd+Shift+S
    map.bind({raylib::KEY_ONE, true, false, false}, Action::FontGaegu);
    map.bind({raylib::KEY_TWO, true, false, false}, Action::FontGaramond);
    map.bind({raylib::KEY_EQUAL, true, false, false}, Action::IncreaseFontSize);
    map.bind({raylib::KEY_KP_ADD, true, false, false},
             Action::IncreaseFontSize);
    map.bind({raylib::KEY_MINUS, true, false, false}, Action::DecreaseFontSize);
    map.bind({raylib::KEY_KP_SUBTRACT, true, false, false},
             Action::DecreaseFontSize);
    map.bind({raylib::KEY_ZERO, true, false, false}, Action::ResetFontSize);
    
    // Paragraph styles: Ctrl+Alt+number for headings (same as Windows)
    map.bind({raylib::KEY_ZERO, true, false, true}, Action::StyleNormal);
    map.bind({raylib::KEY_ONE, true, false, true}, Action::StyleHeading1);
    map.bind({raylib::KEY_TWO, true, false, true}, Action::StyleHeading2);
    map.bind({raylib::KEY_THREE, true, false, true}, Action::StyleHeading3);
    map.bind({raylib::KEY_FOUR, true, false, true}, Action::StyleHeading4);
    map.bind({raylib::KEY_FIVE, true, false, true}, Action::StyleHeading5);
    map.bind({raylib::KEY_SIX, true, false, true}, Action::StyleHeading6);
    
    // Text alignment: Cmd+L/E/R/J (same shortcuts as Windows)
    map.bind({raylib::KEY_L, true, false, false}, Action::AlignLeft);
    map.bind({raylib::KEY_E, true, false, false}, Action::AlignCenter);
    map.bind({raylib::KEY_R, true, false, false}, Action::AlignRight);
    map.bind({raylib::KEY_J, true, false, false}, Action::AlignJustify);
    
    // Indentation: Cmd+] to increase, Cmd+[ to decrease
    map.bind({raylib::KEY_RIGHT_BRACKET, true, false, false}, Action::IndentIncrease);
    map.bind({raylib::KEY_LEFT_BRACKET, true, false, false}, Action::IndentDecrease);
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
        case Action::COUNT:
        default:
            return "";
    }
}

std::string keyName(int keyCode) {
    switch (keyCode) {
        // Letters
        case raylib::KEY_A:
            return "A";
        case raylib::KEY_B:
            return "B";
        case raylib::KEY_C:
            return "C";
        case raylib::KEY_D:
            return "D";
        case raylib::KEY_E:
            return "E";
        case raylib::KEY_F:
            return "F";
        case raylib::KEY_G:
            return "G";
        case raylib::KEY_H:
            return "H";
        case raylib::KEY_I:
            return "I";
        case raylib::KEY_J:
            return "J";
        case raylib::KEY_K:
            return "K";
        case raylib::KEY_L:
            return "L";
        case raylib::KEY_M:
            return "M";
        case raylib::KEY_N:
            return "N";
        case raylib::KEY_O:
            return "O";
        case raylib::KEY_P:
            return "P";
        case raylib::KEY_Q:
            return "Q";
        case raylib::KEY_R:
            return "R";
        case raylib::KEY_S:
            return "S";
        case raylib::KEY_T:
            return "T";
        case raylib::KEY_U:
            return "U";
        case raylib::KEY_V:
            return "V";
        case raylib::KEY_W:
            return "W";
        case raylib::KEY_X:
            return "X";
        case raylib::KEY_Y:
            return "Y";
        case raylib::KEY_Z:
            return "Z";

        // Numbers
        case raylib::KEY_ZERO:
            return "0";
        case raylib::KEY_ONE:
            return "1";
        case raylib::KEY_TWO:
            return "2";
        case raylib::KEY_THREE:
            return "3";
        case raylib::KEY_FOUR:
            return "4";
        case raylib::KEY_FIVE:
            return "5";
        case raylib::KEY_SIX:
            return "6";
        case raylib::KEY_SEVEN:
            return "7";
        case raylib::KEY_EIGHT:
            return "8";
        case raylib::KEY_NINE:
            return "9";

        // Function keys
        case raylib::KEY_F1:
            return "F1";
        case raylib::KEY_F2:
            return "F2";
        case raylib::KEY_F3:
            return "F3";
        case raylib::KEY_F4:
            return "F4";
        case raylib::KEY_F5:
            return "F5";
        case raylib::KEY_F6:
            return "F6";
        case raylib::KEY_F7:
            return "F7";
        case raylib::KEY_F8:
            return "F8";
        case raylib::KEY_F9:
            return "F9";
        case raylib::KEY_F10:
            return "F10";
        case raylib::KEY_F11:
            return "F11";
        case raylib::KEY_F12:
            return "F12";

        // Special keys
        case raylib::KEY_SPACE:
            return "Space";
        case raylib::KEY_ESCAPE:
            return "Escape";
        case raylib::KEY_ENTER:
            return "Enter";
        case raylib::KEY_TAB:
            return "Tab";
        case raylib::KEY_BACKSPACE:
            return "Backspace";
        case raylib::KEY_INSERT:
            return "Insert";
        case raylib::KEY_DELETE:
            return "Delete";
        case raylib::KEY_HOME:
            return "Home";
        case raylib::KEY_END:
            return "End";
        case raylib::KEY_PAGE_UP:
            return "Page Up";
        case raylib::KEY_PAGE_DOWN:
            return "Page Down";

        // Arrow keys
        case raylib::KEY_UP:
            return "Up";
        case raylib::KEY_DOWN:
            return "Down";
        case raylib::KEY_LEFT:
            return "Left";
        case raylib::KEY_RIGHT:
            return "Right";

        // Symbols
        case raylib::KEY_MINUS:
            return "-";
        case raylib::KEY_EQUAL:
            return "=";
        case raylib::KEY_COMMA:
            return ",";
        case raylib::KEY_PERIOD:
            return ".";
        case raylib::KEY_SLASH:
            return "/";
        case raylib::KEY_SEMICOLON:
            return ";";
        case raylib::KEY_APOSTROPHE:
            return "'";
        case raylib::KEY_LEFT_BRACKET:
            return "[";
        case raylib::KEY_RIGHT_BRACKET:
            return "]";
        case raylib::KEY_BACKSLASH:
            return "\\";
        case raylib::KEY_GRAVE:
            return "`";

        // Keypad
        case raylib::KEY_KP_ADD:
            return "Num+";
        case raylib::KEY_KP_SUBTRACT:
            return "Num-";
        case raylib::KEY_KP_MULTIPLY:
            return "Num*";
        case raylib::KEY_KP_DIVIDE:
            return "Num/";
        case raylib::KEY_KP_ENTER:
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
    addBinding(Action::MoveLeft, {raylib::KEY_LEFT, false, false, false});
    addBinding(Action::MoveRight, {raylib::KEY_RIGHT, false, false, false});
    addBinding(Action::MoveUp, {raylib::KEY_UP, false, false, false});
    addBinding(Action::MoveDown, {raylib::KEY_DOWN, false, false, false});
    addBinding(Action::MoveWordLeft, {raylib::KEY_LEFT, true, false, false});
    addBinding(Action::MoveWordRight, {raylib::KEY_RIGHT, true, false, false});
    addBinding(Action::MoveLineStart, {raylib::KEY_HOME, false, false, false});
    addBinding(Action::MoveLineEnd, {raylib::KEY_END, false, false, false});
    addBinding(Action::MoveDocumentStart,
               {raylib::KEY_HOME, true, false, false});
    addBinding(Action::MoveDocumentEnd, {raylib::KEY_END, true, false, false});
    addBinding(Action::PageUp, {raylib::KEY_PAGE_UP, false, false, false});
    addBinding(Action::PageDown, {raylib::KEY_PAGE_DOWN, false, false, false});

    // Editing
    addBinding(Action::InsertNewline, {raylib::KEY_ENTER, false, false, false});
    addBinding(Action::Backspace, {raylib::KEY_BACKSPACE, false, false, false});
    addBinding(Action::Delete, {raylib::KEY_DELETE, false, false, false});

    // Clipboard
    addBinding(Action::SelectAll, {raylib::KEY_A, true, false, false});
    addBinding(Action::Copy, {raylib::KEY_C, true, false, false});
    addBinding(Action::Cut, {raylib::KEY_X, true, false, false});
    addBinding(Action::Paste, {raylib::KEY_V, true, false, false});

    // Undo/Redo
    addBinding(Action::Undo, {raylib::KEY_Z, true, false, false});
    addBinding(Action::Redo, {raylib::KEY_Y, true, false, false});

    // File
    addBinding(Action::New, {raylib::KEY_N, true, false, false});
    addBinding(Action::Open, {raylib::KEY_O, true, false, false});
    addBinding(Action::Save, {raylib::KEY_S, true, false, false});

    // Formatting
    addBinding(Action::ToggleBold, {raylib::KEY_B, true, false, false});
    addBinding(Action::ToggleItalic, {raylib::KEY_I, true, false, false});
    addBinding(Action::ToggleUnderline, {raylib::KEY_U, true, false, false});
    addBinding(Action::ToggleStrikethrough, {raylib::KEY_S, true, true, false});
    addBinding(Action::IncreaseFontSize,
               {raylib::KEY_EQUAL, true, false, false});
    addBinding(Action::DecreaseFontSize,
               {raylib::KEY_MINUS, true, false, false});
    addBinding(Action::ResetFontSize, {raylib::KEY_ZERO, true, false, false});
    
    // Paragraph styles
    addBinding(Action::StyleNormal, {raylib::KEY_ZERO, true, false, true});
    addBinding(Action::StyleHeading1, {raylib::KEY_ONE, true, false, true});
    addBinding(Action::StyleHeading2, {raylib::KEY_TWO, true, false, true});
    addBinding(Action::StyleHeading3, {raylib::KEY_THREE, true, false, true});
    addBinding(Action::StyleHeading4, {raylib::KEY_FOUR, true, false, true});
    addBinding(Action::StyleHeading5, {raylib::KEY_FIVE, true, false, true});
    addBinding(Action::StyleHeading6, {raylib::KEY_SIX, true, false, true});
    
    // Alignment
    addBinding(Action::AlignLeft, {raylib::KEY_L, true, false, false});
    addBinding(Action::AlignCenter, {raylib::KEY_E, true, false, false});
    addBinding(Action::AlignRight, {raylib::KEY_R, true, false, false});
    addBinding(Action::AlignJustify, {raylib::KEY_J, true, false, false});
    
    // Indentation
    addBinding(Action::IndentIncrease, {raylib::KEY_RIGHT_BRACKET, true, false, false});
    addBinding(Action::IndentDecrease, {raylib::KEY_LEFT_BRACKET, true, false, false});

    return result;
}

}  // namespace input
