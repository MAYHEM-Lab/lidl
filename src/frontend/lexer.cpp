#include "lexer.hpp"

#include <lidl/error.hpp>
#include <string_view>
#include <unordered_map>

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

std::optional<std::pair<token_type, int>> try_identifier(std::string_view input) {
    auto orig = input;
    if (!isalpha(input.front())) {
        return {};
    }

    int len = 0;
    while (isalnum(input.front()) || input.front() == '_') {
        ++len;
        input.remove_prefix(1);
    }

    if (auto it = keywords.find(orig.substr(0, len)); it != keywords.end()) {
        return std::pair{it->second, len};
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

std::optional<std::pair<token_type, int>> try_next(std::string_view input) {
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

void lexer::next() {
    if (get().type == token_type::eof) {
        m_finished = true;
        return;
    }

    if (m_current.empty()) {
        m_last = token{.src_info = src_info, .type = token_type::eof, .content = {}};
        return;
    }

    auto type = try_next(m_current);
    if (!type) {
        report_user_error(error_type::fatal, src_info, "Failed to tokenize");
        m_current = {};
        return;
    }

    src_info.len = type->second;

    auto content = m_current.substr(0, src_info.len);

    m_last = token{.src_info = src_info, .type = type->first, .content = content};

    src_info.pos += src_info.len;
    src_info.column += src_info.len;

    if (type->first == token_type::newline) {
        src_info.line++;
        src_info.column = 0;
    } else if (type->first == token_type::block_comment) {
        auto line_count = std::count(content.begin(), content.end(), '\n');
        src_info.line += line_count;
        src_info.column = 0;
    }

    m_current.remove_prefix(src_info.len);
    consume_whitespace();
}

void lexer::consume_whitespace() {
    for (int i = 0; !m_current.empty() && iswspace(m_current.front()); ++i) {
        if (m_current.front() == ' ' || m_current.front() == '\t') {
            src_info.column += 1;
        } else if (m_current.front() == '\n') {
            src_info.line += 1;
            src_info.column = 0;
        }

        src_info.pos += 1;
        m_current.remove_prefix(1);
    }
}

lexer::lexer(std::string_view input)
    : m_input(input)
    , m_current(input) {
    consume_whitespace();
    next();
}

token lexer::get() const {
    return m_last;
}
} // namespace lidl::frontend