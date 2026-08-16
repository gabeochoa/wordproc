#pragma once

// Use wordproc's logging instead of afterhours' default
#include "log.h"

// Must be first among the afterhours headers - defines backend selection, which
// decides the layout of TextureType and friends.
#include "external.h"

#include <afterhours/src/library.h>
#include <afterhours/src/singleton.h>

#include <memory>

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
