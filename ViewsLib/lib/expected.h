#pragma once
#include <expected>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <iterator>

template <typename Range, typename E>
class UnexpectedRange {
public:
    using iterator_base = decltype(std::declval<Range&>().begin());
    using value_type = E;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = E;
        using reference = value_type;
        using pointer = void;
        using difference_type = std::ptrdiff_t;

        iterator(iterator_base curr, iterator_base end)
            : curr_(curr), end_(end) {
            skip_to_unexpected();
        }

        iterator& operator++() {
            ++curr_;
            skip_to_unexpected();
            return *this;
        }

        reference operator*() const { return (*curr_).error(); }
        bool operator!=(const iterator& other) const { return curr_ != other.curr_; }
        bool operator==(const iterator& other) const { return curr_ == other.curr_; }

    private:
        void skip_to_unexpected() {
            while (curr_ != end_ && (*curr_).has_value()) {
                ++curr_;
            }
        }

        iterator_base curr_;
        iterator_base end_;
    };

    UnexpectedRange(Range& range) : range_(range) {}

    iterator begin() { return iterator(range_.begin(), range_.end()); }
    iterator end() { return iterator(range_.end(), range_.end()); }

private:
    Range& range_;
};

template <typename Range, typename T>
class ExpectedRange {
public:
    using iterator_base = decltype(std::declval<Range&>().begin());
    using value_type = T;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using reference = value_type;
        using pointer = void;
        using difference_type = std::ptrdiff_t;

        iterator(iterator_base curr, iterator_base end)
            : curr_(curr), end_(end) {
            skip_to_expected();
        }

        iterator& operator++() {
            ++curr_;
            skip_to_expected();
            return *this;
        }

        reference operator*() const { return *(*curr_); }
        bool operator!=(const iterator& other) const { return curr_ != other.curr_; }
        bool operator==(const iterator& other) const { return curr_ == other.curr_; }

    private:
        void skip_to_expected() {
            while (curr_ != end_ && !(*curr_).has_value()) {
                ++curr_;
            }
        }

        iterator_base curr_;
        iterator_base end_;
    };

    ExpectedRange(Range& range) : range_(range) {}

    iterator begin() { return iterator(range_.begin(), range_.end()); }
    iterator end() { return iterator(range_.end(), range_.end()); }

private:
    Range& range_;
};

template <typename F>
auto SplitExpected(F&&) {
    return [](auto& range) {
        using Exp = std::remove_reference_t<decltype(*range.begin())>;
        static_assert(
            std::is_same_v<Exp, std::expected<typename Exp::value_type, typename Exp::error_type>>,
            "SplitExpected: range must yield std::expected<T, E>"
        );

        using T = typename Exp::value_type;
        using E = typename Exp::error_type;

        std::vector<E> errors;
        std::vector<T> goods;

        for (auto it = range.begin(); it != range.end(); ++it) {
            if ((*it).has_value()) {
                goods.push_back(**it);
            } else {
                errors.push_back((*it).error());
            }
        }

        return std::make_tuple(std::move(errors), std::move(goods));
    };
}

inline auto SplitExpected() {
    return [](auto& range) {
        using Exp = std::remove_reference_t<decltype(*range.begin())>;
        static_assert(
            std::is_same_v<Exp, std::expected<typename Exp::value_type, typename Exp::error_type>>,
            "SplitExpected: range must yield std::expected<T, E>"
        );

        using T = typename Exp::value_type;
        using E = typename Exp::error_type;

        std::vector<E> errors;
        std::vector<T> goods;

        for (auto it = range.begin(); it != range.end(); ++it) {
            if ((*it).has_value()) {
                goods.push_back(**it);
            } else {
                errors.push_back((*it).error());
            }
        }

        return std::make_tuple(std::move(errors), std::move(goods));
    };
}