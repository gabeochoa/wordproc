#pragma once

// <cstdlib> BEFORE the fmt headers: fmt 11.2's allocator uses
// unqualified malloc/free, and under clang 21 + the macOS 26 SDK
// a TU that instantiates fmt::format without <cstdlib> first
// fails two-phase lookup ("use of undeclared identifier
// 'malloc'"). Real TUs include it transitively before any
// instantiation, so the build is green — this pins the order
// for a future TU that uses fmt directly.
#include <cstdlib>

#define FMT_HEADER_ONLY
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>  // fmt 11: std::filesystem::path formatter (was implicit in fmt 8)
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#define AFTER_HOURS_REPLACE_LOGGING
#define AFTER_HOURS_LOG_WITH_COLOR
#include "log/log.h"
