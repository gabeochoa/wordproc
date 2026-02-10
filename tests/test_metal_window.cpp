// Standalone test: Metal window with 2D drawing
// Build: make output/test_metal.exe
// AFTER_HOURS_USE_METAL is set via -D flag from the Makefile

#include <afterhours/src/graphics/graphics.h>
#include <afterhours/src/drawing_helpers.h>
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
    cfg.title = "Metal Drawing Test - ESC to quit";
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

        // Draw some colored rectangles
        afterhours::draw_rectangle(
            RectangleType{50, 50, 200, 100},
            afterhours::Color{220, 50, 50, 255});  // Red

        afterhours::draw_rectangle(
            RectangleType{300, 50, 200, 100},
            afterhours::Color{50, 180, 50, 255});  // Green

        afterhours::draw_rectangle(
            RectangleType{550, 50, 200, 100},
            afterhours::Color{50, 100, 220, 255}); // Blue

        // Draw an outline
        afterhours::draw_rectangle_outline(
            RectangleType{50, 200, 700, 150},
            afterhours::Color{255, 255, 255, 255}, 2.0f);

        // Draw some lines
        afterhours::draw_line(50, 400, 750, 400,
            afterhours::Color{255, 200, 50, 255});  // Yellow line

        // Draw a circle
        afterhours::draw_circle(400, 480, 50.0f,
            afterhours::Color{200, 100, 255, 255});  // Purple circle

        // Draw a triangle
        afterhours::draw_triangle(
            Vector2Type{650, 500},
            Vector2Type{750, 550},
            Vector2Type{650, 550},
            afterhours::Color{50, 220, 220, 255});  // Cyan triangle

        gfx::end_drawing();

        frame_count++;
        if (frame_count == 1) {
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            fprintf(stderr, "[metal-test] first frame (%.1f ms), w=%d h=%d\n",
                   ms, gfx::get_screen_width(), gfx::get_screen_height());
        }

        // ESC to quit
        if (API::is_key_pressed(afterhours::keys::ESCAPE)) {
            gfx::request_quit();
        }
    };

    cfg.cleanup = [&]() {
        fprintf(stderr, "[metal-test] cleanup, %d frames\n", frame_count);
    };

    gfx::run(cfg);
    return 0;
}
