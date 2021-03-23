#include <lidl/module.hpp>

namespace lidl {
module& module::get_child(std::string_view child_name) {
    auto it = std::find_if(children.begin(), children.end(), [&](auto& child) {
        return child.first == child_name;
    });
    if (it != children.end()) {
        return *(it->second);
    }

    return add_child(child_name, std::make_unique<module>(this));
}

module& module::add_child(std::string_view child_name, std::unique_ptr<module> child) {
    children.emplace_back(std::string(child_name), std::move(child));
    auto& mod = *children.back().second;
    define(get_scope(), child_name, &mod);
    mod.parent      = this;
    mod.name_space  = child_name;
    return mod;
}
} // namespace lidl