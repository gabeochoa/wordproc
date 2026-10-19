#include "catch2/catch.hpp"
#include "../src/util/clipboard.h"
#include <afterhours/src/plugins/e2e_testing/native_dialog_responses.h>

TEST_CASE("editor and framework share the scoped clipboard", "[platform]") {
    afterhours::clipboard::MemoryProvider outer;
    afterhours::clipboard::ScopedProvider outer_scope(outer);
    app::clipboard::set_text("outer clipboard");
    {
        afterhours::clipboard::MemoryProvider session;
        afterhours::clipboard::ScopedProvider scope(session);
        app::clipboard::set_text("editor selection");
        REQUIRE(afterhours::clipboard::get_text() == "editor selection");
        afterhours::clipboard::set_text("dialog selection");
        REQUIRE(app::clipboard::get_text() == "dialog selection");
    }
    REQUIRE(app::clipboard::get_text() == "outer clipboard");
}

TEST_CASE("dialog requests preserve result identity and consume once", "[platform]") {
    using namespace afterhours::native_dialogs;
    afterhours::testing::NativeDialogResponses responses;
    responses.push(Selected{"document.wpdoc"});
    responses.push(Cancelled{});
    responses.push(Unsupported{});
    responses.push(Error{"Unavailable"});
    Queue queue{[&](const Request& request) { return responses(request); }};
    const auto opened = queue.submit({.kind = Kind::OpenFile});
    const auto cancelled = queue.submit({.kind = Kind::SaveFile});
    const auto unsupported = queue.submit({});
    const auto failed = queue.submit({});
    REQUIRE_FALSE(queue.take_result(opened).has_value());
    queue.process_pending();
    REQUIRE(std::holds_alternative<Cancelled>(*queue.take_result(cancelled)));
    REQUIRE(std::get<Selected>(*queue.take_result(opened)).path == "document.wpdoc");
    REQUIRE_FALSE(queue.take_result(opened).has_value());
    REQUIRE(std::holds_alternative<Unsupported>(*queue.take_result(unsupported)));
    REQUIRE(std::get<Error>(*queue.take_result(failed)).message == "Unavailable");
}
