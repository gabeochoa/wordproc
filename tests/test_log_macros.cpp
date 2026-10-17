// Via src/log.h (not log/log.h directly): the wrapper pulls in fmt first,
// which log_me needs.
#include "../src/log.h"
#include "catch2/catch.hpp"

// log_* are macros that must behave as a single statement. A bare
// `if (...) log_me(...);` body breaks when the macro is an if/else branch:
// for log_warn/log_info/log_trace the trailing `else` binds to the macro's
// inner `if`, so the outer else never runs; for log_clean/log_if/log_ifx the
// macro's own terminator ends the outer `if` first, leaving a dangling
// `else` that fails to compile. That is exactly how upstream afterhours'
// crash_handler.h (if/else around log_error) failed to compile after the
// submodule bump. Verified: this test fails to compile against the old
// macros and passes against the new ones.
// log_error is excluded here on purpose: it assert(false)s by design, and
// its if/else use is covered by the main build including crash_handler.h.
TEST_CASE("log macros are single statements in if/else", "[logging]") {
    bool else_taken = false;
    if (false)
        log_warn("branch not taken");
    else
        else_taken = true;
    REQUIRE(else_taken);

    else_taken = false;
    if (false)
        log_info("branch not taken");
    else
        else_taken = true;
    REQUIRE(else_taken);

    else_taken = false;
    if (false)
        log_trace("branch not taken");
    else
        else_taken = true;
    REQUIRE(else_taken);

    else_taken = false;
    if (false)
        log_clean(LogLevel::LOG_TRACE, "branch not taken");
    else
        else_taken = true;
    REQUIRE(else_taken);

    else_taken = false;
    if (false)
        log_if(false, "branch not taken");
    else
        else_taken = true;
    REQUIRE(else_taken);

    else_taken = false;
    if (false)
        log_ifx(false, LogLevel::LOG_TRACE, "branch not taken");
    else
        else_taken = true;
    REQUIRE(else_taken);
}
