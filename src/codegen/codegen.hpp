#pragma once

#include <string>
#include <lidl/module.hpp>
#include <lidl/scope.hpp>

namespace lidl::codegen {
class backend {
public:
    virtual void generate(const module& mod, std::ostream& str) = 0;
    virtual std::string get_user_identifier(const module& mod, const name&) const = 0;
    virtual std::string get_local_identifier(const module& mod, const name&) const = 0;
    virtual std::string get_identifier(const module& mod, const name&) const = 0;
    virtual ~backend() = default;
};

namespace detail {
inline backend* current_backend = nullptr;
}
inline backend* current_backend() {
    return detail::current_backend;
}
}