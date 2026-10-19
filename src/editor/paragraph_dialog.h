#pragma once

// Format > Paragraph... dialog model (Word 95 parity).
// Pure field <-> stored-value mapping, unit-tested; the dialog in
// MenuUISystem owns the field strings and applies the result through
// TextBuffer::setLineParagraphFormat + commitParagraphFormat (one
// undo step, like the ruler drag's commitParagraphIndents).
//
// Stored units are the TextBuffer ones: indents in 72-dpi units
// (72 = 1 inch, see ruler_math), spacing in points (1pt = 1 stored
// unit at scale 1, the same unit the renderer adds for
// spaceBefore/spaceAfter). Tab stops are the document-wide
// DocumentSettings.tabWidth (characters per tab): per-paragraph tab
// stops do not exist, so the dialog has no tab-stops field —
// Tabs... opens the Tab Width dialog, which edits the default.

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

#include "document_settings.h"  // TextAlignment for the Alignment dropdown

namespace paragraph_dialog {

// Points (stored units) per inch.
inline constexpr float POINTS_PER_INCH = 72.0f;
// Stored-unit clamp for an indent field: 0..20 inches.
inline constexpr int MAX_INDENT_STORED = 20 * 72;
// Stored-unit clamp for a spacing field: 0..720pt (10 inches).
inline constexpr int MAX_SPACING_PT = 720;
// Tab-width range, matching the Tab Width... dialog (1-16).
inline constexpr int MIN_TAB_STOPS = 1;
inline constexpr int MAX_TAB_STOPS = 16;

// Line-spacing dropdown entries, in dropdown order.
inline constexpr const char* kLineSpacingNames[3] = {"Single", "1.5 lines",
                                                      "Double"};
inline constexpr float kLineSpacingValues[3] = {1.0f, 1.5f, 2.0f};

// Alignment dropdown entries, in dropdown order (Word 95 labels).
// Index maps to TextAlignment: Left / Center / Right / Justify.
inline constexpr const char* kAlignmentNames[4] = {"Left", "Centered",
                                                    "Right", "Justified"};

// Special-indent dropdown entries, in dropdown order.
inline constexpr int kSpecialNone = 0;
inline constexpr int kSpecialFirstLine = 1;
inline constexpr int kSpecialHanging = 2;
inline constexpr const char* kSpecialNames[3] = {"None", "First line",
                                                 "Hanging"};
// Default "By:" amount (inches) when the seeded first-line indent is
// 0, matching Word's 0.5" default for a newly-picked special indent.
inline constexpr double kDefaultByInches = 0.5;

// Inches -> stored indent units, rounded to the nearest unit.
// The signed form keeps a negative first-line (hanging) indent.
inline int inchesToStored(double inches) {
    return std::clamp(static_cast<int>(std::lround(inches * POINTS_PER_INCH)),
                      0, MAX_INDENT_STORED);
}
inline int inchesToStoredSigned(double inches) {
    return std::clamp(static_cast<int>(std::lround(inches * POINTS_PER_INCH)),
                      -MAX_INDENT_STORED, MAX_INDENT_STORED);
}

// Stored indent units -> inches field text ("0.5", "0", "-0.25"),
// trimming trailing zeros like columns_math::pointsToInchesString.
inline std::string storedToInchesString(int stored) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f",
                  static_cast<double>(stored) / POINTS_PER_INCH);
    std::string text = buf;
    while (!text.empty() && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
    if (text.empty() || text == "-0") text = "0";
    return text;
}

// Parse an inches field: empty / non-numeric / NaN / absurd keeps
// the fallback (the field's seeded value), like Page Setup margins.
inline double parseInches(const std::string& text, double fallback) {
    if (text.empty()) return fallback;
    char* end = nullptr;
    double inches = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || inches != inches || inches > 1000.0 ||
        inches < -1000.0) {
        return fallback;
    }
    return inches;
}

// Parse a spacing field in points: integer points, empty /
// non-numeric keeps the fallback, clamped to 0..MAX_SPACING_PT.
inline int parsePoints(const std::string& text, int fallback) {
    if (text.empty()) return fallback;
    char* end = nullptr;
    long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str()) return fallback;
    return std::clamp(static_cast<int>(value), 0, MAX_SPACING_PT);
}

// Parse the Default tab stops field: integer, empty / non-numeric
// keeps the fallback, clamped to the Tab Width range.
inline int parseTabStops(const std::string& text, int fallback) {
    if (text.empty()) return fallback;
    char* end = nullptr;
    long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str()) return fallback;
    return std::clamp(static_cast<int>(value), MIN_TAB_STOPS, MAX_TAB_STOPS);
}

// Dropdown index for a stored line-spacing multiplier: the nearest
// of Single / 1.5 / Double (a non-standard value, e.g. 1.25 from a
// file, reads as its nearest entry rather than a fourth state).
inline int lineSpacingIndexFor(float multiplier) {
    int best = 0;
    float bestDist = std::fabs(multiplier - kLineSpacingValues[0]);
    for (int i = 1; i < 3; ++i) {
        float dist = std::fabs(multiplier - kLineSpacingValues[i]);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

inline float lineSpacingForIndex(int index) {
    if (index < 0 || index > 2) return 1.0f;
    return kLineSpacingValues[index];
}

// Dropdown index <-> TextAlignment, for the Alignment dropdown.
// Out-of-range indices read as Left.
inline int alignmentIndexFor(TextAlignment alignment) {
    switch (alignment) {
        case TextAlignment::Center: return 1;
        case TextAlignment::Right: return 2;
        case TextAlignment::Justify: return 3;
        case TextAlignment::Left:
        default: return 0;
    }
}

inline TextAlignment alignmentForIndex(int index) {
    switch (index) {
        case 1: return TextAlignment::Center;
        case 2: return TextAlignment::Right;
        case 3: return TextAlignment::Justify;
        case 0:
        default: return TextAlignment::Left;
    }
}

// Special-indent dropdown index for a stored first-line indent:
// positive -> First line, negative -> Hanging, zero -> None.
inline int specialIndexFor(int firstLineIndent) {
    if (firstLineIndent > 0) return kSpecialFirstLine;
    if (firstLineIndent < 0) return kSpecialHanging;
    return kSpecialNone;
}

// Stored first-line indent for a (Special, By) pair: None ignores
// By (always 0), First line is +By, Hanging is -By. By is a
// magnitude: a negative typed value clamps to 0 via inchesToStored.
inline int firstLineIndentFor(int specialIndex, double byInches) {
    if (specialIndex == kSpecialFirstLine) {
        return inchesToStored(byInches);
    }
    if (specialIndex == kSpecialHanging) {
        return -inchesToStored(byInches);
    }
    return 0;
}

}  // namespace paragraph_dialog
