#include "lexer.hpp"

#include <string_view>
#include <unordered_map>
#include <lidl/error.hpp>

namespace lidl::frontend {
namespace {
std::unordered_map<std::string_view, token_type> keywords = {
    {"struct", token_type::kw_struct},
    {"union", token_type::kw_union},
    {"enum", token_type::kw_enum},
    {"true", token_type::kw_true},
    {"false", token_type::kw_false},
    {"service", token_type::kw_service},
    {"namespace", token_type::kw_namespace},
    {"import", token_type::kw_import},
    {"static_assert", token_type::kw_static_assert},
};

std::unordered_map<uint32_t, token_type> one_chars = {
    {'.', token_type::dot},
    {':', token_type::colon},
    {';', token_type::semicolon},
    {'=', token_type::eq},
    {'(', token_type::left_parens},
    {')', token_type::right_parens},
    {'{', token_type::left_brace},
    {'}', token_type::right_brace},
    {'<', token_type::left_angular},
    {'>', token_type::right_angular},
    {',', token_type::comma},
    {'\n', token_type::newline},
    {EOF, token_type::eof},
};

std::unordered_map<std::string_view, token_type> two_chars = {
    {"->", token_type::arrow},
    {"=>", token_type::fat_arrow},
    {"::", token_type::coloncolon},
};

std::optional<std::pair<token_type, int>> try_one_char(std::string_view input) {
    if (auto it = one_chars.find(input.front()); it != one_chars.end()) {
        return std::pair{it->second, 1};
    }
    return {};
}

std::optional<std::pair<token_type, int>> try_two_char(std::string_view input) {
    if (auto it = two_chars.find(input.substr(0, 2)); it != two_chars.end()) {
        return std::pair{it->second, 2};
    }
    return {};
}

std::optional<std::pair<token_type, int>> try_keyword(std::string_view input) {
    for (auto& [str, tt] : keywords) {
        if (input.starts_with(str)) {
            return std::pair{tt, str.size()};
        }
    }
    return {};
}

std::optional<std::pair<token_type, int>> try_identifier(std::string_view input) {
    if (!isalpha(input.front())) {
        return {};
    }

    int len = 0;
    while (isalnum(input.front()) || input.front() == '_') {
        ++len;
        input.remove_prefix(1);
    }

    return std::pair{token_type::identifier, len};
}

using try_t = std::optional<std::pair<token_type, int>>;

try_t try_number(std::string_view input) {
    bool have_dot = false;
    int len       = 0;

    while (isdigit(input.front()) || (input.front() == '.' && !have_dot)) {
        len++;
        if (input.front() == '.') {
            have_dot = true;
        }
        input.remove_prefix(1);
    }

    if (len == 0) {
        return {};
    }

    if (have_dot) {
        return std::pair{token_type::floating, len};
    }

    return std::pair{token_type::integer, len};
}

try_t try_comment(std::string_view input) {
    if (input.starts_with("//")) {
        auto next_new_line = input.find_first_of('\n');
        if (next_new_line == input.npos) {
            // error
            exit(1);
        }
        return std::pair{token_type::line_comment, next_new_line};
    }

    if (input.starts_with("/*")) {
        auto next_new_line = input.find_first_of("*/", 2);
        if (next_new_line == input.npos) {
            // error
            exit(1);
        }
        return std::pair{token_type::block_comment, next_new_line + 2};
    }

    return {};
}

try_t try_string_literal(std::string_view input) {
    if (!input.starts_with('"')) {
        return {};
    }

    input.remove_prefix(1);
    int len = 1;

    while (input.front() != '"') {
        ++len;
        if (input.front() == '\n') {
            report_user_error(error_type::fatal, {}, "Newline in string literal");
        }
        input.remove_prefix(1);
    }

    return std::pair{token_type::string_literal, len + 1};
}

int consume_whitespace(std::string_view input) {
    int i = 0;
    while (!input.empty() && iswspace(input.front())) {
        input.remove_prefix(1);
        ++i;
    }
    return i;
}

std::optional<std::pair<token_type, int>> try_next(std::string_view input) {
    if (auto res = try_keyword(input)) {
        return res;
    }

    if (auto res = try_two_char(input)) {
        return res;
    }

    if (auto res = try_one_char(input)) {
        return res;
    }

    if (auto res = try_identifier(input)) {
        return res;
    }

    if (auto res = try_number(input)) {
        return res;
    }

    if (auto res = try_comment(input)) {
        return res;
    }

    if (auto res = try_string_literal(input)) {
        return res;
    }

    return {};
}
} // namespace

tinycoro::Generator<token> tokenize(std::string_view input) {
    source_info src_info{0, 0, 0, 0};
    input.remove_prefix(consume_whitespace(input));

    while (!input.empty()) {
        auto type = try_next(input);
        if (!type) {
            co_return;
        }

        src_info.len = type->second;

        auto content = input.substr(0, src_info.len);

        co_yield token{.src_info = src_info, .type = type->first, .content = content};

        if (type->first == token_type::newline ||
            type->first == token_type::line_comment) {
            src_info.line++;
            src_info.column = 0;
        } else if (type->first == token_type::block_comment) {
            auto line_count = std::count(content.begin(), content.end(), '\n');
            src_info.line += line_count;
            src_info.column = 0;
        }

        src_info.pos += src_info.len;
        input.remove_prefix(src_info.len);
        input.remove_prefix(consume_whitespace(input));
    }

    co_yield token{.src_info = src_info, .type = token_type::eof, .content = {}};
}
} // namespace lidl::frontend