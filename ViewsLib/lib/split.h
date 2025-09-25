#pragma once
#include <string>
#include <iterator>
#include <type_traits>
#include <utility>

#include "stream.h"

template <typename Range>
class SplitRange {
public:
    using iterator = typename Range::iterator;
    using const_iterator = typename Range::const_iterator;
    using value_type = std::string;

    class split_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string;
        using reference = const std::string&;
        using pointer = const std::string*;

        split_iterator(iterator range_it,
                       iterator range_end,
                       const std::string* delims,
                       bool is_end = false)
            : range_it_(range_it), range_end_(range_end), delims_(delims), is_end_(is_end) {
            if (!is_end_) {
                fetch_next_token();
            }
        }

        split_iterator& operator++() {
            fetch_next_token();
            return *this;
        }

        value_type operator*() const { return current_token_; }

        bool operator!=(const split_iterator& other) const {
            return is_end_ != other.is_end_;
        }

        bool operator==(const split_iterator& other) const {
            return is_end_ == other.is_end_;
        }

    private:
        void fetch_next_token() {
            current_token_.clear();

            while (true) {
                if (has_pending_token_) {
                    current_token_ = std::move(pending_token_);
                    has_pending_token_ = false;
                    return;
                }

                if (range_it_ == range_end_) {
                    is_end_ = true;
                    return;
                }

                std::istream& stream = get_istream(*range_it_);
                char c;

                while (stream.get(c)) {
                    bool is_delim = delims_->find(c) != std::string::npos;

                    if (is_delim) {
                        if (!token_buffer_.empty()) {
                            current_token_ = std::move(token_buffer_);
                            token_buffer_.clear();
                            last_was_delim_ = true;
                            return;
                        } else if (last_was_delim_) {
                            current_token_.clear();
                            last_was_delim_ = true;
                            return;
                        } else {
                            last_was_delim_ = true;
                            continue;
                        }
                    } else {
                        token_buffer_ += c;
                        last_was_delim_ = false;
                    }
                }

                if (!token_buffer_.empty()) {
                    pending_token_ = std::move(token_buffer_);
                    has_pending_token_ = true;
                    token_buffer_.clear();
                    ++range_it_;
                    continue;
                }

                ++range_it_;
            }
        }

        iterator range_it_;
        iterator range_end_;
        const std::string* delims_;
        std::string current_token_;
        std::string token_buffer_;
        std::string pending_token_;
        bool has_pending_token_ = false;
        bool is_end_ = false;
        bool last_was_delim_ = false;
    };

    SplitRange(Range& range, std::string delims)
        : range_(range), delims_(std::move(delims)) {}

    split_iterator begin() { return split_iterator(range_.begin(), range_.end(), &delims_, false); }
    split_iterator end() { return split_iterator(range_.end(), range_.end(), &delims_, true); }

    split_iterator begin() const { return split_iterator(range_.begin(), range_.end(), &delims_, false); }
    split_iterator end() const { return split_iterator(range_.end(), range_.end(), &delims_, true); }

private:
    Range& range_;
    std::string delims_;
};

inline auto Split(std::string delims) {
    return [delims = std::move(delims)](auto& range) mutable {
        return SplitRange<std::remove_reference_t<decltype(range)>>(range, std::move(delims));
    };
}