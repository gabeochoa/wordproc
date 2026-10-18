#pragma once

// Spell-check-as-you-type: red squiggles under misspelled words.
//
// The cache only (re)checks the visible lines, and only when the buffer
// version or the visible range changed, so rendering never rechecks the
// whole document per frame. It stores row/column positions (computed once
// during the recheck) so the render pass does no offset->line mapping.
// Suggestions are deliberately NOT computed here: getSuggestions() scans
// the dictionary per word, which is far too slow for as-you-type. The
// Spelling dialog computes suggestions for one word at a time instead.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "spellcheck.h"
#include "text_buffer.h"

struct SpellSquiggle {
    std::size_t row = 0;       // Line index in the document
    std::size_t startCol = 0;  // Column of the first letter (raw line coords)
    std::size_t endCol = 0;    // Column just past the last letter
};

// Map a column in the raw line to a column in the tab-expanded display
// line (render expands each tab to `tabWidth - (col % tabWidth)` spaces).
inline std::size_t rawColumnToDisplayColumn(const char* data, std::size_t len,
                                            std::size_t rawCol,
                                            int tabWidth) {
    if (tabWidth <= 0) return rawCol;
    std::size_t displayCol = 0;
    std::size_t limit = std::min(rawCol, len);
    for (std::size_t i = 0; i < limit; ++i) {
        if (data[i] == '\t') {
            displayCol += static_cast<std::size_t>(
                tabWidth - static_cast<int>(displayCol % static_cast<std::size_t>(tabWidth)));
        } else {
            ++displayCol;
        }
    }
    return displayCol;
}

struct SpellSquiggleCache {
    std::uint64_t version = UINT64_MAX;  // Buffer version last checked
    std::vector<SpellSquiggle> squiggles;  // Sorted by (row, startCol)
    std::size_t recheckCount = 0;           // Rechecks performed (tests)

    // Recompute squiggles for rows [firstRow, lastRow) if the buffer
    // version or the visible range changed since the last call.
    // Returns true when a recheck ran. No-op when nothing changed, so
    // calling this every frame allocates nothing.
    bool update(const TextBuffer& buffer, const SpellChecker& checker,
                std::size_t firstRow, std::size_t lastRow) {
        if (buffer.version() == version && firstRow == firstRow_ &&
            lastRow == lastRow_) {
            return false;
        }
        version = buffer.version();
        firstRow_ = firstRow;
        lastRow_ = lastRow;
        ++recheckCount;

        squiggles.clear();
        // No dictionary, no squiggles: if the dictionary file failed to
        // load (e.g. wrong working directory) every word would otherwise
        // be flagged as misspelled.
        if (checker.dictionarySize() == 0) return true;

        std::size_t lineCount = buffer.lineCount();
        std::size_t begin = std::min(firstRow, lineCount);
        std::size_t end = std::min(lastRow, lineCount);
        for (std::size_t row = begin; row < end; ++row) {
            auto view = buffer.lineView(row);
            std::string lineStorage;
            const char* data = nullptr;
            std::size_t len = 0;
            if (view) {
                data = view.data;
                len = view.length;
            } else {
                lineStorage = buffer.lineString(row);
                data = lineStorage.data();
                len = lineStorage.size();
            }
            std::size_t i = 0;
            while (i < len) {
                while (i < len && !SpellChecker::isWordChar(data[i])) ++i;
                if (i >= len) break;
                std::size_t start = i;
                while (i < len && SpellChecker::isWordChar(data[i])) ++i;
                std::string word(data + start, i - start);
                if (!checker.isCorrect(word)) {
                    squiggles.push_back({row, start, i});
                }
            }
        }
        return true;
    }

    const std::vector<SpellSquiggle>& all() const { return squiggles; }

   private:
    std::size_t firstRow_ = 0;
    std::size_t lastRow_ = 0;
};
