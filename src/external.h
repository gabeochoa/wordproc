#pragma once

#include "std_include.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif

// ── Backend: Metal ──
#ifndef AFTER_HOURS_USE_METAL
#define AFTER_HOURS_USE_METAL
#endif

// ── Custom logging (must precede afterhours headers) ──
#include "log.h"

// ── Single afterhours include (graphics, input, types, e2e testing) ──
#include <afterhours/src/plugins/e2e_testing/platform_test_input.h>

// ── Project-level namespace aliases ──
namespace test_input {
    using namespace afterhours::testing::platform_input;
    inline bool& test_mode = afterhours::testing::test_input::detail::test_mode;
}

namespace input_injector {
    using namespace afterhours::testing::input_injector;
}

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
