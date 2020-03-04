#include <lidl/structure.hpp>

namespace lidl {
bool structure::is_reference_type(const module& mod) const {
    return std::any_of(members.begin(), members.end(), [&](auto& mem) {
        auto& [name, member] = mem;
        return get_type(mod, member.type_)->is_reference_type(mod);
    });
}

raw_layout structure::wire_layout(const module& mod) const {
    aggregate_layout_computer computer;
    for (auto& [name, member] : members) {
        if (get_type(mod, member.type_)->is_reference_type(mod)) {
            computer.add({2, 2});
        } else {
            computer.add(get_type(mod, member.type_)->wire_layout(mod));
        }
    }
    return computer.get();
}

std::pair<YAML::Node, size_t> structure::bin2yaml(const module& mod,
                                                  gsl::span<const uint8_t> span) const {
    YAML::Node node;

    auto cur_span = span;
    for (auto it = members.rbegin(); it != members.rend(); ++it) {
        auto& [name, member] = *it;
        node[name] = get_type(mod, member.type_)->bin2yaml(mod, cur_span).first;
        cur_span = cur_span.subspan(
            0, cur_span.size() - get_type(mod, member.type_)->wire_layout(mod).size());
    }

    return {node, wire_layout(mod).size()};
}
int structure::yaml2bin(const module& mod,
                         const YAML::Node& node,
                         ibinary_writer& writer) const {
    writer.align(wire_layout(mod).alignment());
    auto pos = writer.tell();
    for (auto& [mem_name, mem] : members) {
        auto& elem = node[mem_name];
        writer.align(get_type(mod, mem.type_)->wire_layout(mod).alignment());
        get_type(mod, mem.type_)->yaml2bin(mod, elem, writer);
    }
    return pos;
}
} // namespace lidl