#pragma once

#include <string>
#include <lidl/module.hpp>
#include <lidl/scope.hpp>

namespace lidl::codegen {
class backend {
public:
    virtual void generate(const module& mod, std::ostream& str) = 0;

    // How do users refer to this type name?
    // For instance, we may store a ptr<string> inside a struct, but the user is only
    // interested in getting a string& in the generated code. So this function will return
    // string for ptr<string>
    virtual std::string get_user_identifier(const module& mod, const name&) const = 0;

    // How do we refer to this type name if we are in the same namespace?
    virtual std::string get_local_identifier(const module& mod, const name&) const = 0;

    // How do we globally refer to this type name? Returns a fully qualified name.
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