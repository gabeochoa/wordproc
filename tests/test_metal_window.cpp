// Standalone test: Metal window with input verification
// Build: make output/test_metal.exe
// AFTER_HOURS_USE_METAL is set via -D flag from the Makefile

#include <afterhours/src/graphics/graphics.h>
#include <afterhours/src/core/key_codes.h>
#include <cstdio>
#include <chrono>

namespace gfx = afterhours::graphics;
using API = gfx::MetalPlatformAPI;

int main() {
    auto t0 = std::chrono::high_resolution_clock::now();
    fprintf(stderr, "[metal-test] starting...\n");

    gfx::RunConfig cfg;
    cfg.width = 800;
    cfg.height = 600;
    cfg.title = "Metal Input Test - press keys, move mouse, ESC to quit";
    cfg.target_fps = 60;

    int frame_count = 0;

    cfg.init = [&]() {
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        fprintf(stderr, "[metal-test] init (%.1f ms)\n", ms);
    };

    cfg.frame = [&]() {
        gfx::begin_drawing();
        struct { unsigned char r, g, b, a; } bg{40, 44, 52, 255};
        gfx::clear_background(bg);
        gfx::end_drawing();

        frame_count++;

        // Print mouse position every 60 frames
        if (frame_count % 60 == 0) {
            auto pos = API::get_mouse_position();
            fprintf(stderr, "[frame %d] mouse=(%.0f, %.0f)\n",
                    frame_count, pos.x, pos.y);
        }

        // Report any key presses
        for (int k = 0; k < 512; k++) {
            if (API::is_key_pressed(k)) {
                fprintf(stderr, "[frame %d] key pressed: %d\n", frame_count, k);
            }
        }

        // Report mouse clicks
        for (int b = 0; b < 3; b++) {
            if (API::is_mouse_button_pressed(b)) {
                auto pos = API::get_mouse_position();
                fprintf(stderr, "[frame %d] mouse button %d clicked at (%.0f, %.0f)\n",
                        frame_count, b, pos.x, pos.y);
            }
        }

        // Report scroll
        float scroll = API::get_mouse_wheel_move();
        if (scroll != 0.f) {
            fprintf(stderr, "[frame %d] scroll: %.2f\n", frame_count, scroll);
        }

        // Report char input
        int ch;
        while ((ch = API::get_char_pressed()) != 0) {
            fprintf(stderr, "[frame %d] char: '%c' (%d)\n", frame_count, (char)ch, ch);
        }

        // ESC to quit
        if (API::is_key_pressed(afterhours::keys::ESCAPE)) {
            fprintf(stderr, "[metal-test] ESC pressed, quitting\n");
            gfx::request_quit();
        }
    };

    cfg.cleanup = [&]() {
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        fprintf(stderr, "[metal-test] cleanup, %d frames, %.1f ms total\n", frame_count, ms);
    };

    gfx::run(cfg);
    return 0;
}
