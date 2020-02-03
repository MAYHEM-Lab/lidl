#pragma once

#include <fmt/format.h>
#include <stdexcept>
#include <string_view>

namespace lidl {
class error : std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class unknown_type_error : public error {};

class no_generic_type : public error {
public:
    explicit no_generic_type(std::string_view generic_type)
        : error(fmt::format("No such generic type: {}", generic_type)) {
    }
};

class unknown_attribute_error : public error {
public:
    explicit unknown_attribute_error(std::string_view attribute_name)
        : error(fmt::format("Unknown attribute: {}", attribute_name)) {
    }
};
} // namespace lidl