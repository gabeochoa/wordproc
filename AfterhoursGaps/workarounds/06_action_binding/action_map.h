// Action Binding System for Remappable Keyboard Shortcuts
// This workaround provides a way to bind key combinations to named actions.
//
// INTEGRATION POINT FOR AFTERHOURS:
// The input system should provide an ActionMap that supports:
// - Modifier key combinations (Ctrl/Shift/Alt)
// - Named actions (template or enum-based)
// - Runtime rebinding
// - Serialization for settings

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace action_binding {

// Key binding with modifier keys
struct KeyBinding {
    int key = 0;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    bool operator==(const KeyBinding& other) const {
        return key == other.key && ctrl == other.ctrl && 
               shift == other.shift && alt == other.alt;
    }
};

// Hash function for KeyBinding (for use in unordered_map)
struct KeyBindingHash {
    std::size_t operator()(const KeyBinding& k) const {
        return std::hash<int>()(k.key) ^ 
               (std::hash<bool>()(k.ctrl) << 1) ^
               (std::hash<bool>()(k.shift) << 2) ^
               (std::hash<bool>()(k.alt) << 3);
    }
};

/// Action map that binds key combinations to actions
/// ActionEnum should be your enum of all possible actions
template<typename ActionEnum>
class ActionMap {
public:
    ActionMap() = default;

    /// Bind a key combination to an action
    void bind(const KeyBinding& binding, ActionEnum action) {
        bindings_[binding] = action;
    }

    /// Unbind a key combination
    void unbind(const KeyBinding& binding) {
        bindings_.erase(binding);
    }

    /// Check if an action was triggered this frame
    /// Requires function pointers to check key/modifier state
    template<typename IsKeyPressedFn, typename IsKeyDownFn>
    bool is_action_pressed(ActionEnum action, 
                           IsKeyPressedFn is_key_pressed,
                           IsKeyDownFn is_key_down) const {
        for (const auto& [binding, act] : bindings_) {
            if (act == action && is_binding_pressed(binding, is_key_pressed, is_key_down)) {
                return true;
            }
        }
        return false;
    }

    /// Get the binding for an action (for display in settings UI)
    KeyBinding get_binding(ActionEnum action) const {
        for (const auto& [binding, act] : bindings_) {
            if (act == action) {
                return binding;
            }
        }
        return {};
    }

    /// Get all bindings
    const std::unordered_map<KeyBinding, ActionEnum, KeyBindingHash>& 
    get_all_bindings() const {
        return bindings_;
    }

    /// Clear all bindings
    void clear() {
        bindings_.clear();
    }

private:
    template<typename IsKeyPressedFn, typename IsKeyDownFn>
    bool is_binding_pressed(const KeyBinding& binding,
                            IsKeyPressedFn is_key_pressed,
                            IsKeyDownFn is_key_down) const {
        // Check modifier state matches
        bool ctrl_down = is_key_down(KEY_LEFT_CONTROL) || is_key_down(KEY_RIGHT_CONTROL);
        bool shift_down = is_key_down(KEY_LEFT_SHIFT) || is_key_down(KEY_RIGHT_SHIFT);
        bool alt_down = is_key_down(KEY_LEFT_ALT) || is_key_down(KEY_RIGHT_ALT);

        if (ctrl_down != binding.ctrl || 
            shift_down != binding.shift || 
            alt_down != binding.alt) {
            return false;
        }

        return is_key_pressed(binding.key);
    }

    std::unordered_map<KeyBinding, ActionEnum, KeyBindingHash> bindings_;

    // Key codes for modifiers (raylib values)
    static constexpr int KEY_LEFT_CONTROL = 341;
    static constexpr int KEY_RIGHT_CONTROL = 345;
    static constexpr int KEY_LEFT_SHIFT = 340;
    static constexpr int KEY_RIGHT_SHIFT = 344;
    static constexpr int KEY_LEFT_ALT = 342;
    static constexpr int KEY_RIGHT_ALT = 346;
};

//-----------------------------------------------------------------------------
// Formatting utilities for settings UI
//-----------------------------------------------------------------------------

/// Format a key code as human-readable string
inline std::string key_name(int key_code) {
    // Common keys (raylib key codes)
    switch (key_code) {
        case 65: return "A"; case 66: return "B"; case 67: return "C";
        case 68: return "D"; case 69: return "E"; case 70: return "F";
        case 71: return "G"; case 72: return "H"; case 73: return "I";
        case 74: return "J"; case 75: return "K"; case 76: return "L";
        case 77: return "M"; case 78: return "N"; case 79: return "O";
        case 80: return "P"; case 81: return "Q"; case 82: return "R";
        case 83: return "S"; case 84: return "T"; case 85: return "U";
        case 86: return "V"; case 87: return "W"; case 88: return "X";
        case 89: return "Y"; case 90: return "Z";
        case 48: return "0"; case 49: return "1"; case 50: return "2";
        case 51: return "3"; case 52: return "4"; case 53: return "5";
        case 54: return "6"; case 55: return "7"; case 56: return "8";
        case 57: return "9";
        case 257: return "Enter";
        case 258: return "Tab";
        case 259: return "Backspace";
        case 261: return "Delete";
        case 262: return "Right"; case 263: return "Left";
        case 264: return "Down"; case 265: return "Up";
        case 268: return "Home"; case 269: return "End";
        case 266: return "Page Up"; case 267: return "Page Down";
        default: return "?";
    }
}

/// Format a binding as "Ctrl+Shift+K" style string
inline std::string format_binding(const KeyBinding& binding) {
    std::string result;
    if (binding.ctrl) result += "Ctrl+";
    if (binding.alt) result += "Alt+";
    if (binding.shift) result += "Shift+";
    result += key_name(binding.key);
    return result;
}

}  // namespace action_binding

