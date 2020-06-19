#pragma once

#include <optional>
#include <string>
#include <vector>


namespace lidl {
struct module_meta {
    std::optional<std::string> name;
    std::vector<std::string> imports;
};
} // namespace lidl