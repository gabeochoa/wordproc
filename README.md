# wordproc

A word processor built with C++23, [Afterhours](https://github.com/gabeochoa/afterhours), and Sokol/Metal, with an Emscripten web build.

## Build and run

Install Zig 0.16 and the Xcode command-line tools on macOS, then initialize dependencies:

```bash
git submodule update --init --recursive
nice -n 10 zig build -j2
nice -n 10 zig build run -j2
nice -n 10 zig build run -j2 -- test_files/public_domain/complete_shakespeare.txt
```

Zig owns the build graph and caches each C++ compilation using compiler dependency files, including vendored headers. Native compilation uses Apple Clang and the system macOS SDK. The app and resources are installed under `output/`. Run from the repository root or `output/` so resource paths resolve.

Use `zig build --help` for available steps. `-Dopt=2` enables optimized native builds, `-Dcoverage=true` enables LLVM coverage, `-Dmcp=false` disables MCP, and `-Ddebug-text-overflow=true` enables the red text-overflow diagnostics (off by default). `-Dcxx=/path/to/clang++` selects the native compiler. The makefile forwards common commands to Zig.

## Web

```bash
./scripts/setup_emsdk.sh
source emsdk/emsdk_env.sh
nice -n 10 zig build web -j2
nice -n 10 python3 -m http.server 8080 -d output/web
```

Open `http://localhost:8080/wordproc.html`. Emscripten compiles the WebGL2 backend and bundles the selected fonts, dictionaries, and templates. Use `-Demxx=/path/to/em++` when Emscripten is outside your PATH. The native application uses `src/sokol_impl.mm`; web uses `src/sokol_impl_web.cpp`.

## Verification

```bash
nice -n 10 zig build test -j2
nice -n 10 zig build test -j2 -- "[text_buffer]"
nice -n 10 zig build e2e -j2 -- --recursive basic/
nice -n 10 zig build bench-unit -j2
nice -n 10 zig build benchmark -j2
```

Use `-Dprofiling=true` to enable Afterhours’ bounded frame/system collector for its E2E performance commands:

```bash
nice -n 10 zig build run -j2 -Dprofiling=true -- --headless --test-script=tests/perf/afterhours_profile.e2e
nice -n 10 python3 scripts/verify_upgrade.py
```

The upgrade replay includes known failing document behavior and returns failure when any journey fails. See [upgrade verification](docs/upgrade-verification.md) for results and [Afterhours gaps](docs/afterhours-gaps.md) for upstream limitations. `scripts/verify_web.cjs` captures startup, typing, Find, dismissal, and resizing in Chrome; set `PLAYWRIGHT_MODULE` when Playwright is installed outside this project.

Unit tests run without a window. E2E tests drive the native application and capture screenshots. Arguments after `--` go to Catch2, the E2E runner, or the application, depending on the step. `zig build tests-build` only builds the test executable.

`make clean` clears compilation caches; `make clean-all` also removes native executables, and `make clean-web` removes generated web files.
