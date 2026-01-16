#pragma once

#include "../../vendor/afterhours/src/ecs.h"
#include "../../vendor/afterhours/src/plugins/input_system.h"
#include "../../vendor/afterhours/src/plugins/ui/context.h"
#include <bitset>
#include <optional>
#include <queue>

namespace test_input {

// Forward declaration - actual enum defined in ui_context.h
// We use int-based storage to avoid circular dependency
constexpr size_t MAX_INPUT_ACTIONS = 32;

// TestInputProvider: Component that stores simulated input state for tests
// This integrates with the Afterhours UI state context to inject test input
struct TestInputProvider : afterhours::BaseComponent {
    // Mouse state
    std::optional<afterhours::input::MousePosition> mouse_position;
    bool mouse_left_down = false;
    bool mouse_left_pressed_this_frame = false;
    bool mouse_left_released_this_frame = false;

    // UI actions to simulate (stored as int to avoid circular dependency)
    std::optional<int> pending_action;
    std::bitset<MAX_INPUT_ACTIONS> held_actions;

    // Test mode control
    bool simulation_active = false;

    // Set mouse position for simulation
    void setMousePosition(float x, float y) {
        mouse_position = {x, y};
        simulation_active = true;
    }

    // Simulate mouse button press
    void pressMouseLeft() {
        mouse_left_down = true;
        mouse_left_pressed_this_frame = true;
        simulation_active = true;
    }

    // Simulate mouse button release
    void releaseMouseLeft() {
        mouse_left_down = false;
        mouse_left_released_this_frame = true;
        simulation_active = true;
    }

    // Queue a UI action (like WidgetNext, WidgetPress, etc.)
    // Pass the enum value as int to avoid circular dependency
    void queueAction(int action) {
        pending_action = action;
        simulation_active = true;
    }

    // Set an action as held
    void holdAction(int action) {
        if (action >= 0 && static_cast<size_t>(action) < held_actions.size()) {
            held_actions[static_cast<size_t>(action)] = true;
            simulation_active = true;
        }
    }

    // Release a held action
    void releaseAction(int action) {
        if (action >= 0 && static_cast<size_t>(action) < held_actions.size()) {
            held_actions[static_cast<size_t>(action)] = false;
        }
    }

    // Reset frame state (call at start of each test frame)
    void resetFrame() {
        mouse_left_pressed_this_frame = false;
        mouse_left_released_this_frame = false;
        pending_action = std::nullopt;
    }

    // Full reset (call between tests)
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

// TestInputSystem: Runs after BeginUIContextManager to override UIContext with test input
// This integrates the test input with Afterhours UI state context
template <typename InputAction>
struct TestInputSystem : afterhours::System<afterhours::ui::UIContext<InputAction>> {
    virtual void for_each_with(afterhours::Entity&,
                               afterhours::ui::UIContext<InputAction>& context,
                               float) override {
        // Get TestInputProvider singleton
        auto* provider = afterhours::EntityHelper::get_singleton_cmp<TestInputProvider>();
        if (!provider || !provider->simulation_active) {
            return;
        }

        // Override mouse position if simulated
        if (provider->mouse_position.has_value()) {
            context.mouse_pos = provider->mouse_position.value();
        }

        // Override mouse button state if simulated
        context.mouseLeftDown = provider->mouse_left_down;

        // Inject pending action
        if (provider->pending_action.has_value()) {
            context.last_action = static_cast<InputAction>(provider->pending_action.value());
            provider->pending_action = std::nullopt;
        }

        // Apply held actions to all_actions bitset
        for (size_t i = 0; i < provider->held_actions.size() && i < context.all_actions.size(); ++i) {
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

// Get the TestInputProvider singleton (returns nullptr if not initialized)
inline TestInputProvider* getTestInputProvider() {
    return afterhours::EntityHelper::get_singleton_cmp<TestInputProvider>();
}

// Register TestInputSystem (should run after BeginUIContextManager)
template <typename InputAction>
inline void registerTestInputSystem(afterhours::SystemManager& manager) {
    manager.register_update_system(
        std::make_unique<TestInputSystem<InputAction>>());
}

}  // namespace test_input
