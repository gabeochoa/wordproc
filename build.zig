const std = @import("std");

const app_sources = [_][]const u8{
    "src/editor/autocorrect.cpp",
    "src/editor/document_io.cpp",
    "src/editor/drawing.cpp",
    "src/editor/equation.cpp",
    "src/editor/export/export_html.cpp",
    "src/editor/export/export_pdf.cpp",
    "src/editor/export/export_rtf.cpp",
    "src/editor/gap_buffer.cpp",
    "src/editor/image.cpp",
    "src/editor/spellcheck.cpp",
    "src/editor/table.cpp",
    "src/editor/text_buffer.cpp",
    "src/editor/text_layout.cpp",
    "src/fonts/font_loader.cpp",
    "src/input/action_map.cpp",
    "src/main.cpp",
    "src/preload.cpp",
    "src/settings.cpp",
    "src/testing/e2e_runner.cpp",
    "src/ui/theme.cpp",
    "src/util/logging.cpp",
    "src/sokol_impl.mm",
    "src/util/file_dialog.mm",
    "vendor/afterhours/src/plugins/files.cpp",
};

const test_sources = [_][]const u8{
    "tests/benchmark_text_buffer.cpp",
    "tests/test_autocorrect.cpp",
    "tests/test_bookmark.cpp",
    "tests/test_border_rendering.cpp",
    "tests/test_borders_shading.cpp",
    "tests/test_chips.cpp",
    "tests/test_columns.cpp",
    "tests/test_comments.cpp",
    "tests/test_date_time.cpp",
    "tests/test_document_io.cpp",
    "tests/test_drawing.cpp",
    "tests/test_drop_cap.cpp",
    "tests/test_export.cpp",
    "tests/test_font_dialog.cpp",
    "tests/test_format_painter.cpp",
    "tests/test_format_validator.cpp",
    "tests/test_goto.cpp",
    "tests/test_grammar.cpp",
    "tests/test_hyperlink.cpp",
    "tests/test_image.cpp",
    "tests/test_lambda_commands.cpp",
    "tests/test_log_macros.cpp",
    "tests/test_main.cpp",
    "tests/test_menu_setup.cpp",
    "tests/test_menu_warm.cpp",
    "tests/test_measure_cache.cpp",
    "tests/test_new_window.cpp",
    "tests/test_normal_view.cpp",
    "tests/test_outline.cpp",
    "tests/test_p0_ux.cpp",
    "tests/test_page_setup.cpp",
    "tests/test_paragraph_dialog.cpp",
    "tests/test_perf_load.cpp",
    "tests/test_print_preview.cpp",
    "tests/test_repeat.cpp",
    "tests/test_revisions.cpp",
    "tests/test_ruler.cpp",
    "tests/test_scroll_math.cpp",
    "tests/test_spell_squiggles.cpp",
    "tests/test_spellcheck.cpp",
    "tests/test_spelling_dialog.cpp",
    "tests/test_symbol.cpp",
    "tests/test_table.cpp",
    "tests/test_table_formula.cpp",
    "tests/test_templates.cpp",
    "tests/test_text_buffer.cpp",
    "tests/test_text_input_utils.cpp",
    "tests/test_text_layout.cpp",
    "tests/test_thesaurus.cpp",
    "tests/test_toc.cpp",
    "tests/test_ui_theme.cpp",
    "tests/test_version_history.cpp",
    "src/editor/autocorrect.cpp",
    "src/editor/document_io.cpp",
    "src/editor/drawing.cpp",
    "src/editor/equation.cpp",
    "src/editor/export/export_html.cpp",
    "src/editor/export/export_pdf.cpp",
    "src/editor/export/export_rtf.cpp",
    "src/editor/gap_buffer.cpp",
    "src/editor/image.cpp",
    "src/editor/spellcheck.cpp",
    "src/editor/table.cpp",
    "src/editor/text_buffer.cpp",
    "src/editor/text_layout.cpp",
    "src/util/logging.cpp",
};

