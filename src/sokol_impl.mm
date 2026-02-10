// sokol_impl.mm
// Objective-C++ file that compiles the Sokol implementations for Metal on macOS.
// This must be compiled as a single translation unit with SOKOL_IMPL defined.

#define SOKOL_IMPL
#define SOKOL_METAL
#define SOKOL_NO_ENTRY

#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_glue.h>
#include <sokol/sokol_time.h>
#include <sokol/sokol_log.h>

// 2D drawing and text rendering
#define SOKOL_GL_IMPL
#include <sokol/sokol_gl.h>

#define FONTSTASH_IMPLEMENTATION
#include <fontstash/stb_truetype.h>
#include <fontstash/fontstash.h>

#define SOKOL_FONTSTASH_IMPL
#include <sokol/sokol_fontstash.h>

// ── Screenshot support ──
// Uses macOS screencapture tool with the window ID to capture Metal content.
#import <AppKit/AppKit.h>

extern "C" void metal_take_screenshot(const char* filename) {
    @autoreleasepool {
        // Find our window
        NSWindow* window = [NSApp mainWindow];
        if (!window) {
            // Try keyWindow if mainWindow is nil
            window = [NSApp keyWindow];
        }
        if (!window) {
            NSLog(@"take_screenshot: no window available");
            return;
        }

        CGWindowID windowID = (CGWindowID)[window windowNumber];
        NSString* path = [NSString stringWithUTF8String:filename];

        // Use screencapture command-line tool with -l flag for window ID
        NSString* cmd = [NSString stringWithFormat:@"/usr/sbin/screencapture -x -o -l %u %@",
                         windowID, path];
        int ret = system([cmd UTF8String]);
        if (ret != 0) {
            NSLog(@"take_screenshot: screencapture failed with code %d", ret);
        }
    }
}
