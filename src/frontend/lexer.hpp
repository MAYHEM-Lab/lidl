#pragma once

#include <lidl/source_info.hpp>

namespace lidl::frontend {
enum class token_type
{
    kw_struct,
    kw_union,
    kw_service,
    kw_enum,
    kw_static_assert,
    kw_namespace,
    kw_import,
    left_angular,
    right_angular,
    left_parens,
    right_parens,
    newline,
    line_comment,
    block_comment,
    comma,
    colon,
    identifier,
    coloncolon,
    semicolon,
    dot,
    eqeq,
    left_brace,
    right_brace,
    eq,
    arrow,
    fat_arrow,
    integer,
    floating,
    kw_true,
    kw_false,
    string_literal,
    eof
};

struct token {
    std::optional<source_info> src_info;
    token_type type;
    std::string_view content;
};

struct lexer {
    explicit lexer(std::string_view input);

    bool done() const {
        return m_finished;
    }

    token get() const;

    void next();

private:
    bool m_finished = false;
    source_info src_info{0, 0, 0, 0};
    token m_last;
    std::string_view m_input;
    std::string_view m_current;
};
} // namespace lidl::frontend