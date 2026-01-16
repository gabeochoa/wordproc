// TestInputProvider for Afterhours UIContext integration
// Uses the extracted e2e_testing.h concepts but integrates with Afterhours ECS

#pragma once

#include <bitset>
#include <optional>
#include <queue>

#include "../../vendor/afterhours/src/ecs.h"
#include "../../vendor/afterhours/src/plugins/input_system.h"
#include "../../vendor/afterhours/src/plugins/ui/context.h"

namespace test_input {

constexpr size_t MAX_INPUT_ACTIONS = 32;

// TestInputProvider: Component that stores simulated input state for tests
struct TestInputProvider : afterhours::BaseComponent {
    // Mouse state
    std::optional<afterhours::input::MousePosition> mouse_position;
    bool mouse_left_down = false;
    bool mouse_left_pressed_this_frame = false;
    bool mouse_left_released_this_frame = false;

    // UI actions to simulate
    std::optional<int> pending_action;
    std::bitset<MAX_INPUT_ACTIONS> held_actions;

    // Test mode control
    bool simulation_active = false;

    void setMousePosition(float x, float y) {
        mouse_position = {x, y};
        simulation_active = true;
    }

    void pressMouseLeft() {
        mouse_left_down = true;
        mouse_left_pressed_this_frame = true;
        simulation_active = true;
    }

    void releaseMouseLeft() {
        mouse_left_down = false;
        mouse_left_released_this_frame = true;
        simulation_active = true;
    }

    void queueAction(int action) {
        pending_action = action;
        simulation_active = true;
    }

    void holdAction(int action) {
        if (action >= 0 && static_cast<size_t>(action) < held_actions.size()) {
            held_actions[static_cast<size_t>(action)] = true;
            simulation_active = true;
        }
    }

    void releaseAction(int action) {
        if (action >= 0 && static_cast<size_t>(action) < held_actions.size()) {
            held_actions[static_cast<size_t>(action)] = false;
        }
    }

    void resetFrame() {
        mouse_left_pressed_this_frame = false;
        mouse_left_released_this_frame = false;
        pending_action = std::nullopt;
    }

    void reset() {
        mouse_position = std::nullopt;
        mouse_left_down = false;
        mouse_left_pressed_this_frame = false;
        mouse_left_released_this_frame = false;
        pending_action = std::nullopt;
        held_actions.reset();
        simulation_active = false;
    }
};

// TestInputSystem: Injects test input into UIContext
template<typename InputAction>
struct TestInputSystem
    : afterhours::System<afterhours::ui::UIContext<InputAction>> {
    virtual void for_each_with(afterhours::Entity&,
                               afterhours::ui::UIContext<InputAction>& context,
                               float) override {
        auto* provider =
            afterhours::EntityHelper::get_singleton_cmp<TestInputProvider>();
        if (!provider || !provider->simulation_active) {
            return;
        }

        if (provider->mouse_position.has_value()) {
            context.mouse.pos = provider->mouse_position.value();
        }

        context.mouse.left_down = provider->mouse_left_down;

        if (provider->pending_action.has_value()) {
            context.last_action =
                static_cast<InputAction>(provider->pending_action.value());
            provider->pending_action = std::nullopt;
        }

        for (size_t i = 0; i < provider->held_actions.size() &&
                           i < context.all_actions.size();
             ++i) {
            if (provider->held_actions[i]) {
                context.all_actions[i] = true;
            }
        }
    }
};

// Helper to initialize TestInputProvider as a singleton
inline void initTestInputProvider() {
    auto& entity = afterhours::EntityHelper::createEntity();
    entity.addComponent<TestInputProvider>();
    afterhours::EntityHelper::registerSingleton<TestInputProvider>(entity);
}

// Get the TestInputProvider singleton
inline TestInputProvider* getTestInputProvider() {
    return afterhours::EntityHelper::get_singleton_cmp<TestInputProvider>();
}

// Register TestInputSystem
template<typename InputAction>
inline void registerTestInputSystem(afterhours::SystemManager& manager) {
    manager.register_update_system(
        std::make_unique<TestInputSystem<InputAction>>());
}

}  // namespace test_input