const warnings = [_][]const u8{
    "-g",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Wuninitialized",
    "-Wshadow",
    "-Wconversion",
    "-Wcast-qual",
    "-Wchar-subscripts",
    "-Wcomment",
    "-Wdisabled-optimization",
    "-Wformat=2",
    "-Wformat-nonliteral",
    "-Wformat-security",
    "-Wformat-y2k",
    "-Wimport",
    "-Winit-self",
    "-Winline",
    "-Winvalid-pch",
    "-Wlong-long",
    "-Wmissing-format-attribute",
    "-Wmissing-include-dirs",
    "-Wpacked",
    "-Wpointer-arith",
    "-Wreturn-type",
    "-Wsequence-point",
    "-Wstrict-overflow=5",
    "-Wswitch",
    "-Wswitch-default",
    "-Wswitch-enum",
    "-Wtrigraphs",
    "-Wunused-label",
    "-Wunused-parameter",
    "-Wunused-value",
    "-Wunused-variable",
    "-Wvariadic-macros",
    "-Wvolatile-register-var",
    "-Wwrite-strings",
    "-Warray-bounds",
    "-pipe",
    "-fno-stack-protector",
    "-fno-common",
    "-Wno-deprecated-volatile",
    "-Wno-missing-field-initializers",
    "-Wno-c99-extensions",
    "-Wno-unused-function",
    "-Wno-sign-conversion",
    "-Wno-implicit-int-float-conversion",
    "-Wno-implicit-float-conversion",
    "-Wno-format-nonliteral",
    "-Wno-format-security",
    "-Wno-format-y2k",
    "-Wno-import",
    "-Wno-inline",
    "-Wno-invalid-pch",
    "-Wno-long-long",
    "-Wno-missing-format-attribute",
    "-Wno-missing-noreturn",
    "-Wno-packed",
    "-Wno-redundant-decls",
    "-Wno-sequence-point",
    "-Wno-trigraphs",
    "-Wno-variadic-macros",
    "-Wno-volatile-register-var",
};

