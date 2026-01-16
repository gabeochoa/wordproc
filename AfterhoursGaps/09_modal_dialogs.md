# Modal Dialogs

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

## Notes
- Modal z-ordering: newest modal should be on top
- Multiple modals can stack (e.g., "Are you sure?" on top of a settings dialog)
- Animation: fade in/out for backdrop and dialog would be nice
- Accessibility: focus management, keyboard navigation, screen reader support

