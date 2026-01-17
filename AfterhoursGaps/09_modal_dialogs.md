# Modal Dialogs

## Working Implementation
See these files for a complete working example:
- `src/ui/win95_widgets.h` - DrawMessageDialog, DrawInputDialog, DialogState
- `src/ui/win95_widgets.cpp` - Dialog rendering implementation
- `src/ecs/components.h` - MenuComponent with dialog state flags (showAboutDialog, showFindDialog, etc.)

## Problem
Afterhours UI does not provide modal dialog components for common UI patterns like 
message boxes, confirmation dialogs, input prompts, or complex multi-field dialogs.

## Current Workaround
Custom `win95_widgets.cpp` implements:

```cpp
// Draw a message dialog
// Returns: 0 = OK, 1 = Cancel, -1 = still open
int DrawMessageDialog(raylib::Rectangle dialogRect, const char* title,
                      const char* message, bool hasCancel = false);

// Draw a simple input dialog
// Returns: 0 = OK, 1 = Cancel, -1 = still open
int DrawInputDialog(raylib::Rectangle dialogRect, const char* title,
                    const char* prompt, char* buffer, int bufferSize);
```

Plus manual dialog state management in `MenuComponent`:
- `showAboutDialog`, `showHelpWindow`, `showFindDialog`, `showPageSetup`
- Input buffers for each dialog
- Manual modal blocking logic in render/input systems

**Why This Workaround Works:**
The win95_widgets approach bypasses Afterhours' layout system entirely by:
1. Drawing directly with raylib primitives (not ECS entities)
2. Managing its own input handling (checking mouse positions manually)
3. Returning simple integer results instead of using components/state

This is effective but doesn't integrate with the existing UI framework.

## What Afterhours Currently Provides
- No modal/dialog component
- No focus trapping for modal interactions
- No backdrop/overlay rendering
- No built-in message box, confirm, or prompt dialogs

---

## PR #26 Attempt: Lessons Learned

A previous attempt to implement modal dialogs in Afterhours (branch `feature/modal-dialogs`) 
encountered multiple architectural issues. This section documents what was tried, what broke, 
and proposes alternative approaches.

### What Was Implemented in PR #26

**Branch:** `feature/modal-dialogs` on vendor/afterhours  
**Commit:** `0f9f71d WIP: Modal dialog support`

**Core Components Added (`components.h`):**

```cpp
enum class DialogResult {
  Pending,     // Dialog still open
  Confirmed,   // OK/Yes clicked  
  Cancelled,   // Cancel/No or Escape pressed
  Dismissed,   // Backdrop clicked or X button
  Custom,      // Custom button clicked
};

struct DialogState : BaseComponent {
  DialogResult result = DialogResult::Pending;
  int custom_result = -1;
  std::string input_value;
  bool input_initialized = false;  // Prevents re-init each frame
};

struct ModalOptions {
  Vector2Type size = {400, 200};
  bool auto_size = false;
  bool center_on_screen = true;
  Vector2Type position = {0, 0};
  bool close_on_escape = true;
  bool close_on_backdrop_click = false;
  bool show_close_button = true;
  bool draggable = false;
  Color backdrop_color = {0, 0, 0, 128};
  int render_layer = 1000;  // High layer to render above everything
};

struct IsModal : BaseComponent {
  bool active = true;
  bool close_on_backdrop_click = false;
  bool close_on_escape = true;
  bool show_close_button = true;
  bool draggable = false;
  Color backdrop_color = {0, 0, 0, 128};
  int render_layer = 1000;
  size_t open_order = 0;  // For z-ordering multiple modals
  // ...apply_options() method to update from ModalOptions
};

struct ModalDragState : BaseComponent {
  Vector2Type last_mouse = {0, 0};
  bool dragging = false;
  bool has_dragged = false;  // Prevents re-centering after drag
};
```

**UIContext Extensions (`context.h`):**

