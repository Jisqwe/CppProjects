#pragma once
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

class Dir {
public:
    using value_type = fs::path;
    using iterator = std::vector<fs::path>::iterator;
    using const_iterator = std::vector<fs::path>::const_iterator;

    Dir(const std::string& dirname, bool recursive)
    {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(dirname)) {
                if (entry.is_regular_file())
                    files_.push_back(entry.path());
            }
        } else {
            for (const auto& entry : fs::directory_iterator(dirname)) {
                if (entry.is_regular_file())
                    files_.push_back(entry.path());
            }
        }
    }

    iterator begin() { return files_.begin(); }
    iterator end()   { return files_.end(); }
    const_iterator begin() const { return files_.begin(); }
    const_iterator end()   const { return files_.end(); }

private:
    std::vector<fs::path> files_;
};

struct OpenFiles {
    template <typename Range>
    auto operator()(Range& range) const {
        using Path = typename Range::value_type;
        std::vector<std::shared_ptr<std::ifstream>> files;
        for (const Path& path : range) {
            auto f = std::make_shared<std::ifstream>(path, std::ios::in);
            if (f->is_open())
                files.push_back(f);
        }
        return files;
    }
};