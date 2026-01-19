#pragma once

#include "../../vendor/afterhours/src/plugins/ui.h"
#include "../../vendor/afterhours/src/plugins/window_manager.h"
#include "../rl.h"
#include "../input_mapping.h"  // Use global InputAction enum

namespace ui_imm {

// Use the global InputAction enum (defined in input_mapping.h)
// This ensures consistency with Preload::make_singleton() which registers
// UIContext<::InputAction>
using InputAction = ::InputAction;

// Alias for the UIContext type
using UIContextType = afterhours::ui::UIContext<InputAction>;

// Win95 color palette for Afterhours Theme
namespace win95_colors {
constexpr afterhours::Color WINDOW_BG = {192, 192, 192, 255};
constexpr afterhours::Color TITLE_BAR = {0, 0, 128, 255};
constexpr afterhours::Color TEXT = {0, 0, 0, 255};
constexpr afterhours::Color TEXT_WHITE = {255, 255, 255, 255};
constexpr afterhours::Color TEXT_DISABLED = {128, 128, 128, 255};
constexpr afterhours::Color TEXT_AREA = {255, 255, 255, 255};
constexpr afterhours::Color HIGHLIGHT = {0, 0, 128, 255};
constexpr afterhours::Color BUTTON_FACE = {192, 192, 192, 255};
constexpr afterhours::Color BORDER_LIGHT = {255, 255, 255, 255};
constexpr afterhours::Color BORDER_DARK = {128, 128, 128, 255};
constexpr afterhours::Color ERROR_COLOR = {255, 0, 0, 255};
}  // namespace win95_colors

// Create Win95 theme for Afterhours UI
inline afterhours::ui::Theme createWin95Theme() {
    afterhours::ui::Theme theme;

    // Font colors
    theme.font = win95_colors::TEXT_WHITE;
    theme.darkfont = win95_colors::TEXT;
    theme.font_muted = win95_colors::TEXT_DISABLED;

    // Background colors
    theme.background = win95_colors::WINDOW_BG;
    theme.surface = win95_colors::TEXT_AREA;

    // UI element colors
    theme.primary = win95_colors::TITLE_BAR;
    theme.secondary = win95_colors::BUTTON_FACE;
    theme.accent = win95_colors::HIGHLIGHT;
    theme.error = win95_colors::ERROR_COLOR;

    // Win95 has sharp corners (no rounded corners)
    theme.rounded_corners = std::bitset<4>();
    theme.roundness = 0.0f;
    theme.segments = 0;

    return theme;
}

// Initialize the Win95 theme globally
inline void initWin95Theme() {
    auto& themeDefaults = afterhours::ui::imm::ThemeDefaults::get();
    themeDefaults.set_theme(createWin95Theme());
}

// Initialize the UI context with current screen dimensions
// Note: Preload::make_singleton() already creates UIContext and other singletons
// This function just updates the resolution and applies the Win95 theme
inline void initUIContext(int screenWidth, int screenHeight) {
    using namespace afterhours;

    // Initialize theme first
    initWin95Theme();

    // Update resolution (already registered by Preload::make_singleton)
    auto* resProv = EntityHelper::get_singleton_cmp<window_manager::ProvidesCurrentResolution>();
    if (resProv) {
        resProv->current_resolution = {screenWidth, screenHeight};
    }
}

// Register pre-layout UI systems (context begin, clear children)
// Call this BEFORE registering systems that create UI elements
inline void registerUIPreLayoutSystems(afterhours::SystemManager& manager) {
    using namespace afterhours;

    // Begin context (reads mouse/input state)
    manager.register_update_system(
        std::make_unique<ui::BeginUIContextManager<InputAction>>());

    // Clear UI component children for rebuild
    manager.register_update_system(
        std::make_unique<ui::ClearUIComponentChildren>());
}

// Register post-layout UI systems (autolayout, interactions, cleanup)
// Call this AFTER registering systems that create UI elements
inline void registerUIPostLayoutSystems(afterhours::SystemManager& manager) {
    using namespace afterhours;

    // Run autolayout (must run AFTER all UI elements are created)
    manager.register_update_system(std::make_unique<ui::RunAutoLayout>());

    // Track visibility
    manager.register_update_system(
        std::make_unique<ui::TrackIfComponentWillBeRendered<InputAction>>());

    // Handle interactions
    manager.register_update_system(
        std::make_unique<ui::HandleTabbing<InputAction>>());
    manager.register_update_system(
        std::make_unique<ui::HandleClicks<InputAction>>());
    manager.register_update_system(
        std::make_unique<ui::HandleDrags<InputAction>>());
    manager.register_update_system(
        std::make_unique<ui::HandleLeftRight<InputAction>>());

    // End context (cleanup)
    manager.register_update_system(
        std::make_unique<ui::EndUIContextManager<InputAction>>());

    // Compute visual focus
    manager.register_update_system(
        std::make_unique<ui::ComputeVisualFocusId<InputAction>>());
}

// Register all Afterhours UI update systems with the SystemManager
// Note: If you have systems that create UI elements (like MenuUISystem),
// use registerUIPreLayoutSystems and registerUIPostLayoutSystems instead,
// with your UI-creating systems in between.
inline void registerUIUpdateSystems(afterhours::SystemManager& manager) {
    registerUIPreLayoutSystems(manager);
    registerUIPostLayoutSystems(manager);
}

// Register Afterhours UI render systems with the SystemManager
inline void registerUIRenderSystems(afterhours::SystemManager& manager) {
    using namespace afterhours;

    manager.register_render_system(
        std::make_unique<ui::RenderImm<InputAction>>());
}

// Get the UI context for immediate-mode widget calls
inline UIContextType& getUIContext() {
    auto* ctx = afterhours::EntityHelper::get_singleton_cmp<UIContextType>();
    if (!ctx) {
        throw std::runtime_error("UIContext singleton not found - did you call initUIContext()?");
    }
    return *ctx;
}

// Get the root UIComponent for parenting UI elements
inline afterhours::ui::UIComponent& getUIRoot() {
    auto roots = afterhours::EntityQuery({.force_merge = true})
                     .whereHasComponent<afterhours::ui::AutoLayoutRoot>()
                     .gen();
    if (roots.empty()) {
        throw std::runtime_error("No UI root found");
    }
    return roots[0].get().get<afterhours::ui::UIComponent>();
}

// Get the root entity for parenting UI elements
inline afterhours::Entity& getUIRootEntity() {
    auto roots = afterhours::EntityQuery({.force_merge = true})
                     .whereHasComponent<afterhours::ui::AutoLayoutRoot>()
                     .gen();
    if (roots.empty()) {
        throw std::runtime_error("No UI root found");
    }
    return roots[0].get();
}

}  // namespace ui_imm

namespace ui_imm {

// Initialize test mode UI
// Note: Test input is now handled directly via afterhours::testing::test_input
// which is set up in external.h. This function exists for API compatibility.
inline void initTestModeUI() {
    // Test input is enabled via test_input::set_test_mode(true)
    // No additional setup needed for UI context integration
}

// Note: Test input system registration is no longer needed.
// Test input is handled via afterhours::testing::test_input which intercepts
// raylib input calls when test_input::test_mode is true.

}  // namespace ui_imm
