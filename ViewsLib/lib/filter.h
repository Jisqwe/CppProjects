#pragma once
#include <iterator>
#include <type_traits>
#include <memory>
#include <utility>

template <typename Range, typename Predicate>
class FilterRange {
public:
    using BaseIterator = decltype(std::declval<Range&>().begin());
    using BaseConstIterator = decltype(std::declval<const Range&>().begin());
    using value_type = std::remove_reference_t<decltype(*std::declval<BaseIterator>())>;

    class iterator {
    public:
        using reference = decltype(*std::declval<BaseIterator>());
        using pointer = std::remove_reference_t<reference>*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;

        iterator(BaseIterator curr, BaseIterator end, Predicate* pred)
            : curr_(curr), end_(end), pred_(pred) {
            skip();
        }

        iterator& operator++() {
            ++curr_;
            skip();
            return *this;
        }

        reference operator*() const { return *curr_; }
        pointer operator->() const { return std::addressof(*curr_); }
        bool operator!=(const iterator& other) const { return curr_ != other.curr_; }
        bool operator==(const iterator& other) const { return curr_ == other.curr_; }

    private:
        void skip() {
            while (curr_ != end_ && !(*pred_)(*curr_)) {
                ++curr_;
            }
        }

        BaseIterator curr_;
        BaseIterator end_;
        Predicate* pred_;
    };

    class const_iterator {
    public:
        using reference = decltype(*std::declval<BaseConstIterator>());
        using pointer = std::remove_reference_t<reference>*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;

        const_iterator(BaseConstIterator curr, BaseConstIterator end, const Predicate* pred)
            : curr_(curr), end_(end), pred_(pred) {
            skip();
        }

        const_iterator& operator++() {
            ++curr_;
            skip();
            return *this;
        }

        reference operator*() const { return *curr_; }
        pointer operator->() const { return std::addressof(*curr_); }
        bool operator!=(const const_iterator& other) const { return curr_ != other.curr_; }
        bool operator==(const const_iterator& other) const { return curr_ == other.curr_; }

    private:
        void skip() {
            while (curr_ != end_ && !(*pred_)(*curr_)) {
                ++curr_;
            }
        }

        BaseConstIterator curr_;
        BaseConstIterator end_;
        const Predicate* pred_;
    };

    FilterRange(Range& range, Predicate pred)
        : range_(range), pred_(std::move(pred)) {}

    iterator begin() { return iterator(range_.begin(), range_.end(), &pred_); }
    iterator end() { return iterator(range_.end(), range_.end(), &pred_); }

    const_iterator begin() const { return const_iterator(range_.begin(), range_.end(), &pred_); }
    const_iterator end() const { return const_iterator(range_.end(), range_.end(), &pred_); }

private:
    Range& range_;
    Predicate pred_;
};

template <typename Predicate>
auto Filter(Predicate pred) {
    return [pred](auto& range) {
        using RangeType = std::remove_reference_t<decltype(range)>;
        return FilterRange<RangeType, Predicate>(range, pred);
    };
}