```cpp
template <typename InputAction> struct UIContext : BaseComponent {
  // ... existing fields ...
  
  std::vector<EntityID> modal_stack;
  size_t modal_sequence = 0;
  std::vector<int> render_layer_stack;
  int render_layer_offset = 0;
  
  void begin_frame() {
    modal_stack.clear();
    modal_sequence = 0;
    render_layer_stack.clear();
    render_layer_offset = 0;
  }
  
  [[nodiscard]] bool is_modal_active() const { return !modal_stack.empty(); }
  [[nodiscard]] EntityID top_modal() const {
    return modal_stack.empty() ? ROOT : modal_stack.back();
  }
  
  void push_render_layer_offset(int offset);
  void pop_render_layer_offset();
};
```

**Immediate-Mode API (`imm_components.h`):**

```cpp
// Modal stack management
inline void _push_modal_stack(HasUIContext auto &ctx, EntityID entity_id);
inline void _pop_modal_stack(HasUIContext auto &ctx, EntityID entity_id);
inline void _close_modal(HasUIContext auto &ctx, EntityID entity_id,
                         DialogResult result = DialogResult::Dismissed);

// Main modal entry point
inline ElementResult begin_modal(HasUIContext auto &ctx, EntityParent ep_pair,
                                 const std::string &title,
                                 ModalOptions options = ModalOptions());

// Built-in dialogs
inline ElementResult message_box(HasUIContext auto &ctx, EntityParent ep_pair,
                                 const std::string &title,
                                 const std::string &message,
                                 DialogResult &out_result);

inline ElementResult confirm_dialog(HasUIContext auto &ctx, EntityParent ep_pair,
                                    const std::string &title,
                                    const std::string &message,
                                    bool yes_no_buttons,
                                    DialogResult &out_result);

inline ElementResult input_dialog(HasUIContext auto &ctx, EntityParent ep_pair,
                                  const std::string &title,
                                  const std::string &prompt,
                                  std::string &in_out_value,
                                  DialogResult &out_result);
```

**Modal Input Gating (`systems.h`):**

```cpp
// Helper to check if an entity is inside the modal tree
inline bool is_entity_in_tree(EntityID root_id, EntityID search_id);

// Returns true if entity should process input (either no modal active,
// or entity is part of the top modal's tree)
template <typename InputAction>
inline bool should_process_for_modal(UIContext<InputAction> *context,
                                     EntityID entity_id) {
  if (!context || !context->is_modal_active())
    return true;
  return is_entity_in_tree(context->top_modal(), entity_id);
}

// New system for modal-specific input handling
template <typename InputAction>
struct HandleModalDismiss : System<UIContext<InputAction>> {
  // Tracks mouse state to detect backdrop clicks
  bool prev_mouse_down = false;
  bool should_check_backdrop = false;
  input::MousePosition click_pos{};
  input::MousePosition press_pos{};  // Where the press STARTED
  bool modal_active_on_press = false;  // Was modal open when press started?
  
  // Handles:
  // - Escape key closing (checks MenuBack and PauseButton actions)
  // - Backdrop click closing (only if press started outside modal)
  // - Clearing drag state on mouse release
};
```

**Modal-Aware Rendering (`rendering.h`):**

```cpp
// In RenderImm::for_each_with_derived:
if (!context.is_modal_active()) {
  // Normal rendering
  for (auto &cmd : context.render_cmds) { render(...); }
} else {
  // Separate into modal vs non-modal buckets
  std::vector<RenderInfo> non_modal_cmds;
  std::map<EntityID, std::vector<RenderInfo>> modal_cmds;
  
  for (auto &cmd : context.render_cmds) {
    EntityID modal_owner = /* find which modal owns this entity */;
    if (modal_owner == context.ROOT)
      non_modal_cmds.push_back(cmd);
    else
      modal_cmds[modal_owner].push_back(cmd);
  }
  
  // Render non-modal UI first
  for (auto &cmd : non_modal_cmds) { render(...); }
  
  // Then for each modal in stack order:
  // 1. Draw backdrop
  // 2. Render modal contents
  for (auto modal_id : context.modal_stack) {
    draw_rectangle(/* full screen */, modal.backdrop_color);
    for (auto &cmd : modal_cmds[modal_id]) { render(...); }
  }
}
```

