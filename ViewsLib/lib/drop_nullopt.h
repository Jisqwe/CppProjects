#pragma once
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

template <typename Range>
class DropNulloptRange {
public:
    using BaseIterator = decltype(std::declval<Range&>().begin());
    using value_type = std::remove_reference_t<decltype(**std::declval<BaseIterator>())>;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = typename DropNulloptRange::value_type;
        using reference = value_type;
        using pointer = void;

        iterator(BaseIterator curr, BaseIterator end)
            : curr_(curr), end_(end) {
            skip_nullopt();
        }

        iterator& operator++() {
            ++curr_;
            skip_nullopt();
            return *this;
        }

        reference operator*() const {
            return **curr_;
        }

        bool operator!=(const iterator& other) const { return curr_ != other.curr_; }
        bool operator==(const iterator& other) const { return curr_ == other.curr_; }

    private:
        void skip_nullopt() {
            while (curr_ != end_ && !(*curr_)) {
                ++curr_;
            }
        }

        BaseIterator curr_;
        BaseIterator end_;
    };

    DropNulloptRange(Range& range) : range_(range) {}

    iterator begin() { return iterator(range_.begin(), range_.end()); }
    iterator end() { return iterator(range_.end(), range_.end()); }

private:
    Range& range_;
};

inline auto DropNullopt() {
    return [](auto& range) {
        using RangeT = std::remove_reference_t<decltype(range)>;
        return DropNulloptRange<RangeT>(range);
    };
}