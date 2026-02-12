// sokol_impl_web.cpp
// Plain C++ file that compiles the Sokol implementations for WebGL2/Emscripten.
// Replaces sokol_impl.mm for WASM builds.

#define SOKOL_IMPL
#define SOKOL_GLES3
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

// Stubs for platform-specific functions called by backend.h
// These are not meaningful on the web platform.

extern "C" void metal_minimize_window() {
    // No-op on web
}

extern "C" void metal_take_screenshot(const char* /*filename*/) {
    // No-op on web
}
