#pragma once
#include <type_traits>

template <typename Range>
class DataFlowRef {
public:
    using iterator = typename Range::iterator;
    using const_iterator = typename Range::const_iterator;
    using value_type = typename Range::value_type;

    explicit DataFlowRef(Range& range) : range_(&range) {}

    iterator begin() { return range_->begin(); }
    iterator end() { return range_->end(); }
    const_iterator begin() const { return range_->begin(); }
    const_iterator end() const { return range_->end(); }

    Range& get() { return *range_; }
    const Range& get() const { return *range_; }

private:
    Range* range_;
};

template <typename Range>
auto AsDataFlow(Range& range) {
    return DataFlowRef<Range>(range);
}

template <typename Range, typename Adapter>
auto operator|(Range&& range, Adapter&& adapter) {
    return adapter(range);
}