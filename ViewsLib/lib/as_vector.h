#pragma once
#include <vector>

struct AsVector {
    template <typename Range>
    auto operator()(Range& range) const {
        using value_type = typename Range::value_type;
        std::vector<value_type> result;
        for (auto it = range.begin(); it != range.end(); ++it)
            result.push_back(*it);
        return result;
    }
};