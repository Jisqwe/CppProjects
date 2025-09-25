#pragma once
#include <istream>

inline std::istream& get_istream(std::istream& is) {
    return is;
}

template <typename Stream>
inline std::istream& get_istream(Stream* p) {
    return *p;
}

template <typename T>
inline auto get_istream(T& t) -> decltype(static_cast<std::istream&>(*t)) {
    return *t;
}