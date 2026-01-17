# Action Binding System

## Working Implementation
See these files for a complete working example:
- `src/input/action_map.h` - Action enum, KeyBinding struct, ActionMap class
- `src/input/action_map.cpp` - Implementation with Windows/macOS presets, key formatting

## Problem
Afterhours provides low-level input detection (`is_key_pressed`, `is_key_down`, etc.) but no 
high-level action binding system for remappable keyboard shortcuts with modifier keys.

The existing `InputAction` template system works well for gamepad/basic UI navigation, but 
doesn't support:
- Modifier key combinations (Ctrl+S, Cmd+C, etc.)
- Rebindable shortcuts at runtime
- Platform-specific preset bindings

## Current Workaround
Custom `src/input/action_map.h` implements:
- `Action` enum with all editor actions (Copy, Paste, Save, ToggleBold, etc.)
- `KeyBinding` struct with key code + Ctrl/Shift/Alt modifiers
- `ActionMap` class that maps bindings to actions
- Preset support (Windows vs macOS style shortcuts)
- Human-readable formatting for keybindings

## What Afterhours Currently Provides

### Low-Level Input (input_system.h)
```cpp
static bool is_key_pressed(const KeyCode keycode);
static bool is_key_down(const KeyCode keycode);
static bool is_mouse_button_pressed(const MouseButton button);
```

### Existing InputAction System
The UI layer uses a templated `InputAction` enum for gamepad/navigation:

```cpp
// User defines their action enum
enum class InputAction {
  None,           // Required: unmapped input
  WidgetNext,     // Required: tab forward
  WidgetBack,     // Required: tab backward
  WidgetPress,    // Required: activate/click
  WidgetMod,      // Required: modifier for reverse tabbing
  WidgetLeft,     // Navigation
  WidgetRight,
  // ... app-specific actions
};

// UIContext is templated on this enum
template <typename InputAction> struct UIContext : BaseComponent {
  InputAction last_action;
  InputBitset all_actions;
  
  [[nodiscard]] bool pressed(const InputAction& name);
  [[nodiscard]] bool is_held_down(const InputAction& name);
};
```

### Current Limitations
- No modifier key support (Ctrl, Shift, Alt)
- Single key per action only
- No runtime rebinding
- No serialization of bindings
- No human-readable key display

## Required Feature: Action Binding API

### Concepts for InputAction Validation

Replace the current macro-based validation with proper C++20 concepts:

```cpp
namespace afterhours::input {

// Core concept: Validates that an InputAction enum has all required values
template <typename T>
concept ValidInputAction = std::is_enum_v<T> && requires {
  // Required for UI context to function
  { T::None } -> std::convertible_to<T>;
  { T::WidgetNext } -> std::convertible_to<T>;
  { T::WidgetBack } -> std::convertible_to<T>;
  { T::WidgetPress } -> std::convertible_to<T>;
  { T::WidgetMod } -> std::convertible_to<T>;
};

// Extended concept for text editing support
template <typename T>
concept TextEditingInputAction = ValidInputAction<T> && requires {
  { T::WidgetLeft } -> std::convertible_to<T>;
  { T::WidgetRight } -> std::convertible_to<T>;
  { T::TextBackspace } -> std::convertible_to<T>;
  { T::TextDelete } -> std::convertible_to<T>;
  { T::TextHome } -> std::convertible_to<T>;
  { T::TextEnd } -> std::convertible_to<T>;
};

// Concept for action enums that support modifier keys
template <typename T>
concept ModifierAwareAction = std::is_enum_v<T> && requires {
  // magic_enum compatible (for iteration/names)
  { magic_enum::enum_count<T>() } -> std::convertible_to<std::size_t>;
};

} // namespace afterhours::input
```

### Using Concepts in Templates

```cpp
// UIContext now validates at compile time
template <ValidInputAction InputAction>
struct UIContext : BaseComponent {
  // ...
};

// Text input requires additional actions
template <TextEditingInputAction InputAction>
void process_text_input(UIContext<InputAction>& ctx, TextInputState& state) {
  if (ctx.pressed(InputAction::TextBackspace)) { /* ... */ }
  if (ctx.pressed(InputAction::TextHome)) { /* ... */ }
}

// ActionMap works with any enum
template <ModifierAwareAction ActionEnum>
class ActionMap {
  // ...
};
```

### Proposed KeyBinding API Addition

