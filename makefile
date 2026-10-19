ZIG ?= zig
JOBS ?= 2
ZIG_ARGS ?=
ARGS ?=
.DEFAULT_GOAL := all

.PHONY: all output run test test-verbose bench-unit e2e e2e-full benchmark launch-benchmark profile-startup web clean clean-all clean-web
all output:
	$(ZIG) build -j$(JOBS) $(ZIG_ARGS)
run test bench-unit e2e benchmark launch-benchmark profile-startup web:
	$(ZIG) build $@ -j$(JOBS) $(ZIG_ARGS) -- $(ARGS)
test-verbose:
	$(ZIG) build test -j$(JOBS) $(ZIG_ARGS) -- --success $(ARGS)
e2e-full:
	$(ZIG) build e2e -j$(JOBS) $(ZIG_ARGS) -- --recursive $(ARGS)
clean:
	rm -rf .zig-cache output/objs output/objs_web
clean-all: clean
	rm -f output/wordproc.exe output/run_tests.exe
clean-web:
	rm -f output/web/wordproc.html output/web/wordproc.js output/web/wordproc.wasm output/web/wordproc.data
