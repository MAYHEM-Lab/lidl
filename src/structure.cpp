#include <cassert>
#include <lidl/structure.hpp>

namespace lidl {
type_categories structure::category(const module& mod) const {
    return std::all_of(members.begin(),
                       members.end(),
                       [&](auto& mem) {
                           auto& [name, member] = mem;
                           return get_wire_type(mod, member.type_)->is_value(mod);
                       })
               ? type_categories::value
               : type_categories::reference;
}

raw_layout structure::wire_layout(const module& mod) const {
    return layout(mod).get();
}

compound_layout structure::layout(const module& mod) const {
    compound_layout computer;
    for (auto& [name, member] : members) {
        auto wire_type = get_wire_type(mod, member.type_);
        computer.add_member(name, wire_type->wire_layout(mod));
    }
    return computer;
}

YAML::Node structure::bin2yaml(const module& mod, ibinary_reader& reader) const {
    YAML::Node node;

    auto l = layout(mod);

    auto struct_begin = reader.tell();

    for (auto it = members.rbegin(); it != members.rend(); ++it) {
        auto& [name, member] = *it;

        /**
         * We have to compute the exact end of the member here due to possible
         padding.
         * Our layout keeps track of the offsets of our members.
         */

        auto member_begin_offset = l.offset_of(name).value();
        reader.seek(struct_begin + member_begin_offset, std::ios::beg);

        node[name] = lidl::get_wire_type(mod, member.type_)->bin2yaml(mod, reader);
    }

    return node;
}

int structure::yaml2bin(const module& mod,
                        const YAML::Node& node,
                        ibinary_writer& writer) const {
    std::unordered_map<std::string, int> references;
    for (auto& [mem_name, mem] : members) {
        auto t = lidl::get_wire_type(mod, mem.type_);

        if (t->is_value(mod)) {
            // Continue, we'll place it inline
            continue;
        }

        auto pointee = lidl::get_wire_type(mod, deref_ptr(mod, mem.type_));
        references.emplace(mem_name, pointee->yaml2bin(mod, node[mem_name], writer));
    }

    writer.align(wire_layout(mod).alignment());
    auto struct_pos = writer.tell(); // Struct beginning
    auto l          = layout(mod);
    for (auto& [mem_name, mem] : members) {
        auto t = lidl::get_wire_type(mod, mem.type_);
        if (t->is_reference_type(mod)) {
            const auto pos = references[mem_name];
            YAML::Node ptr_node(pos);
            writer.align(t->wire_layout(mod).alignment());

            auto expected_offset = l.offset_of(mem_name).value();
            auto cur_offset      = writer.tell() - struct_pos;
            assert(expected_offset == cur_offset);

            t->yaml2bin(mod, ptr_node, writer);
        } else {
            auto& elem = node[mem_name];
            writer.align(t->wire_layout(mod).alignment());

            auto expected_offset = l.offset_of(mem_name).value();
            auto cur_offset      = writer.tell() - struct_pos;
            assert(expected_offset == cur_offset);

            t->yaml2bin(mod, elem, writer);
        }
    }
    // Must add the trailing padding bytes!
    writer.align(wire_layout(mod).alignment());
    return struct_pos;
}

void structure::add_member(std::string name, member mem) {
//    if (mem.type_ != lidl::get_wire_type_name(*find_parent_module(this), mem.type_)) {
//        report_user_error(error_type::warning,
//                          mem.src_info,
//                          "Member type for {} is not a wire type!",
//                          name);
//    }
    members.emplace_back(std::move(name), std::move(mem));
    define(get_scope(), members.back().first, &members.back().second);
}
} // namespace lidl