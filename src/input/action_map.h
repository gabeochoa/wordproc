#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "../rl.h"

namespace input {

// Enum of all editor actions that can be remapped
enum class Action {
    // Navigation
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    MoveWordLeft,
    MoveWordRight,
    MoveLineStart,
    MoveLineEnd,
    MoveDocumentStart,
    MoveDocumentEnd,
    PageUp,
    PageDown,

    // Editing
    InsertNewline,
    Backspace,
    Delete,

    // Selection
    SelectAll,

    // Clipboard
    Copy,
    Cut,
    Paste,

    // Undo/Redo
    Undo,
    Redo,

    // File operations
    New,
    Open,
    Save,
    SaveAs,

    // Formatting
    ToggleBold,
    ToggleItalic,
    ToggleUnderline,
    ToggleStrikethrough,
    FontGaegu,
    FontGaramond,
    IncreaseFontSize,
    DecreaseFontSize,
    ResetFontSize,
    
    // Paragraph styles
    StyleNormal,
    StyleTitle,
    StyleSubtitle,
    StyleHeading1,
    StyleHeading2,
    StyleHeading3,
    StyleHeading4,
    StyleHeading5,
    StyleHeading6,
    
    // Text alignment
    AlignLeft,
    AlignCenter,
    AlignRight,
    AlignJustify,
    
    // Indentation
    IndentIncrease,
    IndentDecrease,
    
    // Line spacing
    LineSpacingSingle,
    LineSpacing1_5,
    LineSpacingDouble,
    
    // Paragraph spacing
    IncreaseSpaceBefore,
    DecreaseSpaceBefore,
    IncreaseSpaceAfter,
    DecreaseSpaceAfter,
    
    // Lists
    ToggleBulletedList,
    ToggleNumberedList,
    
    // Find and Replace
    Find,
    FindNext,
    FindPrevious,
    Replace,
    
    // Table operations
    InsertTable,
    TableInsertRowAbove,
    TableInsertRowBelow,
    TableInsertColumnLeft,
    TableInsertColumnRight,
    TableDeleteRow,
    TableDeleteColumn,
    TableMergeCells,
    TableSplitCell,
    TableMoveNextCell,      // Tab in table
    TableMovePrevCell,      // Shift+Tab in table
    
    // Page breaks
    InsertPageBreak,        // Ctrl+Enter - insert page break before current line
    TogglePageBreak,        // Toggle page break before current line

    // Count (for iteration)
    COUNT
};

// Key binding: key code + modifiers
struct KeyBinding {
    int key;
    bool ctrl;
    bool shift;
    bool alt;

    bool operator==(const KeyBinding& other) const {
        return key == other.key && ctrl == other.ctrl && shift == other.shift &&
               alt == other.alt;
    }
};

// Hash function for KeyBinding
struct KeyBindingHash {
    std::size_t operator()(const KeyBinding& k) const {
        return std::hash<int>()(k.key) ^ (std::hash<bool>()(k.ctrl) << 1) ^
               (std::hash<bool>()(k.shift) << 2) ^
               (std::hash<bool>()(k.alt) << 3);
    }
};

class ActionMap {
   public:
    ActionMap();

    // Bind a key to an action
    void bind(const KeyBinding& binding, Action action);

    // Unbind a key
    void unbind(const KeyBinding& binding);

    // Check if an action was just triggered this frame
    bool isActionPressed(Action action) const;

    // Get the action for the current key press, if any
    Action getActionForCurrentInput() const;

    // Get action name for debugging
    static const char* actionName(Action action);

   private:
    std::unordered_map<KeyBinding, Action, KeyBindingHash> bindings_;

    // Check if a specific key binding is pressed this frame
    bool isBindingPressed(const KeyBinding& binding) const;
};

// Keyboard shortcut presets
enum class Preset {
    SystemDefault,  // Auto-detect based on platform
    WindowsCtrl,    // Windows-style: Ctrl+key for commands
    MacOSCmd        // macOS-style: Uses Ctrl as "Cmd" equivalent (since raylib
                    // doesn't expose Cmd)
};

// Get preset name for display
const char* presetName(Preset preset);

// Create action map with specified preset
ActionMap createActionMapWithPreset(Preset preset);

// Default action map with standard bindings (system default preset)
ActionMap createDefaultActionMap();

// Get human-readable display name for an action
const char* actionDisplayName(Action action);

// Get human-readable key name
std::string keyName(int keyCode);

// Format a key binding as a human-readable string (e.g., "Ctrl+S")
std::string formatBinding(const KeyBinding& binding);

// Get all bindings as displayable list (action name -> binding string)
struct BindingInfo {
    Action action;
    std::string actionName;
    std::string bindingStr;
};
std::vector<BindingInfo> getBindingsList(const ActionMap& map);

}  // namespace input
