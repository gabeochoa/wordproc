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
    ToggleUIDebug,
    ToggleUILayoutDebug,
    // Text input actions (required by afterhours text_input)
    TextBackspace,
    TextDelete,
    TextHome,
    TextEnd,
    TextSelectAll,
    TextWordLeft,
    TextWordRight,
    TextDeleteWordBack,
    TextDeleteWordForward,
    TextUndo,
    TextRedo,
    TextCopy,
    TextCut,
    TextPaste,
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
    using KC = afterhours::input::KeyChord;

    mapping[to_int(InputAction::WidgetLeft)] = {
        afterhours::keys::LEFT,
        KC(afterhours::keys::LEFT, KC::MOD_SHIFT),
    };

    mapping[to_int(InputAction::WidgetRight)] = {
        afterhours::keys::RIGHT,
        KC(afterhours::keys::RIGHT, KC::MOD_SHIFT),
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
        KC(afterhours::keys::HOME, KC::MOD_SHIFT),
    };

    mapping[to_int(InputAction::TextEnd)] = {
        afterhours::keys::END,
        KC(afterhours::keys::END, KC::MOD_SHIFT),
    };

    mapping[to_int(InputAction::TextSelectAll)] = {
        KC(afterhours::keys::A, KC::MOD_CTRL),
    };

    mapping[to_int(InputAction::TextWordLeft)] = {
        KC(afterhours::keys::LEFT, KC::MOD_ALT),
        KC(afterhours::keys::LEFT, KC::MOD_CTRL),
    };
    mapping[to_int(InputAction::TextWordRight)] = {
        KC(afterhours::keys::RIGHT, KC::MOD_ALT),
        KC(afterhours::keys::RIGHT, KC::MOD_CTRL),
    };
    mapping[to_int(InputAction::TextDeleteWordBack)] = {
        KC(afterhours::keys::BACKSPACE, KC::MOD_ALT),
        KC(afterhours::keys::BACKSPACE, KC::MOD_CTRL),
    };
    mapping[to_int(InputAction::TextDeleteWordForward)] = {
        KC(afterhours::keys::DELETE_KEY, KC::MOD_ALT),
        KC(afterhours::keys::DELETE_KEY, KC::MOD_CTRL),
    };

    mapping[to_int(InputAction::TextUndo)] = {KC(afterhours::keys::Z, KC::MOD_CTRL)};
    mapping[to_int(InputAction::TextRedo)] = {KC(afterhours::keys::Z, KC::MOD_CTRL | KC::MOD_SHIFT)};

    mapping[to_int(InputAction::TextCopy)] = {
        KC(afterhours::keys::C, KC::MOD_CTRL),
    };

    mapping[to_int(InputAction::TextCut)] = {
        KC(afterhours::keys::X, KC::MOD_CTRL),
    };

    mapping[to_int(InputAction::TextPaste)] = {
        KC(afterhours::keys::V, KC::MOD_CTRL),
    };

    return mapping;
}
