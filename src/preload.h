#pragma once

#include <afterhours/src/library.h>
#include <afterhours/src/singleton.h>

#include <memory>

#include "external.h"  // Must be first - defines AFTER_HOURS_USE_RAYLIB

SINGLETON_FWD(Preload)
struct Preload {
    SINGLETON(Preload)

    Preload();
    ~Preload();

    Preload(const Preload &) = delete;
    void operator=(const Preload &) = delete;

    Preload &init(const char *title);
    Preload &make_singleton();
};
