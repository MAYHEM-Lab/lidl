#pragma once

#include <iterator>
#include <string_view>
#include <vector>

namespace lidl {
template<class OutIt>
void split(std::string_view sv, std::string_view delim, OutIt it) {
    while (true) {
        if (sv.empty())
            break;
        auto i = sv.find(delim);
        *it++  = sv.substr(0, i);
        if (i == sv.npos) {
            break;
        }
        sv = sv.substr(i + delim.size());
    }
}

inline std::vector<std::string_view> split(std::string_view sv, std::string_view delim) {
    std::vector<std::string_view> splitted;
    split(sv, delim, std::back_inserter(splitted));
    return splitted;
}
} // namespace lidl