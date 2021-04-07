#pragma once

#include <lidl/module.hpp>
#include <lidl/scope.hpp>
#include <string>

namespace lidl::codegen {
class backend {
public:
    virtual void generate(const module& mod, std::ostream& str) = 0;

    // How do users refer to this type name?
    // For instance, we may store a ptr<string> inside a struct, but the user is only
    // interested in getting a string& in the generated code. So this function will return
    // string for ptr<string>
    virtual std::string get_user_identifier(const module& mod, const name&) const = 0;

    // How do we globally refer to this type name? Returns a fully qualified name.
    virtual std::string get_identifier(const module& mod, const name&) const = 0;
    virtual ~backend()                                                       = default;
};

namespace detail {
inline backend* current_backend = nullptr;
}
inline backend* current_backend() {
    return detail::current_backend;
}

template<class GeneratorT, class DefT, class InstanceT>
auto do_generate(const module& mod, DefT* def, InstanceT& ins) {
    auto sym      = *recursive_definition_lookup(mod.symbols(), def);
    auto name     = local_name(sym);
    auto abs_name = current_backend()->get_identifier(mod, lidl::name{sym});

    auto generator = GeneratorT(mod, name, abs_name, ins);
    return generator.generate();
}

template<class T, class U>
auto do_generate(const module& mod, U* x) {
    return do_generate<T>(mod, x, *x);
}
} // namespace lidl::codegen