#pragma once

// All log_* macros expand to a single do { ... } while (0) statement.
// A bare `if (...) log_me(...);` body is not statement-safe: it breaks when
// a caller uses the macro as an if/else branch. Upstream afterhours does
// exactly that since it added src/crash_handler.h, whose report() calls
// log_error() on both sides of an if/else — the trailing `else` then had no
// matching `if` and the build failed. AFTER_HOURS_REPLACE_LOGGING documents
// log_* as functions; these macros must at least behave like one statement.
#define log_trace(...)                                                        \
    do {                                                                      \
        if (static_cast<int>(LogLevel::LOG_TRACE) >=                          \
            static_cast<int>(AFTER_HOURS_LOG_LEVEL))                          \
            log_me(LogLevel::LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__);     \
    } while (0)

#define log_info(...)                                                         \
    do {                                                                      \
        if (static_cast<int>(LogLevel::LOG_INFO) >=                           \
            static_cast<int>(AFTER_HOURS_LOG_LEVEL))                          \
            log_me(LogLevel::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__);      \
    } while (0)
#define log_warn(...)                                                         \
    do {                                                                      \
        if (static_cast<int>(LogLevel::LOG_WARN) >=                           \
            static_cast<int>(AFTER_HOURS_LOG_LEVEL))                          \
            log_me(LogLevel::LOG_WARN, __FILE__, __LINE__, __VA_ARGS__);      \
    } while (0)
#define log_error(...)                                                        \
    do {                                                                      \
        if (static_cast<int>(LogLevel::LOG_ERROR) >=                          \
            static_cast<int>(AFTER_HOURS_LOG_LEVEL))                          \
            log_me(LogLevel::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);     \
        assert(false);                                                        \
    } while (0)

#define log_clean(level, ...)                                                 \
    do {                                                                      \
        if (static_cast<int>(level) >=                                        \
            static_cast<int>(AFTER_HOURS_LOG_LEVEL))                          \
            log_me(level, "", -1, __VA_ARGS__);                               \
    } while (0)

#define log_if(x, ...)                                                        \
    do {                                                                      \
        if (x) log_me(LogLevel::LOG_IF, __FILE__, __LINE__, __VA_ARGS__);     \
    } while (0)

#define log_ifx(x, level, ...)                                                \
    do {                                                                      \
        if (x) log_me(level, __FILE__, __LINE__, __VA_ARGS__);                \
    } while (0)

#define log_once_per(interval, level, ...) \
    log_once_per(interval, level, __FILE__, __LINE__, __VA_ARGS__)
