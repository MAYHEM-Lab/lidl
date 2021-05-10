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

struct qualified_identifier {
    static constexpr auto rule =
        ws +
        dsl::capture(dsl::ascii::alpha +
                     dsl::while_(dsl::ascii::alnum / dsl::lit_c<'_'> / dsl::lit_c<':'>));
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
            ws + dsl::p<name_arg>, dsl::trailing_sep(dsl::comma));
        static constexpr auto value =
            lexy::as_list<std::vector<std::variant<int64_t, ast::name>>>;
    };

    static constexpr auto rule = (LEXY_MEM(base) = dsl::p<qualified_identifier>)+(
        LEXY_MEM(args) = dsl::opt(dsl::p<name_args>));
    static constexpr auto value = lexy::as_aggregate<ast::name>;
};

struct generic_parameters {
    struct parameter {
        static constexpr auto rule  = (LEXY_MEM(name) = dsl::p<identifier>);
        static constexpr auto value = lexy::as_aggregate<ast::generic_parameter>;
    };

    static constexpr auto rule = dsl::angle_bracketed.opt_list(
        dsl::p<parameter>, dsl::trailing_sep(dsl::comma >> ws));
    static constexpr auto value = lexy::as_list<std::vector<ast::generic_parameter>>;
};

struct member_list {
    struct member {
        static constexpr auto rule = (LEXY_MEM(name) = dsl::p<identifier>)+ws +
                                     dsl::colon + ws +
                                     (LEXY_MEM(type_name) = dsl::p<name>);
        static constexpr auto value = lexy::as_aggregate<ast::member>;
    };

    static constexpr auto rule = dsl::curly_bracketed.opt_list(
        dsl::p<member>, dsl::trailing_sep(dsl::semicolon + dsl::newline));
    static constexpr auto value = lexy::as_list<std::vector<ast::member>>;
};

struct structure_body {
    static constexpr auto rule =
        dsl::if_(dsl::colon >> (LEXY_MEM(extends) = dsl::p<name>)) + ws +
        (LEXY_MEM(members) = dsl::p<member_list>);

    static constexpr auto value = lexy::as_aggregate<ast::structure_body>;
};

struct structure {
    static constexpr auto rule = LEXY_LIT("struct") + dsl::ascii::blank +
                                 (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
                                 (LEXY_MEM(body) = dsl::p<structure_body>);

    static constexpr auto value = lexy::as_aggregate<ast::structure>;
};

struct generic_structure {
    static constexpr auto rule =
        LEXY_LIT("struct") +
        (LEXY_MEM(params) = dsl::p<generic_parameters>)+dsl::ascii::blank +
        (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
        (LEXY_MEM(body) = dsl::p<structure_body>);

    static constexpr auto value = lexy::as_aggregate<ast::generic_structure>;
};

struct union_body {
    static constexpr auto rule =
        dsl::if_(dsl::colon >> (LEXY_MEM(extends) = dsl::p<name>)) + ws +
        (LEXY_MEM(members) = dsl::p<member_list>);

