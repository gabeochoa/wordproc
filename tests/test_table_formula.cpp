#include "../src/editor/table.h"
#include "../src/editor/table_formula.h"

#include "catch2/catch.hpp"

// A small table for formula evaluation: column 0 holds numbers with
// a gap and a text cell; row 0 holds a LEFT range.
static Table formulaTable() {
    Table table(3, 3);
    table.setCellContent(0, 0, "10");
    table.setCellContent(0, 1, "20");
    table.setCellContent(0, 2, "30");
    table.setCellContent(1, 0, "5");
    table.setCellContent(1, 1, "not a number");
    table.setCellContent(2, 0, "");
    return table;
}

TEST_CASE("Table formula SUM(ABOVE) - skips gaps and non-numeric cells",
          "[table][formula]") {
    Table table = formulaTable();
    // Target (2, 0): ABOVE = 10, 5, "" (gap) -> 15.
    auto result = table_formula::evaluate(table, 2, 0, "=SUM(ABOVE)");
    REQUIRE(result.ok);
    REQUIRE(result.value == "15");
}

TEST_CASE("Table formula AVERAGE / COUNT over ABOVE", "[table][formula]") {
    Table table = formulaTable();
    SECTION("AVERAGE over 10 and 5") {
        auto result = table_formula::evaluate(table, 2, 0, "=AVERAGE(ABOVE)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "7.5");
    }
    SECTION("COUNT counts numeric cells only (gap + text skipped)") {
        auto result = table_formula::evaluate(table, 2, 0, "=COUNT(ABOVE)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "2");
    }
    SECTION("AVERAGE over no numeric cells is 0") {
        Table empty(2, 1);
        auto result = table_formula::evaluate(empty, 1, 0, "=AVERAGE(ABOVE)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "0");
    }
    SECTION("SUM over no numeric cells is 0, COUNT is 0") {
        Table empty(2, 1);
        auto sum = table_formula::evaluate(empty, 1, 0, "=SUM(ABOVE)");
        REQUIRE(sum.ok);
        REQUIRE(sum.value == "0");
        auto count = table_formula::evaluate(empty, 1, 0, "=COUNT(ABOVE)");
        REQUIRE(count.ok);
        REQUIRE(count.value == "0");
    }
}

TEST_CASE("Table formula LEFT range", "[table][formula]") {
    Table table = formulaTable();
    SECTION("SUM(LEFT) over 10 + 20") {
        auto result = table_formula::evaluate(table, 0, 2, "=SUM(LEFT)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "30");
    }
    SECTION("SUM(LEFT) on the first column is empty -> 0") {
        auto result = table_formula::evaluate(table, 0, 0, "=SUM(LEFT)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "0");
    }
    SECTION("COUNT(LEFT) skips text") {
        auto result = table_formula::evaluate(table, 1, 2, "=COUNT(LEFT)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "1");  // only "5" is numeric
    }
}

TEST_CASE("Table formula spelling - case, '=' and whitespace tolerant",
          "[table][formula]") {
    Table table = formulaTable();
    SECTION("Lowercase function and range, no '='") {
        auto result = table_formula::evaluate(table, 2, 0, "sum(above)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "15");
    }
    SECTION("Whitespace around tokens") {
        auto result =
            table_formula::evaluate(table, 2, 0, " = SUM ( ABOVE ) ");
        REQUIRE(result.ok);
        REQUIRE(result.value == "15");
    }
    SECTION("Negative and decimal numbers") {
        Table numbers(3, 1);
        numbers.setCellContent(0, 0, "-2.5");
        numbers.setCellContent(1, 0, "10");
        auto result = table_formula::evaluate(numbers, 2, 0, "=SUM(ABOVE)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "7.5");
    }
    SECTION("'12x' is text, not 12") {
        Table mixed(3, 1);
        mixed.setCellContent(0, 0, "12x");
        mixed.setCellContent(1, 0, "3");
        auto result = table_formula::evaluate(mixed, 2, 0, "=SUM(ABOVE)");
        REQUIRE(result.ok);
        REQUIRE(result.value == "3");
    }
}

TEST_CASE("Table formula errors", "[table][formula]") {
    Table table = formulaTable();
    SECTION("Unknown function") {
        auto result = table_formula::evaluate(table, 2, 0, "=MAX(ABOVE)");
        REQUIRE_FALSE(result.ok);
        REQUIRE_FALSE(result.error.empty());
    }
    SECTION("Unknown range") {
        auto result = table_formula::evaluate(table, 2, 0, "=SUM(BELOW)");
        REQUIRE_FALSE(result.ok);
        REQUIRE_FALSE(result.error.empty());
    }
    SECTION("Malformed formulas") {
        REQUIRE_FALSE(table_formula::evaluate(table, 2, 0, "SUM ABOVE").ok);
        REQUIRE_FALSE(table_formula::evaluate(table, 2, 0, "=SUM()").ok);
        REQUIRE_FALSE(table_formula::evaluate(table, 2, 0, "").ok);
        REQUIRE_FALSE(
            table_formula::evaluate(table, 2, 0, "=SUM(ABOVE) extra").ok);
    }
    SECTION("Target outside the table") {
        auto result = table_formula::evaluate(table, 9, 0, "=SUM(ABOVE)");
        REQUIRE_FALSE(result.ok);
    }
}
