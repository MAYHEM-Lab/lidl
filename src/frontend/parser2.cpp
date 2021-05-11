#include "parser2.hpp"

#include "lexer.hpp"

#include <array>
#include <iostream>
#include <lidl/error.hpp>
#include <numeric>
#include <span>

namespace lidl::frontend {
namespace {
template<auto...>
struct list {};
struct parser {
    std::optional<ast::identifier> parse_id() {
        if (auto res = match(token_type::identifier)) {
            auto [id] = *res;
            return ast::identifier{std::string(id.content)};
        }
        return {};
    }

    std::optional<ast::qualified_identifier> parse_qualified_id() {
        std::vector<ast::identifier> parts;
        if (auto id = parse_id()) {
            parts.emplace_back(std::move(*id));
        } else {
            return {};
        }

        while (match(token_type::coloncolon)) {
            if (auto id = parse_id()) {
                parts.emplace_back(std::move(*id));
            } else {
                break;
            }
        }

        return ast::qualified_identifier{std::accumulate(
            parts.begin(), parts.end(), std::string(), [](auto cur, auto& next) {
                if (cur.empty()) {
                    return next;
                }
                return cur + "::" + next;
            })};
    }

    std::optional<std::variant<int64_t, ast::name>> parse_generic_arg() {
        if (auto name = parse_name()) {
            return name;
        }

        // TODO: parse integral expression here!

        return {};
    }

    std::optional<ast::name> parse_name() {
        auto backup = m_tokens;
        auto base   = parse_qualified_id();

        if (!base) {
            return {};
        }

        ast::name res;
        res.src_info = backup.front().src_info;
        res.base     = *base;

        auto members = parse_list<&parser::parse_generic_arg,
                                  token_type::left_angular,
                                  token_type::comma,
                                  token_type::right_angular>();

        if (members) {
            res.args = std::move(*members);
        }

        return res;
    }

    std::optional<ast::service::procedure::parameter> parse_proc_param() {
        auto backup = m_tokens;
        auto id     = parse_id();

        if (!id) {
            return {};
        }

        if (!match(token_type::colon)) {
            m_tokens = backup;
            return {};
        }

        auto type_id = parse_name();

        if (!type_id) {
            report_user_error(error_type::fatal,
                              m_tokens.front().src_info,
                              "Expected parameter type name");
        }

        ast::service::procedure::parameter res;
        res.src_info = backup.front().src_info;
        res.name     = std::move(*id);
        res.type     = std::move(*type_id);
        return res;
    }

    std::optional<ast::service::procedure> parse_procedure() {
        auto backup = m_tokens;

        auto id = parse_id();
        if (!id) {
            return {};
        }

        auto params = parse_list<&parser::parse_proc_param,
                                 token_type::left_parens,
                                 token_type::comma,
                                 token_type::right_parens>();

        if (!params) {
            report_user_error(error_type::fatal,
                              m_tokens.front().src_info,
                              "Expected procedure parameter list");
        }

        if (!match(token_type::arrow)) {
            report_user_error(
                error_type::fatal, m_tokens.front().src_info, "Expected arrow");
        }

        auto ret_type_name = parse_name();

        if (!ret_type_name) {
            report_user_error(
                error_type::fatal, m_tokens.front().src_info, "Expected return type");
        }

        ast::service::procedure res;
        res.src_info    = backup.front().src_info;
        res.name        = std::move(*id);
        res.return_type = std::move(*ret_type_name);
        res.params      = std::move(*params);
        return res;
    }

    std::optional<ast::service> parse_service() {
        auto backup = m_tokens;

        if (!match(token_type::kw_service)) {
            return {};
        }

        auto name = parse_id();
        if (!name) {
            report_user_error(
                error_type::fatal, m_tokens.front().src_info, "Expected service name");
        }

        ast::service res;
        res.src_info = backup.front().src_info;
        res.name     = std::move(*name);

        if (match(token_type::colon)) {
            auto base = parse_name();
            if (!base) {
                report_user_error(error_type::fatal,
                                  m_tokens.front().src_info,
                                  "Expected service base");
            }
            res.extends = std::move(*base);
        }

        auto mems = parse_list<&parser::parse_procedure,
                               token_type::left_brace,
                               token_type::semicolon,
                               token_type::right_brace>();

        if (!mems) {
            report_user_error(
                error_type::fatal, m_tokens.front().src_info, "Expected procedure list");
        }

        res.procedures = std::move(*mems);

        return res;
    }

    std::optional<ast::member> parse_member() {
        // identifier: type;

        auto backup = m_tokens;
        auto id     = parse_id();

        if (!id) {
            return {};
        }

        if (!match(token_type::colon)) {
            m_tokens = backup;
            return {};
        }

        auto type_id = parse_name();

        if (!type_id) {
            m_tokens = backup;
            return {};
        }

        ast::member res;
        res.src_info  = backup.front().src_info;
        res.name      = std::move(*id);
        res.type_name = std::move(*type_id);
        return res;
    }