    static constexpr auto value = lexy::as_aggregate<ast::union_body>;
};

struct union_ {
    static constexpr auto rule = LEXY_LIT("union") + dsl::ascii::blank +
                                 (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
                                 (LEXY_MEM(body) = dsl::p<union_body>);

    static constexpr auto value = lexy::as_aggregate<ast::union_>;
};

struct generic_union {
    static constexpr auto rule =
        LEXY_LIT("union") +
        (LEXY_MEM(params) = dsl::p<generic_parameters>)+dsl::ascii::blank +
        (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
        (LEXY_MEM(body) = dsl::p<union_body>);

    static constexpr auto value = lexy::as_aggregate<ast::generic_union>;
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

struct service {
    struct proc_list {
        struct procedure {
            struct param_list {
                struct parameter {
                    static constexpr auto rule =
                        (LEXY_MEM(name) = dsl::p<identifier>)+ws + dsl::colon + ws +
                        (LEXY_MEM(type) = dsl::p<name>);
                    static constexpr auto value =
                        lexy::as_aggregate<ast::service::procedure::parameter>;
                };

                static constexpr auto rule = dsl::parenthesized.opt_list(
                    dsl::p<parameter>, dsl::sep(dsl::comma >> ws));
                static constexpr auto value =
                    lexy::as_list<std::vector<ast::service::procedure::parameter>>;
            };

            static constexpr auto rule =
                (LEXY_MEM(name) = dsl::p<identifier>)+(LEXY_MEM(params) =
                                                           dsl::p<param_list>)+ws +
                LEXY_LIT("->") + ws + (LEXY_MEM(return_type) = dsl::p<name>);
            static constexpr auto value = lexy::as_aggregate<ast::service::procedure>;
        };

        static constexpr auto rule = dsl::curly_bracketed.opt_list(
            dsl::p<procedure>, dsl::trailing_sep(dsl::semicolon + dsl::newline));
        static constexpr auto value = lexy::as_list<std::vector<ast::service::procedure>>;
    };

    static constexpr auto rule =
        LEXY_LIT("service") + dsl::ascii::blank +
        (LEXY_MEM(name) = dsl::p<identifier>)+dsl::ascii::blank +
        dsl::if_(dsl::colon >> (LEXY_MEM(extends) = dsl::p<name>)) + ws +
        (LEXY_MEM(procedures) = dsl::p<proc_list>);

    static constexpr auto value = lexy::as_aggregate<ast::service>;
};

struct metadata {
    struct import_list {
        struct import {
            static constexpr auto rule = dsl::quoted(dsl::code_point);

            static constexpr auto value =
                lexy::as_string<std::string, lexy::utf8_encoding>;
        };

        static constexpr auto rule =
            dsl::list(dsl::if_(LEXY_LIT("import") >> ws + dsl::p<import> + ws),
                      dsl::sep(dsl::semicolon >> ws));

        static constexpr auto value = lexy::as_list<std::vector<std::string>>;
    };

    struct name_space {
        static constexpr auto rule =
            LEXY_LIT("namespace") + ws + dsl::capture(dsl::until(dsl::semicolon));

        static constexpr auto value = lexy::as_string<std::string, lexy::utf8_encoding>;
    };

    static constexpr auto rule = dsl::partial_combination(
        dsl::peek(LEXY_LIT("import")) >> (LEXY_MEM(imports) = dsl::p<import_list>)+ws,
        dsl::peek(LEXY_LIT("namespace")) >>
            (LEXY_MEM(name_space) = dsl::p<name_space>)+ws);

    static constexpr auto value = lexy::as_aggregate<ast::metadata>;
};

struct argument_list {
    struct parameter {
        static constexpr auto rule = (LEXY_MEM(name) = dsl::p<identifier>)+ws +
                                     dsl::colon + ws + (LEXY_MEM(type) = dsl::p<name>);
        static constexpr auto value =
            lexy::as_aggregate<ast::service::procedure::parameter>;
    };

    static constexpr auto rule =
        dsl::parenthesized.opt_list(dsl::p<parameter>, dsl::sep(dsl::comma >> ws));
    static constexpr auto value =
        lexy::as_list<std::vector<ast::service::procedure::parameter>>;
};

struct string_literal_expression {
    static constexpr auto rule = dsl::quoted(dsl::code_point);

    static constexpr auto value =
        lexy::as_string<std::string, lexy::utf8_encoding> >>
        lexy::new_<ast::string_literal_expression,
                   std::shared_ptr<ast::string_literal_expression>>;
};

struct static_assertion {
    static constexpr auto rule =
        LEXY_LIT("static_assert(") +
        (LEXY_MEM(error_message) = dsl::p<string_literal_expression>)+dsl::lit<')'>;
    static constexpr auto value = lexy::as_aggregate<ast::static_assertion>;
};

struct element {
    static constexpr auto rule =
        (dsl::peek(LEXY_LIT("static_assert")) >> dsl::p<static_assertion>) |
        (dsl::peek(LEXY_LIT("struct<")) >> dsl::p<generic_structure>) |
        (dsl::peek(LEXY_LIT("struct")) >> dsl::p<structure>) |
        (dsl::peek(LEXY_LIT("enum")) >> dsl::p<enumeration>) |
        (dsl::peek(LEXY_LIT("union<")) >> dsl::p<generic_union>) |
        (dsl::peek(LEXY_LIT("union")) >> dsl::p<union_>) |
        (dsl::peek(LEXY_LIT("service")) >> dsl::p<service>);

    static constexpr auto value = lexy::construct<ast::element>;
};

struct module {
    struct module_elements {
        static constexpr auto rule =
            dsl::list(ws + dsl::p<element>, dsl::sep(dsl::newline >> ws));

        static constexpr auto value = lexy::as_list<std::vector<ast::element>>;
    };

    static constexpr auto rule = (LEXY_MEM(meta) = dsl::p<metadata>)+(
                                     LEXY_MEM(elements) = dsl::p<module_elements>)+ws +
                                 dsl::eof;

    static constexpr auto value = lexy::as_aggregate<ast::module>;
};
} // namespace
} // namespace lidl::grammar

namespace lidl::frontend {
std::optional<ast::module> parse_module(std::string_view input_text) {
    lexy::string_input<lexy::utf8_encoding> input(input_text.data(), input_text.size());

    auto result = lexy::parse<lidl::grammar::element>(input, lexy_ext::report_error);
    //    if (!result)
    //        return {};
    //    if (result.value().meta && result.value().meta->name_space) {
    //        //        result.value().meta->name_space->pop_back();
    //    }
    //    return result.value();
    return {};
}
} // namespace lidl::frontend
