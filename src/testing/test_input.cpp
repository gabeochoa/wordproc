// Provides implementations for test_input functions
// This file exists so that the inline functions in test_input.h are
// compiled into an object file for linking.

#include "test_input.h"

// Force instantiation of inline functions by taking their address
// This ensures they have definitions in the final binary
namespace test_input {

// Explicit instantiations to satisfy linker
auto* _is_mouse_button_pressed_ptr = &is_mouse_button_pressed;
auto* _is_mouse_button_down_ptr = &is_mouse_button_down;
auto* _is_mouse_button_released_ptr = &is_mouse_button_released;
auto* _is_mouse_button_up_ptr = &is_mouse_button_up;
auto* _get_char_pressed_ptr = &get_char_pressed;
auto* _is_key_pressed_ptr = &is_key_pressed;
auto* _get_mouse_position_fwd_ptr = &get_mouse_position_fwd;
auto* _get_mouse_wheel_move_ptr = &get_mouse_wheel_move;

}  // namespace test_input

