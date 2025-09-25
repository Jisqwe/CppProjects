#pragma once
#include <iterator>
#include <type_traits>
#include <utility>

template <typename Range, typename UnaryOp>
class TransformRange {
public:
    using Iterator = decltype(std::declval<Range&>().begin());
    using ConstIterator = decltype(std::declval<const Range&>().begin());
    using value_type =
        std::remove_cv_t<std::remove_reference_t<decltype(std::declval<UnaryOp>()(*std::declval<Iterator>()))>>;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;

        iterator(Iterator it, UnaryOp* op) : it_(it), op_(op) {}

        iterator& operator++() {
            ++it_;
            return *this;
        }

        decltype(auto) operator*() const { return (*op_)(*it_); }

        bool operator!=(const iterator& other) const { return it_ != other.it_; }
        bool operator==(const iterator& other) const { return it_ == other.it_; }

    private:
        Iterator it_;
        UnaryOp* op_;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;

        const_iterator(ConstIterator it, const UnaryOp* op) : it_(it), op_(op) {}

        const_iterator& operator++() {
            ++it_;
            return *this;
        }

        decltype(auto) operator*() const { return (*op_)(*it_); }

        bool operator!=(const const_iterator& other) const { return it_ != other.it_; }
        bool operator==(const const_iterator& other) const { return it_ == other.it_; }

    private:
        ConstIterator it_;
        const UnaryOp* op_;
    };

    TransformRange(Range& range, UnaryOp op) : range_(range), op_(std::move(op)) {}

    iterator begin() { return iterator(range_.begin(), &op_); }
    iterator end() { return iterator(range_.end(), &op_); }

    const_iterator begin() const { return const_iterator(range_.begin(), &op_); }
    const_iterator end() const { return const_iterator(range_.end(), &op_); }

private:
    Range& range_;
    UnaryOp op_;
};

template <typename UnaryOp>
auto Transform(UnaryOp op) {
    return [op](auto& range) {
        return TransformRange<std::remove_reference_t<decltype(range)>, UnaryOp>(range, op);
    };
}