**Modifier Recursion (`ui_modifiers.h`):**

```cpp
// Walks parent chain and applies all HasUIModifiers transforms
static inline RectangleType apply_ui_modifiers_recursive(EntityID entity_id,
                                                         RectangleType rect) {
  std::vector<EntityID> chain;
  // Build chain from entity to root
  // Then apply modifiers from root to entity (correct order)
  for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
    if (ent.has<HasUIModifiers>()) {
      rect = ent.get<HasUIModifiers>().apply_modifier(rect);
    }
  }
  return rect;
}
```

**Escape Key Helper (`utilities.h`):**

```cpp
// Call this in main loop to prevent window close when modal is active
template <typename InputAction> 
static inline bool should_close_on_escape() {
  auto *ui_context = EntityHelper::get_singleton_cmp<ui::UIContext<InputAction>>();
  return !ui_context || !ui_context->is_modal_active();
}
```

### Issues Encountered and Their Causes

#### Issue 1: `mk()` Entity Conflicts
**Problem:** Calling `mk()` every frame for modal entities caused ID conflicts.  
**Root Cause:** Afterhours' immediate-mode pattern expects entities to be created once and reused, but the natural pattern for modals is to call `mk()` in render code.  
**Fix Applied:** Created persistent entity IDs stored in the screen struct and used `ensure_modal_entity()` pattern.  
**Better Solution:** Don't rely on `mk()` for modal entities. Pre-create modal entities during screen initialization.

#### Issue 2: Escape Key Closing Both Modal and Window
**Problem:** Pressing Escape closed the modal AND triggered window close.  
**Root Cause:** The input mapping had KEY_ESCAPE mapped to both `MenuBack` and `PauseButton`. The main loop consumed the same key press.  
**Fix Applied:** Added `should_close_on_escape()` utility that checks if a modal is active.  
**Better Solution:** Modal input handling needs to "consume" the escape key so it doesn't propagate. Consider adding an input consumption mechanism.

#### Issue 3: Backdrop Click Instant Close
**Problem:** Clicking a button to open a modal also triggered backdrop dismiss on mouse release.  
**Root Cause:** The backdrop dismiss logic checked for mouse release, but the release from clicking the "Open Modal" button was still active in the same frame.  
**Fix Applied:** Tracked where the mouse press started and only allowed backdrop close if press started outside the modal while a modal was already active.  
**Better Solution:** Use a frame delay or "mouse down started on backdrop" tracking. The fix applied is actually correct; this is inherent to immediate-mode UI.

#### Issue 4: Modal Content Not Positioned Correctly
**Problem:** Child elements weren't inheriting parent translations when using `with_translate()`.  
**Root Cause:** The autolayout system doesn't propagate `HasUIModifiers` transforms down the hierarchy. Translation is applied only during rendering, not during layout calculation.  
**Fix Applied:** Created `apply_ui_modifiers_recursive()` to traverse parent chain and apply cumulative translate/scale.  
**Better Solution:** Consider if modals should use absolute positioning (`with_absolute_position()`) instead of translations. Or add transform inheritance to the layout system.

#### Issue 5: Circular Include Dependency
**Problem:** `systems.h` and `utilities.h` had circular dependencies.  
**Fix Applied:** Moved `apply_ui_modifiers_recursive()` to a new `ui_modifiers.h` file.  
**Better Solution:** Keep helper functions in separate utility headers that don't depend on system headers.