```cpp
namespace afterhours::input {

// Modifier flags (bitwise combinable)
enum class Modifiers : uint8_t {
  None  = 0,
  Ctrl  = 1 << 0,
  Shift = 1 << 1,
  Alt   = 1 << 2,
  Meta  = 1 << 3,  // Cmd on macOS, Win on Windows
};

constexpr Modifiers operator|(Modifiers a, Modifiers b) {
  return static_cast<Modifiers>(
    static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr bool operator&(Modifiers a, Modifiers b) {
  return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

// A key binding with modifiers
struct KeyBinding {
  KeyCode key = 0;
  Modifiers modifiers = Modifiers::None;

  constexpr bool operator==(const KeyBinding& other) const = default;
  
  // Check if this binding matches current input state
  [[nodiscard]] bool is_pressed() const;
  [[nodiscard]] bool is_down() const;
};

// Hash function for use in unordered_map
struct KeyBindingHash {
  std::size_t operator()(const KeyBinding& k) const {
    return std::hash<KeyCode>{}(k.key) ^ 
           (std::hash<uint8_t>{}(static_cast<uint8_t>(k.modifiers)) << 8);
  }
};

// Action map that binds keys to named actions
template <ModifierAwareAction ActionEnum>
class ActionMap {
public:
  // Bind a key combination to an action
  void bind(KeyBinding binding, ActionEnum action);
  
  // Unbind a key
  void unbind(KeyBinding binding);
  
  // Check if an action was triggered this frame
  [[nodiscard]] bool is_action_pressed(ActionEnum action) const;
  
  // Check if an action is currently held
  [[nodiscard]] bool is_action_down(ActionEnum action) const;
  
  // Get current binding for an action (for display in settings)
  [[nodiscard]] std::optional<KeyBinding> get_binding(ActionEnum action) const;
  
  // Get all bindings (for settings UI)
  [[nodiscard]] auto get_all_bindings() const 
    -> std::vector<std::pair<ActionEnum, KeyBinding>>;
  
  // Clear all bindings
  void clear();

private:
  std::unordered_map<KeyBinding, ActionEnum, KeyBindingHash> bindings_;
  std::unordered_map<ActionEnum, KeyBinding> reverse_lookup_;
};

// Formatting utilities
[[nodiscard]] std::string format_key(KeyCode key);
[[nodiscard]] std::string format_binding(const KeyBinding& binding);
[[nodiscard]] std::string format_modifiers(Modifiers mods);

// Check current modifier state
[[nodiscard]] Modifiers get_current_modifiers();

} // namespace afterhours::input
```

## Integration with Existing InputAction System

The key insight is that `ActionMap` and `InputAction` serve different purposes:

| Feature | InputAction (existing) | ActionMap (proposed) |
|---------|----------------------|---------------------|
| Purpose | Gamepad/UI navigation | Keyboard shortcuts |
| Modifiers | No | Yes (Ctrl, Shift, Alt, Meta) |
| Rebindable | Via component mapping | Via ActionMap |
| Multiple bindings | No | Yes (multiple keys → one action) |
| Integration | UIContext, InputCollector | Standalone or integrated |

### Approach 1: Parallel Systems (Recommended)

Keep both systems, let them coexist:

```cpp
// User's action enum still needs InputAction values for UI
enum class AppAction {
  // Required for UI (ValidInputAction concept)
  None, WidgetNext, WidgetBack, WidgetPress, WidgetMod,
  WidgetLeft, WidgetRight,
  
  // Required for text editing (TextEditingInputAction concept)
  TextBackspace, TextDelete, TextHome, TextEnd,
  
  // App-specific keyboard shortcuts (any values)
  Save, Open, Copy, Paste, Undo, Redo,
  ToggleBold, ToggleItalic,
};

// UIContext handles gamepad/navigation via InputCollector
ui::UIContext<AppAction> ui_context;

// ActionMap handles keyboard shortcuts with modifiers
input::ActionMap<AppAction> shortcuts;
shortcuts.bind({KEY_S, Modifiers::Ctrl}, AppAction::Save);
shortcuts.bind({KEY_B, Modifiers::Ctrl}, AppAction::ToggleBold);

// In update:
if (shortcuts.is_action_pressed(AppAction::Save)) {
  save_document();
}
```

### Approach 2: Unified ActionMap with InputCollector Integration

Extend `ProvidesInputMapping` to support modifier-aware bindings:

```cpp
// Enhanced input mapping that supports modifiers
struct ProvidesModifierInputMapping : BaseComponent {
  using ModifierMapping = std::unordered_map<KeyBinding, int, KeyBindingHash>;
  ModifierMapping modifier_mapping;
  
  // Legacy support
  input::ProvidesInputMapping::GameMapping legacy_mapping;
};

// InputSystem checks modifier bindings first, then legacy bindings
struct EnhancedInputSystem : System<InputCollector, ProvidesModifierInputMapping> {
  void for_each_with(Entity& entity, InputCollector& collector,
                     ProvidesModifierInputMapping& mapping, float dt) override {
    // Check modifier-aware bindings first
    Modifiers current_mods = get_current_modifiers();
    for (const auto& [binding, action] : mapping.modifier_mapping) {
      if (binding.modifiers == current_mods && is_key_pressed(binding.key)) {
        collector.inputs_pressed.push_back(
          ActionDone(DeviceMedium::Keyboard, 0, action, 1.0f, dt));
      }
    }
    // ... then check legacy bindings
  }
};
```

