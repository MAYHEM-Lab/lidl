#include <lidl/structure.hpp>

namespace lidl {
bool structure::is_reference_type(const module& mod) const {
    return std::any_of(members.begin(), members.end(), [&](auto& mem) {
        auto& [name, member] = mem;
        return get_type(mod, member.type_)->is_reference_type(mod);
    });
}

raw_layout structure::wire_layout(const module& mod) const {
    return layout(mod).get();
}

compound_layout structure::layout(const module& mod) const {
    compound_layout computer;
    for (auto& [name, member] : members) {
        if (get_type(mod, member.type_)->is_reference_type(mod)) {
            computer.add_member(name, {2, 2});
        } else {
            computer.add_member(name, get_type(mod, member.type_)->wire_layout(mod));
        }
    }
    return computer;
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
    std::unordered_map<std::string, int> references;
    for (auto& [mem_name, mem] : members) {
        auto t = get_type(mod, mem.type_);
        if (!t->is_reference_type(mod)) {
            // Continue, we'll place it inline
            continue;
        }
        auto pointee = get_type(mod, std::get<name>(mem.type_.args[0].get_variant()));
        references.emplace(mem_name, pointee->yaml2bin(mod, node[mem_name], writer));
    }

    writer.align(wire_layout(mod).alignment());
    auto pos = writer.tell();
    for (auto& [mem_name, mem] : members) {
        auto t = get_type(mod, mem.type_);
        if (t->is_reference_type(mod)) {
            const auto pos = references[mem_name];
            YAML::Node ptr_node(pos);
            writer.align(t->wire_layout(mod).alignment());
            t->yaml2bin(mod, ptr_node, writer);
        } else {
            auto& elem = node[mem_name];
            writer.align(t->wire_layout(mod).alignment());
            t->yaml2bin(mod, elem, writer);
        }
    }
    return pos;
}
} // namespace lidl