#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lidl::ast {
struct name {
    std::string base;
    std::optional<std::vector<std::variant<int64_t, name>>> args;
};

struct member {
    std::string name;
    ast::name type_name;
};

struct structure {
    std::string name;
    std::vector<member> members;
    std::optional<ast::name> extends;
};

struct union_ {
    std::string name;
    std::vector<member> members;
    std::optional<ast::name> extends;
};

struct enumeration {
    std::string name;
    std::vector<std::pair<std::string, std::optional<int64_t>>> values;
    std::optional<ast::name> extends;
};

using element = std::variant<structure, union_, enumeration>;

struct module {
    std::vector<element> elements;
};
} // namespace lidl::ast
