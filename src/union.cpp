#include <lidl/module.hpp>
#include <lidl/union.hpp>

namespace lidl {
raw_layout union_type::wire_layout(const lidl::module& mod) const {
    union_layout_computer computer;
    for (auto& [name, member] : members) {
        if (get_type(mod, member.type_)->is_reference_type(mod)) {
            computer.add({2, 2});
        } else {
            computer.add(get_type(mod, member.type_)->wire_layout(mod));
        }
    }

    compound_layout overall_computer;
    if (!raw) {
        overall_computer.add_member("discriminator", get_enum().wire_layout(mod));
    }
    overall_computer.add_member("val", computer.get());
    return overall_computer.get();
}

YAML::Node union_type::bin2yaml(const module& mod, ibinary_reader& reader) const {
    YAML::Node node;

    auto& enumerator = get_enum();
    auto enum_layout = enumerator.wire_layout(mod);

    auto alternative_node = enumerator.bin2yaml(mod, reader);
    int alternative       = enumerator.find_by_name(alternative_node.as<std::string>());

    reader.seek(enum_layout.size());

    auto& [name, member] = members[alternative];
    auto member_type     = get_type(mod, member.type_);
    reader.align(member_type->wire_layout(mod).alignment());
    node[name] = member_type->bin2yaml(mod, reader);

    return node;
}

bool union_type::is_reference_type(const module& mod) const {
    return std::any_of(members.begin(), members.end(), [&](auto& mem) {
        auto& [name, member] = mem;
        return get_type(mod, member.type_)->is_reference_type(mod);
    });
}

const enumeration& union_type::get_enum() const {
    return *attributes.get<union_enum_attribute>("union_enum")->union_enum;
}

int union_type::yaml2bin(const module& mod,
                         const YAML::Node& node,
                         ibinary_writer& writer) const {
    if (node.size() != 1) {
        throw std::runtime_error("Union has not exactly 1 member!");
    }

    auto active_member    = node.begin()->first.as<std::string>();
    auto& enumerator      = get_enum();
    auto enum_index       = enumerator.find_by_name(active_member);
    auto& [mem_name, mem] = members[enum_index];
    auto t                = get_type(mod, mem.type_);

    int pointee_pos = 0;
    if (t->is_reference_type(mod)) {
        auto pointee      = std::get<name>(mem.type_.args[0].get_variant());
        auto pointee_type = get_type(mod, pointee);
        writer.align(pointee_type->wire_layout(mod).alignment());
        pointee_pos = pointee_type->yaml2bin(mod, node.begin()->second, writer);
    }

    writer.align(wire_layout(mod).alignment());
    auto pos = writer.tell();

    YAML::Node enumerator_val(enum_index);
    enumerator.yaml2bin(mod, enumerator_val, writer);

    if (!t->is_reference_type(mod)) {
        writer.align(t->wire_layout(mod).alignment());
        t->yaml2bin(mod, node.begin()->second, writer);
    } else {
        YAML::Node ptr_node(pointee_pos);
        t->yaml2bin(mod, ptr_node, writer);
    }

    return pos;
}
} // namespace lidl