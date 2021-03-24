#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lidl::ast {
struct node {};

struct name : node {
    std::string base;
    std::optional<std::vector<std::variant<int64_t, name>>> args;
};

struct member : node {
    std::string name;
    ast::name type_name;
};

struct structure : node {
    std::string name;
    std::vector<member> members;
    std::optional<ast::name> extends;
};

struct union_ : node {
    std::string name;
    std::vector<member> members;
    std::optional<ast::name> extends;
};

struct enumeration : node {
    std::string name;
    std::vector<std::pair<std::string, std::optional<int64_t>>> values;
    std::optional<ast::name> extends;
};

struct service : node {
    struct procedure : node {
        struct parameter : node {
            ast::name type;
            std::string name;
        };
        std::string name;
        std::vector<parameter> params;
        ast::name return_type;
    };

    std::string name;
    std::vector<procedure> procedures;
    std::optional<ast::name> extends;
};

struct metadata {
    std::optional<std::string> name_space;
    std::vector<std::string> imports;
};

using element = std::variant<structure, union_, enumeration, service>;

struct module {
    std::optional<metadata> meta;
    std::vector<element> elements;
};
} // namespace lidl::ast
