#include "parser.hpp"

#include "ast.hpp"

#include <lexy/dsl.hpp>
#include <lexy/input/file.hpp> // lexy::read_file
#include <lexy/input/string_input.hpp>
#include <lexy/parse.hpp> // lexy::parse
#include <lexy_ext/report_error.hpp>

namespace lidl::grammar {
namespace {
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

struct name {
    struct name_args {
        struct name_arg {
            static constexpr auto rule =
                (dsl::peek_not(dsl::ascii::alpha) >>
                 dsl::integer<int64_t>(dsl::digits<>)) |
                (dsl::peek(dsl::ascii::alpha) >> dsl::recurse<name>);
            static constexpr auto value =
                lexy::construct<std::variant<int64_t, ast::name>>;
        };

        static constexpr auto rule = dsl::angle_bracketed.opt_list(
            dsl::p<name_arg>, dsl::trailing_sep(dsl::comma));
        static constexpr auto value =
            lexy::as_list<std::vector<std::variant<int64_t, ast::name>>>;
    };

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
    static constexpr auto rule = dsl::curly_bracketed.opt_list(
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

struct enumeration {
    struct enum_value_list {
        struct enum_value {
            static constexpr auto rule =
                dsl::p<identifier> + ws +
                dsl::opt(dsl::lit_c<'='> >> ws + dsl::integer<int64_t>(dsl::digits<>));

            static constexpr auto value =
                lexy::construct<std::pair<std::string, std::optional<int64_t>>>;
        };

        static constexpr auto rule = dsl::curly_bracketed.list(
            dsl::p<enum_value>, dsl::trailing_sep(dsl::comma + dsl::newline));
        static constexpr auto value =
            lexy::as_list<std::vector<std::pair<std::string, std::optional<int64_t>>>>;
    };

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

    static constexpr auto value = lexy::construct<ast::element>;
};

struct module {
    struct module_elements {
        static constexpr auto rule =
            dsl::list(ws + dsl::p<element>, dsl::sep(dsl::newline >> ws));

        static constexpr auto value = lexy::as_list<std::vector<ast::element>>;
    };

    static constexpr auto rule = dsl::p<module_elements> + dsl::eof;

    static constexpr auto value = lexy::construct<ast::module>;
};
} // namespace
} // namespace lidl::grammar

namespace lidl::frontend {
std::optional<ast::module> parse_module(std::string_view input_text) {
    lexy::string_input input(input_text.data(), input_text.size());
    auto result = lexy::parse<lidl::grammar::module>(input, lexy_ext::report_error);
    if (!result)
        return {};
    return result.value();
}
} // namespace lidl::frontend
