// Standalone test: open a Metal window via afterhours::graphics::run()
// Build: make test-metal
// AFTER_HOURS_USE_METAL is set via -D flag from the Makefile

#include <afterhours/src/graphics/graphics.h>
#include <cstdio>
#include <chrono>

int main() {
    auto t0 = std::chrono::high_resolution_clock::now();
    fprintf(stderr, "[metal-test] starting...\n");

    afterhours::graphics::RunConfig cfg;
    cfg.width = 800;
    cfg.height = 600;
    cfg.title = "Metal Test - afterhours";
    cfg.target_fps = 60;

    int frame_count = 0;

    cfg.init = [&]() {
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        fprintf(stderr, "[metal-test] init callback fired (%.1f ms since main)\n", ms);
    };

    cfg.frame = [&]() {
        if (frame_count == 0) {
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            fprintf(stderr, "[metal-test] first frame start (%.1f ms since main)\n", ms);
        }

        afterhours::graphics::begin_drawing();
        // Any struct with r,g,b,a satisfies ColorLike — no extra includes needed
        struct { unsigned char r, g, b, a; } bg{40, 44, 52, 255};
        afterhours::graphics::clear_background(bg);
        afterhours::graphics::end_drawing();

        frame_count++;
        if (frame_count == 1) {
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            fprintf(stderr, "[metal-test] first frame done, w=%d h=%d (%.1f ms since main)\n",
                   afterhours::graphics::get_screen_width(),
                   afterhours::graphics::get_screen_height(), ms);
        }

        // Auto-quit after 60 frames (~1 second)
        if (frame_count >= 60) {
            afterhours::graphics::request_quit();
        }
    };

    cfg.cleanup = [&]() {
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        fprintf(stderr, "[metal-test] cleanup, %d frames, total %.1f ms\n", frame_count, ms);
    };

    afterhours::graphics::run(cfg);
    return 0;
}