    std::optional<ast::union_body> parse_union_body() {
        ast::union_body res;
        if (match(token_type::colon)) {
            auto base = parse_name();
            if (!base) {
                report_user_error(
                    error_type::fatal, m_tokens.front().src_info, "Expected union base");
            }
            res.extends = std::move(*base);
        }

        auto mems = parse_list<&parser::parse_member,
                               token_type::left_brace,
                               token_type::semicolon,
                               token_type::right_brace>();

        if (!mems) {
            report_user_error(
                error_type::fatal, m_tokens.front().src_info, "Expected union members");
        }

        res.members = std::move(*mems);

        return res;
    }

    std::optional<ast::structure_body> parse_struct_body() {
        auto backup = m_tokens;
        ast::structure_body res;
        if (match(token_type::colon)) {
            auto base = parse_name();
            if (!base) {
                m_tokens = backup;
                return {};
            }
            res.extends = std::move(*base);
        }

        auto mems = parse_list<&parser::parse_member,
                               token_type::left_brace,
                               token_type::semicolon,
                               token_type::right_brace>();

        if (!mems) {
            m_tokens = backup;
            return {};
        }

        res.members = std::move(*mems);

        return res;
    }

    std::optional<ast::generic_union> parse_generic_union() {
        auto backup = m_tokens;
        if (!match(token_type::kw_union)) {
            return {};
        }

        auto params = parse_list<&parser::parse_generic_param,
                                 token_type::left_angular,
                                 token_type::comma,
                                 token_type::right_angular>();

        if (!params) {
            m_tokens = backup;
            return {};
        }

        ast::generic_union res;
        res.src_info = backup.front().src_info;
        res.params   = std::move(*params);

        auto id = parse_id();

        if (!id) {
            m_tokens = backup;
            return {};
        }

        res.name = std::move(*id);

        auto body = parse_union_body();
        if (!body) {
            m_tokens = backup;
            return {};
        }

        res.body = std::move(*body);

        return res;
    }

    std::optional<ast::union_> parse_union() {
        auto backup = m_tokens;
        if (!match(token_type::kw_union)) {
            return {};
        }

        ast::union_ res;
        res.src_info = backup.front().src_info;

        auto id = parse_id();

        if (!id) {
            m_tokens = backup;
            return {};
        }

        res.name = std::move(*id);

        auto body = parse_union_body();
        if (!body) {
            m_tokens = backup;
            return {};
        }

        res.body = std::move(*body);

        return res;
    }

    std::optional<ast::generic_parameter> parse_generic_param() {
        auto backup = m_tokens;
        auto id     = parse_id();
        if (!id) {
            return {};
        }
        ast::generic_parameter param;
        param.src_info = backup.front().src_info;
        param.name     = std::move(*id);
        return param;
    }

    std::optional<ast::generic_structure> parse_generic_structure() {
        auto backup = m_tokens;
        if (!match(token_type::kw_struct)) {
            return {};
        }

        auto params = parse_list<&parser::parse_generic_param,
                                 token_type::left_angular,
                                 token_type::comma,
                                 token_type::right_angular>();

        if (!params) {
            m_tokens = backup;
            return {};
        }

        ast::generic_structure res;
        res.src_info = backup.front().src_info;
        res.params   = std::move(*params);

        auto id = parse_id();

        if (!id) {
            m_tokens = backup;
            return {};
        }

        res.name = std::move(*id);

        auto body = parse_struct_body();
        if (!body) {
            m_tokens = backup;
            return {};
        }

        res.body = std::move(*body);

        return res;
    }

    std::optional<ast::structure> parse_structure() {
        auto backup = m_tokens;
        if (!match(token_type::kw_struct)) {
            return {};
        }

        ast::structure res;
        res.src_info = backup.front().src_info;

        auto id = parse_id();

        if (!id) {
            m_tokens = backup;
            return {};
        }

        res.name = std::move(*id);

        auto body = parse_struct_body();
        if (!body) {
            m_tokens = backup;
            return {};
        }

        res.body = std::move(*body);

        return res;
    }

    std::optional<std::pair<std::string, std::optional<int64_t>>> parse_enum_val() {
        auto backup = m_tokens;
        auto nm     = parse_id();
        if (!nm) {
            return {};
        }

        if (match(token_type::eq)) {
            if (auto mres = match(token_type::integer)) {
                auto [res] = *mres;
                return std::pair{
                    *nm, static_cast<int64_t>(std::stoll(std::string(res.content)))};
            } else {
                // error
                m_tokens = backup;
                return {};
            }
        }

        return std::pair{*nm, std::nullopt};
    }

