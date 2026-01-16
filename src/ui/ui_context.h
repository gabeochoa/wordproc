#pragma once

#include "../../vendor/afterhours/src/plugins/ui.h"
#include "../../vendor/afterhours/src/plugins/window_manager.h"
#include "../rl.h"

namespace ui_imm {

// InputAction enum required by Afterhours UIContext
// Must have specific values that the UI systems check for
enum class InputAction {
    None = 0,

    // Widget navigation (Tab/Shift+Tab)
    WidgetNext,
    WidgetBack,
    WidgetMod,  // Shift modifier

    // Widget activation (Enter/Space)
    WidgetPress,

    // Slider/list navigation
    WidgetLeft,
    WidgetRight,
    WidgetUp,
    WidgetDown,

    // Common actions
    Confirm,
    Cancel,

    COUNT
};

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

// Create and initialize the UI context entity
inline void initUIContext(int screenWidth, int screenHeight) {
    using namespace afterhours;

    // Initialize theme first
    initWin95Theme();

    // Check if resolution provider already exists (may be registered by Preload)
    auto* existingRes = EntityHelper::get_singleton_cmp<window_manager::ProvidesCurrentResolution>();
    if (existingRes) {
        // Just update the resolution
        existingRes->current_resolution = {screenWidth, screenHeight};
    } else {
        // Create resolution provider entity (required by RunAutoLayout)
        auto& resEntity = EntityHelper::createEntity();
        auto& resProv =
            resEntity.addComponent<window_manager::ProvidesCurrentResolution>();
        resProv.current_resolution = {screenWidth, screenHeight};
        EntityHelper::registerSingleton<window_manager::ProvidesCurrentResolution>(
            resEntity);
    }

    // Check if UI context already exists
    auto* existingCtx = EntityHelper::get_singleton_cmp<UIContextType>();
    if (!existingCtx) {
        // Create UI context entity with the context component
        auto& ctxEntity = EntityHelper::createEntity();
        ctxEntity.addComponent<UIContextType>();
        EntityHelper::registerSingleton<UIContextType>(ctxEntity);
    }

    // Check if UI root already exists
    auto roots = EntityQuery({.ignore_temp_warning = true})
                     .whereHasComponent<ui::AutoLayoutRoot>()
                     .gen();
    if (roots.empty()) {
        // Create the root entity for all UI elements
        auto& rootEntity = EntityHelper::createEntity();
        rootEntity.addComponent<ui::AutoLayoutRoot>();
        auto& rootCmp = rootEntity.addComponent<ui::UIComponent>(rootEntity.id);
        rootCmp.set_desired_width(ui::percent(1.0f))
            .set_desired_height(ui::percent(1.0f));
    }
}

// Register all Afterhours UI update systems with the SystemManager
inline void registerUIUpdateSystems(afterhours::SystemManager& manager) {
    using namespace afterhours;

    // Begin context (reads mouse/input state)
    manager.register_update_system(
        std::make_unique<ui::BeginUIContextManager<InputAction>>());

    // Clear UI component children for rebuild
    manager.register_update_system(
        std::make_unique<ui::ClearUIComponentChildren>());

    // Run autolayout
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

// Register Afterhours UI render systems with the SystemManager
inline void registerUIRenderSystems(afterhours::SystemManager& manager) {
    using namespace afterhours;

    manager.register_render_system(
        std::make_unique<ui::RenderImm<InputAction>>());
}

// Get the UI context for immediate-mode widget calls
inline UIContextType& getUIContext() {
    return *afterhours::EntityHelper::get_singleton_cmp<UIContextType>();
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

// Include test input provider after ui_imm is fully defined (to avoid circular
// deps)
#include "../testing/test_input_provider.h"

namespace ui_imm {

// Initialize test input provider for UI context integration
// Call this after initUIContext() when running in test mode
inline void initTestModeUI() { test_input::initTestInputProvider(); }

// Register test input system that integrates with Afterhours UIContext
// Call this after registerUIUpdateSystems() when running in test mode
// The system runs after BeginUIContextManager to override input state
inline void registerTestInputSystem(afterhours::SystemManager& manager) {
    test_input::registerTestInputSystem<InputAction>(manager);
}

}  // namespace ui_imm
