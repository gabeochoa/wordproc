#include "../src/ui/new_window.h"
#include "catch2/catch.hpp"

TEST_CASE("Window > New Window command", "[menu][window]") {
    SECTION("launches own exe with the current file, detached") {
        std::string cmd = window_menu::newWindowCommand(
            "/path/wordproc.exe", "/docs/notes.txt");
#if defined(_WIN32)
        REQUIRE(cmd == "start \"\" \"/path/wordproc.exe\" \"/docs/notes.txt\"");
#else
        REQUIRE(cmd ==
                "'/path/wordproc.exe' '/docs/notes.txt' >/dev/null 2>&1 &");
#endif
    }
    SECTION("no file: plain new instance, still detached") {
        std::string cmd =
            window_menu::newWindowCommand("/path/wordproc.exe", "");
#if defined(_WIN32)
        REQUIRE(cmd == "start \"\" \"/path/wordproc.exe\"");
#else
        REQUIRE(cmd == "'/path/wordproc.exe' >/dev/null 2>&1 &");
#endif
    }
    SECTION("no exe path: no command (nothing to launch)") {
        REQUIRE(window_menu::newWindowCommand("", "/docs/notes.txt") ==
                "");
        REQUIRE(window_menu::newWindowCommand("", "") == "");
    }
    SECTION("paths with spaces and quotes survive shell quoting") {
        std::string cmd = window_menu::newWindowCommand(
            "/my dir/wordproc.exe", "/my docs/a'b.txt");
#if defined(_WIN32)
        REQUIRE(cmd.find("\"/my dir/wordproc.exe\"") !=
                std::string::npos);
        REQUIRE(cmd.find("\"/my docs/a'b.txt\"") != std::string::npos);
#else
        REQUIRE(cmd.find("'/my dir/wordproc.exe'") != std::string::npos);
        REQUIRE(cmd.find("'/my docs/a'\\''b.txt'") != std::string::npos);
#endif
    }
}
