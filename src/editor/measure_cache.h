#pragma once

// Bounded LRU cache for text-width measurements.
//
// The render pass measures the same strings over and over: every frame it
// re-measures each visible line's prefixes (squiggle x positions, selection
// and caret x), and on an error-dense document that is ~2 fontstash walks
// per squiggle per frame even though the text has not changed. A measurement
// is a pure function of (active font, font size, text), so caching it is
// exact -- a hit returns the same integer the uncached call would.
//
// The cache is keyed on (fontKey, fontSize, text). fontKey is opaque to this
// class: the render wiring folds the active fontstash id and the DPI scale
// into it, i.e. everything `graphics::measure_text` depends on besides the
// text and size. Widths are text-keyed, so buffer edits never invalidate
// anything -- an edited line simply measures under its new text.
//
// Exactness, not hash trust: the map key carries two independent hashes of
// the text, and a hit still verifies the stored full string against the
// query. A hash collision can only evict an entry, never serve a wrong
// width.
//
// Eviction is plain LRU with a fixed cap (4096 entries, the same bound
// afterhours' TextMeasureCache uses). That cache cannot be reused here:
// it answers advance widths through a Font, while the document renderer
// measures ink bounds through the active font
// (`graphics::measure_text`); swapping one for the other would move
// every squiggle, caret, and selection edge by the side bearings.

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>

class MeasureCache {
  public:
    static constexpr std::size_t kMaxEntries = 4096;

    using MeasureFn = std::function<int(std::string_view, int)>;

    explicit MeasureCache(MeasureFn measureFn = {}, std::size_t maxEntries = kMaxEntries)
        : measureFn_(std::move(measureFn)), maxEntries_(maxEntries < 1 ? 1 : maxEntries) {}

    void setMeasureFn(MeasureFn measureFn) { measureFn_ = std::move(measureFn); }

    // Width of `text` at `fontSize` under `fontKey`. On a miss the stored
    // measure function runs once and its answer is cached; without a
    // measure function the answer is 0.
    int measure(std::string_view text, int fontSize, std::uint64_t fontKey) {
        const Key key{hashText(text), checkText(text), fontKey, fontSize};
        const auto it = lookup_.find(key);
        if (it != lookup_.end()) {
            if (it->second->text == text) {
                lru_.splice(lru_.begin(), lru_, it->second);
                ++hits_;
                return it->second->width;
            }
            // Hash collision with different text: drop the colliding
            // entry rather than serving or keeping a wrong answer.
            lru_.erase(it->second);
            lookup_.erase(it);
        }
        ++misses_;
        const int width = measureFn_ ? measureFn_(text, fontSize) : 0;
        while (lookup_.size() >= maxEntries_) {
            const auto& victim = lru_.back();
            lookup_.erase(victim.key);
            lru_.pop_back();
        }
        lru_.push_front(Entry{key, std::string(text), width});
        lookup_[key] = lru_.begin();
        return width;
    }

    void clear() {
        lru_.clear();
        lookup_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    [[nodiscard]] std::size_t size() const { return lookup_.size(); }
    [[nodiscard]] std::uint64_t hits() const { return hits_; }
    [[nodiscard]] std::uint64_t misses() const { return misses_; }

  private:
    struct Key {
        std::uint64_t hash = 0;
        std::uint64_t check = 0;
        std::uint64_t fontKey = 0;
        int fontSize = 0;
        bool operator==(const Key&) const = default;
    };

    struct KeyHash {
        std::size_t operator()(const Key& key) const {
            return static_cast<std::size_t>(key.hash ^ (key.check << 1) ^
                                             (key.fontKey << 2) ^
                                             (static_cast<std::uint64_t>(
                                                  static_cast<std::uint32_t>(key.fontSize)) << 3));
        }
    };

    struct Entry {
        Key key;
        std::string text;
        int width = 0;
    };

    // Two independent FNV-1a hashes (forward and reversed) so one
    // collision cannot alias a different string into a hit.
    static std::uint64_t hashText(std::string_view text) {
        std::uint64_t hash = 14695981039346656037ULL;
        for (unsigned char c : text) {
            hash ^= static_cast<std::uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    static std::uint64_t checkText(std::string_view text) {
        std::uint64_t hash = 14695981039346656037ULL;
        for (auto it = text.rbegin(); it != text.rend(); ++it) {
            hash *= 1099511628211ULL;
            hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(*it));
        }
        return hash;
    }

    MeasureFn measureFn_;
    std::size_t maxEntries_;
    std::list<Entry> lru_;
    std::unordered_map<Key, std::list<Entry>::iterator, KeyHash> lookup_;
    std::uint64_t hits_ = 0;
    std::uint64_t misses_ = 0;
};