#### Issue 6: Modal Backdrop Rendering On Top of Modal Content
**Problem:** Modal content was rendering behind the backdrop.  
**Root Cause:** Render layer management wasn't properly separating backdrop from modal content.  
**Fix Applied:** Added `render_layer_offset` management with push/pop in `begin_modal`.  
**Better Solution:** Use a dedicated modal render pass that draws: (1) backdrop, (2) modal container, (3) modal children. Don't mix modal entities into the regular render queue.

#### Issue 7: Dragging Stopped Working After Modifier Fixes
**Problem:** After adding `apply_ui_modifiers_recursive`, `HandleDrags` still used raw `component.rect()` for hit-testing.  
**Fix Applied:** Applied modifiers in `HandleDrags` too.  
**Better Solution:** All input systems need a consistent way to get "screen-space" rects. Consider adding `UIComponent::screen_rect()` that applies all modifiers.

#### Issue 8: Parent Chain Breaking on Entity Reuse
**Problem:** `UIComponent.parent` was only set on creation, not updated on subsequent frames.  
**Fix Applied:** Always update parent in `_add_missing_components()`.  
**Better Solution:** This is an inherent issue with immediate-mode + retained entities. Consider making parent a frame-local property that's always set.

### Remaining Unsolved Issues

#### Close Buttons Have 0 Width
**Problem:** Buttons inside modal bodies consistently showed `container=0x19.999998` (0 width).  
**Analysis:** This appears to be a deeper layout issue where child elements in the modal body aren't getting proper width from their parent.

**Specific code that fails:**
```cpp
// In begin_modal - the body gets percent(1.0f) height
Size body_height = options.auto_size ? children() : percent(1.0f);
auto body = div(ctx, mk(modal_root.ent(), 1),
                ComponentConfig{}.with_size(ComponentSize{percent(1.0f), body_height}));

// In message_box - button inside body gets 0 width
if (button(ctx, mk(actions.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{pixels(120), pixels(36)})  // 120px width requested
               // But button ends up with 0 width!
```

**Suspected Causes:**
1. The `actions` row uses `percent(1.0f)` width - works fine
2. But its parent (body) uses `percent(1.0f)` which depends on modal_root
3. Modal_root uses `pixels(options.size.x)` - should be absolute, not circular
4. Something in the layout pass doesn't properly propagate the fixed pixel sizes

#### Title/Label Sizing with children()
**Problem:** Labels using `children()` for sizing get 0 because labels don't have child elements.  
**Workaround:** Use explicit heights like `h720(24)` instead.

```cpp
// This fails (label has 0 height):
div(ctx, mk(body.ent(), 0),
    ComponentConfig{}.with_label(message)
        .with_size(ComponentSize{percent(1.0f), children()}));  // children() = 0!

// Workaround:
div(ctx, mk(body.ent(), 0),
    ComponentConfig{}.with_label(message)
        .with_size(ComponentSize{percent(1.0f), h720(24)}));  // Explicit height
```

**Better Solution:** `children()` on a label should fall back to measuring text size. This requires adding a special case in the layout algorithm to check for `HasLabel` when `children()` is used.

#### Flex Layout Conflicts
**Problem:** In a flex row with `SpaceBetween`, if one child uses `percent(1.0f)` width, siblings get 0 width.

```cpp
// Modal header with title + close button:
auto header = div(ctx, mk(modal_root.ent(), 0),
    ComponentConfig{}.with_flex_direction(FlexDirection::Row)
                     .with_justify_content(JustifyContent::SpaceBetween));

// Title wants to fill remaining space:
div(ctx, mk(header.ent(), 0),
    ComponentConfig{}.with_size(ComponentSize{percent(1.0f), percent(1.0f)}));  // BAD

// Close button gets 0 width because sibling took 100%!
button(ctx, mk(header.ent(), 1),
    ComponentConfig{}.with_size(ComponentSize{pixels(32), pixels(32)}));  // Width = 0!
```

**Workaround:** Use `percent(0.9f)` for titles to leave room for close button.

```cpp
Size title_width = options.show_close_button ? percent(0.9f) : percent(1.0f);
```

