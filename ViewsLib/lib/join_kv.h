#pragma once
#include <optional>
#include <type_traits>
#include <map>
#include <iterator>
#include <utility>

template <typename K, typename V>
struct KV {
    K key;
    V value;

    bool operator==(const KV& other) const {
        return key == other.key && value == other.value;
    }
};

template <typename Left, typename Right>
struct JoinResult {
    Left left;
    std::optional<Right> right;

    bool operator==(const JoinResult& other) const {
        return left == other.left && right == other.right;
    }
};

template <typename T>
struct is_kv : std::false_type {};

template <typename K, typename V>
struct is_kv<KV<K, V>> : std::true_type {};

template <typename LeftRange, typename RightRange, typename LeftKeyFn, typename RightKeyFn, typename Merger>
class JoinRange {
public:
    using left_iterator = decltype(std::declval<LeftRange&>().begin());
    using right_iterator = decltype(std::declval<RightRange&>().begin());
    using left_value_type = std::remove_reference_t<decltype(*std::declval<left_iterator>())>;
    using right_value_type = std::remove_reference_t<decltype(*std::declval<right_iterator>())>;
    using left_key_type = decltype(std::declval<LeftKeyFn>()(std::declval<left_value_type>()));
    using right_key_type = decltype(std::declval<RightKeyFn>()(std::declval<right_value_type>()));
    using result_type = decltype(std::declval<Merger>()(
        std::declval<const left_value_type&>(),
        std::optional<right_value_type>()
    ));
    using left_const_iterator = decltype(std::declval<const LeftRange&>().begin());

    using value_type = result_type;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = result_type;
        using reference = value_type;
        using pointer = void;
        using difference_type = std::ptrdiff_t;

        iterator(left_iterator lcur,
                 left_iterator lend,
                 const std::map<right_key_type, right_value_type>* rmap,
                 const LeftKeyFn* left_key,
                 const Merger* merger)
            : lcur_(lcur), lend_(lend), rmap_(rmap), left_key_(left_key), merger_(merger) {}

        iterator& operator++() {
            ++lcur_;
            return *this;
        }

        value_type operator*() const {
            const left_value_type& left = *lcur_;
            auto key = (*left_key_)(left);
            auto found = rmap_->find(key);
            if (found != rmap_->end()) {
                return (*merger_)(left, found->second);
            } else {
                return (*merger_)(left, std::nullopt);
            }
        }

        bool operator!=(const iterator& other) const { return lcur_ != other.lcur_; }
        bool operator==(const iterator& other) const { return lcur_ == other.lcur_; }

    private:
        left_iterator lcur_;
        left_iterator lend_;
        const std::map<right_key_type, right_value_type>* rmap_;
        const LeftKeyFn* left_key_;
        const Merger* merger_;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = result_type;
        using reference = value_type;
        using pointer = void;
        using difference_type = std::ptrdiff_t;

        const_iterator(left_const_iterator lcur,
                       left_const_iterator lend,
                       const std::map<right_key_type, right_value_type>* rmap,
                       const LeftKeyFn* left_key,
                       const Merger* merger)
            : lcur_(lcur), lend_(lend), rmap_(rmap), left_key_(left_key), merger_(merger) {}

        const_iterator& operator++() {
            ++lcur_;
            return *this;
        }

        value_type operator*() const {
            const left_value_type& left = *lcur_;
            auto key = (*left_key_)(left);
            auto found = rmap_->find(key);
            if (found != rmap_->end()) {
                return (*merger_)(left, found->second);
            } else {
                return (*merger_)(left, std::nullopt);
            }
        }

        bool operator!=(const const_iterator& other) const { return lcur_ != other.lcur_; }
        bool operator==(const const_iterator& other) const { return lcur_ == other.lcur_; }

    private:
        left_const_iterator lcur_;
        left_const_iterator lend_;
        const std::map<right_key_type, right_value_type>* rmap_;
        const LeftKeyFn* left_key_;
        const Merger* merger_;
    };

    JoinRange(LeftRange& left,
              RightRange& right,
              LeftKeyFn left_key,
              RightKeyFn right_key,
              Merger merger)
        : left_(left), left_key_(std::move(left_key)), merger_(std::move(merger)) {
        for (auto&& elem : right) {
            auto key = right_key(elem);
            right_map_.emplace(key, elem);
        }
    }

    iterator begin() {
        return iterator(left_.begin(), left_.end(), &right_map_, &left_key_, &merger_);
    }

    iterator end() {
        return iterator(left_.end(), left_.end(), &right_map_, &left_key_, &merger_);
    }

    const_iterator begin() const {
        return const_iterator(left_.begin(), left_.end(), &right_map_, &left_key_, &merger_);
    }

    const_iterator end() const {
        return const_iterator(left_.end(), left_.end(), &right_map_, &left_key_, &merger_);
    }

private:
    LeftRange& left_;
    std::map<right_key_type, right_value_type> right_map_;
    LeftKeyFn left_key_;
    Merger merger_;
};

template <typename RightRange, typename LeftKeyFn, typename RightKeyFn>
auto Join(RightRange& right, LeftKeyFn left_key, RightKeyFn right_key) {
    return [&right, left_key, right_key](auto& left) {
        using LeftRange = std::remove_reference_t<decltype(left)>;
        using RightRangeT = std::remove_reference_t<decltype(right)>;
        using LeftVal = std::remove_reference_t<decltype(*std::declval<LeftRange&>().begin())>;
        using RightVal = std::remove_reference_t<decltype(*std::declval<RightRangeT&>().begin())>;

        auto merger = [](const LeftVal& l, std::optional<RightVal> r) {
            return JoinResult<LeftVal, RightVal>{l, r};
        };

        return JoinRange<LeftRange, RightRangeT, LeftKeyFn, RightKeyFn, decltype(merger)>(
            left, right, left_key, right_key, merger
        );
    };
}

template <typename RightRange>
auto Join(RightRange& right) {
    return [&right](auto& left) {
        using LeftRange = std::remove_reference_t<decltype(left)>;
        using RightRangeT = std::remove_reference_t<decltype(right)>;
        using LeftKV = typename LeftRange::value_type;
        using RightKV = typename RightRangeT::value_type;

        static_assert(is_kv<LeftKV>::value && is_kv<RightKV>::value, "Join(range) is only for KV!");

        using LeftVal = decltype(std::declval<LeftKV>().value);
        using RightVal = decltype(std::declval<RightKV>().value);

        auto left_key = [](const LeftKV& kv) { return kv.key; };
        auto right_key = [](const RightKV& kv) { return kv.key; };

        auto merger = [](const LeftKV& l, std::optional<RightKV> r) {
            if (r) {
                return JoinResult<LeftVal, RightVal>{l.value, r->value};
            } else {
                return JoinResult<LeftVal, RightVal>{l.value, std::nullopt};
            }
        };

        return JoinRange<LeftRange, RightRangeT, decltype(left_key), decltype(right_key), decltype(merger)>(
            left, right, left_key, right_key, merger
        );
    };
}