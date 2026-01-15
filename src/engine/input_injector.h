#pragma once

#include "../rl.h"

namespace input_injector {

void schedule_mouse_click_at(const raylib::Rectangle &rect);
void inject_scheduled_click();
void release_scheduled_click();
void inject_key_press(int keycode);
void hold_key_for_duration(int keycode, float duration);
void set_key_down(int keycode);
void set_key_up(int keycode);
bool consume_synthetic_press(int keycode);
void update_key_hold(float dt);
bool is_key_synthetically_down(int keycode);

void set_mouse_position(int x, int y);
vec2 get_mouse_position();
bool is_mouse_button_down(int button);
bool is_mouse_button_pressed(int button);
bool is_mouse_button_released(int button);

void reset_frame();

} // namespace input_injector



