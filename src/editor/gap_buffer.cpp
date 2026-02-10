#include "gap_buffer.h"

#include <algorithm>
#include <cstring>
#include <ostream>

GapBuffer::GapBuffer(std::size_t initial_capacity) {
    buffer_.resize(initial_capacity);
    gap_start_ = 0;
    gap_end_ = initial_capacity;
}

void GapBuffer::moveGapTo(std::size_t pos) {
    if (pos == gap_start_) {
        return;
    }

    gap_moves_++;

    if (pos < gap_start_) {
        // Move gap backwards: shift characters forward
        std::size_t shift = gap_start_ - pos;
        std::memmove(&buffer_[gap_end_ - shift], &buffer_[pos], shift);
        gap_end_ -= shift;
        gap_start_ = pos;
    } else {
        // Move gap forwards: shift characters backward
        std::size_t shift = pos - gap_start_;
        std::memmove(&buffer_[gap_start_], &buffer_[gap_end_], shift);
        gap_start_ += shift;
        gap_end_ += shift;
    }
}

void GapBuffer::ensureCapacity(std::size_t needed) {
    std::size_t gap_size = gap_end_ - gap_start_;
    if (gap_size >= needed) {
        return;
    }

    reallocations_++;

    // Grow by 2x or to fit needed, whichever is larger
    std::size_t current_size = buffer_.size();
    std::size_t new_size =
        std::max(current_size * 2, current_size + needed - gap_size);

    std::vector<char> new_buffer(new_size);

    // Copy before gap
    if (gap_start_ > 0) {
        std::memcpy(&new_buffer[0], &buffer_[0], gap_start_);
    }

    // Copy after gap
    std::size_t after_gap = current_size - gap_end_;
    if (after_gap > 0) {
        std::memcpy(&new_buffer[new_size - after_gap], &buffer_[gap_end_],
                    after_gap);
    }

    gap_end_ = new_size - after_gap;
    buffer_ = std::move(new_buffer);
}

void GapBuffer::insert(std::size_t pos, char ch) {
    moveGapTo(pos);
    ensureCapacity(1);
    buffer_[gap_start_++] = ch;
}

void GapBuffer::insertString(std::size_t pos, const char* str,
                             std::size_t len) {
    if (len == 0) return;
    moveGapTo(pos);
    ensureCapacity(len);
    std::memcpy(&buffer_[gap_start_], str, len);
    gap_start_ += len;
}

void GapBuffer::erase(std::size_t pos, std::size_t count) {
    if (count == 0) return;
    moveGapTo(pos);
    // Expand gap to "delete" characters after gap
    gap_end_ = std::min(gap_end_ + count, buffer_.size());
}

char GapBuffer::at(std::size_t pos) const {
    if (pos < gap_start_) {
        return buffer_[pos];
    }
    return buffer_[gap_end_ + (pos - gap_start_)];
}

std::size_t GapBuffer::size() const {
    return buffer_.size() - (gap_end_ - gap_start_);
}

bool GapBuffer::empty() const { return size() == 0; }

const char* GapBuffer::data(std::size_t pos, std::size_t len) const {
    // This only works if the range doesn't span the gap
    if (pos < gap_start_ && pos + len <= gap_start_) {
        return &buffer_[pos];
    }
    if (pos >= gap_start_) {
        return &buffer_[gap_end_ + (pos - gap_start_)];
    }
    // Range spans gap - return nullptr to indicate copy needed
    return nullptr;
}

void GapBuffer::copyTo(std::size_t pos, std::size_t len, char* out) const {
    if (len == 0) return;

    // Fast path: range entirely before gap
    if (pos + len <= gap_start_) {
        std::memcpy(out, &buffer_[pos], len);
        return;
    }

    // Fast path: range entirely after gap
    if (pos >= gap_start_) {
        std::size_t buf_pos = gap_end_ + (pos - gap_start_);
        std::memcpy(out, &buffer_[buf_pos], len);
        return;
    }

    // Range spans the gap: copy in two parts
    std::size_t before_len = gap_start_ - pos;
    std::memcpy(out, &buffer_[pos], before_len);
    std::size_t after_len = len - before_len;
    std::memcpy(out + before_len, &buffer_[gap_end_], after_len);
}

std::string GapBuffer::toString() const {
    std::string result;
    std::size_t total = size();
    result.resize(total);

    // Two memcpy calls instead of per-char pushback
    if (gap_start_ > 0) {
        std::memcpy(&result[0], buffer_.data(), gap_start_);
    }
    std::size_t after_gap_len = buffer_.size() - gap_end_;
    if (after_gap_len > 0) {
        std::memcpy(&result[gap_start_], buffer_.data() + gap_end_, after_gap_len);
    }

    return result;
}

void GapBuffer::writeTo(std::ostream& out) const {
    if (gap_start_ > 0) {
        out.write(buffer_.data(), static_cast<std::streamsize>(gap_start_));
    }
    std::size_t after_gap_len = buffer_.size() - gap_end_;
    if (after_gap_len > 0) {
        out.write(buffer_.data() + gap_end_,
                  static_cast<std::streamsize>(after_gap_len));
    }
}

void GapBuffer::clear() {
    gap_start_ = 0;
    gap_end_ = buffer_.size();
}

void GapBuffer::reserve(std::size_t capacity) {
    if (capacity > buffer_.size()) {
        std::size_t gap_size = gap_end_ - gap_start_;
        std::size_t content_size = size();
        std::size_t new_size = capacity + gap_size;
        
        if (new_size > buffer_.size()) {
            // Move gap to end before resizing
            moveGapTo(content_size);
            buffer_.resize(new_size);
            gap_end_ = new_size;
        }
    }
}

void GapBuffer::pushBack(char ch) {
    // Ensure gap is at end and has space
    std::size_t content_size = size();
    moveGapTo(content_size);
    
    if (gap_start_ >= gap_end_) {
        ensureCapacity(content_size + 1);
    }
    
    buffer_[gap_start_] = ch;
    ++gap_start_;
}

void GapBuffer::setContent(const char* data, std::size_t len) {
    // Resize buffer to exactly fit content + gap
    constexpr std::size_t GAP_SIZE = 4096;
    buffer_.resize(len + GAP_SIZE);
    
    // Copy content at start, gap at end
    std::memcpy(buffer_.data(), data, len);
    gap_start_ = len;
    gap_end_ = buffer_.size();
}

