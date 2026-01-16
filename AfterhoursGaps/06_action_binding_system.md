# Action Binding System

## Problem
Afterhours provides low-level input detection (`is_key_pressed`, `is_key_down`, etc.) but no 
high-level action binding system for remappable keyboard shortcuts with modifier keys.

## Current Workaround
Custom `src/input/action_map.h` implements:
- `Action` enum with all editor actions (Copy, Paste, Save, ToggleBold, etc.)
- `KeyBinding` struct with key code + Ctrl/Shift/Alt modifiers
- `ActionMap` class that maps bindings to actions
- Preset support (Windows vs macOS style shortcuts)
- Human-readable formatting for keybindings

## What Afterhours Currently Provides
In `input_system.h`:
```cpp
static bool is_key_pressed(const KeyCode keycode);
static bool is_key_down(const KeyCode keycode);
static bool is_mouse_button_pressed(const MouseButton button);
// etc.
```

No support for:
- Named actions mapped to key combinations
- Modifier key combinations (Ctrl+S, Cmd+C, etc.)
- Rebindable shortcuts at runtime
- Serialization of keybindings
- Human-readable key/binding display

## Required Feature: Action Binding API

### Proposed API Addition to `input_system.h`

```cpp
namespace afterhours {
namespace input {

// Modifier flags
enum class Modifiers : uint8_t {
  None  = 0,
  Ctrl  = 1 << 0,
  Shift = 1 << 1,
  Alt   = 1 << 2,
  // Meta/Cmd could be added for macOS
};

// A key binding with modifiers
struct KeyBinding {
  KeyCode key = 0;
  Modifiers modifiers = Modifiers::None;

  bool operator==(const KeyBinding& other) const;
};

// Hash function for use in unordered_map
struct KeyBindingHash {
  std::size_t operator()(const KeyBinding& k) const;
};

// Action map that binds keys to named actions
template <typename ActionEnum>
class ActionMap {
public:
  // Bind a key combination to an action
  void bind(const KeyBinding& binding, ActionEnum action);
  
  // Unbind a key
  void unbind(const KeyBinding& binding);
  
  // Check if an action was triggered this frame
  bool is_action_pressed(ActionEnum action) const;
  
  // Check if an action is currently held
  bool is_action_down(ActionEnum action) const;
  
  // Get current binding for an action (for display in settings)
  std::optional<KeyBinding> get_binding(ActionEnum action) const;
  
  // Get all bindings (for settings UI)
  std::vector<std::pair<ActionEnum, KeyBinding>> get_all_bindings() const;
  
  // Clear all bindings
  void clear();

private:
  std::unordered_map<KeyBinding, ActionEnum, KeyBindingHash> bindings_;
};

// Formatting utilities
std::string format_key(KeyCode key);
std::string format_binding(const KeyBinding& binding);
std::string format_modifiers(Modifiers mods);

// Check current modifier state
Modifiers get_current_modifiers();

}} // namespace afterhours::input
```

### Usage Example

```cpp
enum class EditorAction { Save, Copy, Paste, Undo, Redo, ToggleBold };

afterhours::input::ActionMap<EditorAction> actions;

// Setup bindings
actions.bind({KEY_S, Modifiers::Ctrl}, EditorAction::Save);
actions.bind({KEY_C, Modifiers::Ctrl}, EditorAction::Copy);
actions.bind({KEY_V, Modifiers::Ctrl}, EditorAction::Paste);
actions.bind({KEY_Z, Modifiers::Ctrl}, EditorAction::Undo);
actions.bind({KEY_Z, Modifiers::Ctrl | Modifiers::Shift}, EditorAction::Redo);
actions.bind({KEY_B, Modifiers::Ctrl}, EditorAction::ToggleBold);

// In update loop
if (actions.is_action_pressed(EditorAction::Save)) {
  save_document();
}

// Display in settings
for (auto& [action, binding] : actions.get_all_bindings()) {
  draw_text(format_binding(binding));  // "Ctrl+S"
}
```

### Optional: Preset System

```cpp
template <typename ActionEnum>
using BindingPreset = std::function<void(ActionMap<ActionEnum>&)>;

template <typename ActionEnum>
void apply_preset(ActionMap<ActionEnum>& map, BindingPreset<ActionEnum> preset);

// Or enum-based:
enum class KeyboardPreset { Windows, MacOS, Emacs, Vim };

template <typename ActionEnum>
ActionMap<ActionEnum> create_with_preset(KeyboardPreset preset,
                                         const std::map<ActionEnum, KeyBinding>& defaults);
```

## Benefits
- Decouples application logic from specific key codes
- Enables runtime rebinding for accessibility
- Simplifies settings UI for keyboard shortcuts
- Provides consistent modifier handling across platforms

