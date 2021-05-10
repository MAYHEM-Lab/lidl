#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lidl::ast {
struct node {};
using identifier           = std::string;
using qualified_identifier = std::string;

struct name : node {
    qualified_identifier base;
    std::optional<std::vector<std::variant<int64_t, name>>> args;
};

struct member : node {
    identifier name;
    ast::name type_name;
};

struct structure_body {
    std::vector<member> members;
    std::optional<ast::name> extends;
};

struct union_body {
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
    union_body body;
};

struct generic_union : node {
    std::vector<generic_parameter> params;
    identifier name;
    union_body body;
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

struct expression : node {};

struct string_literal_expression : expression {
    explicit string_literal_expression(std::string val)
        : literal(std::move(val)) {
    }
    std::string literal;
};

struct id_expression : expression {
    ast::name name;
};

struct function_call_expression : expression {
    std::unique_ptr<expression> function_name;
    std::vector<std::unique_ptr<expression>> arguments;
};

struct static_assertion : node {
    // std::unique_ptr<expression> body;
    std::shared_ptr<expression> error_message;
};

using element = std::variant<structure,
                             union_,
                             enumeration,
                             service,
                             generic_structure,
                             generic_union,
                             static_assertion>;

struct module {
    std::optional<metadata> meta;
    std::vector<element> elements;
};
} // namespace lidl::ast
