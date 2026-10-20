#pragma once

#include "rl.h"

enum class InputAction {
    None,
    WidgetRight,
    WidgetLeft,
    WidgetUp,
    WidgetDown,
    WidgetNext,
    WidgetPress,
    WidgetMod,
    WidgetBack,
    MenuBack,
    PauseButton,
    ToggleUIDebug,
    ToggleUILayoutDebug,
    // Text input actions (required by afterhours text_input)
    TextBackspace,
    TextDelete,
    TextHome,
    TextEnd,
    TextSelectAll,
    TextSelectLeft,
    TextSelectRight,
    TextCopy,
    TextCut,
    TextPaste,
    // Word/line editing + field undo/redo. afterhours compiles each of
    // these out (warn_missing_text_actions / text_input) when the name is
    // absent, so the enumerator names must match exactly.
    //
    // ORDER MATTERS: UIContext keeps only the LAST matching action per
    // frame, and InputCollector reports matches in enum order, while
    // KeyChord matching is subset-based (Ctrl+Shift+Z also matches the
    // Ctrl+Z chord). So the more specific chord must sort AFTER the one
    // it overlaps: TextRedo after TextUndo, TextDeleteLineBack after
    // TextDeleteWordBack, TextWordLeft after TextSelectLeft.
    TextUndo,
    TextRedo,
    TextWordLeft,
    TextWordRight,
    TextDeleteWordBack,
    TextDeleteWordForward,
    TextDeleteLineBack,
    TextDeleteLineForward,
};

inline int to_int(InputAction action) { return static_cast<int>(action); }

inline InputAction from_int(int value) {
    return static_cast<InputAction>(value);
}

inline bool action_matches(int action, InputAction expected) {
    return from_int(action) == expected;
}

