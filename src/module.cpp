#include <lidl/module.hpp>

namespace lidl {
module& module::get_child(std::string_view child_name) const {
    auto it = std::find_if(children.begin(), children.end(), [&](auto& child) {
        return child.first == child_name;
    });
    if (it != children.end()) {
        return *(it->second);
    }

    children.emplace_back(std::string(child_name), std::make_unique<module>());
    auto& res = *children.back().second;
    res.parent = this;
    res.symbols = symbols->add_child_scope();
    res.module_name = child_name;
    return res;
}
} // namespace lidl