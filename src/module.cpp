#include <lidl/algorithm.hpp>
#include <lidl/basic.hpp>
#include <lidl/module.hpp>

namespace lidl {
module& module::get_child(qualified_name child_name) {
    if (child_name.size() == 1) {
        auto it = std::find_if(children.begin(), children.end(), [&](auto& child) {
            return child.first == child_name.front();
        });
        if (it != children.end()) {
            return *(it->second);
        }

        return add_child(child_name, std::make_unique<module>(this));
    }

    auto it = this;

    while (!child_name.empty()) {
        it = &it->get_child(child_name.front());
        child_name.erase(child_name.begin());
    }

    return *it;
}

module& module::get_child(std::string_view child_name) {
    auto& res = get_child(split(child_name, scope_separator));
    res.name_space = std::string(child_name);
    return res;
}

module& module::add_child(qualified_name child_name, std::unique_ptr<module> child) {
    assert(child_name.size() <= 1);
    children.emplace_back(child_name.empty() ? "" : std::string(child_name.front()),
                          std::move(child));
    auto& mod = *children.back().second;
    define(get_scope(), child_name.empty() ? "" : child_name.front(), &mod);
    mod.parent     = this;
    mod.name_space = child_name.empty() ? "" : child_name.front();
    return mod;
}

module& module::add_child(std::string_view child_name, std::unique_ptr<module> child) {
    return add_child(split(child_name, scope_separator), std::move(child));
}

const basic_generic_instantiation&
module::create_or_get_instantiation(const name& ins) const {
    auto it = std::find_if(
        name_ins.begin(), name_ins.end(), [&](auto& p) { return p.first == ins; });
    if (it != name_ins.end()) {
        return *it->second;
    }

    std::unique_ptr<basic_generic_instantiation> instantiation;

    if (dynamic_cast<const generic_wire_type*>(get_symbol(ins.base))) {
        instantiation = std::make_unique<generic_wire_type_instantiation>(ins);
    } else if (dynamic_cast<const generic_view_type*>(get_symbol(ins.base))) {
        instantiation = std::make_unique<generic_view_type_instantiation>(ins);
    }

    assert(instantiation);

    instantiations.emplace_back(std::move(instantiation));
    name_ins.emplace_back(ins, instantiations.back().get());
    return *instantiations.back();
}
} // namespace lidl