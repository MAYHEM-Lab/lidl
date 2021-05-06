#include <lidl/enumeration.hpp>

namespace lidl {

YAML::Node enumeration::bin2yaml(const module& mod, ibinary_reader& reader) const {
    auto integral =
        lidl::get_wire_type(mod, underlying_type)->bin2yaml(mod, reader).as<uint64_t>();

    auto it = std::find_if(members.begin(), members.end(), [integral](auto& member) {
        return member.second.value == integral;
    });

    if (it == members.end()) {
        throw std::runtime_error("unknown enum value");
    }

    return YAML::Node(it->first);
}

enum_member::enum_member(enumeration& en, int val, std::optional<source_info> src_info)
    : cbase{&en, std::move(src_info)}
    , value(val) {
}
} // namespace lidl