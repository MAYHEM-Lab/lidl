#include <lidl/union.hpp>
#include <lidl/module.hpp>

namespace lidl {
raw_layout union_type::wire_layout(const lidl::module& mod) const {
    union_layout_computer computer;
    for (auto& [name, member] : members) {
        if (get_type(member.type_)->is_reference_type(mod)) {
            computer.add({2, 2});
        } else {
            computer.add(get_type(member.type_)->wire_layout(mod));
        }
    }

    aggregate_layout_computer overall_computer;
    overall_computer.add(get_enum().wire_layout(mod));
    overall_computer.add(computer.get());
    return overall_computer.get();
}

std::pair<YAML::Node, size_t> union_type::bin2yaml(const module& module,
                                                   gsl::span<const uint8_t> span) const {
    YAML::Node node;

    auto cur_span = span;
    for (auto it = members.rbegin(); it != members.rend(); ++it) {
        auto& [name, member] = *it;
        node[name] = get_type(member.type_)->bin2yaml(module, cur_span).first;
        cur_span = cur_span.subspan(
            0, cur_span.size() - get_type(member.type_)->wire_layout(module).size());
    }

    return {node, wire_layout(module).size()};
}

bool union_type::is_reference_type(const module& mod) const {
    return std::any_of(members.begin(), members.end(), [&](auto& mem) {
      auto& [name, member] = mem;
      return get_type(member.type_)->is_reference_type(mod);
    });
}

const enumeration& union_type::get_enum() const {
    return *attributes.get<union_enum_attribute>("union_enum")->union_enum;
}
}