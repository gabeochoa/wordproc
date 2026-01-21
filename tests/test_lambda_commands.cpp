#include "../vendor/afterhours/src/plugins/command_history.h"
#include "catch2/catch.hpp"

// Simple test state
struct TestState {
    int value = 0;
    std::string text = "";
    bool flag = false;
};

TEST_CASE("LambdaCommand - Basic functionality", "[command_history][lambda]") {
    TestState state;
    afterhours::CommandHistory<TestState> history;

    SECTION("execute and undo simple value change") {
        int old_value = state.value;
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 42; },
                [old_value](TestState &s) { s.value = old_value; },
                "Set value to 42"),
            state);

        REQUIRE(state.value == 42);
        REQUIRE(history.can_undo());

        history.undo(state);
        REQUIRE(state.value == 0);
        REQUIRE(history.can_redo());
    }

    SECTION("redo after undo") {
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 100; },
                [](TestState &s) { s.value = 0; }, "Set value"),
            state);

        REQUIRE(state.value == 100);

        history.undo(state);
        REQUIRE(state.value == 0);

        history.redo(state);
        REQUIRE(state.value == 100);
    }

    SECTION("description is preserved") {
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 10; },
                [](TestState &s) { s.value = 0; },
                "Custom description"),
            state);

        REQUIRE(history.next_undo_description() == "Custom description");
    }

    SECTION("description is mandatory") {
        // Description parameter is now required (no default)
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 10; },
                [](TestState &s) { s.value = 0; },
                "Required description"),
            state);

        REQUIRE(history.next_undo_description() == "Required description");
    }
}

TEST_CASE("LambdaCommand - Multiple state changes", "[command_history][lambda]") {
    TestState state;
    afterhours::CommandHistory<TestState> history;

    SECTION("modify multiple fields in one command") {
        auto old_value = state.value;
        auto old_text = state.text;
        auto old_flag = state.flag;

        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) {
                    s.value = 42;
                    s.text = "hello";
                    s.flag = true;
                },
                [old_value, old_text, old_flag](TestState &s) {
                    s.value = old_value;
                    s.text = old_text;
                    s.flag = old_flag;
                },
                "Batch update"),
            state);

        REQUIRE(state.value == 42);
        REQUIRE(state.text == "hello");
        REQUIRE(state.flag == true);

        history.undo(state);
        REQUIRE(state.value == 0);
        REQUIRE(state.text == "");
        REQUIRE(state.flag == false);
    }

    SECTION("sequence of commands") {
        // Command 1
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 10; },
                [](TestState &s) { s.value = 0; }, "Set to 10"),
            state);

        REQUIRE(state.value == 10);

        // Command 2
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 20; },
                [](TestState &s) { s.value = 10; }, "Set to 20"),
            state);

        REQUIRE(state.value == 20);

        // Command 3
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 30; },
                [](TestState &s) { s.value = 20; }, "Set to 30"),
            state);

        REQUIRE(state.value == 30);

        // Undo all
        history.undo(state);
        REQUIRE(state.value == 20);

        history.undo(state);
        REQUIRE(state.value == 10);

        history.undo(state);
        REQUIRE(state.value == 0);

        REQUIRE_FALSE(history.can_undo());
    }
}

TEST_CASE("LambdaCommand - Captured variables", "[command_history][lambda]") {
    TestState state;
    afterhours::CommandHistory<TestState> history;

    SECTION("capture by value") {
        int new_value = 100;
        int old_value = state.value;

        history.execute(
            afterhours::make_command<TestState>(
                [new_value](TestState &s) { s.value = new_value; },
                [old_value](TestState &s) { s.value = old_value; },
                "Captured change"),
            state);

        REQUIRE(state.value == 100);

        // Change the local variable (shouldn't affect command)
        new_value = 999;

        history.undo(state);
        REQUIRE(state.value == 0);
    }

    SECTION("capture by reference for external state") {
        struct ExternalData {
            int counter = 0;
        } external;

        history.execute(
            afterhours::make_command<TestState>(
                [&external](TestState &s) {
                    external.counter++;
                    s.value = 42;
                },
                [&external](TestState &s) {
                    external.counter--;
                    s.value = 0;
                },
                "External side effect"),
            state);

        REQUIRE(state.value == 42);
        REQUIRE(external.counter == 1);

        history.undo(state);
        REQUIRE(state.value == 0);
        REQUIRE(external.counter == 0);
    }
}

