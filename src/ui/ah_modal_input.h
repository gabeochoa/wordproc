#pragma once

// Afterhours Modal Input Dialog
// Provides a modal dialog with a text input field
// Extends the Afterhours modal system with input capability

#include <afterhours/src/plugins/modal.h>
#include <afterhours/src/plugins/ui/text_input/text_input.h>
#include <string>

namespace ah_modal {

using namespace afterhours;
using namespace afterhours::ui;
using namespace afterhours::ui::imm;

// Import modal types from afterhours::modal struct
using Modal = afterhours::modal::Modal;
using ModalConfig = afterhours::ModalConfig;
using ModalResult = afterhours::modal::ModalResult;

// Result of an input dialog
struct InputDialogResult {
    DialogResult dialog_result = DialogResult::Pending;
    std::string value;  // The text entered by the user
    
    // Conversion to bool for easy checking
    explicit operator bool() const { return dialog_result == DialogResult::Confirmed; }
    
    // Check if cancelled
    bool cancelled() const { return dialog_result == DialogResult::Cancelled; }
    
    // Check if still pending
    bool pending() const { return dialog_result == DialogResult::Pending; }
};

// State holder for input dialog
struct InputDialogState {
    bool open = false;
    std::string value;
    std::string title;
    std::string prompt;
    std::string placeholder;
    std::string confirm_label = "OK";
    std::string cancel_label = "Cancel";
    
    void show(const std::string& t, const std::string& p, const std::string& initial_value = "",
              const std::string& placeholder_text = "") {
        open = true;
        title = t;
        prompt = p;
        value = initial_value;
        placeholder = placeholder_text;
    }
    
    void hide() {
        open = false;
    }
};

// Render an input dialog modal
// Returns the dialog result and the entered text
template<typename UIContextT>
InputDialogResult input_dialog(
    UIContextT& ctx,
    Entity& parent,
    InputDialogState& state
) {
    InputDialogResult result;
    
    if (!state.open) {
        return result;
    }
    
    constexpr int CONTENT_LAYER = 1001;
    
    // Create modal
    auto modal_result = afterhours::modal::detail::modal_impl(
        ctx, mk(parent, 9999),
        state.open,
        ModalConfig{}
            .with_size(h720(400), h720(180))
            .with_title(state.title)
            .with_show_close_button(true)
    );
    
    if (modal_result) {
        // Prompt text
        if (!state.prompt.empty()) {
            div(ctx, mk(modal_result.ent(), 0),
                ComponentConfig{}
                    .with_label(state.prompt)
                    .with_size(ComponentSize{percent(1.0f), children()})
                    .with_padding(Padding{
                        .top = pixels(4),
                        .right = pixels(8),
                        .bottom = pixels(8),
                        .left = pixels(8)
                    })
                    .with_render_layer(CONTENT_LAYER));
        }
        
        // Text input field
        auto input_config = ComponentConfig{}
            .with_size(ComponentSize{percent(1.0f), h720(32)})
            .with_roundness(0.0f)  // Win95 style
            .with_background(Theme::Usage::Background)
            .with_margin(Margin{
                .top = pixels(4),
                .right = pixels(8),
                .bottom = pixels(8),
                .left = pixels(8)
            })
            .with_render_layer(CONTENT_LAYER);
        
        if (!state.placeholder.empty()) {
            input_config.with_label(state.placeholder);
        }
        
        afterhours::text_input::text_input(ctx, mk(modal_result.ent(), 1), state.value, input_config);
        
        // Button row
        auto button_row = div(ctx, mk(modal_result.ent(), 2),
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.0f), h720(44)})
                .with_flex_direction(FlexDirection::Row)
                .with_justify_content(JustifyContent::Center)
                .with_align_items(AlignItems::Center)
                .with_flex_wrap(FlexWrap::NoWrap)
                .with_render_layer(CONTENT_LAYER));
        
        // OK button
        if (button(ctx, mk(button_row.ent(), 0),
                   ComponentConfig{}
                       .with_label(state.confirm_label)
                       .with_size(ComponentSize{h720(80), h720(32)})
                       .with_background(Theme::Usage::Primary)
                       .with_roundness(0.0f)  // Win95 style
                       .with_margin(Margin{
                           .left = DefaultSpacing::small(),
                           .right = DefaultSpacing::small()
                       })
                       .with_render_layer(CONTENT_LAYER))) {
            result.dialog_result = DialogResult::Confirmed;
            result.value = state.value;
            state.open = false;
        }
        
        // Cancel button
        if (button(ctx, mk(button_row.ent(), 1),
                   ComponentConfig{}
                       .with_label(state.cancel_label)
                       .with_size(ComponentSize{h720(80), h720(32)})
                       .with_roundness(0.0f)  // Win95 style
                       .with_margin(Margin{
                           .left = DefaultSpacing::small(),
                           .right = DefaultSpacing::small()
                       })
                       .with_render_layer(CONTENT_LAYER))) {
            result.dialog_result = DialogResult::Cancelled;
            state.open = false;
        }
    }
    
    // Check if modal was closed by escape/backdrop
    if (!state.open && modal_result.ent().template has<Modal>()) {
        Modal& m = modal_result.ent().template get<Modal>();
        if (m.result != DialogResult::Pending) {
            result.dialog_result = m.result;
        }
    }
    
    return result;
}

// Convenience function for simple input dialogs
// Similar to win95::DrawInputDialog but using Afterhours
template<typename UIContextT>
InputDialogResult input(
    UIContextT& ctx,
    Entity& parent,
    bool& open,
    const std::string& title,
    const std::string& prompt,
    std::string& value,
    const std::string& confirm_label = "OK",
    const std::string& cancel_label = "Cancel"
) {
    static InputDialogState static_state;  // Note: This only works for one dialog at a time
    
    if (open && !static_state.open) {
        static_state.show(title, prompt, value);
        static_state.confirm_label = confirm_label;
        static_state.cancel_label = cancel_label;
    }
    
    auto result = input_dialog(ctx, parent, static_state);
    
    if (result.dialog_result != DialogResult::Pending) {
        open = false;
        if (result.dialog_result == DialogResult::Confirmed) {
            value = result.value;
        }
    }
    
    return result;
}

}  // namespace ah_modal

