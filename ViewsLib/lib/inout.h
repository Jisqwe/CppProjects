#pragma once
#include <iostream>

struct Out {
    explicit Out(std::ostream& os = std::cout) : os_(&os) {}
    template <typename Range>
    void operator()(Range& range) const {
        for (const auto& e : range) {
            *os_ << e << " ";
        }
        *os_ << std::endl;
    }
private:
    std::ostream* os_;
};

struct Write {
    Write(std::ostream& os, char sep) : os_(&os), sep_(sep) {}
    template <typename Range>
    bool operator()(Range& range) const {
        try {
            for (const auto& e : range) {
                *os_ << e << sep_;
                if (!(*os_)) {
                    return false;
                }
            }
            return true;
        } catch (...) {
            return false;
        }
    }
private:
    std::ostream* os_;
    char sep_;
};