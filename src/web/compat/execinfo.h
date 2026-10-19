#pragma once

#ifndef __EMSCRIPTEN__
#error Web compatibility headers require Emscripten
#endif

#include <emscripten/emscripten.h>
#include <stdio.h>

inline int backtrace(void**, int) {
    char stack[4096];
    emscripten_get_callstack(EM_LOG_C_STACK | EM_LOG_JS_STACK, stack, sizeof(stack));
    fputs(stack, stderr);
    return 0;
}

inline char** backtrace_symbols(void* const*, int) { return nullptr; }
