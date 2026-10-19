#pragma once

// Table > Formula... evaluation (no UI): SUM / AVERAGE / COUNT over
// ABOVE / LEFT numeric cells of the caret cell. A formula is
// `=SUM(ABOVE)` (the leading '=' is optional, function and range
// names are case-insensitive, surrounding whitespace is ignored).
// Non-numeric cells (gaps, text) are skipped, not errors; AVERAGE
// and SUM over no numeric cells are 0. Results format as integers
// when integral, else up to 10 significant digits ("%.10g").

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "table.h"

namespace table_formula {

struct FormulaResult {
    bool ok = false;
    std::string value;  // The result text, when ok
    std::string error;  // Human wording, when !ok
};

// Parse a cell's whole (trimmed) content as a number. Only a full
// parse counts: "12x" and "1,000" are text, not numbers.
inline bool parseNumericCell(const std::string& content, double& out) {
    std::size_t begin = 0;
    while (begin < content.size() &&
           std::isspace(static_cast<unsigned char>(content[begin]))) {
        ++begin;
    }
    std::size_t end = content.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(content[end - 1]))) {
        --end;
    }
    if (begin == end) return false;
    std::string trimmed = content.substr(begin, end - begin);
    const char* text = trimmed.c_str();
    char* endPtr = nullptr;
    double value = std::strtod(text, &endPtr);
    if (endPtr == text || *endPtr != '\0') return false;
    if (value != value) return false;  // NaN is not a table number
    out = value;
    return true;
}

inline std::string formatNumber(double value) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.10g", value);
    return std::string(buf);
}

inline std::string upperTrimmed(const std::string& value) {
    std::string out;
    for (char ch : value) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            out.push_back(
                static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }
    return out;
}

inline FormulaResult evaluate(const Table& table, std::size_t row,
                              std::size_t col, const std::string& formula) {
    FormulaResult result;
    if (row >= table.rowCount() || col >= table.colCount()) {
        result.error = "No cell at that position";
        return result;
    }
    std::string text = upperTrimmed(formula);
    if (!text.empty() && text[0] == '=') text.erase(text.begin());
    std::size_t open = text.find('(');
    std::size_t close = text.rfind(')');
    if (open == std::string::npos || close == std::string::npos ||
        close != text.size() - 1) {
        result.error = "Invalid formula (expected =SUM(ABOVE))";
        return result;
    }
    std::string function = text.substr(0, open);
    std::string range = text.substr(open + 1, close - open - 1);
    if (function != "SUM" && function != "AVERAGE" && function != "COUNT") {
        result.error = "Unknown function (SUM, AVERAGE, COUNT)";
        return result;
    }
    if (range != "ABOVE" && range != "LEFT") {
        result.error = "Unknown range (ABOVE, LEFT)";
        return result;
    }
    double sum = 0.0;
    std::size_t count = 0;
    auto visit = [&](const std::string& content) {
        double value = 0.0;
        if (parseNumericCell(content, value)) {
            sum += value;
            ++count;
        }
    };
    if (range == "ABOVE") {
        for (std::size_t r = 0; r < row; ++r) {
            visit(table.getCellContent(r, col));
        }
    } else {
        for (std::size_t c = 0; c < col; ++c) {
            visit(table.getCellContent(row, c));
        }
    }
    result.ok = true;
    if (function == "COUNT") {
        result.value = std::to_string(count);
    } else if (function == "AVERAGE") {
        result.value =
            formatNumber(count == 0 ? 0.0 : sum / static_cast<double>(count));
    } else {
        result.value = formatNumber(sum);
    }
    return result;
}

}  // namespace table_formula
