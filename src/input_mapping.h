#pragma once

#include "rl.h"

enum class InputAction {
    None,
    WidgetRight,
    WidgetLeft,
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

    return mapping;
}
