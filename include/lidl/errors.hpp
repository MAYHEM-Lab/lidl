#pragma once

#include <fmt/format.h>
#include <lidl/source_info.hpp>
#include <stdexcept>
#include <string_view>

namespace lidl {
class error : public std::exception {
public:
    explicit error(std::string message, std::optional<source_info> source = {})
        : m_message(std::move(message))
        , m_src_info(std::move(source)) {
        if (m_src_info) {
            m_message = fmt::format("{} at {}:{} in {}",
                                    m_message,
                                    m_src_info->line + 1,
                                    m_src_info->column + 1,
                                    m_src_info->origin ? *m_src_info->origin : std::string(""));
        }
    }

    const char* what() const noexcept override {
        return m_message.c_str();
    }

    const source_info* src_info() const {
        if (m_src_info) {
            return &*m_src_info;
        }
        return nullptr;
    }

private:
    std::string m_message;
    std::optional<source_info> m_src_info;
};

class unknown_type_error : public error {
public:
    unknown_type_error(std::string_view type_name, std::optional<source_info> source)
        : error(fmt::format("Unknown type \"{}\"", type_name), std::move(source)) {
    }
};

class no_generic_type : public error {
public:
    explicit no_generic_type(std::string_view generic_type,
                             std::optional<source_info> source)
        : error(fmt::format("Unknown generic type: {}", generic_type),
                std::move(source)) {
    }
};
} // namespace lidl