TEST_CASE("LambdaCommand - Complex state types", "[command_history][lambda]") {
    struct ComplexState {
        std::vector<int> numbers;
    };

    ComplexState state;
    afterhours::CommandHistory<ComplexState> history;

    SECTION("vector operations") {
        history.execute(
            afterhours::make_command<ComplexState>(
                [](ComplexState &s) { s.numbers.push_back(42); },
                [](ComplexState &s) { s.numbers.pop_back(); },
                "Add number"),
            state);

        REQUIRE(state.numbers.size() == 1);
        REQUIRE(state.numbers[0] == 42);

        history.undo(state);
        REQUIRE(state.numbers.empty());
    }

    SECTION("multiple vector operations") {
        // Add three numbers
        for (int i = 1; i <= 3; ++i) {
            history.execute(
                afterhours::make_command<ComplexState>(
                    [i](ComplexState &s) { s.numbers.push_back(i * 10); },
                    [](ComplexState &s) { s.numbers.pop_back(); },
                    "Add " + std::to_string(i * 10)),
                state);
        }

        REQUIRE(state.numbers.size() == 3);
        REQUIRE(state.numbers[0] == 10);
        REQUIRE(state.numbers[1] == 20);
        REQUIRE(state.numbers[2] == 30);

        // Undo all
        history.undo(state);
        history.undo(state);
        history.undo(state);
        REQUIRE(state.numbers.empty());
    }
}

TEST_CASE("LambdaCommand - History management", "[command_history][lambda]") {
    TestState state;
    afterhours::CommandHistory<TestState> history(3); // Max 3 commands

    SECTION("respects max depth") {
        for (int i = 1; i <= 5; ++i) {
            history.execute(
                afterhours::make_command<TestState>(
                    [i](TestState &s) { s.value = i; },
                    [i](TestState &s) { s.value = i - 1; },
                    "Set value " + std::to_string(i)),
                state);
        }

        REQUIRE(state.value == 5);
        REQUIRE(history.undo_count() == 3); // Should only keep last 3

        // Undo 3 times
        history.undo(state);
        REQUIRE(state.value == 4);

        history.undo(state);
        REQUIRE(state.value == 3);

        history.undo(state);
        REQUIRE(state.value == 2);

        // Can't undo more (first 2 commands were dropped)
        REQUIRE_FALSE(history.can_undo());
    }

    SECTION("clear history") {
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 42; },
                [](TestState &s) { s.value = 0; }, "Command"),
            state);

        REQUIRE(history.can_undo());

        history.clear();
        REQUIRE_FALSE(history.can_undo());
        REQUIRE_FALSE(history.can_redo());
    }

    SECTION("new action clears redo stack") {
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 10; },
                [](TestState &s) { s.value = 0; }, "First"),
            state);

        history.undo(state);
        REQUIRE(history.can_redo());

        // New action should clear redo
        history.execute(
            afterhours::make_command<TestState>(
                [](TestState &s) { s.value = 20; },
                [](TestState &s) { s.value = 0; }, "Second"),
            state);

        REQUIRE_FALSE(history.can_redo());
    }
}

TEST_CASE("LambdaCommand - Error handling", "[command_history][lambda]") {
    TestState state;
    afterhours::CommandHistory<TestState> history;

    SECTION("undo on empty history returns false") {
        REQUIRE_FALSE(history.undo(state));
    }

    SECTION("redo on empty redo stack returns false") {
        REQUIRE_FALSE(history.redo(state));
    }

    SECTION("next_undo_description on empty") {
        REQUIRE(history.next_undo_description() == "");
    }

    SECTION("next_redo_description on empty") {
        REQUIRE(history.next_redo_description() == "");
    }
}

