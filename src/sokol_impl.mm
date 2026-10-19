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

// ── afterhours sokol backend impl ──
// Image decoding: stb_image rides along with this SOKOL_IMPL TU (the header
// defines STB_IMAGE_IMPLEMENTATION itself when SOKOL_IMPL is set). Including it
// here also emits the impl-TU sentinel that afterhours::graphics::run()
// references.
#include <afterhours/src/backends/sokol/image_decode.h>

// Metal device creation + render-texture readback/PNG export, called by
// afterhours' metal backend (metal_create_system_device, metal_capture_render_texture).
#define AFTER_HOURS_USE_METAL
#include <afterhours/src/backends/sokol/capture_impl.h>

// ── Screenshot support ──
// Uses macOS screencapture tool with the window ID to capture Metal content.
#import <AppKit/AppKit.h>

extern "C" void metal_set_window_size(int width, int height) {
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)sapp_macos_get_window();
        if (window) {
            // Size by CONTENT, not frame: setFrame: sizes the outer frame
            // (title bar included), so a resize to 1280x720 left only
            // 1280x688 of drawable area. The E2E `resize` command and
            // the layout system both treat the requested size as the
            // content size (it matches the size the window is created
            // with), and a resize test that restores 1280x720 must
            // restore the full 569px text area, not 537px. Convert via
            // frameRectForContentRect: and keep the current origin.
            NSRect frame = [window frame];
            NSRect contentRect = NSMakeRect(0, 0, (CGFloat)width, (CGFloat)height);
            NSRect frameRect = [window frameRectForContentRect:contentRect];
            NSRect newFrame = NSMakeRect(frame.origin.x, frame.origin.y,
                                         frameRect.size.width, frameRect.size.height);
            [window setFrame:newFrame display:YES animate:NO];
        }
    }
}

extern "C" void metal_minimize_window() {
    @autoreleasepool {
        NSWindow* window = (__bridge NSWindow*)sapp_macos_get_window();
        if (window) {
            [window miniaturize:nil];
        }
    }
}

extern "C" void metal_take_screenshot(const char* filename) {
    @autoreleasepool {
        // Find our window — try mainWindow, keyWindow, then fall back to
        // the first window in the app's window list (needed for test mode
        // where the window may not be key/main).
        NSWindow* window = [NSApp mainWindow];
        if (!window) {
            window = [NSApp keyWindow];
        }
        if (!window) {
            NSArray<NSWindow*>* windows = [NSApp windows];
            for (NSWindow* w in windows) {
                if ([w isVisible]) {
                    window = w;
                    break;
                }
            }
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

#include <afterhours/src/backends/sokol/capture_impl.h>

#include <afterhours/src/backends/sokol/image_decode.h>