inline auto get_mapping() {
    std::map<int, afterhours::input::ValidInputs> mapping;

    mapping[to_int(InputAction::WidgetLeft)] = {
        afterhours::keys::LEFT,
    };

    mapping[to_int(InputAction::WidgetRight)] = {
        afterhours::keys::RIGHT,
    };

    mapping[to_int(InputAction::WidgetUp)] = {
        afterhours::keys::UP,
    };

    mapping[to_int(InputAction::WidgetDown)] = {
        afterhours::keys::DOWN,
    };

    mapping[to_int(InputAction::WidgetBack)] = {
        afterhours::keys::UP,
    };

    mapping[to_int(InputAction::WidgetNext)] = {
        afterhours::keys::TAB,
        afterhours::keys::DOWN,
    };

    mapping[to_int(InputAction::WidgetPress)] = {
        afterhours::keys::ENTER,
    };

    mapping[to_int(InputAction::WidgetMod)] = {
        afterhours::keys::LEFT_SHIFT,
    };

    mapping[to_int(InputAction::MenuBack)] = {
        afterhours::keys::ESCAPE,
    };

    mapping[to_int(InputAction::PauseButton)] = {
        afterhours::keys::ESCAPE,
    };

    mapping[to_int(InputAction::ToggleUIDebug)] = {
        afterhours::keys::GRAVE,
    };

    mapping[to_int(InputAction::ToggleUILayoutDebug)] = {
        afterhours::keys::EQUAL,
    };

    // Text input actions
    mapping[to_int(InputAction::TextBackspace)] = {
        afterhours::keys::BACKSPACE,
    };

    mapping[to_int(InputAction::TextDelete)] = {
        afterhours::keys::DELETE_KEY,
    };

    mapping[to_int(InputAction::TextHome)] = {
        afterhours::keys::HOME,
    };

    mapping[to_int(InputAction::TextEnd)] = {
        afterhours::keys::END,
    };

    using KC = afterhours::input::KeyChord;

    mapping[to_int(InputAction::TextSelectAll)] = {
        KC(afterhours::keys::A, KC::MOD_CTRL),
    };

    // NOTE: TextSelectLeft / TextSelectRight deliberately have NO key
    // chord here (the enumerators stay: afterhours gates the branch on
    // the enum name, and `action TextSelectLeft` still works). KeyChord
    // matching is subset-based, so on a Ctrl+Shift+Left press BOTH
    // TextSelectLeft (Shift+Left) and TextWordLeft (Ctrl+Left) match,
    // and text_input consumes BOTH via
    // pressed_or_repeat (the repeat bitset is not exclusive like
    // last_action): Ctrl+Shift+Left moved a word AND a char
    // (selection [5, 10) instead of [6, 10) on "alpha beta").
    // Shift-selection does not need these chords: text_input and
    // text_area both extend the selection from WidgetLeft / WidgetRight
    // plus the raw Shift state, and TextWordLeft/Right are the only
    // explicit chords claiming Ctrl/Alt+arrows.

    mapping[to_int(InputAction::TextCopy)] = {
        KC(afterhours::keys::C, KC::MOD_CTRL),
    };

    mapping[to_int(InputAction::TextCut)] = {
        KC(afterhours::keys::X, KC::MOD_CTRL),
    };

    mapping[to_int(InputAction::TextPaste)] = {
        KC(afterhours::keys::V, KC::MOD_CTRL),
    };

    // Field undo/redo. Primary is Ctrl on every platform, matching the
    // existing TextCopy/Cut/Paste chords and both document presets
    // (which use Ctrl as the Cmd substitute). Redo is Ctrl+Shift+Z and
    // Ctrl+Y, as in afterhours' default_keymap (non-mac branch).
    mapping[to_int(InputAction::TextUndo)] = {
        KC(afterhours::keys::Z, KC::MOD_CTRL),
    };

    mapping[to_int(InputAction::TextRedo)] = {
        KC(afterhours::keys::Z,
           static_cast<std::uint8_t>(KC::MOD_CTRL | KC::MOD_SHIFT)),
        KC(afterhours::keys::Y, KC::MOD_CTRL),
    };

    // Word navigation / deletion. Both Ctrl and Alt (Option on mac)
    // chords: Ctrl+Arrow is the Windows-preset / E2E chord, Alt+Arrow
    // is the mac-preset word chord and afterhours' mac WORD modifier,
    // so a field does word ops under either preset's convention.
    mapping[to_int(InputAction::TextWordLeft)] = {
        KC(afterhours::keys::LEFT, KC::MOD_CTRL),
        KC(afterhours::keys::LEFT, KC::MOD_ALT),
    };

    mapping[to_int(InputAction::TextWordRight)] = {
        KC(afterhours::keys::RIGHT, KC::MOD_CTRL),
        KC(afterhours::keys::RIGHT, KC::MOD_ALT),
    };

    mapping[to_int(InputAction::TextDeleteWordBack)] = {
        KC(afterhours::keys::BACKSPACE, KC::MOD_CTRL),
        KC(afterhours::keys::BACKSPACE, KC::MOD_ALT),
    };

    mapping[to_int(InputAction::TextDeleteWordForward)] = {
        KC(afterhours::keys::DELETE_KEY, KC::MOD_CTRL),
        KC(afterhours::keys::DELETE_KEY, KC::MOD_ALT),
    };

    // Delete to line start / end. Afterhours' own chords:
    // mac: Cmd+Backspace (SUPER) / Ctrl+K; elsewhere Ctrl+Shift+Backspace
    // for line-start and nothing for line-end. Ctrl+Shift+Backspace is
    // bound on every platform (it is the portable / E2E-testable chord);
    // the mac chords are added under __APPLE__.
    mapping[to_int(InputAction::TextDeleteLineBack)] = {
        KC(afterhours::keys::BACKSPACE,
           static_cast<std::uint8_t>(KC::MOD_CTRL | KC::MOD_SHIFT)),
#if defined(__APPLE__)
        KC(afterhours::keys::BACKSPACE, KC::MOD_SUPER),
#endif
    };

#if defined(__APPLE__)
    mapping[to_int(InputAction::TextDeleteLineForward)] = {
        KC(afterhours::keys::K, KC::MOD_CTRL),
    };
#else
    // No platform-standard chord on Windows/Linux: the action still
    // exists (and `action TextDeleteLineForward` still works); the
    // handler is not compiled out for want of a binding.
    mapping[to_int(InputAction::TextDeleteLineForward)] = {};
#endif

    return mapping;
}
