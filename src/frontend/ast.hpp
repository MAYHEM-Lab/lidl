#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lidl::ast {
struct node {};
using identifier = std::string;
using qualified_identifier = std::string;

struct name : node {
    qualified_identifier base;
    std::optional<std::vector<std::variant<int64_t, name>>> args;
};

struct member : node {
    identifier name;
    ast::name type_name;
};

struct structure_body : node {
    std::vector<member> members;
    std::optional<ast::name> extends;
};

struct structure : node {
    identifier name;
    structure_body body;
};

struct generic_parameter : node {
    identifier name;
};

struct generic_structure : node {
    std::vector<generic_parameter> params;
    identifier name;
    structure_body body;
};

struct union_ : node {
    identifier name;
    std::vector<member> members;
    std::optional<ast::name> extends;
};

struct enumeration : node {
    identifier name;
    std::vector<std::pair<std::string, std::optional<int64_t>>> values;
    std::optional<ast::name> extends;
};

struct service : node {
    struct procedure : node {
        struct parameter : node {
            ast::name type;
            identifier name;
        };
        identifier name;
        std::vector<parameter> params;
        ast::name return_type;
    };

    identifier name;
    std::vector<procedure> procedures;
    std::optional<ast::name> extends;
};

struct metadata {
    std::optional<std::string> name_space;
    std::vector<std::string> imports;
};

using element = std::variant<structure, union_, enumeration, service, generic_structure>;

struct module {
    std::optional<metadata> meta;
    std::vector<element> elements;
};
} // namespace lidl::ast
