#pragma once

#include <fmt/format.h>
#include <optional>
#include <string>

namespace lidl {
struct source_info {
    int line = -1, column = -1, pos = -1;
    std::optional<std::string> origin;
};

inline std::string to_string(const source_info& src_info) {
    if (!src_info.origin) {
        return fmt::format("{}:{}", src_info.line + 1, src_info.column + 1);
    }
    return fmt::format(
        "{}:{} in {}", src_info.line + 1, src_info.column + 1, *src_info.origin);
}
} // namespace lidl