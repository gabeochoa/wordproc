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

// Image decoding: image_decode.h defines STB_IMAGE_IMPLEMENTATION off SOKOL_IMPL,
// so stb rides along with this TU, and it emits the impl-TU sentinel that
// afterhours::graphics::run() references. The native TU (sokol_impl.mm) does the
// same; without it the web build fails at link exactly as the native one did.
// No capture_impl.h here: that is Metal-only.
#include <afterhours/src/backends/sokol/image_decode.h>

// Stubs for platform-specific functions called by backend.h
// These are not meaningful on the web platform.

extern "C" void metal_minimize_window() {
    // No-op on web
}

extern "C" void metal_take_screenshot(const char* /*filename*/) {
    // No-op on web
}
