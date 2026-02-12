# wordproc

A word processor built with C++23, the [afterhours](https://github.com/gabeochoa/afterhours) ECS framework, and [Sokol](https://github.com/floooh/sokol) for cross-platform rendering.

## Prerequisites

- **C++23 compiler** — Clang 16+ or GCC 13+
- **macOS** — Xcode command-line tools (for Metal frameworks)

## Building

### Native (macOS / Metal)

```bash
make
./output/wordproc.exe
```

### Web (WebAssembly / WebGL2)

First, install the Emscripten SDK (one-time setup):

```bash
./scripts/setup_emsdk.sh        # installs to ./emsdk/
source emsdk/emsdk_env.sh       # adds em++ to your PATH
```

Then build and serve:

```bash
make web
python3 -m http.server 8080 -d output/web
# Open http://localhost:8080/wordproc.html
```

### Clean

```bash
make clean          # native build artifacts
make clean-web      # web build artifacts
```

## Build architecture

The project uses platform-specific source files swapped at build time:

| | Native (Metal) | Web (WebGL2) |
|---|---|---|
| **Graphics** | `src/sokol_impl.mm` | `src/sokol_impl_web.cpp` |
| **Filesystem** | `vendor/afterhours/.../files.cpp` (uses `sago::platform_folders`) | same file (auto-detects `__EMSCRIPTEN__`, uses virtual FS paths) |

The native makefile target excludes `sokol_impl_web.cpp`. The web target includes all `src/*.cpp` files (which picks up the web variant) and skips `sokol_impl.mm`.

## Tests

```bash
make test              # unit tests
make e2e               # end-to-end tests
make benchmark         # performance benchmarks
```