### Approach 3: Bridge Component

Create a component that bridges ActionMap to InputCollector:

```cpp
template <ModifierAwareAction ActionEnum>
struct ActionMapBridge : BaseComponent {
  ActionMap<ActionEnum>* action_map = nullptr;
};

template <ModifierAwareAction ActionEnum>
struct SyncActionMapToCollector : System<ActionMapBridge<ActionEnum>, InputCollector> {
  void for_each_with(Entity&, ActionMapBridge<ActionEnum>& bridge, 
                     InputCollector& collector, float dt) override {
    if (!bridge.action_map) return;
    
    for (auto& [action, binding] : bridge.action_map->get_all_bindings()) {
      if (binding.is_pressed()) {
        collector.inputs_pressed.push_back(
          ActionDone(DeviceMedium::Keyboard, 0, 
                     static_cast<int>(action), 1.0f, dt));
      }
    }
  }
};
```

## Usage Example

```cpp
// Define action enum satisfying both concepts
enum class EditorAction { 
  // UI navigation (ValidInputAction)
  None, WidgetNext, WidgetBack, WidgetPress, WidgetMod,
  WidgetLeft, WidgetRight,
  // Text editing (TextEditingInputAction)
  TextBackspace, TextDelete, TextHome, TextEnd,
  // Editor shortcuts
  Save, Copy, Paste, Undo, Redo, ToggleBold 
};

// Verify at compile time
static_assert(afterhours::input::ValidInputAction<EditorAction>);
static_assert(afterhours::input::TextEditingInputAction<EditorAction>);

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

## Preset System

```cpp
// Type-erased preset for flexibility
template <ModifierAwareAction ActionEnum>
using BindingPreset = std::function<void(ActionMap<ActionEnum>&)>;

// Platform detection
enum class KeyboardPreset { Windows, MacOS, Emacs, Vim };

[[nodiscard]] constexpr KeyboardPreset detect_platform_preset() {
#if defined(__APPLE__)
  return KeyboardPreset::MacOS;
#else
  return KeyboardPreset::Windows;
#endif
}

// Preset application with concept constraint
template <ModifierAwareAction ActionEnum>
void apply_preset(ActionMap<ActionEnum>& map, BindingPreset<ActionEnum> preset) {
  map.clear();
  preset(map);
}

// Factory with defaults
template <ModifierAwareAction ActionEnum>
[[nodiscard]] ActionMap<ActionEnum> create_with_preset(
    KeyboardPreset preset,
    const std::map<ActionEnum, KeyBinding>& defaults) {
  ActionMap<ActionEnum> map;
  // Apply platform-specific modifier (Ctrl vs Cmd)
  Modifiers primary = (preset == KeyboardPreset::MacOS) 
                      ? Modifiers::Meta : Modifiers::Ctrl;
  for (auto& [action, base_binding] : defaults) {
    KeyBinding adjusted = base_binding;
    if (adjusted.modifiers & Modifiers::Ctrl) {
      adjusted.modifiers = primary | (adjusted.modifiers & ~Modifiers::Ctrl);
    }
    map.bind(adjusted, action);
  }
  return map;
}
```

## Integration with 08_scrollable_containers.md

The action binding system could support scroll-related actions:

```cpp
// Optional concept for scroll-enabled input actions
template <typename T>
concept ScrollableInputAction = ValidInputAction<T> && requires {
  { T::ScrollUp } -> std::convertible_to<T>;
  { T::ScrollDown } -> std::convertible_to<T>;
  { T::ScrollPageUp } -> std::convertible_to<T>;
  { T::ScrollPageDown } -> std::convertible_to<T>;
  { T::ScrollToTop } -> std::convertible_to<T>;
  { T::ScrollToBottom } -> std::convertible_to<T>;
};
```

**Default scroll bindings:**

| Action | Default Binding |
|--------|-----------------|
| ScrollUp | Up Arrow (when focused) |
| ScrollDown | Down Arrow |
| ScrollPageUp | Page Up |
| ScrollPageDown | Page Down |
| ScrollToTop | Ctrl+Home |
| ScrollToBottom | Ctrl+End |

This allows users to rebind scroll navigation keys in accessibility settings.

## Benefits
- Decouples application logic from specific key codes
- Enables runtime rebinding for accessibility
- Simplifies settings UI for keyboard shortcuts
- Provides consistent modifier handling across platforms
- Compile-time validation via concepts prevents missing required actions
- Clean integration with existing InputAction system