**Better Solution:** Implement proper flex-basis/flex-grow/flex-shrink semantics like CSS Flexbox:

```cpp
// Desired behavior:
struct FlexItem {
  float flex_grow = 0;     // How much to grow when extra space
  float flex_shrink = 1;   // How much to shrink when not enough space
  Size flex_basis = auto_size();  // Initial size before flex
};

// Title: flex_grow=1 (take extra space), flex_basis=0 (no min)
// Button: flex_grow=0 (don't grow), flex_basis=32px (fixed)
```

---

## Recommended Implementation Approach

Based on the lessons from PR #26, here are recommended approaches for implementing modals:

### Approach A: Bypass Afterhours Layout (Like win95_widgets)

Keep modals completely outside the ECS/layout system:

```cpp
struct ModalManager {
  struct ModalState {
    bool active = false;
    std::string title;
    std::string message;
    ModalOptions options;
    std::function<void(DialogResult)> on_close;
  };
  
  std::vector<ModalState> modal_stack;
  
  void open_message(const std::string& title, const std::string& msg,
                    std::function<void(DialogResult)> callback);
  
  // Call in render loop, draws all modals on top of everything
  void render_and_update();
};
```

**Pros:**
- No layout system conflicts
- Simple implementation
- Predictable behavior

**Cons:**
- Doesn't use Afterhours theming
- Can't compose modals from regular UI widgets
- Separate code path to maintain

### Approach B: Pre-created Entity Pool

Create modal entities at initialization, show/hide them:

```cpp
// In screen initialization:
struct MyScreen {
  EntityID confirm_dialog_id = -1;
  bool show_confirm = false;
  DialogResult confirm_result = DialogResult::Pending;
  
  void init() {
    // Create entity once, configure size/position/children
    Entity& dialog = EntityHelper::createEntity();
    dialog.addComponent<UIComponent>(dialog.id)
      .set_desired_width(pixels(400))
      .set_desired_height(pixels(200));
    dialog.addComponent<ui::HasColor>(theme.background);
    dialog.addComponent<ShouldHide>(); // Start hidden
    confirm_dialog_id = dialog.id;
    
    // Create children (buttons, labels) similarly...
  }
  
  void render(UIContext& ctx) {
    Entity& dialog = EntityHelper::getEntityForIDEnforce(confirm_dialog_id);
    
    if (show_confirm) {
      dialog.removeComponentIfExists<ShouldHide>();
      // Update label text, check button clicks, etc.
    } else {
      dialog.addComponentIfMissing<ShouldHide>();
    }
  }
};
```

**Pros:**
- Uses existing layout system
- Entities are stable (no ID conflicts)
- Can use regular UI components

**Cons:**
- More boilerplate for setup
- Must manually manage show/hide
- Still has layout issues with nested components

### Approach C: Hybrid - Modal Shell + Direct Rendering Content

Use Afterhours for the modal container, but render content directly:

```cpp
bool begin_modal_hybrid(UIContext& ctx, EntityID container_id,
                        const std::string& title,
                        ModalOptions options) {
  Entity& container = EntityHelper::getEntityForIDEnforce(container_id);
  
  // Use Afterhours for:
  // - Container positioning (centered on screen)
  // - Background color
  // - Shadow/border
  // - Click-outside detection
  
  // But DON'T create child entities for buttons/labels
  // Instead, return the container rect so caller can draw with raylib
  
  return true; // modal is open
}

// Usage:
if (begin_modal_hybrid(ctx, modal_id, "Confirm", opts)) {
  Rectangle content_area = get_modal_content_rect(modal_id);
  
  // Draw content directly with raylib (like win95_widgets)
  DrawText("Are you sure?", content_area.x + 20, content_area.y + 40, 14, BLACK);
  
  if (DrawButton({...}, "OK")) {
    end_modal_hybrid(ctx, modal_id, DialogResult::Confirmed);
  }
  if (DrawButton({...}, "Cancel")) {
    end_modal_hybrid(ctx, modal_id, DialogResult::Cancelled);
  }
}
```

