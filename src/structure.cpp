#include <lidl/structure.hpp>
#include <cassert>

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

    auto l = layout(mod);

    auto span_size = span.size();
    std::cerr << span_size << '\n';
    auto struct_begin = span.size() - l.get().size();

    for (auto it = members.rbegin(); it != members.rend(); ++it) {
        auto& [name, member] = *it;

        /**
         * We have to compute the exact end of the member here due to possible padding.
         * Our layout keeps track of the offsets of our members.
         */
        auto member_layout = get_type(mod, member.type_)->wire_layout(mod);

        auto member_begin_offset = l.offset_of(name).value();
        auto actual_end = member_begin_offset + member_layout.size();
        auto cur_span = span.subspan(0, struct_begin + actual_end);

        node[name] = get_type(mod, member.type_)->bin2yaml(mod, cur_span).first;
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
    auto struct_pos = writer.tell(); // Struct beginning
    auto l = layout(mod);
    for (auto& [mem_name, mem] : members) {
        auto t = get_type(mod, mem.type_);
        if (t->is_reference_type(mod)) {
            const auto pos = references[mem_name];
            YAML::Node ptr_node(pos);
            writer.align(t->wire_layout(mod).alignment());

            auto expected_offset = l.offset_of(mem_name).value();
            auto cur_offset = writer.tell() - struct_pos;
            assert(expected_offset == cur_offset);

            t->yaml2bin(mod, ptr_node, writer);
        } else {
            auto& elem = node[mem_name];
            writer.align(t->wire_layout(mod).alignment());

            auto expected_offset = l.offset_of(mem_name).value();
            auto cur_offset = writer.tell() - struct_pos;
            assert(expected_offset == cur_offset);

            t->yaml2bin(mod, elem, writer);
        }
    }
    // Must add the trailing padding bytes!
    writer.align(wire_layout(mod).alignment());
    return struct_pos;
}
} // namespace lidl