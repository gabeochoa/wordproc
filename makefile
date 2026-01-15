# Detect OS
UNAME_S := $(shell uname -s)

# Compiler settings
ifeq ($(UNAME_S),Darwin)
    CXX := clang++
    EXT := .exe
    RAYLIB_FLAGS := $(shell pkg-config --cflags raylib)
    RAYLIB_LIB := $(shell pkg-config --libs raylib)
    MACOS_FLAGS := -DBACKWARD
    FRAMEWORKS := -framework CoreFoundation
else ifeq ($(OS),Windows_NT)
    CXX := g++
    EXT := .exe
    RAYLIB_FLAGS := -IF:/RayLib/include
    RAYLIB_LIB := F:/RayLib/lib/raylib.dll
    MACOS_FLAGS :=
    FRAMEWORKS :=
else
    CXX := clang++
    EXT :=
    RAYLIB_FLAGS := $(shell pkg-config --cflags raylib)
    RAYLIB_LIB := $(shell pkg-config --libs raylib)
    MACOS_FLAGS :=
    FRAMEWORKS :=
endif

# C++ standard
CXXSTD := -std=c++23

# Base compiler flags
CXXFLAGS_BASE := -g \
    -Wall -Wextra -Wpedantic \
    -Wuninitialized -Wshadow -Wconversion \
    -Wcast-qual -Wchar-subscripts \
    -Wcomment -Wdisabled-optimization -Wformat=2 \
    -Wformat-nonliteral -Wformat-security -Wformat-y2k \
    -Wimport -Winit-self -Winline -Winvalid-pch \
    -Wlong-long -Wmissing-format-attribute \
    -Wmissing-include-dirs \
    -Wpacked -Wpointer-arith \
    -Wreturn-type -Wsequence-point \
    -Wstrict-overflow=5 -Wswitch -Wswitch-default \
    -Wswitch-enum -Wtrigraphs \
    -Wunused-label -Wunused-parameter -Wunused-value \
    -Wunused-variable -Wvariadic-macros -Wvolatile-register-var \
    -Wwrite-strings -Warray-bounds \
    -pipe \
    -fno-stack-protector \
    -fno-common

# Warning suppressions
CXXFLAGS_SUPPRESS := -Wno-deprecated-volatile -Wno-missing-field-initializers \
    -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
    -Wno-implicit-int-float-conversion -Wno-implicit-float-conversion \
    -Wno-format-nonliteral -Wno-format-security -Wno-format-y2k \
    -Wno-import -Wno-inline -Wno-invalid-pch \
    -Wno-long-long -Wno-missing-format-attribute \
    -Wno-missing-noreturn -Wno-packed -Wno-redundant-decls \
    -Wno-sequence-point -Wno-trigraphs -Wno-variadic-macros \
    -Wno-volatile-register-var

# Time tracing for ClangBuildAnalyzer
CXXFLAGS_TIME_TRACE := -ftime-trace

# Coverage flags
COVERAGE_CXXFLAGS :=
COVERAGE_LDFLAGS :=
ifeq ($(COVERAGE),1)
    ifeq ($(UNAME_S),Darwin)
        COVERAGE_CXXFLAGS := -fprofile-instr-generate -fcoverage-mapping
        COVERAGE_LDFLAGS := -fprofile-instr-generate
    else
        COVERAGE_CXXFLAGS := --coverage
        COVERAGE_LDFLAGS := --coverage
    endif
endif

# MCP support (enabled by default, disable with ENABLE_MCP=0)
ENABLE_MCP ?= 1
MCP_CXXFLAGS :=
ifeq ($(ENABLE_MCP),1)
    MCP_CXXFLAGS := -DAFTER_HOURS_ENABLE_MCP
endif

# Accessibility enforcement (warn and clamp small font sizes)
ACCESSIBILITY_CXXFLAGS := -DAFTERHOURS_ENFORCE_MIN_FONT_SIZE

# Debug text overflow (show red indicators when text can't fit in containers)
# Enabled by default, disable with DEBUG_TEXT_OVERFLOW=0
DEBUG_TEXT_OVERFLOW ?= 1
ifeq ($(DEBUG_TEXT_OVERFLOW),1)
    DEBUG_TEXT_OVERFLOW_CXXFLAGS := -DAFTERHOURS_DEBUG_TEXT_OVERFLOW
else
    DEBUG_TEXT_OVERFLOW_CXXFLAGS :=
endif

# Combine all CXXFLAGS
CXXFLAGS := $(CXXSTD) $(CXXFLAGS_BASE) $(CXXFLAGS_SUPPRESS) $(CXXFLAGS_TIME_TRACE) \
    $(MACOS_FLAGS) $(COVERAGE_CXXFLAGS) $(MCP_CXXFLAGS) $(ACCESSIBILITY_CXXFLAGS) \
    $(DEBUG_TEXT_OVERFLOW_CXXFLAGS) $(RAYLIB_FLAGS)