**Pros:**
- Modal positioning handled by layout
- Content has no layout conflicts (direct drawing)
- Flexible content (can draw anything)

**Cons:**
- Mixed paradigms (ECS + direct drawing)
- Theme integration requires manual work

### Approach D: Fix the Layout System (Long-term)

Address the underlying layout issues:

1. **Add screen-space rect calculation:**
```cpp
struct UIComponent {
  // ... existing fields ...
  
  // Returns rect with all parent transforms applied
  RectangleType screen_rect() const {
    RectangleType r = rect();
    // Walk parent chain, apply HasUIModifiers
    return r;
  }
};
```

2. **Handle circular sizing dependencies:**
```cpp
// In autolayout:
// If parent uses children() and child uses percent(), 
// resolve by giving child 0 size initially,
// then expand parent to minimum size,
// then re-layout children with known parent size
```

3. **Add proper flex layout:**
```cpp
struct FlexItem {
  float flex_grow = 0;
  float flex_shrink = 1;
  Size flex_basis = auto_size();
};
```

4. **Input consumption for modals:**
```cpp
struct UIContext {
  bool input_consumed = false;
  
  // Systems check this before processing
  bool consume_input() {
    if (input_consumed) return false;
    input_consumed = true;
    return true;
  }
};
```

---

## What Afterhours Currently Provides
- No modal/dialog component
- No focus trapping for modal interactions
- No backdrop/overlay rendering
- No built-in message box, confirm, or prompt dialogs

## Required Feature: Modal Dialog System

### Proposed Core Components

```cpp
namespace afterhours::ui {

// Modal state - blocks input to elements behind it
struct IsModal : BaseComponent {
  bool active = true;
  bool close_on_backdrop_click = false;
  bool close_on_escape = true;
  Color backdrop_color = {0, 0, 0, 128};  // Semi-transparent black
};

// Dialog result for async handling
enum class DialogResult {
  Pending,    // Dialog still open
  Confirmed,  // OK/Yes/Confirm clicked
  Cancelled,  // Cancel/No clicked or Escape pressed
  Dismissed,  // Backdrop clicked or X button
  Custom,     // Custom button clicked (check custom_result)
};

struct DialogState : BaseComponent {
  DialogResult result = DialogResult::Pending;
  int custom_result = -1;  // For dialogs with custom buttons
  std::string input_value;  // For input dialogs
};

} // namespace afterhours::ui
```

### Proposed Immediate Mode API

```cpp
namespace afterhours::ui::imm {

// Simple message box
// Returns true when closed, sets result
bool message_box(UIContext<InputAction>& ctx, EntityID id,
                 const std::string& title,
                 const std::string& message,
                 DialogResult& out_result);

// Confirmation dialog (OK/Cancel or Yes/No)
bool confirm_dialog(UIContext<InputAction>& ctx, EntityID id,
                    const std::string& title,
                    const std::string& message,
                    bool yes_no_buttons,  // true = Yes/No, false = OK/Cancel
                    DialogResult& out_result);

// Input prompt
bool input_dialog(UIContext<InputAction>& ctx, EntityID id,
                  const std::string& title,
                  const std::string& prompt,
                  std::string& in_out_value,
                  DialogResult& out_result);

// Generic modal container
// Usage:
// if (begin_modal(ctx, id, "Title")) {
//   // Draw dialog contents
//   if (button(ctx, id_ok, "OK").clicked) {
//     close_modal(ctx, id, DialogResult::Confirmed);
//   }
//   end_modal();
// }
bool begin_modal(UIContext<InputAction>& ctx, EntityID id,
                 const std::string& title,
                 ModalOptions options = {});
void end_modal();
void close_modal(UIContext<InputAction>& ctx, EntityID id, 
                 DialogResult result = DialogResult::Dismissed);

// Check if any modal is active (for input blocking)
bool is_modal_active(const UIContext<InputAction>& ctx);

} // namespace afterhours::ui::imm
```