pub fn build(b: *std.Build) void {
    b.resolveInstallPrefix(b.pathFromRoot("output"), .{});
    const cxx = b.option([]const u8, "cxx", "Native C++ compiler") orelse "clang++";
    const emxx = b.option([]const u8, "emxx", "Emscripten C++ compiler") orelse "em++";
    const optimize = b.option(u8, "opt", "Native optimization level, 0 through 3") orelse 2;  // -O2 default: -O0 was the perf root cause (2026-10-19)
    if (optimize > 3) @panic("opt must be between 0 and 3");
    const coverage = b.option(bool, "coverage", "Enable LLVM coverage instrumentation") orelse false;
    const profiling = b.option(bool, "profiling", "Enable bounded Afterhours frame and system profiling") orelse false;
    const mcp = b.option(bool, "mcp", "Enable MCP support") orelse true;
    const overflow = b.option(bool, "debug-text-overflow", "Show text overflow diagnostics") orelse true;
    var flags = std.ArrayList([]const u8).empty;
    flags.appendSlice(b.allocator, &warnings) catch @panic("oom");
    flags.appendSlice(b.allocator, &.{ "-std=c++23", b.fmt("-O{d}", .{optimize}), "-DBACKWARD", "-include", "src/log.h", "-DAFTER_HOURS_ENABLE_E2E_TESTING", "-DAFTERHOURS_ENFORCE_MIN_FONT_SIZE", "-DAFTER_HOURS_UI_SINGLE_COLLECTION" }) catch @panic("oom");
    if (profiling) flags.append(b.allocator, "-DAFTERHOURS_ENABLE_PROFILING=1") catch @panic("oom");
    if (mcp) flags.append(b.allocator, "-DAFTER_HOURS_ENABLE_MCP") catch @panic("oom");
    if (overflow) flags.append(b.allocator, "-DAFTERHOURS_DEBUG_TEXT_OVERFLOW") catch @panic("oom");
    if (coverage) flags.appendSlice(b.allocator, &.{ "-fprofile-instr-generate", "-fcoverage-mapping" }) catch @panic("oom");
    const native_link = b.addSystemCommand(&.{cxx});
    native_link.setName("link wordproc");
    for (app_sources) |src| native_link.addFileArg(compile(b, cxx, src, flags.items, false));
    for ([_][]const u8{ "CoreFoundation", "Metal", "MetalKit", "Cocoa", "QuartzCore", "ImageIO", "UniformTypeIdentifiers" }) |fw| native_link.addArgs(&.{ "-framework", fw });
    if (coverage) native_link.addArg("-fprofile-instr-generate");
    native_link.addArg("-o");
    const native = native_link.addOutputFileArg("wordproc.exe");
    const app_install = b.addInstallFile(native, "wordproc.exe");
    const resources = b.addInstallDirectory(.{ .source_dir = b.path("resources"), .install_dir = .prefix, .install_subdir = "resources" });
    b.getInstallStep().dependOn(&app_install.step);
    b.getInstallStep().dependOn(&resources.step);
    const run = command(b, &.{b.pathFromRoot("output/wordproc.exe")}, b.getInstallStep());
    if (b.args) |args| run.addArgs(args);
    b.step("run", "Build and run wordproc; pass application arguments after --").dependOn(&run.step);

    const test_link = b.addSystemCommand(&.{cxx});
    test_link.setName("link unit tests");
    var test_flags = std.ArrayList([]const u8).empty;
    test_flags.appendSlice(b.allocator, &.{ "-std=c++23", "-O2", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Wno-deprecated-volatile", "-Wno-missing-field-initializers" }) catch @panic("oom");
    if (coverage) test_flags.appendSlice(b.allocator, &.{ "-fprofile-instr-generate", "-fcoverage-mapping" }) catch @panic("oom");
    for (test_sources) |src| test_link.addFileArg(compile(b, cxx, src, test_flags.items, false));
    if (coverage) test_link.addArg("-fprofile-instr-generate");
    test_link.addArg("-o");
    const test_install = b.addInstallFile(test_link.addOutputFileArg("run_tests.exe"), "run_tests.exe");
    b.step("tests-build", "Build unit tests without running them").dependOn(&test_install.step);
    const test_run = command(b, &.{b.pathFromRoot("output/run_tests.exe")}, &test_install.step);
    if (b.args) |args| test_run.addArgs(args);
    b.step("test", "Run unit tests; pass Catch2 arguments after --").dependOn(&test_run.step);
    const bench = command(b, &.{ b.pathFromRoot("output/run_tests.exe"), "[benchmark]", "--success" }, &test_install.step);
    b.step("bench-unit", "Run text buffer unit benchmarks").dependOn(&bench.step);
    b.step("bench", "Alias for bench-unit").dependOn(&bench.step);
    const recipes = [_][3][]const u8{
        .{ "e2e", "tests/run_e2e.sh", "Run scripted UI tests; pass runner arguments after --" },
        .{ "benchmark", "tests/run_benchmark.sh", "Measure headless document loading" },
        .{ "launch-benchmark", "tests/run_launch_benchmark.sh", "Measure native launch time" },
        .{ "profile-startup", "tests/run_sample_profile.sh", "Profile startup with macOS sample" },
    };
    for (recipes) |recipe| {
        const r = command(b, &.{ "bash", recipe[1] }, b.getInstallStep());
        if (b.args) |args| r.addArgs(args);
        b.step(recipe[0], recipe[2]).dependOn(&r.step);
    }

    const web_link = b.addSystemCommand(&.{emxx});
    web_link.setName("link wordproc web");
    const web_flags = &.{ "-std=c++23", "-O2", "-fexceptions", "-DAFTER_HOURS_USE_WEB", "-Isrc/web/compat", "-DAFTER_HOURS_USE_METAL", "-include", "src/log.h", "-DFMT_CONSTEVAL=", "-DAFTERHOURS_ENFORCE_MIN_FONT_SIZE", "-DAFTER_HOURS_UI_SINGLE_COLLECTION", "-Wno-deprecated-volatile" };
    for (app_sources) |src| {
        if (std.mem.endsWith(u8, src, "native_dialogs_macos.mm")) continue;
        if (std.mem.eql(u8, src, "src/sokol_impl.mm")) {
            web_link.addFileArg(compile(b, emxx, "src/sokol_impl_web.cpp", web_flags, true));
        } else {
            web_link.addFileArg(compile(b, emxx, src, web_flags, true));
        }
    }
    web_link.addArgs(&.{ "-O2", "-g2", "-fexceptions", "-sSTACK_SIZE=1048576", "-sSTACK_OVERFLOW_CHECK=2", "-sUSE_WEBGL2=1", "-sFULL_ES3=1", "-sALLOW_MEMORY_GROWTH=1", "-sINITIAL_MEMORY=67108864", "--shell-file" });
    web_link.addFileArg(b.path("src/web/shell.html"));
    for ([_][]const u8{ "resources/fonts/Roboto-Regular.ttf", "resources/fonts/EBGaramond-Regular.ttf", "resources/fonts/Gaegu-Bold.ttf", "resources/fonts/Fredoka-VariableFont_wdth,wght.ttf", "resources/fonts/BlackOpsOne-Regular.ttf", "resources/fonts/AtkinsonHyperlegible-Regular.ttf", "resources/fonts/eqprorounded-regular.ttf", "resources/fonts/SymbolsNerdFont-Regular.ttf", "resources/dictionaries/en_basic.txt", "resources/templates/essay.txt", "resources/templates/letter.txt", "resources/templates/memo.txt", "resources/templates/report.txt", "resources/templates/resume.txt" }) |asset| {
        web_link.addArg("--preload-file");
        web_link.addArg(b.fmt("{s}@/{s}", .{ b.pathFromRoot(asset), asset }));
        web_link.addFileInput(b.path(asset));
    }
    web_link.addArg("-o");
    const web_html = web_link.addOutputFileArg("wordproc.html");
    const web_dir = web_html.dirname();
    const web_install = b.addInstallDirectory(.{ .source_dir = web_dir, .install_dir = .prefix, .install_subdir = "web" });
    b.step("web", "Build WebAssembly with Emscripten").dependOn(&web_install.step);
}

fn compile(b: *std.Build, cxx: []const u8, src: []const u8, flags: []const []const u8, web: bool) std.Build.LazyPath {
    const r = b.addSystemCommand(&.{cxx});
    r.setName(b.fmt("compile {s}", .{src}));
    r.addArgs(flags);
    r.addArgs(&.{ "-isystem", b.pathFromRoot("vendor"), "-isystem", b.pathFromRoot("vendor/afterhours/vendor") });
    if (std.mem.endsWith(u8, src, ".mm")) r.addArgs(if (web) &.{ "-x", "c++" } else &.{ "-ObjC++" });
    r.addArg("-c");
    r.addFileArg(b.path(src));
    r.addArg("-o");
    const stem = std.fs.path.stem(src);
    const obj = r.addOutputFileArg(b.fmt("{s}.o", .{stem}));
    r.addArgs(&.{ "-MD", "-MF" });
    _ = r.addDepFileOutputArg(b.fmt("{s}.d", .{stem}));
    return obj;
}

fn command(b: *std.Build, args: []const []const u8, after: *std.Build.Step) *std.Build.Step.Run {
    const r = b.addSystemCommand(args);
    r.setCwd(b.path("."));
    r.has_side_effects = true;
    r.step.dependOn(after);
    return r;
}
