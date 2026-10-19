#pragma once

// @ smart chips (Docs smart chips, Win95 dress): pure helpers over a
// TextBuffer + chip list. A chip is the label text in the buffer plus
// a record {startOffset, endOffset, kind, label}; the render pass
// draws a raised BUTTON_FACE box behind the label when the buffer
// still holds the label at the recorded offset.
//
// Limits (documented, not faked): chips are offset-anchored, like
// comments/revisions. An edit INSIDE a chip orphans it (its label no
// longer matches the buffer, so the render pass skips it and
// backspace-at-end will not treat it as a chip); edits elsewhere
// keep chips valid because insertChipAt/eraseChipAt re-anchor the
// chips after the edit point. Caret movement is per-character —
// only deletion is unit-wise (Backspace at a chip's end deletes
// the whole chip).

#include <algorithm>
#include <cctype>
#include <ctime>
#include <string>
#include <vector>

#include "date_time.h"
#include "text_buffer.h"

enum class ChipKind {
    Date,    // "Today — 10/19/2026" / "Tomorrow — ..."
    Person,  // "Gabe Ochoa"
    App,     // "Front Door (AI)"
};

struct Chip {
    std::size_t startOffset = 0;
    std::size_t endOffset = 0;  // exclusive
    ChipKind kind = ChipKind::Person;
    std::string label;
};

// One popup row.
struct ChipItem {
    ChipKind kind = ChipKind::Person;
    std::string label;
};

namespace chips {

// "Today — 10/19/2026" for `now` (local time).
inline std::string todayLabel(std::time_t now) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    return "Today — " + date_time::formatDateTime(tm, 0);
}

// "Tomorrow — <date>" for `now` (local time). Calendar
// arithmetic (mday + 1, normalized by mktime), not now + 86400:
// the fixed-offset version drifts an hour across a DST boundary
// (and is wrong in exactly the "which date is it" question this
// label answers).
inline std::string tomorrowLabel(std::time_t now) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    tm.tm_mday += 1;
    tm.tm_isdst = -1;
    const std::time_t tomorrow = std::mktime(&tm);
    if (tomorrow == static_cast<std::time_t>(-1)) {
        return "Tomorrow — " + date_time::formatDateTime(tm, 0);
    }
    std::tm out{};
#ifdef _WIN32
    localtime_s(&out, &tomorrow);
#else
    localtime_r(&tomorrow, &out);
#endif
    return "Tomorrow — " + date_time::formatDateTime(out, 0);
}

// Popup items for a filter (the text after "@", may be empty).
// All four items when the filter is empty; otherwise only items
// whose label contains the filter (case-insensitive). Order is
// fixed: Today, Tomorrow, Gabe Ochoa, Front Door (AI).
inline std::vector<ChipItem> popupItems(const std::string& filter,
                                        std::time_t now) {
    std::vector<ChipItem> all{
        {ChipKind::Date, todayLabel(now)},
        {ChipKind::Date, tomorrowLabel(now)},
        {ChipKind::Person, "Gabe Ochoa"},
        {ChipKind::App, "Front Door (AI)"},
    };
    if (filter.empty()) return all;
    std::string needle = filter;
    for (auto& ch : needle) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    std::vector<ChipItem> out;
    for (const auto& item : all) {
        std::string hay = item.label;
        for (auto& ch : hay) {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }
        if (hay.find(needle) != std::string::npos) out.push_back(item);
    }
    return out;
}

// Does the buffer still hold this chip's label at its offset?
inline bool bufferMatches(const TextBuffer& buffer, const Chip& chip) {
    if (chip.endOffset < chip.startOffset) return false;
    if (chip.endOffset - chip.startOffset != chip.label.size()) {
        return false;
    }
    for (std::size_t i = 0; i < chip.label.size(); ++i) {
        if (buffer.charAtOffset(chip.startOffset + i) != chip.label[i]) {
            return false;
        }
    }
    return true;
}

// Replace [atOffset, endOffset) — the "@" + filter text — with the
// chip label, as one undo step. Drops any chip overlapping the
// replaced range, re-anchors chips at/after endOffset by the delta,
// and records the new chip. Returns false when the range is invalid.
inline bool insertChipAt(TextBuffer& buffer, std::vector<Chip>& chipList,
                         std::size_t atOffset, std::size_t endOffset,
                         ChipKind kind, const std::string& label) {
    if (atOffset > endOffset || endOffset > buffer.textSize()) return false;
    const std::ptrdiff_t delta = static_cast<std::ptrdiff_t>(label.size()) -
                                 static_cast<std::ptrdiff_t>(endOffset - atOffset);
    buffer.replaceRange(atOffset, endOffset - atOffset, label);
    chipList.erase(
        std::remove_if(chipList.begin(), chipList.end(),
                       [&](const Chip& chip) {
                           return chip.startOffset < endOffset &&
                                  chip.endOffset > atOffset;
                       }),
        chipList.end());
    for (auto& chip : chipList) {
        if (chip.startOffset >= endOffset) {
            chip.startOffset = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(chip.startOffset) + delta);
            chip.endOffset = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(chip.endOffset) + delta);
        }
    }
    chipList.push_back(Chip{atOffset, atOffset + label.size(), kind, label});
    return true;
}

// Index of the chip whose end is exactly the caret (and whose label
// still matches), or -1. Backspace at a chip's end deletes the
// whole chip.
inline int chipEndingAt(const TextBuffer& buffer,
                        const std::vector<Chip>& chipList,
                        std::size_t caretOffset) {
    for (std::size_t i = 0; i < chipList.size(); ++i) {
        if (chipList[i].endOffset == caretOffset &&
            bufferMatches(buffer, chipList[i])) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Erase chip `index` from the buffer (one undo step) and the list,
// re-anchoring later chips. False when the index is invalid.
inline bool eraseChipAt(TextBuffer& buffer, std::vector<Chip>& chipList,
                        std::size_t index) {
    if (index >= chipList.size()) return false;
    Chip chip = chipList[index];
    if (!bufferMatches(buffer, chip)) return false;
    const std::size_t len = chip.endOffset - chip.startOffset;
    buffer.replaceRange(chip.startOffset, len, "");
    chipList.erase(chipList.begin() + static_cast<std::ptrdiff_t>(index));
    for (auto& other : chipList) {
        if (other.startOffset >= chip.endOffset) {
            other.startOffset -= len;
            other.endOffset -= len;
        } else if (other.startOffset >= chip.startOffset) {
            other.startOffset = chip.startOffset;
            other.endOffset = chip.startOffset;
        }
    }
    return true;
}

}  // namespace chips
