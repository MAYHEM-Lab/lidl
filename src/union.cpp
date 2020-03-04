#include <lidl/union.hpp>
#include <lidl/module.hpp>

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

    aggregate_layout_computer overall_computer;
    overall_computer.add(get_enum().wire_layout(mod));
    overall_computer.add(computer.get());
    return overall_computer.get();
}

std::pair<YAML::Node, size_t> union_type::bin2yaml(const module& mod,
                                                   gsl::span<const uint8_t> span) const {
    YAML::Node node;

    auto& enumerator = get_enum();
    auto my_layout = wire_layout(mod);
    auto enum_layout = enumerator.wire_layout(mod);
    auto enum_span = span.subspan(0,  span.size() - my_layout.size() + enum_layout.size());

    auto alternative_node = enumerator.bin2yaml(mod, enum_span);
    int alternative = alternative_node.first.as<uint64_t>();

    auto& [name, member] = members[alternative];
    node[name] = get_type(mod, member.type_)->bin2yaml(mod, span).first;

    return {node, my_layout.size()};
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

    writer.align(wire_layout(mod).alignment());
    auto pos = writer.tell();

    auto active_member = node.begin()->first.as<std::string>();
    auto& enumerator = get_enum();
    auto enum_index = enumerator.find_by_name(active_member);
    YAML::Node enumerator_val(enum_index);
    enumerator.yaml2bin(mod, enumerator_val, writer);
    auto& [name, mem] = members[enum_index];
    writer.align(get_type(mod, mem.type_)->wire_layout(mod).alignment());
    get_type(mod, mem.type_)->yaml2bin(mod, node.begin()->second, writer);

    return pos;
}
}