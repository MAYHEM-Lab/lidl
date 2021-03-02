#include <lexy/dsl.hpp>
#include <lexy_ext/report_error.hpp> // lexy_ext::report_error
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ast {
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
} // namespace ast

namespace {
namespace grammar {
namespace dsl = lexy::dsl;
struct comment {
    static constexpr auto rule  = LEXY_LIT("//") >> dsl::until(dsl::newline);
    static constexpr auto value = lexy::noop;
};

constexpr auto ws = dsl::whitespace(dsl::ascii::blank | dsl::newline | dsl::p<comment>);

struct identifier {
    static constexpr auto rule =
        ws + dsl::capture(dsl::ascii::alpha +
                          dsl::while_(dsl::ascii::alnum / dsl::lit_c<'_'>));
    static constexpr auto value = lexy::as_string<std::string>;
};

struct name;

struct name_arg {
    static constexpr auto rule =
        (dsl::peek_not(dsl::ascii::alpha) >> dsl::integer<int64_t>(dsl::digits<>)) |
        (dsl::peek(dsl::ascii::alpha) >> dsl::recurse<name>);
    static constexpr auto value = lexy::construct<std::variant<int64_t, ast::name>>;
};

struct name_args {
    static constexpr auto rule =
        dsl::angle_bracketed.opt_list(dsl::p<name_arg>, dsl::trailing_sep(dsl::comma));
    static constexpr auto value =
        lexy::as_list<std::vector<std::variant<int64_t, ast::name>>>;
};

struct name {
    static constexpr auto rule = (LEXY_MEM(base) = dsl::p<identifier>)+(
        LEXY_MEM(args) = dsl::opt(dsl::p<name_args>));
    static constexpr auto value = lexy::as_aggregate<ast::name>;
};

struct member {
    static constexpr auto rule = (LEXY_MEM(name) = dsl::p<identifier>)+ws + dsl::colon +
                                 ws + (LEXY_MEM(type_name) = dsl::p<name>);
    static constexpr auto value = lexy::as_aggregate<ast::member>;
};

struct member_list {
    static constexpr auto rule = dsl::curly_bracketed.list(
        dsl::p<member>, dsl::trailing_sep(dsl::semicolon + dsl::newline));
    static constexpr auto value = lexy::as_list<std::vector<ast::member>>;
};

struct structure {
    static constexpr auto rule =
        LEXY_LIT("struct") + dsl::ascii::blank +
        (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
        dsl::if_(dsl::colon >> (LEXY_MEM(extends) = dsl::p<name>)) + ws +
        (LEXY_MEM(members) = dsl::p<member_list>);

    static constexpr auto value = lexy::as_aggregate<ast::structure>;
};

struct union_ {
    static constexpr auto rule =
        LEXY_LIT("union") + dsl::ascii::blank +
        (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
        dsl::if_(dsl::colon >> (LEXY_MEM(extends) = dsl::p<name>)) + ws +
        (LEXY_MEM(members) = dsl::p<member_list>);

    static constexpr auto value = lexy::as_aggregate<ast::union_>;
};

struct enum_value {
    static constexpr auto rule =
        dsl::p<identifier> + ws +
        dsl::opt(dsl::lit_c<'='> >> ws + dsl::integer<int64_t>(dsl::digits<>));

    static constexpr auto value =
        lexy::construct<std::pair<std::string, std::optional<int64_t>>>;
};

struct enum_value_list {
    static constexpr auto rule = dsl::curly_bracketed.list(
        dsl::p<enum_value>, dsl::trailing_sep(dsl::comma + dsl::newline));
    static constexpr auto value =
        lexy::as_list<std::vector<std::pair<std::string, std::optional<int64_t>>>>;
};

struct enumeration {
    static constexpr auto rule =
        LEXY_LIT("enum") + dsl::ascii::blank +
        (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
        dsl::if_(dsl::colon >> (LEXY_MEM(extends) = dsl::p<name>)) + ws +
        (LEXY_MEM(values) = dsl::p<enum_value_list>);

    static constexpr auto value = lexy::as_aggregate<ast::enumeration>;
};

struct element {
    static constexpr auto rule = (dsl::peek(LEXY_LIT("struct")) >> dsl::p<structure>) |
                                 (dsl::peek(LEXY_LIT("enum")) >> dsl::p<enumeration>) |
                                 (dsl::peek(LEXY_LIT("union")) >> dsl::p<union_>);

    static constexpr auto value =
        lexy::construct<std::variant<ast::structure, ast::enumeration, ast::union_>>;
};

struct module {
    static constexpr auto rule =
        dsl::list(ws + dsl::p<element>, dsl::sep(dsl::newline >> ws)) + dsl::eof;

    static constexpr auto value = lexy::as_list<
        std::vector<std::variant<ast::structure, ast::enumeration, ast::union_>>>;
};
} // namespace grammar
} // namespace

#include <iostream>
#include <lexy/input/file.hpp> // lexy::read_file
#include <lexy/input/string_input.hpp>
#include <lexy/parse.hpp> // lexy::parse

int main(int argc, char** argv) {
    constexpr auto in = R"__(enum vec3f : base {
    foo,
    bar = 5,
    yolo,
}


union vec3f : base {
    x: vec<vec<f32>>;
    // some member
    y: f32;
    z: f32;
}

// some vector
struct vec3f : base {
    x: vec<vec<f32>>;
    y: f32;
    z: f32;
})__";

    lexy::string_input input(in, strlen(in));
    auto result = lexy::parse<grammar::module>(input, lexy_ext::report_error);
    if (!result)
        return 2;
    auto& config = result.value();
    std::cerr << config.size() << '\n';
    //    for (auto& mem : config.members) {
    //        std::cerr << mem.type_name.base << '\n';
    //        if (mem.type_name.args) {
    //            for (auto& arg : *mem.type_name.args) {
    //                std::cerr << std::get<ast::name>(arg).base << '\n';
    //            }
    //        }
    //    }
}