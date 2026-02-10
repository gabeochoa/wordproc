#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Gap buffer for efficient text editing
// Stores characters contiguously with a "gap" at the edit position
// This allows O(1) inserts and deletes at the cursor
class GapBuffer {
   public:
    GapBuffer(std::size_t initial_capacity = 4096);

    void insert(std::size_t pos, char ch);
    void insertString(std::size_t pos, const char* str, std::size_t len);
    void erase(std::size_t pos, std::size_t count = 1);

    char at(std::size_t pos) const;
    std::size_t size() const;
    bool empty() const;

    // Get substring without allocating (returns pointer and length)
    const char* data(std::size_t pos, std::size_t len) const;

    // Copy substring to output
    void copyTo(std::size_t pos, std::size_t len, char* out) const;

    // Get entire buffer as string (for compatibility)
    std::string toString() const;

    // Write contents directly to an output stream (zero intermediate string)
    void writeTo(std::ostream& out) const;

    // Call a function for each contiguous region of content.
    // The callback receives (const char* data, size_t len) once or twice
    // (before-gap region, then after-gap region). This is the zero-copy
    // primitive that everything else should build on.
    template <typename Fn>
    void forEachRegion(Fn&& fn) const {
        if (gap_start_ > 0) {
            fn(buffer_.data(), gap_start_);
        }
        std::size_t after_gap_len = buffer_.size() - gap_end_;
        if (after_gap_len > 0) {
            fn(buffer_.data() + gap_end_, after_gap_len);
        }
    }

    // Raw region accessors for direct pointer access
    const char* beforeGapData() const { return buffer_.data(); }
    std::size_t beforeGapSize() const { return gap_start_; }
    const char* afterGapData() const { return buffer_.data() + gap_end_; }
    std::size_t afterGapSize() const { return buffer_.size() - gap_end_; }

    void clear();

    // Reserve capacity for bulk loading (avoids reallocations)
    void reserve(std::size_t capacity);
    
    // Push back for efficient sequential loading
    void pushBack(char ch);
    
    // Bulk load - replaces all content efficiently (for initial file loading)
    void setContent(const char* data, std::size_t len);

    // Performance tracking
    std::size_t gapMoves() const { return gap_moves_; }
    std::size_t reallocations() const { return reallocations_; }
    void resetStats() {
        gap_moves_ = 0;
        reallocations_ = 0;
    }

   private:
    void moveGapTo(std::size_t pos);
    void ensureCapacity(std::size_t needed);

    std::vector<char> buffer_;
    std::size_t gap_start_ = 0;
    std::size_t gap_end_ = 0;
    std::size_t gap_moves_ = 0;
    std::size_t reallocations_ = 0;
};