### Modal Options

```cpp
struct ModalOptions {
  // Size
  Vector2 size = {400, 200};           // Dialog size
  bool auto_size = false;              // Size to content
  
  // Position
  bool center_on_screen = true;
  Vector2 position = {0, 0};           // If not centered
  
  // Behavior
  bool close_on_escape = true;
  bool close_on_backdrop_click = false;
  bool show_close_button = true;
  
  // Appearance
  Color backdrop_color = {0, 0, 0, 128};
  bool draggable = false;
};
```

### Rendering Considerations

The modal system needs to:
1. Render a backdrop over all other UI
2. Render the dialog on top of the backdrop
3. Block input to elements behind the backdrop
4. Handle focus trapping (Tab cycles within dialog)
5. Handle Escape key for closing

```cpp
// In rendering system
void render_modals(const UIContext<InputAction>& ctx) {
  for (auto& entity : get_modal_entities()) {
    const auto& modal = entity.get<IsModal>();
    if (!modal.active) continue;
    
    // Draw backdrop
    DrawRectangle(0, 0, screen_width, screen_height, modal.backdrop_color);
    
    // Draw dialog
    render(ctx, entity);
  }
}

// In input system
bool handle_input(UIContext<InputAction>& ctx) {
  // If modal is active, only process input for modal and its children
  if (is_modal_active(ctx)) {
    auto* modal_entity = get_top_modal(ctx);
    return process_input_for_entity(*modal_entity);
  }
  // Normal input processing
  return process_all_input();
}
```

### Usage Example

```cpp
// State
bool show_save_dialog = false;
DialogResult save_result = DialogResult::Pending;

// In update
if (save_button_clicked && document_dirty) {
  show_save_dialog = true;
  save_result = DialogResult::Pending;
}

// In render
if (show_save_dialog) {
  if (confirm_dialog(ctx, dialog_id,
                     "Unsaved Changes",
                     "Do you want to save before closing?",
                     true,  // Yes/No
                     save_result)) {
    show_save_dialog = false;
    if (save_result == DialogResult::Confirmed) {
      save_document();
    }
    close_application();
  }
}
```

## Additional Dialog Types to Consider

1. **File picker** - Open/Save file dialogs (may need native OS integration)
2. **Color picker** - Color selection dialog
3. **Font picker** - Font selection dialog  
4. **Progress dialog** - Modal with progress bar
5. **Multi-page wizard** - Dialog with multiple steps

## Integration with Other Gaps

### 08_scrollable_containers.md - Scrollable Modal Content

Modals may contain scrollable content (e.g., long EULA, settings lists):

```cpp
if (begin_modal(ctx, dialog_id, "License Agreement", {.size = {500, 400}})) {
  
  // Scrollable content area inside modal
  auto& scroll = entity.get<HasScrollView>();
  if (scroll.content_size.y > scroll.viewport_size.y) {
    // Show scroll indicator
  }
  
  begin_scroll_view(ctx, scroll_area_id);
  text_block(ctx, long_license_text);
  end_scroll_view();
  
  // Fixed footer with buttons (not scrolled)
  if (button(ctx, agree_id, "I Agree").clicked) {
    close_modal(ctx, dialog_id, DialogResult::Confirmed);
  }
  end_modal();
}
```

### 05_render_system_const_constraint.md - BLOCKED

Modal dialogs need mutable render access for:
- Closing when backdrop clicked
- Button hover/active state updates
- Escape key handling

This gap depends on **05** being resolved first.

## Notes
- Modal z-ordering: newest modal should be on top
- Multiple modals can stack (e.g., "Are you sure?" on top of a settings dialog)
- Animation: fade in/out for backdrop and dialog would be nice
- Accessibility: focus management, keyboard navigation, screen reader support

