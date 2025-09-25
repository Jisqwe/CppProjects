#pragma once
#include <vector>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <iterator>

template <typename InitAcc, typename AggFunc, typename KeyFunc>
struct AggregateByKeyAdapter {
    InitAcc init_acc;
    AggFunc agg_func;
    KeyFunc key_func;

    template <typename Range>
    auto operator()(Range& range) const {
        using Element = typename std::remove_reference_t<Range>::value_type;
        using Key = decltype(key_func(std::declval<Element>()));
        using Acc = InitAcc;

        std::vector<std::pair<Key, Acc>> acc_vec;

        for (auto it = range.begin(); it != range.end(); ++it) {
            const auto& elem = *it;
            Key key = key_func(elem);

            auto found = std::find_if(
                acc_vec.begin(),
                acc_vec.end(),
                [&key](const std::pair<Key, Acc>& p) { return p.first == key; }
            );

            if (found == acc_vec.end()) {
                acc_vec.emplace_back(key, init_acc);
                found = std::prev(acc_vec.end());
            }

            agg_func(elem, found->second);
        }

        return acc_vec;
    }
};

template <typename InitAcc, typename AggFunc, typename KeyFunc>
auto AggregateByKey(InitAcc init_acc, AggFunc agg_func, KeyFunc key_func) {
    return AggregateByKeyAdapter<InitAcc, AggFunc, KeyFunc>{init_acc, agg_func, key_func};
}