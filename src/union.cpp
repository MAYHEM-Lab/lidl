#include <cassert>
#include <lidl/module.hpp>
#include <lidl/union.hpp>

namespace lidl {
raw_layout union_type::wire_layout(const lidl::module& mod) const {
    auto overall_computer = layout(mod);
    return overall_computer.get();
}

YAML::Node union_type::bin2yaml(const module& mod, ibinary_reader& reader) const {
    if (raw) {
        std::cerr << "[WARN]"
                  << " Cannot determine the type of a raw union. Skipping the field\n";
        return {};
    }
    YAML::Node node;

    const auto& enumerator = get_enum(mod);
    auto enum_layout       = enumerator.wire_layout(mod);

    auto alternative_node = enumerator.bin2yaml(mod, reader);
    int alternative       = enumerator.find_by_name(alternative_node.as<std::string>());

    reader.seek(enum_layout.size());

    auto& [name, member] = members[alternative];
    auto member_type     = get_type(mod, member.type_);
    reader.align(member_type->wire_layout(mod).alignment());
    node[name] = member_type->bin2yaml(mod, reader);

    return node;
}

type_categories union_type::category(const module& mod) const {
    return std::any_of(members.begin(),
                       members.end(),
                       [&](auto& mem) {
                           auto& [name, member] = mem;
                           return get_type(mod, member.type_)->is_reference_type(mod);
                       })
               ? type_categories::reference
               : type_categories::value;
}

const enumeration& union_type::get_enum(const module& m) const {
    if (!m_enumeration) {
        m_enumeration = enum_for_union(m, *this);
        define(const_cast<union_type*>(this)->get_scope(),
               "alternatives",
               m_enumeration.get());
    }
    return *m_enumeration;
}

int union_type::yaml2bin(const module& mod,
                         const YAML::Node& node,
                         ibinary_writer& writer) const {
    if (node.size() != 1) {
        throw std::runtime_error("Union has not exactly 1 member!");
    }

    auto active_member     = node.begin()->first.as<std::string>();
    const auto& enumerator = get_enum(mod);
    auto enum_index        = enumerator.find_by_name(active_member);
    auto& [mem_name, mem]  = members[enum_index];
    auto t                 = get_type(mod, mem.type_);

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

compound_layout union_type::layout(const module& mod) const {
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
        overall_computer.add_member("discriminator", get_enum(mod).wire_layout(mod));
    }
    overall_computer.add_member("val", computer.get());
    return overall_computer;
}

std::unique_ptr<enumeration> enum_for_union(const module& m, const union_type& u) {
    auto e = std::make_unique<enumeration>(const_cast<union_type*>(&u), u.src_info);
    auto i8handle = recursive_name_lookup(e->get_scope(), "i8");
    assert(i8handle);
    e->underlying_type = name{i8handle.value()};

    // TODO: since the enumerations are anonymous, we can't set inheritance relationship
    // between them yet...
    if (auto base = u.get_base()) {
        auto& base_enum = base->get_enum(m);
        auto enum_sym =
            recursive_definition_lookup(base->get_scope(), &base_enum).value();
        e->set_base({enum_sym});
    }

    //    static_cast<base*>(&e)->get_scope()->set_parent(u.get_scope()->shared_from_this());

    for (auto& [name, _] : u.own_members()) {
        e->add_member(std::string(name));
    }
    return e;
}
} // namespace lidl