#pragma once

#include <string>
#include <optional>

namespace lidl {
struct source_info {
    int line = -1, column = -1, pos = -1;
    std::optional<std::string> origin;
};
}