    std::optional<ast::enumeration> parse_enum() {
        auto backup = m_tokens;
        if (!match(token_type::kw_enum)) {
            return {};
        }

        ast::enumeration res;
        res.src_info = backup.front().src_info;

        auto id = parse_id();

        if (!id) {
            m_tokens = backup;
            return {};
        }

        res.name = std::move(*id);

        if (match(token_type::colon)) {
            auto base = parse_name();
            if (!base) {
                m_tokens = backup;
                return {};
            }
            res.extends = std::move(*base);
        }

        auto vals = parse_list<&parser::parse_enum_val,
                               token_type::left_brace,
                               token_type::comma,
                               token_type::right_brace>();

        if (!vals) {
            m_tokens = backup;
            return {};
        }

        res.values = *vals;

        return res;
    }

    std::optional<ast::element> parse_element() {
        if (auto el = parse_generic_structure()) {
            return *el;
        }

        if (auto el = parse_structure()) {
            return *el;
        }

        if (auto el = parse_generic_union()) {
            return *el;
        }

        if (auto el = parse_union()) {
            return *el;
        }

        if (auto el = parse_enum()) {
            return *el;
        }

        if (auto el = parse_service()) {
            return *el;
        }

        return {};
    }

    std::optional<std::string> parse_import() {
        auto mres = match(
            token_type::kw_import, token_type::string_literal, token_type::semicolon);

        if (!mres) {
            return {};
        }

        auto& [_, import_path, __] = *mres;

        return std::string(import_path.content.begin() + 1,
                           import_path.content.end() - 1);
    }

    std::optional<ast::metadata> parse_metadata() {
        ast::metadata res;

        if (match(token_type::kw_namespace)) {
            auto nspace = parse_qualified_id();
            if (!nspace) {
                report_user_error(error_type::fatal,
                                  m_tokens.front().src_info,
                                  "Missing namespace name");
            }
            res.name_space = std::move(*nspace);
            if (!match(token_type::semicolon)) {
                report_user_error(error_type::fatal,
                                  m_tokens.front().src_info,
                                  "Missing semicolon after namespace name");
            }
        }

        while (auto import = parse_import()) {
            res.imports.emplace_back(std::move(*import));
        }

        return res;
    }

    std::optional<ast::module> parse_module() {
        ast::module res;

        auto metadata = parse_metadata();

        if (metadata) {
            res.meta = std::move(*metadata);
        }

        while (auto el = parse_element()) {
            res.elements.push_back(std::move(*el));
        }

        if (!match(token_type::eof)) {
            std::cerr << "Failed to parse!\n";
        }

        return res;
    }

    template<class... X>
    std::optional<const token> oneof(X... xs) {
        token_type tags[] = {xs...};
        return oneof_impl(tags);
    }

    template<class... X>
    std::optional<std::array<const token, sizeof...(X)>> peek(X... xs) {
        return peek_impl(std::make_index_sequence<sizeof...(xs)>(), xs...);
    }

    template<class... X>
    std::optional<std::array<const token, sizeof...(X)>> match(X... xs) {
        auto peek_res = peek(xs...);
        if (peek_res) {
            m_tokens = m_tokens.subspan<sizeof...(X)>();
        }
        return peek_res;
    }

    template<auto... xs>
    std::optional<std::array<const token, sizeof...(xs)>> match(list<xs...>) {
        return match(xs...);
    }

    std::optional<const token> oneof_impl(std::span<const token_type> tags) {
        for (auto tag : tags) {
            if (m_tokens.front().type == tag) {
                auto res = m_tokens.front();
                m_tokens = m_tokens.subspan<1>();
                return res;
            }
        }
        return {};
    }

    template<class... X, size_t... Is>
    std::optional<std::array<const token, sizeof...(X)>>
    peek_impl(std::index_sequence<Is...>, X... xs) {
        auto all_of = ((m_tokens.size() > Is && m_tokens[Is].type == xs) && ...);
        if (!all_of) {
            return {};
        }
        return std::array<const token, sizeof...(X)>{m_tokens[Is]...};
    }

    template<auto Rule, auto Start, auto Sep, auto End>
    auto parse_list() -> std::optional<
        std::vector<std::remove_reference_t<decltype(*std::invoke(Rule, this))>>> {
        auto backup = m_tokens;
        std::vector<std::remove_reference_t<decltype(*std::invoke(Rule, this))>> res;

        if (!match(Start)) {
            return {};
        }

        while (auto elem_res = std::invoke(Rule, this)) {
            res.emplace_back(std::move(*elem_res));
            if (!match(Sep)) {
                if (match(End)) {
                    return res;
                }
                m_tokens = backup;
                return {};
            }
        }

        if (match(End)) {
            return res;
        }

        return {};
    }

    std::span<const token> m_tokens;
};
} // namespace

std::optional<ast::module> parse_module(std::string_view input) {
    auto lexer = tokenize(input);
    std::vector<token> toks;
    std::copy_if(
        lexer.begin(), lexer.end(), std::back_inserter(toks), [](const token& tok) {
            return tok.type != token_type::block_comment &&
                   tok.type != token_type::line_comment;
        });
    parser p{.m_tokens = toks};
    return p.parse_module();
}
} // namespace lidl::frontend