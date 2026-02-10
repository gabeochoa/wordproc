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
