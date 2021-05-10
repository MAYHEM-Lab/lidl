#pragma once

#include <fmt/format.h>
#include <iostream>
#include <lidl/source_info.hpp>
#include <optional>

namespace lidl {
enum class error_type
{
    warning,
    fatal
};

template<class... Args>
void report_user_error(error_type type,
                       std::optional<source_info> src_info,
                       Args&&... args) {
    switch (type) {

    case error_type::warning:
        std::cerr << "Warning";
        break;
    case error_type::fatal:
        std::cerr << "Fatal error";
        break;
    }
    if (src_info) {
        std::cerr << " at " << to_string(*src_info);
    }
    std::cerr << ": " << fmt::format(std::forward<Args>(args)...) << '\n';
    if (type == error_type::fatal) {
        exit(1);
    }
}
} // namespace lidl