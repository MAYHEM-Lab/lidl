#include <lidl/module.hpp>

namespace lidl {
module& module::get_child(std::string_view child_name) {
    auto it = std::find_if(children.begin(), children.end(), [&](auto& child) {
        return child.first == child_name;
    });
    if (it != children.end()) {
        return *(it->second);
    }

    return add_child(child_name, std::make_unique<module>());
}

module& module::add_child(std::string_view child_name, std::unique_ptr<module> child) {
    children.emplace_back(std::string(child_name), std::move(child));
    auto& mod = *children.back().second;
    m_symbols->add_child_scope(std::string(child_name), mod.m_symbols);
    mod.parent = this;
    mod.name_space = child_name;
    mod.module_name = child_name;
    return mod;
}
} // namespace lidl