# Include directories (use -isystem for vendor to suppress their warnings)
INCLUDES := -isystem vendor/

# Library flags
LDFLAGS := -L. -Lvendor/ $(RAYLIB_LIB) $(FRAMEWORKS) $(COVERAGE_LDFLAGS)

# Directories
OBJ_DIR := output/objs
OUTPUT_DIR := output

# Source files
MAIN_SRC := $(wildcard src/*.cpp)
MAIN_SRC += $(wildcard src/components/*.cpp)
MAIN_SRC += $(wildcard src/systems/*.cpp)
MAIN_SRC += $(wildcard src/ui/*.cpp)
MAIN_SRC += $(wildcard src/testing/*.cpp)
MAIN_SRC += $(wildcard src/engine/*.cpp)

# Object files
MAIN_OBJS := $(MAIN_SRC:src/%.cpp=$(OBJ_DIR)/main/%.o)
MAIN_OBJS += $(OBJ_DIR)/main/vendor_afterhours_files.o

# Dependency files
MAIN_DEPS := $(MAIN_OBJS:.o=.d)

# Output executable
MAIN_EXE := $(OUTPUT_DIR)/ui_tester$(EXT)

# Create directories
$(OUTPUT_DIR)/.stamp:
	@mkdir -p $(OUTPUT_DIR)
	@touch $@

$(OBJ_DIR)/main:
	@mkdir -p $(OBJ_DIR)/main

# Default target
.DEFAULT_GOAL := all
all: $(MAIN_EXE)

# Main executable
$(MAIN_EXE): $(MAIN_OBJS) | $(OUTPUT_DIR)/.stamp
	@echo "Linking $(MAIN_EXE)..."
	$(CXX) $(CXXFLAGS) $(MAIN_OBJS) $(LDFLAGS) -o $@
	@echo "Built $(MAIN_EXE)"

# Include dependency files early to ensure header changes trigger rebuilds
-include $(MAIN_DEPS)

# Compile main object files
$(OBJ_DIR)/main/%.o: src/%.cpp | $(OBJ_DIR)/main
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MP -MF $(@:.o=.d) -MT $@

# Compile afterhours files.cpp
$(OBJ_DIR)/main/vendor_afterhours_files.o: vendor/afterhours/src/plugins/files.cpp | $(OBJ_DIR)/main
	@echo "Compiling vendor/afterhours/src/plugins/files.cpp..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MP -MF $(@:.o=.d) -MT $@

# Force dependency regeneration by removing dependency files
deps:
	@echo "Regenerating dependency files..."
	rm -f $(MAIN_DEPS)
	@echo "Dependency files removed - next build will regenerate them"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR)
	@echo "Clean complete"

clean-all: clean
	rm -f $(MAIN_EXE)
	@echo "Cleaned all"

# Resource copying
ifeq ($(UNAME_S),Darwin)
    mkdir_cmd := mkdir -p $(OUTPUT_DIR)/resources/
    cp_resources_cmd := cp -r resources/* $(OUTPUT_DIR)/resources/
    sign_cmd := codesign -s - -f --verbose --entitlements ent.plist
else ifeq ($(OS),Windows_NT)
    mkdir_cmd := powershell -command "& {&'New-Item' -Path .\ -Name $(OUTPUT_DIR)\resources -ItemType directory -ErrorAction SilentlyContinue}"
    cp_resources_cmd := powershell -command "& {&'Copy-Item' .\resources\* $(OUTPUT_DIR)\resources -ErrorAction SilentlyContinue}"
    sign_cmd :=
else
    mkdir_cmd := mkdir -p $(OUTPUT_DIR)/resources/
    cp_resources_cmd := cp -r resources/* $(OUTPUT_DIR)/resources/
    sign_cmd :=
endif

output: $(MAIN_EXE)
	$(mkdir_cmd)
	$(cp_resources_cmd)

sign: $(MAIN_EXE)
	$(sign_cmd) $(MAIN_EXE)

run: output
	./$(MAIN_EXE)

# Utility targets
.PHONY: all clean clean-all deps output sign run

# Code counting
count:
	git ls-files | grep "src" | grep -v "resources" | grep -v "vendor" | xargs wc -l | sort -rn | pr -2 -t -w 100

countall:
	git ls-files | xargs wc -l | sort -rn

# Static analysis
cppcheck:
	cppcheck src/ -Ivendor/afterhours --enable=all --std=c++23 --language=c++ \
		--suppress=noConstructor --suppress=noExplicitConstructor \
		--suppress=useStlAlgorithm --suppress=unusedStructMember \
		--suppress=useInitializationList --suppress=duplicateCondition \
		--suppress=nullPointerRedundantCheck --suppress=cstyleCast

.PHONY: count countall cppcheck

