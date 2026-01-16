#pragma once

#include "../../vendor/afterhours/src/plugins/ui.h"

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

// Create and initialize the UI context entity
inline void initUIContext() {
  using namespace afterhours;
  
  // Create UI context entity with the context component
  auto& ctxEntity = EntityHelper::createEntity();
  ctxEntity.addComponent<UIContextType>();
  ctxEntity.addComponent<afterhours::SingletonComponent<UIContextType>>();
  
  // Create the root entity for all UI elements
  auto& rootEntity = EntityHelper::createEntity();
  rootEntity.addComponent<afterhours::ui::AutoLayoutRoot>();
  auto& rootCmp = rootEntity.addComponent<afterhours::ui::UIComponent>(rootEntity.id);
  rootCmp.set_desired_width(afterhours::ui::percent(1.0f))
         .set_desired_height(afterhours::ui::percent(1.0f));
}

} // namespace ui_imm
